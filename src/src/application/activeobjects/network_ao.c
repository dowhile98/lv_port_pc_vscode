/**
 * @file network_ao.c
 * @brief NetworkAO: FSM de ciclo de vida completo de la pila TCP/IP.
 *
 * Orquesta: stack init → port configure → poll link → poll IP → start services.
 * Ante caída de enlace: para servicios y reintenta desde WAITING_LINK.
 *
 * @note SRP: Solo orquesta stack/port/link/service. Sin lógica WiFi ni HTTP.
 * @note DIP: Depende únicamente de interfaces puras (INetworkStack/Port/Link/Service).
 * @note Memoria: 100% estática — cero malloc/free.
 *
 * @version 2.0.0  Refactoring N.8 — eliminado acoplamiento WiFi
 * @date    2026-03-20
 */

#include "application/activeobjects/network_ao.h"
#include "interfaces/i_network_stack.h"
#include "interfaces/i_network_port.h"
#include "interfaces/i_network_link.h"
#include "interfaces/i_network_service.h"
#include "interfaces/network_port_types.h"
#include "infrastructure/osal/osal.h"
#include <string.h>

/* ===================================================================
 * Forward declarations (private)
 * =================================================================== */

static void handle_msg_start(NetworkAO_t *self);
static void handle_msg_transport_ready(NetworkAO_t *self);
static void handle_msg_reconfigure(NetworkAO_t *self);
static void handle_msg_stop(NetworkAO_t *self);
static void handle_tick(NetworkAO_t *self);
static void start_all_services(NetworkAO_t *self);
static void stop_all_services(NetworkAO_t *self);
static bool any_link_up(const NetworkAO_t *self);
static bool any_has_ip(const NetworkAO_t *self);
static void enter_link_down(NetworkAO_t *self);
static void ao_thread_entry(void *arg);
static bool port_needs_spi_transport(NetworkPortType_t port_type);
static void configure_ready_ports(NetworkAO_t *self);
static bool any_port_configured(const NetworkAO_t *self);

/* ===================================================================
 * Private helpers — port transport detection
 * =================================================================== */

/**
 * @brief Returns true if the given port type requires ESP-Hosted SPI transport.
 *
 * @param[in] port_type  Port type identifier (WIFI_STA, WIFI_AP, RNDIS, ETH).
 * @return true if WiFi (STA/AP), false if RNDIS or Ethernet (ready immediately).
 */
static bool port_needs_spi_transport(NetworkPortType_t port_type)
{
    return (port_type == NETWORK_PORT_TYPE_WIFI_STA) ||
           (port_type == NETWORK_PORT_TYPE_WIFI_AP);
}

/**
 * @brief Configures all ports whose transport is ready but not yet configured.
 *
 * @param[in,out] self  NetworkAO instance.
 *
 * @note Errors from individual port Configure() calls are non-fatal — the AO
 *       logs the failure and continues with remaining ports.
 */
static void configure_ready_ports(NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        /* Skip if already configured or transport not ready */
        if (self->port_configured[i] || !self->port_transport_ready[i])
        {
            continue;
        }

        /* Configure this port */
        if (self->ports[i] != NULL && self->port_configs[i] != NULL)
        {
            if (NetworkPort_Configure(self->ports[i], self->port_configs[i]) == ERR_OK)
            {
                self->port_configured[i] = true;
            }
            else
            {
                /* Log error but continue — non-fatal for other ports */
                if (self->logger != NULL)
                {
                    LOG_ERROR(self->logger, "NETWORK_AO",
                              "Port %u Configure() failed", (unsigned)i);
                }
            }
        }
    }
}

/**
 * @brief Returns true if at least one port has been successfully configured.
 */
static bool any_port_configured(const NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->port_configured[i])
        {
            return true;
        }
    }
    return false;
}

/* ===================================================================
 * Private helpers — services
 * =================================================================== */

static void start_all_services(NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->service_count; i++)
    {
        if (self->services[i] != NULL)
        {
            if (NetworkService_Start(self->services[i]) != ERR_OK)
            {
                LOG_ERROR(self->logger, "NETWORK_AO", "Failed to start service %u", i);
            }
        }
    }
}

static void stop_all_services(NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->service_count; i++)
    {
        if (self->services[i] != NULL)
        {
            (void)NetworkService_Stop(self->services[i]);
        }
    }
}

/* ===================================================================
 * Private helper — enter LINK_DOWN state
 *
 * Centralises: services stop, state transition, backoff init.
 * Called from both the tick (SERVICES_RUNNING link drop) and from
 * MSG_LINK_DOWN (async event from WifiControlAdapter callback).
 * =================================================================== */

static void enter_link_down(NetworkAO_t *self)
{
    stop_all_services(self);
    self->fsm_state = NET_AO_STATE_LINK_DOWN;
    /* Arm the countdown for the current backoff interval. */
    self->reconnect_ticks_remaining = self->reconnect_backoff_ticks;
}

/* ===================================================================
 * Private helpers — link/IP probing
 * =================================================================== */

/**
 * @brief Returns true if at least one registered link reports IsLinkUp().
 */
static bool any_link_up(const NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->links[i] != NULL && NetworkLink_IsLinkUp(self->links[i]))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Returns true if at least one registered link reports HasIpAddress().
 */
static bool any_has_ip(const NetworkAO_t *self)
{
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->links[i] != NULL && NetworkLink_HasIpAddress(self->links[i]))
        {
            return true;
        }
    }
    return false;
}

/* ===================================================================
 * FSM message handlers
 * =================================================================== */

/**
 * @brief Handles MSG_START from IDLE.
 *
 * Phase 1 of network bring-up:
 *   1. Calls INetworkStack_Init() — `netInit()` (doesn't require SPI transport).
 *   2. Marks which ports are transport-ready (RNDIS/ETH: immediately; WiFi: waits for SPI).
 *   3. Configures all transport-ready ports immediately.
 *   4. If any WiFi port exists, launches ESP-Hosted SPI transport in background.
 *   5. Transitions to WAITING_LINK if at least one port is configured.
 *
 * @note UNIT_TEST: simulates transport ready synchronously so tests do not
 *       need a real ESP task chain. handle_msg_transport_ready is called
 *       inline via the #ifdef UNIT_TEST path.
 *
 * @note BREAKING CHANGE: RNDIS ports no longer wait for SPI transport — they
 *       configure immediately, allowing the network to become operational
 *       even before WiFi is ready.
 */
static void handle_msg_start(NetworkAO_t *self)
{
    self->fsm_state = NET_AO_STATE_INITIALIZING;

    /* Step 1: Initialise TCP/IP stack (netInit) — doesn't require SPI transport */
    if (INetworkStack_Init(self->stack) != ERR_OK)
    {
        self->fsm_state = NET_AO_STATE_IDLE;
        self->running = false; /* thread exits → signals stopped_sem */
        return;
    }

    /* Step 2: Mark which ports are transport-ready */
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->port_configs[i] != NULL)
        {
            /* RNDIS/ETH ports are ready immediately (no external transport).
             * WiFi ports must wait for ESP-Hosted SPI to become active. */
            self->port_transport_ready[i] =
                !port_needs_spi_transport(self->port_configs[i]->port_type);
        }
        self->port_configured[i] = false;
    }

    /* Step 3: Configure all ports that are already transport-ready (RNDIS, ETH) */
    configure_ready_ports(self);

    /* Step 4: One-time preparation of each service (e.g. httpServerInit + task launch).
     * Mirrors the CycloneTCP demo sequence where httpServerInit is called after
     * netConfigInterface, before the RTOS scheduler loop.  Errors are non-fatal:
     * the network can run without every service being available. */
    for (uint8_t i = 0U; i < self->service_count; i++)
    {
        if (self->services[i] != NULL)
        {
            (void)NetworkService_Prepare(self->services[i]);
        }
    }

    /* Step 5: If any WiFi port exists, launch ESP-Hosted SPI transport in background.
     * When the transport becomes ready, WiFi ports will be configured via
     * handle_msg_transport_ready(). */
    if (self->needs_transport)
    {
        INetworkStack_LaunchTransport(self->stack);

#ifdef UNIT_TEST
        /* In PC tests there is no real ESP-Hosted task chain to fire the callback.
         * Simulate transport ready immediately so tests remain synchronous. */
        handle_msg_transport_ready(self);
#endif
    }

    /* Step 6: Transition to WAITING_LINK if at least one port is configured.
     * This allows RNDIS to start link polling immediately while WiFi ports
     * wait for their transport to become ready. */
    if (any_port_configured(self))
    {
        self->fsm_state = NET_AO_STATE_WAITING_LINK;
    }
    /* else: remain in INITIALIZING until TRANSPORT_READY arrives and configures WiFi */
}

/**
 * @brief Handles NET_AO_MSG_TRANSPORT_READY: ESP-Hosted SPI transport active.
 *
 * Port-aware WiFi initialization:
 *   1. Marks all WiFi ports as transport-ready.
 *   2. For each WiFi port:
 *      - If NOT configured → Configure() (first boot)
 *      - If configured → Reassociate() (ESP32 rebooted mid-operation)
 *   3. Transitions to WAITING_LINK if we're still in INITIALIZING.
 *
 * @note  Runs within the NetworkAO thread context.
 * @note  Does NOT call netInit() (already called by handle_msg_start()).
 * @note  Does NOT touch RNDIS/ETH ports (they don't require SPI transport).
 * @note  Works correctly regardless of FSM state — uses per-port configured flag.
 */
static void handle_msg_transport_ready(NetworkAO_t *self)
{
    /* Step 1: Mark all WiFi ports as transport-ready now that SPI is active */
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->port_configs[i] != NULL &&
            port_needs_spi_transport(self->port_configs[i]->port_type))
        {
            self->port_transport_ready[i] = true;
        }
    }

    /* Step 2: Process each WiFi port individually:
     *   - First boot (not configured) → Configure()
     *   - ESP32 reboot (already configured) → Reassociate() */
    bool any_reassociated = false;
    bool any_configured_now = false;

    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->port_configs[i] == NULL ||
            !port_needs_spi_transport(self->port_configs[i]->port_type))
        {
            continue; /* Skip non-WiFi ports */
        }

        if (!self->port_configured[i])
        {
            /* First boot: configure WiFi port now that SPI transport is ready */
            if (self->ports[i] != NULL)
            {
                if (NetworkPort_Configure(self->ports[i], self->port_configs[i]) == ERR_OK)
                {
                    self->port_configured[i] = true;
                    any_configured_now = true;
                }
                else
                {
                    if (self->logger != NULL)
                    {
                        LOG_ERROR(self->logger, "NETWORK_AO",
                                  "WiFi port %u Configure() failed", (unsigned)i);
                    }
                }
            }
        }
        else
        {
            /* ESP32 rebooted: re-apply WiFi credentials without full reconfiguration */
            if (self->ports[i] != NULL)
            {
                (void)NetworkPort_Reassociate(self->ports[i], self->port_configs[i]);
                any_reassociated = true;
            }
        }
    }

    /* Step 3: Transition to WAITING_LINK if:
     *   - We're still in INITIALIZING (first boot path), OR
     *   - We performed reassociate (ESP32 reboot — need to wait for reconnection) */
    if (self->fsm_state == NET_AO_STATE_INITIALIZING || any_reassociated || any_configured_now)
    {
        self->fsm_state = NET_AO_STATE_WAITING_LINK;
    }
}

/**
 * @brief Handles MSG_STOP: stops all services and signals graceful shutdown.
 */
static void handle_msg_reconfigure(NetworkAO_t *self)
{
    bool has_wifi_port = false;

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, "NETWORK_AO", "Applying runtime WiFi reconfiguration");
    }

    /* Only touch WiFi-related ports. Leave unrelated interfaces/services running. */
    for (uint8_t i = 0U; i < self->port_count; i++)
    {
        if (self->port_configs[i] == NULL)
        {
            continue;
        }

        if (port_needs_spi_transport(self->port_configs[i]->port_type))
        {
            has_wifi_port = true;
            self->port_configured[i] = false;
            self->port_transport_ready[i] = false; /* Wait for fresh TRANSPORT_READY after reset */
        }
    }

    if (!has_wifi_port)
    {
        return;
    }

    if ((self->wifi_module != NULL) && WifiModule_IsEnabled(self->wifi_module))
    {
        if ((self->wifi_transport != NULL) && WifiTransport_IsValid(self->wifi_transport))
        {
            Result_t res = WifiTransport_ResetESP32(self->wifi_transport, true);
            if (res == ERR_OK)
            {
                (void)os_thread_sleep(50U);
                res = WifiTransport_ResetESP32(self->wifi_transport, false);
            }

            if ((res != ERR_OK) && (self->logger != NULL))
            {
                LOG_ERROR(self->logger, "NETWORK_AO",
                          "WifiTransport_ResetESP32 failed during reconfiguration (%d)", res);
            }
        }
        else if (self->logger != NULL)
        {
            LOG_WARN(self->logger, "NETWORK_AO",
                     "WiFi transport not available; waiting for next manual enable/reset");
        }
    }
    else if (self->logger != NULL)
    {
        LOG_INFO(self->logger, "NETWORK_AO",
                 "WiFi module disabled; new WiFi parameters staged without reset");
    }
}

static void handle_msg_stop(NetworkAO_t *self)
{
    stop_all_services(self);
    self->fsm_state = NET_AO_STATE_IDLE;
    self->running = false; /* thread exits → signals stopped_sem */
}

/**
 * @brief Monitoring tick — executed every NET_AO_MONITOR_INTERVAL_MS when
 *        the queue receive times out.
 *
 * Drives link/IP polling transitions:
 *   WAITING_LINK     → WAITING_IP        (any link UP)
 *   WAITING_IP       → SERVICES_RUNNING  (any link has IP)
 *   SERVICES_RUNNING → LINK_DOWN         (all links dropped)
 *   LINK_DOWN        → WAITING_LINK      (immediate retry)
 */
static void handle_tick(NetworkAO_t *self)
{
    switch (self->fsm_state)
    {
    case NET_AO_STATE_WAITING_LINK:
        if (any_link_up(self))
        {
            self->fsm_state = NET_AO_STATE_WAITING_IP;
        }
        break;

    case NET_AO_STATE_WAITING_IP:
        if (any_has_ip(self))
        {
            start_all_services(self);
            self->fsm_state = NET_AO_STATE_SERVICES_RUNNING;
            /* Link established: reset reconnect backoff for next disconnect. */
            self->reconnect_backoff_ticks = NET_AO_RECONNECT_BACKOFF_INIT_TICKS;
            self->reconnect_ticks_remaining = 0U;
            self->reconnect_retry_count = 0U;
        }
        break;

    case NET_AO_STATE_SERVICES_RUNNING:
        if (!any_link_up(self))
        {
            enter_link_down(self);
        }
        break;

    case NET_AO_STATE_LINK_DOWN:
        /* Exponential backoff countdown before attempting reconnect. */
        if (self->reconnect_ticks_remaining > 0U)
        {
            self->reconnect_ticks_remaining--;
        }
        else
        {
            /* Trigger WiFi re-association on each registered port. */
            for (uint8_t i = 0U; i < self->port_count; i++)
            {
                if (self->ports[i] != NULL && self->port_configs[i] != NULL)
                {
                    (void)NetworkPort_Reconnect(self->ports[i], self->port_configs[i]);
                }
            }

            /* Advance backoff interval (exponential, capped). */
            self->reconnect_retry_count++;
            if (self->reconnect_backoff_ticks < NET_AO_RECONNECT_BACKOFF_MAX_TICKS / 2U)
            {
                self->reconnect_backoff_ticks *= 2U;
            }
            else
            {
                self->reconnect_backoff_ticks = NET_AO_RECONNECT_BACKOFF_MAX_TICKS;
            }

            /* Move to WAITING_LINK to poll the driver for link state. */
            self->fsm_state = NET_AO_STATE_WAITING_LINK;
        }
        break;

    default:
        /* No tick action in IDLE / INITIALIZING. */
        break;
    }
}

/* ===================================================================
 * AO thread entry
 * =================================================================== */

static void ao_thread_entry(void *arg)
{
    NetworkAO_t *self = (NetworkAO_t *)arg;

    while (self->running)
    {
        NetworkAoMsg_t msg;
        memset(&msg, 0, sizeof(msg));

        Result_t res = os_queue_receive(self->queue, &msg,
                                        NET_AO_MONITOR_INTERVAL_MS);
        if (res == ERR_OK)
        {
            /* Message received → dispatch */
            switch (msg.type)
            {
            case NET_AO_MSG_START:
                handle_msg_start(self);
                break;
            case NET_AO_MSG_STOP:
                handle_msg_stop(self);
                break;
            case NET_AO_MSG_RECONFIGURE_IP:
                handle_msg_reconfigure(self);
                break;
            case NET_AO_MSG_TERMINATE:
                self->running = false;
                break;
            case NET_AO_MSG_TRANSPORT_READY:
                /* Transport active: handler is port-aware (Configure or Reassociate per port). */
                handle_msg_transport_ready(self);
                break;
            case NET_AO_MSG_LINK_UP:
                /* Event-driven accelerator: WiFi STA connected, skip next tick. */
                if (self->fsm_state == NET_AO_STATE_WAITING_LINK ||
                    self->fsm_state == NET_AO_STATE_LINK_DOWN)
                {
                    self->fsm_state = NET_AO_STATE_WAITING_IP;
                }
                break;
            case NET_AO_MSG_LINK_DOWN:
                /* Event-driven accelerator: WiFi STA disconnected, stop services. */
                if (self->fsm_state == NET_AO_STATE_SERVICES_RUNNING)
                {
                    enter_link_down(self);
                }
                else if (self->fsm_state == NET_AO_STATE_WAITING_IP)
                {
                    enter_link_down(self);
                }
                break;
            default:
                break;
            }
        }
        else
        {
            /* ERR_TIMEOUT → monitoring tick */
            handle_tick(self);
        }
    }

    /* Signal WaitStopped() that the thread has exited. */
    (void)os_semaphore_put(self->stopped_sem);
}

/* ===================================================================
 * Public API
 * =================================================================== */

Result_t NetworkAO_Init(NetworkAO_t *self, const NetworkAO_Config_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (config->stack == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (config->port_count == 0U || config->port_count > NET_AO_MAX_PORTS)
    {
        return ERR_INVALID_PARAM;
    }
    if (config->service_count > NET_AO_MAX_SERVICES)
    {
        return ERR_INVALID_PARAM;
    }

    memset(self, 0, sizeof(*self));

    /* Copy injected interfaces */
    self->stack = config->stack;
    self->port_count = config->port_count;
    self->service_count = config->service_count;
    self->logger = config->logger;
    self->wifi_transport = config->wifi_transport;
    self->wifi_module = config->wifi_module;
    self->fsm_state = NET_AO_STATE_IDLE;
    self->running = false;
    self->initialized = false;
    self->needs_transport = config->needs_transport;

    /* Initialise reconnect backoff. */
    self->reconnect_backoff_ticks = NET_AO_RECONNECT_BACKOFF_INIT_TICKS;
    self->reconnect_ticks_remaining = 0U;
    self->reconnect_retry_count = 0U;

    for (uint8_t i = 0U; i < config->port_count; i++)
    {
        self->ports[i] = config->ports[i];
        self->links[i] = config->links[i];
        /* Copy the config struct internally (safe ownership — no dangling pointer risk) */
        if (config->port_configs[i] != NULL)
        {
            self->port_cfg_store[i] = *config->port_configs[i];
        }
        self->port_configs[i] = &self->port_cfg_store[i];
    }
    for (uint8_t i = 0U; i < config->service_count; i++)
    {
        self->services[i] = config->services[i];
    }

    /* Create stopped semaphore (initial count = 0 — must be Put before Get) */
    Result_t res = os_semaphore_create(&self->stopped_sem, "net_ao_stop", 0U);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Create message queue */
    os_queue_config_t q_cfg = {
        .name = "net_ao_q",
        .buffer = self->queue_buf,
        .buffer_size = sizeof(self->queue_buf),
        .item_size = sizeof(NetworkAoMsg_t),
    };
    res = os_queue_create(&self->queue, &q_cfg);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Create thread suspended — started by NetworkAO_Start() */
    os_thread_config_t t_cfg = {
        .name = "NetworkAO",
        .entry = ao_thread_entry,
        .arg = self,
        .stack_ptr = self->stack_mem,
        .stack_size = NET_AO_STACK_SIZE,
        .priority = NET_AO_PRIORITY,
        .auto_start = false,
    };
    res = os_thread_create(&self->thread, &t_cfg);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    self->initialized = true;
    return ERR_OK;
}

Result_t NetworkAO_Start(NetworkAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (!self->initialized)
    {
        return ERR_ERROR;
    }
    if (self->running)
    {
        return ERR_BUSY;
    }

    self->running = true;
    (void)os_thread_resume(self->thread);

    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = NET_AO_MSG_START;
    return NetworkAO_PostMsg(self, &msg);
}

Result_t NetworkAO_RequestReconfigure(NetworkAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    NetworkAoMsg_t msg = {
        .type = NET_AO_MSG_RECONFIGURE_IP,
    };

    return os_queue_send(self->queue, &msg, OS_NO_WAIT);
}

Result_t NetworkAO_Stop(NetworkAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = NET_AO_MSG_STOP;
    return NetworkAO_PostMsg(self, &msg);
}

Result_t NetworkAO_WaitStopped(NetworkAO_t *self, uint32_t timeout_ms)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t NetworkAO_Deinit(NetworkAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (self->running)
    {
        return ERR_BUSY;
    }
    (void)os_semaphore_delete(self->stopped_sem);
    (void)os_queue_delete(self->queue);
    (void)os_thread_terminate(self->thread);
    self->initialized = false;
    return ERR_OK;
}

Result_t NetworkAO_PostMsg(NetworkAO_t *self, const NetworkAoMsg_t *msg)
{
    if (self == NULL || msg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return os_queue_send(self->queue, msg, OS_NO_WAIT);
}

Result_t NetworkAO_NotifyLinkUp(NetworkAO_t *self)
{
    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = NET_AO_MSG_LINK_UP;
    return NetworkAO_PostMsg(self, &msg);
}

Result_t NetworkAO_NotifyLinkDown(NetworkAO_t *self)
{
    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = NET_AO_MSG_LINK_DOWN;
    return NetworkAO_PostMsg(self, &msg);
}

Result_t NetworkAO_NotifyTransportReady(NetworkAO_t *self)
{
    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = NET_AO_MSG_TRANSPORT_READY;
    return NetworkAO_PostMsg(self, &msg);
}

/* ===================================================================
 * Test-only API
 * =================================================================== */

#ifdef UNIT_TEST
Result_t NetworkAO_ProcessNextMsg_TestOnly(NetworkAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    NetworkAoMsg_t msg;
    memset(&msg, 0, sizeof(msg));
    Result_t res = os_queue_receive(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return ERR_BUSY; /* queue empty */
    }

    switch (msg.type)
    {
    case NET_AO_MSG_START:
        handle_msg_start(self);
        break;
    case NET_AO_MSG_STOP:
        handle_msg_stop(self);
        break;
    case NET_AO_MSG_RECONFIGURE_IP:
        handle_msg_reconfigure(self);
        break;
    case NET_AO_MSG_TERMINATE:
        self->running = false;
        break;
    case NET_AO_MSG_TRANSPORT_READY:
        /* Transport active: handler is port-aware (Configure or Reassociate per port). */
        handle_msg_transport_ready(self);
        break;
    case NET_AO_MSG_LINK_UP:
        if (self->fsm_state == NET_AO_STATE_WAITING_LINK ||
            self->fsm_state == NET_AO_STATE_LINK_DOWN)
        {
            self->fsm_state = NET_AO_STATE_WAITING_IP;
        }
        break;
    case NET_AO_MSG_LINK_DOWN:
        if (self->fsm_state == NET_AO_STATE_SERVICES_RUNNING)
        {
            enter_link_down(self);
        }
        else if (self->fsm_state == NET_AO_STATE_WAITING_IP)
        {
            enter_link_down(self);
        }
        break;
    default:
        break;
    }
    return ERR_OK;
}

void NetworkAO_Tick_TestOnly(NetworkAO_t *self)
{
    if (self != NULL)
    {
        handle_tick(self);
    }
}

void NetworkAO_SetState_TestOnly(NetworkAO_t *self, NetworkAoState_t state)
{
    if (self != NULL)
    {
        self->fsm_state = state;
    }
}

void NetworkAO_MarkPortConfigured_TestOnly(NetworkAO_t *self, uint8_t port_index)
{
    if (self != NULL && port_index < NET_AO_MAX_PORTS)
    {
        self->port_configured[port_index] = true;
        self->port_transport_ready[port_index] = true;
    }
}
#endif /* UNIT_TEST */
