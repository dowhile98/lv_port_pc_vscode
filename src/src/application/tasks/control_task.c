/**
 * @file control_task.c
 * @brief Main Control Task - System Monitoring and Periodic Updates
 *
 * @note Priority 12 (between RelayAO=10 and GPSAo=15).
 * @note Runs every 100ms for system health monitoring.
 */

#include "application/tasks/control_task.h"
#include "infrastructure/startup/system_init.h"
#include "infrastructure/osal/osal.h"
#include "bsp/stm32u5/bsp_init.h"
#include "application/services/wifi_enable_service.h"

/* ===== Task Configuration ===== */
#ifndef CONTROL_TASK_STACK_SIZE
#define CONTROL_TASK_STACK_SIZE (1024 * 2)
#endif

#include "common/task_priorities.h" /* Mapa centralizado de prioridades */
#define CONTROL_TASK_PRIORITY TASK_PRIO_CONTROL

#ifndef CONTROL_TASK_PERIOD_MS
#define CONTROL_TASK_PERIOD_MS 20
#endif
/* ===== Transport-Active Flag ===== */
/* Set by DI_ESPHosted_EventHandler(TRANSPORT_ACTIVE) via DI container.
 * Exposed here so control_task can poll before sending control requests. */
#ifndef TRANSPORT_ACTIVE
#define TRANSPORT_ACTIVE 1 /* must match value used in hosted.h / spi_drv.c */
#endif

/* ===== Static Resources ===== */
static os_thread_t s_control_thread;
static uint8_t s_control_stack[CONTROL_TASK_STACK_SIZE];
static volatile bool s_task_running = false;
static ControlTaskDeps_t s_deps; /**< Static deps - survives thread lifetime */

/**
 * @brief Control thread entry point.
 * @param arg Pointer to ControlTaskDeps_t (injected via Init).
 * @note Runs every 100ms for system health monitoring.
 */
static void control_task_entry(void *arg)
{
    ControlTaskDeps_t *deps = (ControlTaskDeps_t *)arg;

    /* Optional: LED blink for visual heartbeat (500ms toggle) */
    s_task_running = true;

    /* ===== Wait for ESP-Hosted transport to become active =====
     * The PRIV_IF INIT event from ESP32 triggers TRANSPORT_ACTIVE.
     * control_path_task also needs ~3s to start + init time.
     * 12 seconds is safe margin: transport init (1-2s) + control_path startup (3s) + margin.
     * TODO: Replace with an event flag set by DI_ESPHosted_EventHandler(TRANSPORT_ACTIVE).
     */
    os_thread_sleep(8000);

    /* ===== Sync WiFi module state from persisted config (EEPROM) =====
     * Load wifi_enable + wifi_timeout_minutes and apply to ESP32 reset line.
     * Done here (after startup delay) to avoid blocking DI initialization.
     */
    if (deps != NULL && deps->wifi_service != NULL)
    {
        (void)WifiEnableService_SyncStartupState(deps->wifi_service);
    }

    /* ===== MAC address test — retry loop =====
     * ctrl_req_sem serialises all control requests (including those from
     * control_path_task at priority 10).  ctrl_app_send_req() waits up to
     * WAIT_TIME_B2B_CTRL_REQ (3 s) for the semaphore; if another request is
     * in progress it returns FAILURE immediately.  We retry several times
     * with a short back-off so a transient busy-semaphore is not fatal.
     */

    while (s_task_running)
    {
        /* ===== Core System Updates ===== */
        /* Calls TimeAdapter_Update(), CycleScheduler_UpdateMulticycleIndex() */
        System_Update();

        /* ===== Future Extensions ===== */
        // Battery_UpdateLevel(&g_battery_monitor);
        // EventLogger_FlushIfNeeded(&g_event_logger);
        // Watchdog_Kick(g_watchdog);
        // NetworkMonitor_CheckConnectivity(&g_network_monitor);

        /* Sleep until next period */
        os_thread_sleep(CONTROL_TASK_PERIOD_MS);
    }
}

/**
 * @brief Initialize and start control task.
 * @note Called from System_Init() after all adapters are initialized.
 *
 * @param[in] deps  Dependency bundle (can be NULL for minimal init).
 */
Result_t ControlTask_Init(const ControlTaskDeps_t *deps)
{
    /* Copy deps to static storage (survives thread lifetime) */
    if (deps != NULL)
    {
        s_deps = *deps;
    }
    else
    {
        s_deps.wifi_service = NULL;
    }

    os_thread_config_t cfg = {
        .name = "ControlTask",
        .entry = control_task_entry,
        .arg = &s_deps,
        .stack_ptr = s_control_stack,
        .stack_size = CONTROL_TASK_STACK_SIZE,
        .priority = CONTROL_TASK_PRIORITY,
        .auto_start = true};

    return os_thread_create(&s_control_thread, &cfg);
}

/**
 * @brief Stop the control task.
 * @note For testing/shutdown scenarios.
 */
Result_t ControlTask_Stop(void)
{
    s_task_running = false;

    /* Wait for task to finish (with timeout) */
    os_thread_sleep(200); /* 2 cycles max wait */

    return ERR_OK;
}
