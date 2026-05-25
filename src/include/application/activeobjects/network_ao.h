/**
 * @file network_ao.h
 * @brief Active Object de red: FSM de ciclo de vida de pila TCP/IP.
 *
 * @details
 * `NetworkAO_t` orquesta el ciclo de vida completo de la pila de red:
 *   1. Inicialización del stack TCP/IP  (`INetworkStack_t::Init()`)
 *   2. Configuración de puertos físicos (`INetworkPort_t::Configure()`)
 *   3. Espera de enlace físico          (`INetworkLink_t::IsLinkUp()`)
 *   4. Espera de IP asignada            (`INetworkLink_t::HasIpAddress()`)
 *   5. Arranque de servicios            (`INetworkService_t::Start()`)
 *   6. Recuperación ante caída de enlace (servicios parados, retry automático)
 *
 * @par Dependencias inyectadas (DIP)
 *   - `INetworkStack_t *`            — pila TCP/IP (CycloneTCP, mock…)
 *   - `INetworkPort_t *[ports]`      — puertos físicos (STA, AP…)
 *   - `INetworkLink_t *[links]`      — estado de enlace de cada puerto
 *   - `INetworkService_t *[svcs]`    — servicios de protocolo (HTTP, MQTT…)
 *
 * @par Threading (Pattern B)
 * El hilo interno hace `os_queue_receive(&queue, &msg, NET_AO_MONITOR_INTERVAL_MS)`.
 * Si recibe mensaje → despacha handler. Si timeout → ejecuta tick de monitoreo.
 *
 * @note SRP:  NetworkAO solo orquesta. No contiene lógica HTTP ni CycloneTCP.
 * @note DIP:  NUNCA incluye `stm32u5xx_hal.h` ni headers de terceros.
 * @note Mem:  Allocación 100% estática — todos los buffers embebidos en la struct.
 *
 * @version 2.0.0  Refactoring N.8 — eliminado acoplamiento WiFi
 * @date    2026-03-20
 *
 * @see docs/architecture/network/NETWORK_AO_ARCHITECTURE.md
 */

#ifndef NETWORK_AO_H
#define NETWORK_AO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_types.h"
#include "common/task_priorities.h"
#include "interfaces/i_network_stack.h"
#include "interfaces/i_network_port.h"
#include "interfaces/i_network_link.h"
#include "interfaces/i_network_service.h"
#include "interfaces/i_logger.h"
#include "interfaces/i_wifi_transport.h"
#include "interfaces/i_wifi_module.h"
#include "infrastructure/osal/osal.h"

    /* =========================================================================
     * Compile-time limits
     * ========================================================================= */

    /** Maximum number of physical ports managed by the AO. */
#ifndef NET_AO_MAX_PORTS
#define NET_AO_MAX_PORTS (4U)
#endif

    /** Maximum number of protocol services managed by the AO. */
#ifndef NET_AO_MAX_SERVICES
#define NET_AO_MAX_SERVICES (8U)
#endif

    /** Link/IP monitoring interval (ms) — queue receive timeout. */
#ifndef NET_AO_MONITOR_INTERVAL_MS
#define NET_AO_MONITOR_INTERVAL_MS (500U)
#endif

    /** Internal message queue depth. */
#ifndef NET_AO_QUEUE_SIZE
#define NET_AO_QUEUE_SIZE (8U)
#endif

    /** AO thread stack size (bytes). */
#ifndef NET_AO_STACK_SIZE
#define NET_AO_STACK_SIZE (4096U + 1024U)
#endif

    /** AO thread priority (ThreadX: 0 = highest). */
#ifndef NET_AO_PRIORITY
#define NET_AO_PRIORITY (TASK_PRIO_NETWORK)
#endif

    /**
     * @brief Initial reconnect backoff (in monitoring ticks).
     * @note  1 tick = NET_AO_MONITOR_INTERVAL_MS = 500 ms.
     *        2 ticks = 1 s first retry.
     */
#ifndef NET_AO_RECONNECT_BACKOFF_INIT_TICKS
#define NET_AO_RECONNECT_BACKOFF_INIT_TICKS (2U)
#endif

    /**
     * @brief Maximum reconnect backoff (in ticks).
     * @note  60 × 500 ms = 30 s max interval.
     */
#ifndef NET_AO_RECONNECT_BACKOFF_MAX_TICKS
#define NET_AO_RECONNECT_BACKOFF_MAX_TICKS (60U)
#endif

    /* =========================================================================
     * FSM States
     * ========================================================================= */

    /**
     * @brief NetworkAO finite-state machine state identifiers.
     */
    typedef enum
    {
        NET_AO_STATE_IDLE = 0,             /**< After Init(), before Start().              */
        NET_AO_STATE_TRANSPORT_INIT = 1,   /**< Waiting for ESP-Hosted transport ready.    */
        NET_AO_STATE_INITIALIZING = 2,     /**< netInit() + Port Configure in progress.    */
        NET_AO_STATE_WAITING_LINK = 3,     /**< Polling IsLinkUp() every 500 ms.           */
        NET_AO_STATE_WAITING_IP = 4,       /**< Polling HasIpAddress() every 500 ms.       */
        NET_AO_STATE_SERVICES_RUNNING = 5, /**< All services active.                       */
        NET_AO_STATE_LINK_DOWN = 6,        /**< Link lost — services stopped, retrying.    */
        NET_AO_STATE_COUNT,                /**< Sentinel — do not use as state.            */
    } NetworkAoState_t;

    /* =========================================================================
     * Message Types
     * ========================================================================= */

    /**
     * @brief Control messages posted to the AO queue.
     */
    typedef enum
    {
        NET_AO_MSG_NONE = 0,            /**< No-op / padding.                     */
        NET_AO_MSG_START = 1,           /**< NetworkAO_Start() posts this.        */
        NET_AO_MSG_STOP = 2,            /**< NetworkAO_Stop()  posts this.        */
        NET_AO_MSG_TERMINATE = 3,       /**< NetworkAO_Deinit() posts this.       */
        NET_AO_MSG_RECONFIGURE_IP = 4,  /**< Dynamic IP/DHCP change (OTA etc.).   */
        NET_AO_MSG_LINK_UP = 5,         /**< External notify: link came up.         */
        NET_AO_MSG_LINK_DOWN = 6,       /**< External notify: link went down.       */
        NET_AO_MSG_TRANSPORT_READY = 7, /**< ESP-Hosted SPI transport is ready.     */
    } NetworkAoMsgType_t;

    /**
     * @brief Message element stored in the AO queue.
     */
    typedef struct
    {
        NetworkAoMsgType_t type;
        union
        {
            uint32_t raw;                    /**< Generic 32-bit payload.               */
            NetworkPortConfig_t *new_config; /**< Payload for MSG_RECONFIGURE_IP.       */
        } payload;
    } NetworkAoMsg_t;

    /* =========================================================================
     * Configuration (injected at Init time)
     * ========================================================================= */

    /**
     * @brief Dependency-injection bag passed to NetworkAO_Init().
     *
     * @note `stack` and `port_count >= 1` are mandatory.
     *       `logger` and services are optional (may be NULL / 0).
     */
    typedef struct
    {
        INetworkStack_t *stack; /**< TCP/IP stack — mandatory (non-NULL).          */

        /** Physical ports to configure in INITIALIZING state. */
        INetworkPort_t *ports[NET_AO_MAX_PORTS];
        /** Link-state probes for each port. */
        INetworkLink_t *links[NET_AO_MAX_PORTS];
        /** Configuration for each port (passed to Configure()). */
        const NetworkPortConfig_t *port_configs[NET_AO_MAX_PORTS];
        uint8_t port_count; /**< Number of valid entries above. */

        /** Protocol services to start when IP is obtained. */
        INetworkService_t *services[NET_AO_MAX_SERVICES];
        uint8_t service_count;

        /**
         * @brief Set to true if any port requires the ESP-Hosted SPI transport
         *        to be active before netInit() and Configure() can proceed.
         *
         * @details When true (default for WiFi setups), handle_msg_start() calls
         *          INetworkStack_LaunchTransport() and waits for
         *          NET_AO_MSG_TRANSPORT_READY before configuring ports.
         *          When false (pure RNDIS / Ethernet setups), the AO transitions
         *          directly to INITIALIZING without waiting for any external event.
         */
        bool needs_transport;

        /** Optional logger (pass NULL to disable logging). */
        const ILogger *logger;

        /** Optional WiFi transport used for ESP32 reset during runtime WiFi reconfiguration. */
        IWifiTransport *wifi_transport;

        /** Optional WiFi module state source; reset only occurs when enabled. */
        IWifiModule_t *wifi_module;
    } NetworkAO_Config_t;

    /* =========================================================================
     * AO Instance (static allocation)
     * ========================================================================= */

    /**
     * @brief Active Object instance — allocate statically in the DI container.
     *
     * Private fields must not be accessed directly except via the public API.
     */
    typedef struct
    {
        /* --- OSAL primitives (private) --- */
        os_thread_t thread;
        os_queue_t queue;
        os_semaphore_t stopped_sem; /**< Signalled by thread when it exits.    */

        /* --- Static buffers (private) --- */
        uint8_t stack_mem[NET_AO_STACK_SIZE];
        uint8_t queue_buf[NET_AO_QUEUE_SIZE * sizeof(NetworkAoMsg_t)];

        /* --- Injected interfaces (private) --- */
        INetworkStack_t *stack;
        INetworkPort_t *ports[NET_AO_MAX_PORTS];
        INetworkLink_t *links[NET_AO_MAX_PORTS];
        NetworkPortConfig_t port_cfg_store[NET_AO_MAX_PORTS]; /**< Internal copies of port configs. */
        const NetworkPortConfig_t *port_configs[NET_AO_MAX_PORTS];
        uint8_t port_count;

        INetworkService_t *services[NET_AO_MAX_SERVICES];
        uint8_t service_count;

        const ILogger *logger;
        IWifiTransport *wifi_transport;
        IWifiModule_t *wifi_module;

        /* --- FSM state (private) --- */
        NetworkAoState_t fsm_state;

        /* --- Per-port state (private) --- */
        /**
         * @brief Transport readiness tracking for each port.
         *
         * true → port's external transport is ready (SPI link active for WiFi, or
         *        port doesn't need external transport like RNDIS/Ethernet).
         * false → port requires transport that is not yet ready.
         *
         * @note WiFi ports (STA/AP) require ESP-Hosted SPI transport to be active
         *       before NetworkPort_Configure() can be called.
         *       RNDIS/Ethernet ports are always ready (no external transport).
         */
        bool port_transport_ready[NET_AO_MAX_PORTS];

        /**
         * @brief Configuration status for each port.
         *
         * true → NetworkPort_Configure() has been successfully called for port[i].
         * false → port not yet configured.
         *
         * @note Once a port is configured, the AO can transition to WAITING_LINK
         *       even if other ports are still waiting for their transport.
         */
        bool port_configured[NET_AO_MAX_PORTS];

        /* --- Reconnect backoff (private) --- */
        /** Backoff interval in ticks — doubles on each failed attempt, capped. */
        uint16_t reconnect_backoff_ticks;
        /** Countdown to next ConnectAP() call. Set to backoff_ticks on LINK_DOWN entry. */
        uint16_t reconnect_ticks_remaining;
        /** Total reconnect attempts since last successful IP acquisition. */
        uint16_t reconnect_retry_count;

        /* --- Lifecycle flags (private) --- */
        volatile bool running; /**< true while AO thread is active.       */
        bool initialized;      /**< true after successful Init().          */
        bool needs_transport;  /**< true → wait for TRANSPORT_READY before init. */
    } NetworkAO_t;

    /* =========================================================================
     * Public API
     * ========================================================================= */

    /**
     * @brief Initialises the AO with its injected dependencies.
     *
     * Creates the internal OSAL queue and semaphore. Does NOT start the thread.
     * Call NetworkAO_Start() to begin operation.
     *
     * @param[in,out] self    AO instance (non-NULL, statically allocated).
     * @param[in]     config  Dependency bag (non-NULL; stack + port_count >= 1).
     *
     * @return ERR_OK             on success.
     * @return ERR_NULL_POINTER   if self, config, or config->stack is NULL.
     * @return ERR_INVALID_PARAM  if config->port_count == 0 or exceeds max.
     * @return ERR_ERROR          if OSAL resource creation fails.
     *
     * @note Thread-safety: must be called before any other API. Not re-entrant.
     */
    Result_t NetworkAO_Init(NetworkAO_t *self, const NetworkAO_Config_t *config);

    /**
     * @brief Starts the AO thread and initiates the network bring-up sequence.
     *
     * Posts MSG_START to the internal queue and resumes the thread.
     *
     * @param[in] self  Initialised AO instance.
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_ERROR        if NetworkAO_Init() was not called.
     * @return ERR_BUSY         if the AO is already running.
     *
     * @note Thread-safety: safe to call from any thread after Init() returns.
     */
    Result_t NetworkAO_Start(NetworkAO_t *self);

    /**
     * @brief Requests a runtime network reconfiguration using the latest port configs.
     *
     * Posts NET_AO_MSG_RECONFIGURE_IP to the internal queue. Intended for
     * non-blocking application of WiFi/IP changes after config storage observers
     * update the AO internal port configuration copy.
     *
     * @param[in] self  Initialised AO instance.
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_ERROR        if NetworkAO_Init() was not called.
     * @return ERR_BUSY         if the internal queue is full.
     */
    Result_t NetworkAO_RequestReconfigure(NetworkAO_t *self);

    /**
     * @brief Requests a graceful shutdown — stops services and signals the thread.
     *
     * Posts MSG_STOP. Returns immediately; use NetworkAO_WaitStopped() to block
     * until the thread has fully stopped.
     *
     * @param[in] self  Running AO instance.
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self is NULL.
     *
     * @note ISR-safe: posts to queue with OS_NO_WAIT.
     */
    Result_t NetworkAO_Stop(NetworkAO_t *self);

    /**
     * @brief Blocks the caller until the AO thread has fully stopped.
     *
     * Must be called after NetworkAO_Stop() before NetworkAO_Deinit().
     *
     * @param[in] self        AO instance.
     * @param[in] timeout_ms  Maximum wait time in milliseconds.
     *
     * @return ERR_OK           when the thread stopped within the timeout.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_TIMEOUT      if the thread did not stop within timeout_ms.
     */
    Result_t NetworkAO_WaitStopped(NetworkAO_t *self, uint32_t timeout_ms);

    /**
     * @brief Releases all OSAL resources allocated by NetworkAO_Init().
     *
     * The AO must be stopped before calling Deinit.
     *
     * @param[in] self  Stopped AO instance.
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_BUSY         if the AO thread is still running.
     */
    Result_t NetworkAO_Deinit(NetworkAO_t *self);

    /**
     * @brief Posts an arbitrary control message to the AO queue.
     *
     * @param[in] self  Running AO instance.
     * @param[in] msg   Message to post (non-NULL).
     *
     * @return ERR_OK           on success.
     * @return ERR_NULL_POINTER if self or msg is NULL.
     * @return ERR_BUSY         if the queue is full (OS_NO_WAIT).
     *
     * @note ISR-safe: uses OS_NO_WAIT.
     */
    Result_t NetworkAO_PostMsg(NetworkAO_t *self, const NetworkAoMsg_t *msg);

    /**
     * @brief Event-driven notification: a link came up.
     *
     * Posts NET_AO_MSG_LINK_UP. The AO transitions WAITING_LINK → WAITING_IP
     * (or LINK_DOWN → WAITING_IP) without waiting for the next 500 ms tick.
     *
     * @note ISR-safe (OS_NO_WAIT). Typically wired by the DI container via
     *       IWifiControl_OnStaConnected().
     *
     * @param[in] self  Running AO instance (non-NULL).
     * @return ERR_OK on success.
     */
    Result_t NetworkAO_NotifyLinkUp(NetworkAO_t *self);

    /**
     * @brief Event-driven notification: a link went down.
     *
     * Posts NET_AO_MSG_LINK_DOWN. The AO stops services and transitions to
     * LINK_DOWN immediately, without waiting for the next 500 ms tick.
     *
     * @note ISR-safe (OS_NO_WAIT). Typically wired by the DI container via
     *       IWifiControl_OnStaDisconnected().
     *
     * @param[in] self  Running AO instance (non-NULL).
     * @return ERR_OK on success.
     */
    Result_t NetworkAO_NotifyLinkDown(NetworkAO_t *self);

    /**
     * @brief Event-driven notification: ESP-Hosted SPI transport is ready.
     *
     * Posts NET_AO_MSG_TRANSPORT_READY. The AO transitions TRANSPORT_INIT →
     * INITIALIZING, runs netInit() + port configure, then WAITING_LINK.
     *
     * @note ISR-safe (OS_NO_WAIT). Wired by DI container via
     *       IWifiControl_OnTransportReady().
     *
     * @param[in] self  Running AO instance (non-NULL).
     * @return ERR_OK on success.
     */
    Result_t NetworkAO_NotifyTransportReady(NetworkAO_t *self);

    /* =========================================================================
     * Test-only API  (UNIT_TEST builds only — do NOT call in production)
     * ========================================================================= */

#ifdef UNIT_TEST
    /**
     * @brief Dequeues and processes one message synchronously.
     *
     * @param[in] self  Initialised AO instance.
     * @return ERR_OK   if a message was dequeued and dispatched.
     * @return ERR_BUSY if the queue was empty.
     */
    Result_t NetworkAO_ProcessNextMsg_TestOnly(NetworkAO_t *self);

    /**
     * @brief Executes one monitoring tick synchronously.
     *
     * Simulates the 500ms queue-receive timeout, polling IsLinkUp() /
     * HasIpAddress() and driving FSM state transitions.
     *
     * @param[in] self  Initialised AO instance.
     */
    void NetworkAO_Tick_TestOnly(NetworkAO_t *self);

    /**
     * @brief Directly sets the FSM state (for isolated transition tests).
     *
     * @param[in] self   Initialised AO instance.
     * @param[in] state  Target state.
     */
    void NetworkAO_SetState_TestOnly(NetworkAO_t *self, NetworkAoState_t state);

    /**
     * @brief Marks a port as already configured (for ESP32 reboot simulation tests).
     *
     * @param[in,out] self       Initialised AO instance.
     * @param[in]     port_index Port index to mark as configured.
     */
    void NetworkAO_MarkPortConfigured_TestOnly(NetworkAO_t *self, uint8_t port_index);
#endif /* UNIT_TEST */

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_AO_H */
