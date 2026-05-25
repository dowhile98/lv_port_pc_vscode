/**
 * @file dependency_container_pc.c
 * @brief PC Dependency Injection container with mock services.
 *
 * Replaces dependency_container.c (4944 lines of STM32U5 HW wiring) with
 * PC-friendly implementations using mock interfaces.
 *
 * DI_InitUISubsystem() mirrors the real firmware wiring:
 *   UiAO_Init → InputRouter_Init → EezScreenRouter_Init →
 *   PresentationLayer_SetRouter → PresentationLayer_Init (all screens) →
 *   EezScreenRouter_SetScreen (all) → EezScreenRouter_SetInitialScreen(STARTUP)
 *
 * DI_StartActiveObjects() calls UiAO_Start() (creates FreeRTOS task).
 * ui_ao_thread_entry() runs after vTaskStartScheduler():
 *   lv_init → UI_Port_Disp_Init(SDL) → UI_Port_Indev_Init → ui_init → ui_actions_init → loop
 *
 * @note This file does NOT include dependency_container.h (BSP HAL dependency).
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "hal_types.h"
#include "interfaces/i_logger.h"
#include "interfaces/i_digital_input_source.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_wifi_status_source.h"
#include "interfaces/i_config_storage.h"

#include "application/activeobjects/ui_ao.h"

#include "infrastructure/presentation/input_router.h"
#include "infrastructure/presentation/screens/eez_screen_router.h"
#include "infrastructure/presentation/presentation_layer.h"
#include "presentation/ui/screens.h"
#include "infrastructure/presentation/screens/security/security_screen_presenter.h"

/* EEZ view headers — REQUIRED to get correct 64-bit pointer return types.
 * Without these, the compiler uses implicit int return (C89), truncating the
 * 64-bit pointer to 32 bits and causing a segfault when the view is used. */
#include "infrastructure/presentation/screens/home/eez_home_screen_view.h"
#include "infrastructure/presentation/screens/security/eez_security_screen_view.h"
#include "infrastructure/presentation/screens/menu/eez_menu_screen_view.h"
#include "infrastructure/presentation/screens/system_information/eez_system_information_screen_view.h"
#include "infrastructure/presentation/screens/wifi_module/eez_wifi_module_screen_view.h"
#include "infrastructure/presentation/screens/information_show/eez_information_show_screen_view.h"
#include "infrastructure/presentation/screens/qr_info/eez_qr_info_screen_view.h"
#include "infrastructure/presentation/screens/settings/eez_settings_screen_view.h"
#include "infrastructure/presentation/screens/wifi_configuration/eez_wifi_configuration_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration/eez_int_configuration_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_state/eez_int_configuration_state_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_start/eez_int_configuration_start_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_predefined/eez_int_configuration_predefined_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_days/eez_int_configuration_days_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_period/eez_int_configuration_period_screen_view.h"
#include "infrastructure/presentation/screens/int_configuration_period_menu/eez_int_configuration_period_menu_screen_view.h"
#include "infrastructure/presentation/screens/access_denied/eez_access_denied_screen_view.h"
#include "infrastructure/presentation/screens/configuration_input/eez_configuration_input_screen_view.h"
#include "infrastructure/presentation/screens/gps_configuration/eez_gps_configuration_screen_view.h"
#include "infrastructure/presentation/screens/gps_configuration_antenna/eez_gps_configuration_antenna_screen_view.h"
#include "infrastructure/presentation/screens/contact_configuration/eez_contact_configuration_screen_view.h"
#include "infrastructure/presentation/screens/contact_configuration_type/eez_contact_configuration_type_screen_view.h"
#include "infrastructure/presentation/screens/general_configuration/eez_general_configuration_screen_view.h"
#include "infrastructure/presentation/screens/general_configuration_alarm/eez_general_configuration_alarm_screen_view.h"
#include "infrastructure/presentation/screens/admin_configuration/eez_admin_configuration_screen_view.h"
#include "infrastructure/presentation/screens/admin_operation_mode/eez_admin_operation_mode_screen_view.h"

#include "pc_printf_logger.h"
#include "bsp/mock/mock_digital_input_source.h"
#include "bsp/mock/mock_bq27441_adapter.h"
#include "bsp/mock/mock_pc_adapters.h"
#include "pc_keyboard_input_adapter.h"
#include "pc_file_config_storage.h"
#include "pc_event_log_storage.h"

/* ---------------------------------------------------------------------------
 * Default security password — matches real firmware dependency_container.c
 * SEC_KEY_UP = 1, SEC_KEY_DOWN = 2
 * ------------------------------------------------------------------------- */
static const uint8_t s_default_password[] = {SEC_KEY_UP, SEC_KEY_DOWN, SEC_KEY_DOWN, SEC_KEY_UP};
/** @brief Super user password: DOWN UP DOWN DOWN UP DOWN (6 keys). */
static const uint8_t s_super_password[] = {SEC_KEY_DOWN, SEC_KEY_UP, SEC_KEY_DOWN, SEC_KEY_DOWN, SEC_KEY_UP, SEC_KEY_DOWN};
/** @brief Maintenance password: UP UP DOWN UP DOWN (5 keys). */
static const uint8_t s_maintenance_password[] = {SEC_KEY_UP, SEC_KEY_UP, SEC_KEY_DOWN, SEC_KEY_UP, SEC_KEY_DOWN};

/* ---------------------------------------------------------------------------
 * PC container struct — mirrors the real DependencyContainer_t subset needed
 * for UI wiring without STM32U5 HAL dependencies.
 * ------------------------------------------------------------------------- */
typedef struct
{
    /* Mock services */
    ILogger *logger;
    IDigitalInputSource *digital_input_source;
    ITimeSource *time_source;
    IGPSSource *gps_source;
    IWifiStatusSource_t *wifi_status;
    IConfigStorage *config_storage;

    /* Active Objects */
    UiAO_t ui_ao;

    /* Presentation routing */
    InputRouter_t input_router;
    EezScreenRouter_t eez_router;

    /* Battery monitor (Li-ion simulation) */
    MockBQ27441Adapter_t battery_monitor;

    /* Initialisation flags */
    bool logging_initialized;
    bool digital_input_initialized;
    bool battery_initialized;
    bool ui_initialized;
} DependencyContainer_t;

/* Single container instance — populated once by DI_Container_Init */
static DependencyContainer_t s_container;

/* PC printf logger adapter — stdio output with ANSI colors */
static PcPrintfLogger_t s_printf_logger;

/* PC keyboard input adapter — maps SDL keys to DI button events */
static PCKeyboardInputAdapter_t s_kbd_adapter;

/* ============================================================================
 * PRE-NAVIGATE HOOKS (mirror firmware dependency_container.c)
 * ========================================================================== */

static void system_info_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    PresentationLayer_SetPendingInfoItem(item_idx);
    PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_SYSTEM_INFORMATION);
}

/* Reset security to the default login context (password + screens). */
static inline void reset_security_context_to_default(void)
{
    PresentationLayer_SetSecurityContext(s_default_password,
                                         (uint8_t)sizeof(s_default_password),
                                         (uint8_t)SCREEN_ID_MENU,
                                         (uint8_t)SCREEN_ID_HOME);
}

static void menu_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    if (item_idx == 2U) /* 3. Historical Events */
    {
        PresentationLayer_SetPendingInfoItem(13U);
        PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_MENU);
    }
    else if (item_idx == 3U) /* 4. Super User Menu */
    {
        reset_security_context_to_default();
        PresentationLayer_SetSecurityContext(s_super_password,
                                             (uint8_t)sizeof(s_super_password),
                                             (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION,
                                             (uint8_t)SCREEN_ID_MENU);
    }
}

/* PC execute callbacks for destructive General Config actions -------------- */

static void execute_clear_events(void *ctx)
{
    (void)ctx;
    IEventLogStorage *log = PCEventLogStorage_GetInstance();
    (void)EventLogStorage_Clear(log);
    PresentationLayer_SetConfirmationMessage("Events cleared");
    PresentationLayer_SetPendingInfoItem(15U);
    PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

static void execute_restore_config(void *ctx)
{
    DependencyContainer_t *c = (DependencyContainer_t *)ctx;
    if (c == NULL)
        return;
    (void)ConfigStorage_ResetToDefaults(c->config_storage);
    PresentationLayer_SetConfirmationMessage("Config restored");
    PresentationLayer_SetPendingInfoItem(15U);
    PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

static void execute_reset_hourmeter(void *ctx)
{
    (void)ctx; /* No hourmeter AO on PC */
    PresentationLayer_SetConfirmationMessage("Hourmeter reset");
    PresentationLayer_SetPendingInfoItem(15U);
    PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

/* INT_CONFIGURATION pre-navigate: items 1 and 2 go to CONFIGURATION_INPUT. */
static void int_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    switch (item_idx)
    {
    case 1U: /* Start time */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_START_TIME, 0U,
            (uint8_t)SCREEN_ID_INT_CONFIGURATION);
        break;
    case 2U: /* Stop time */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_STOP_TIME, 0U,
            (uint8_t)SCREEN_ID_INT_CONFIGURATION);
        break;
    default:
        break;
    }
}

/* GPS_CONFIGURATION pre-navigate: items 1-3 go to CONFIGURATION_INPUT. */
static void gps_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    switch (item_idx)
    {
    case 1U: /* Time offset */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_GPS_TIME_OFFSET, 0U,
            (uint8_t)SCREEN_ID_GPS_CONFIGURATION);
        break;
    case 2U: /* UTC offset index */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_UTC_OFFSET_INDEX, 0U,
            (uint8_t)SCREEN_ID_GPS_CONFIGURATION);
        break;
    case 3U: /* Antenna auto-switch timeout */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_ANTENNA_SWITCH_TIMEOUT_MIN, 0U,
            (uint8_t)SCREEN_ID_GPS_CONFIGURATION);
        break;
    default:
        break;
    }
}

/* CONTACT_CONFIGURATION pre-navigate: items 1-2 go to CONFIGURATION_INPUT. */
static void contact_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    switch (item_idx)
    {
    case 1U: /* Comp. ON -> OFF */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_TON_MARGIN_MS, 0U,
            (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
        break;
    case 2U: /* Comp. OFF -> ON */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_TOFF_MARGIN_MS, 0U,
            (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
        break;
    default:
        break;
    }
}

/* GENERAL_CONFIGURATION pre-navigate. */
static void general_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    switch (item_idx)
    {
    case 0U: /* Screen ON time */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_SCREEN_TIMEOUT_S, 0U,
            (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
        break;
    case 1U: /* Buzzer sound time */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_BUZZER_ON_TIME_MS, 0U,
            (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
        break;
    case 3U: /* Delete events -> Security */
        reset_security_context_to_default();
        PresentationLayer_SetSecurityContext(s_maintenance_password,
                                             (uint8_t)sizeof(s_maintenance_password),
                                             (uint8_t)SCREEN_ID_INFORMATION_SHOW,
                                             (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
        PresentationLayer_SetSecuritySuccessAction(execute_clear_events, ctx);
        break;
    case 4U: /* Restore config -> Security */
        reset_security_context_to_default();
        PresentationLayer_SetSecurityContext(s_maintenance_password,
                                             (uint8_t)sizeof(s_maintenance_password),
                                             (uint8_t)SCREEN_ID_INFORMATION_SHOW,
                                             (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
        PresentationLayer_SetSecuritySuccessAction(execute_restore_config, ctx);
        break;
    case 5U: /* Reset hourmeter -> Security */
        reset_security_context_to_default();
        PresentationLayer_SetSecurityContext(s_maintenance_password,
                                             (uint8_t)sizeof(s_maintenance_password),
                                             (uint8_t)SCREEN_ID_INFORMATION_SHOW,
                                             (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
        PresentationLayer_SetSecuritySuccessAction(execute_reset_hourmeter, ctx);
        break;
    default:
        break;
    }
}

/* ADMIN_CONFIGURATION pre-navigate: item 1 goes to CONFIGURATION_INPUT. */
static void admin_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
    (void)ctx;
    if (item_idx == 1U) /* 2. Select end date */
    {
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_EXPIRATION_DATE, 0U,
            (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION);
    }
}

/* ============================================================================
 * CONTAINER LIFECYCLE
 * ========================================================================== */

Result_t DI_Container_Init(DependencyContainer_t *container, const void *bsp)
{
    (void)bsp;
    if (container == NULL)
        container = &s_container;

    memset(container, 0, sizeof(*container));

    /* Initialise printf logger (once — safe to call multiple times) */
    static bool s_logger_ready = false;
    if (!s_logger_ready)
    {
        PcPrintfLoggerConfig_t lcfg = {
            .min_level = LOG_LEVEL_DEBUG,
            .enable_colors = true,
            .show_file_line = true,
        };
        PcPrintfLogger_Init(&s_printf_logger, &lcfg);
        s_logger_ready = true;
    }
    container->logger = PcPrintfLogger_GetInterface(&s_printf_logger);
    container->digital_input_source = MockDigitalInputSource_GetInstance();
    container->time_source = MockTimeSource_GetInstance();
    container->gps_source = MockGpsSource_GetInstance();
    container->wifi_status = MockWifiStatusSource_GetInstance();
    container->config_storage = PCFileConfigStorage_GetInstance();
    LOG_INFO(container->logger, "DI", "Config storage: pc_file_config_storage ready");

    return ERR_OK;
}

Result_t DI_Container_Deinit(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}

/* ============================================================================
 * SUBSYSTEM INITIALISATION
 * ========================================================================== */

Result_t DI_InitStorageSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitGPSSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitTimeSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
/**
 * @brief FreeRTOS software timer callback — drives Li-ion discharge simulation.
 *
 * Called by the timer daemon task every second.  Updating the mock here keeps
 * the simulation decoupled from the LVGL/UI thread.
 */
static void battery_update_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (s_container.battery_initialized)
    {
        (void)MockBQ27441Adapter_Update(&s_container.battery_monitor);
    }
}

Result_t DI_InitBatteryMonitorSubsystem(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;
    if (container->battery_initialized)
        return ERR_OK;

    /* 1000 mAh design capacity, 3.7 V nominal, ~300 mA consumption */
    Result_t res = MockBQ27441Adapter_Init(&container->battery_monitor, 1000);
    if (res != ERR_OK)
        return res;

    container->battery_initialized = true;

    /* Drive the discharge simulation once per second via a FreeRTOS software
     * timer.  xTimerCreate + xTimerStart can be called before the scheduler
     * starts; the command is queued and processed when the timer daemon task
     * runs after vTaskStartScheduler(). */
    TimerHandle_t tmr = xTimerCreate("bat_sim",
                                     pdMS_TO_TICKS(1000U),
                                     pdTRUE, /* auto-reload */
                                     NULL,
                                     battery_update_timer_cb);
    if (tmr != NULL)
    {
        (void)xTimerStart(tmr, 0U);
    }

    return ERR_OK;
}
Result_t DI_InitDeviceIdentitySubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitLicenseSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
void *DI_GetLicenseStatus(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
Result_t DI_InitUpdateSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
void *DI_GetUpdateManager(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
Result_t DI_InitRelaySubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitBuzzerSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitHourmeterSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitTemperatureSensorSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitExtFlashSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
void *DI_GetExtFlashHandle(const DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
Result_t DI_InitEventNotifierSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitWifiSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitWifiEnableSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitSideButtonSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitNetworkSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_InitUsbRndisSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
void *DI_GetRndisPortInterface(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
Result_t DI_InitLedStatusSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 * Logger — uses PcPrintfLogger (stdio + ANSI colors)
 * ------------------------------------------------------------------------- */
Result_t DI_InitLoggingSubsystem(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;
    container->logger = PcPrintfLogger_GetInterface(&s_printf_logger);
    container->logging_initialized = true;
    LOG_INFO(container->logger, "DI", "PC simulator logger ready (stdio + ANSI colors)");
    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 * Digital Input — PC keyboard via SDL (mirrors DigitalInputAdapter on firmware)
 *
 * SDL scancode defaults:
 *   UP    = 82  (SDL_SCANCODE_UP)
 *   DOWN  = 81  (SDL_SCANCODE_DOWN)
 *   ENTER = 40  (SDL_SCANCODE_RETURN)
 *   BACK  = 41  (SDL_SCANCODE_ESCAPE)
 * ------------------------------------------------------------------------- */
Result_t DI_InitDigitalInputSubsystem(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;

    const PCKeyboardInputConfig_t kbd_cfg = {
        .scancode_up = 82,    /* SDL_SCANCODE_UP */
        .scancode_down = 81,  /* SDL_SCANCODE_DOWN */
        .scancode_enter = 40, /* SDL_SCANCODE_RETURN */
        .scancode_back = 41,  /* SDL_SCANCODE_ESCAPE */
    };

    Result_t res = PCKeyboardInputAdapter_Init(&s_kbd_adapter, &kbd_cfg);
    if (res != ERR_OK)
        return res;

    container->digital_input_source = PCKeyboardInputAdapter_GetInterface(&s_kbd_adapter);
    container->digital_input_initialized = true;
    LOG_INFO(container->logger, "DI", "Digital input: keyboard adapter ready "
                                      "(UP=82 DOWN=81 ENTER=40 BACK=41)");
    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 * UI — full presenter wiring matching the real firmware DI_InitUISubsystem.
 *
 * Order:
 *   1. UiAO_Init (SDL/LVGL hw config, task NOT started yet)
 *   2. InputRouter_Init (routes DI buttons to active screen)
 *   3. EezScreenRouter_Init (EEZ LVGL screen switching + InputRouter bridge)
 *   4. PresentationLayer_SetRouter (wire router before any Init)
 *   5. PresentationLayer_Init + Init* for all screens
 *   6. EezScreenRouter_SetScreen for all 26 screens
 *   7. EezScreenRouter_SetInitialScreen(STARTUP)
 *   8. container->ui_initialized = true
 * DI_StartActiveObjects() then calls UiAO_Start() to create the FreeRTOS task.
 * ------------------------------------------------------------------------- */

/* ============================================================================
 * GUARD FUNCTIONS — mirror firmware dependency_container.c guards exactly.
 *
 * ctx is IConfigStorage * (PCFileConfigStorage instance) in all cases.
 * Fail-open: return true on storage error so the user is never locked out by
 * a transient read failure.
 * ========================================================================== */

/**
 * @brief Guard for INT_CONFIGURATION ListNav.
 *        Item 0 (Interrupt State toggle) always allowed.
 *        Items 1-N blocked while relay.enabled != 0.
 */
static bool relay_guard_fn(uint8_t item_idx, void *ctx)
{
    if (item_idx == 0U)
    {
        return true; /* "1. Interrupt State" — never blocked */
    }
    IConfigStorage *storage = (IConfigStorage *)ctx;
    RelayConfig_t cfg;
    if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
    {
        return true; /* fail open */
    }
    return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied for INT_CONFIGURATION — sets return screen.
 */
static void int_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
    (void)item_idx;
    (void)ctx;
    PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_INT_CONFIGURATION);
    PresentationLayer_SetAccessDeniedMessage(NULL); /* default: "CYCLE IN PROGRESS" */
}

/**
 * @brief Guard for GPS_CONFIGURATION ListNav.
 *        All items blocked while relay.enabled != 0.
 */
static bool gps_guard_fn(uint8_t item_idx, void *ctx)
{
    (void)item_idx;
    IConfigStorage *storage = (IConfigStorage *)ctx;
    RelayConfig_t cfg;
    if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
    {
        return true; /* fail open */
    }
    return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied for GPS_CONFIGURATION — sets return screen.
 */
static void gps_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
    (void)item_idx;
    (void)ctx;
    PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_GPS_CONFIGURATION);
    PresentationLayer_SetAccessDeniedMessage(NULL);
}

/**
 * @brief Guard for CONTACT_CONFIGURATION ListNav.
 *        All items blocked while relay.enabled != 0.
 */
static bool contact_guard_fn(uint8_t item_idx, void *ctx)
{
    (void)item_idx;
    IConfigStorage *storage = (IConfigStorage *)ctx;
    RelayConfig_t cfg;
    if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
    {
        return true; /* fail open */
    }
    return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied for CONTACT_CONFIGURATION — sets return screen.
 */
static void contact_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
    (void)item_idx;
    (void)ctx;
    PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
    PresentationLayer_SetAccessDeniedMessage(NULL);
}

Result_t DI_InitUISubsystem(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;

    if (!container->digital_input_initialized)
        return ERR_INVALID_STATE;

    if (container->digital_input_source == NULL)
        return ERR_ERROR;

    /* ── 1. UiAO_Init (BSP SDL config, task created later) ─────────── */
    UiAO_Config_t ui_cfg = {
        .di_source = container->digital_input_source,
        .display_width = 794U,
        .display_height = 1123U,
    };
    Result_t res = UiAO_Init(&container->ui_ao, &ui_cfg);
    if (res != ERR_OK)
        return res;
    LOG_INFO(container->logger, "DI", "UiAO initialized (%ux%u)",
             ui_cfg.display_width, ui_cfg.display_height);

    /* ── 2. InputRouter — routes encoder buttons to active screen ───── */
    InputRouterConfig_t ir_cfg = {
        .di_source = container->digital_input_source,
    };
    res = InputRouter_Init(&container->input_router, &ir_cfg);
    if (res != ERR_OK)
        return res;

    /* ── 3. EEZ Screen Router ──────────────────────────────────────── */
    res = EezScreenRouter_Init(&container->eez_router, &container->input_router);
    if (res != ERR_OK)
        return res;

    IScreenRouter_t *router = EezScreenRouter_GetInterface(&container->eez_router);

    /* ── 4. Wire the router into PresentationLayer ──────────────────── */
    PresentationLayer_SetRouter(router);

    /* ── 5a. Home screen presenter ──────────────────────────────────── */
    HomeScreenPresenterDeps_t deps = {0};
    deps.view = EezHomeScreenView_GetInterface();
    deps.time_source = container->time_source;
    deps.gps_source = container->gps_source;
    deps.wifi_status = container->wifi_status;
    deps.config_storage = container->config_storage;
    deps.battery_monitor = MockBQ27441Adapter_GetInterface(&container->battery_monitor);
    deps.relay_controller = NULL;
    deps.router = router;
    deps.security_screen_id = (uint8_t)SCREEN_ID_SEGURITY;
    deps.di_source = container->digital_input_source;
    deps.pending_info_item_ptr = (uint8_t *)PresentationLayer_GetPendingInfoItemPtr();
    deps.info_back_screen_ptr = (uint8_t *)PresentationLayer_GetInfoBackScreenPtr();
    deps.alarm_screen_id = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    deps.update_period_ms = 300U;
    (void)PresentationLayer_Init(&deps);

    /* ── 5b. Security screen presenter ─────────────────────────────── */
    SecurityScreenPresenterDeps_t sec_deps = {0};
    sec_deps.view = EezSecurityScreenView_GetInterface();
    sec_deps.router = router;
    sec_deps.password = s_default_password;
    sec_deps.password_len = (uint8_t)sizeof(s_default_password);
    sec_deps.success_screen_id = (uint8_t)SCREEN_ID_MENU;
    sec_deps.cancel_screen_id = (uint8_t)SCREEN_ID_HOME;
    sec_deps.update_period_ms = 100U;
    (void)PresentationLayer_InitSecurity(&sec_deps);

    /* ── 5c. Menu screen presenter ──────────────────────────────────── */
    ListNavScreenPresenterDeps_t menu_deps = {0};
    menu_deps.view = EezMenuScreenView_GetInterface();
    menu_deps.router = router;
    menu_deps.item_count = 4U;
    menu_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_SETTINGS;
    menu_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
    menu_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    menu_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_SEGURITY;
    menu_deps.back_screen_id = (uint8_t)SCREEN_ID_HOME;
    menu_deps.pre_navigate_fn = menu_pre_navigate_fn;
    (void)PresentationLayer_InitMenu(&menu_deps);

    /* ── 5d. System Information screen presenter ────────────────────── */
    ListNavScreenPresenterDeps_t system_info_deps = {0};
    system_info_deps.view = EezSystemInformationScreenView_GetInterface();
    system_info_deps.router = router;
    system_info_deps.item_count = 8U;
    system_info_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    system_info_deps.item_screen_ids[7] = (uint8_t)SCREEN_ID_WIFI_MODULE;
    system_info_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
    system_info_deps.pre_navigate_fn = system_info_pre_navigate_fn;
    (void)PresentationLayer_InitSystemInformation(&system_info_deps);

    /* ── 5e. WiFi Module screen presenter ───────────────────────────── */
    ListNavScreenPresenterDeps_t wifi_module_deps = {0};
    wifi_module_deps.view = EezWifiModuleScreenView_GetInterface();
    wifi_module_deps.router = router;
    wifi_module_deps.item_count = 8U;
    wifi_module_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    wifi_module_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    wifi_module_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    wifi_module_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    wifi_module_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
    wifi_module_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_QR_INFO;
    wifi_module_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_QR_INFO;
    wifi_module_deps.item_screen_ids[7] = (uint8_t)SCREEN_ID_QR_INFO;
    wifi_module_deps.back_screen_id = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
    (void)PresentationLayer_InitWifiModule(&wifi_module_deps);

    /* ── 5f. Information Show screen presenter ──────────────────────── */
    InformationShowScreenPresenterDeps_t info_show_deps = {0};
    info_show_deps.view = EezInformationShowScreenView_GetInterface();
    info_show_deps.router = router;
    info_show_deps.back_screen_id = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
    info_show_deps.pending_item_ptr = PresentationLayer_GetPendingInfoItemPtr();
    info_show_deps.back_screen_id_ptr = PresentationLayer_GetInfoBackScreenPtr();
    info_show_deps.time_source = container->time_source;
    info_show_deps.config_storage = container->config_storage;
    info_show_deps.gps_source = container->gps_source;
    info_show_deps.wifi_status = container->wifi_status;
    info_show_deps.event_log_storage = PCEventLogStorage_GetInstance();
    info_show_deps.di_source = container->digital_input_source;
    info_show_deps.battery = MockBQ27441Adapter_GetInterface(&container->battery_monitor);
    info_show_deps.pending_confirmation_msg = PresentationLayer_GetConfirmationMessagePtr();
    (void)PresentationLayer_InitInformationShow(&info_show_deps);

    /* ── 5g. QR Info screen presenter ───────────────────────────────── */
    QrInfoScreenPresenterDeps_t qr_info_deps = {0};
    qr_info_deps.view = EezQrInfoScreenView_GetInterface();
    qr_info_deps.router = router;
    qr_info_deps.pending_item_ptr = PresentationLayer_GetPendingQrItemPtr();
    qr_info_deps.back_screen_id_ptr = PresentationLayer_GetQrBackScreenPtr();
    qr_info_deps.config_storage = container->config_storage;
    (void)PresentationLayer_InitQrInfo(&qr_info_deps);

    /* ── 5h. Settings screen presenter ──────────────────────────────── */
    ListNavScreenPresenterDeps_t settings_deps = {0};
    settings_deps.view = EezSettingsScreenView_GetInterface();
    settings_deps.router = router;
    settings_deps.item_count = 5U;
    settings_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    settings_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_GPS_CONFIGURATION;
    settings_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION;
    settings_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION;
    settings_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_WIFI_CONFIGURATION;
    settings_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
    (void)PresentationLayer_InitSettings(&settings_deps);

    /* ── 5i. WiFi Configuration screen presenter ─────────────────────  */
    WifiConfigurationScreenPresenterDeps_t wifi_config_deps = {0};
    wifi_config_deps.view = EezWifiConfigurationScreenView_GetInterface();
    wifi_config_deps.router = router;
    wifi_config_deps.wifi_module = NULL; /* no WiFi module on PC */
    wifi_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
    (void)PresentationLayer_InitWifiConfiguration(&wifi_config_deps);

    /* ── 5j. Interrupt Configuration screen presenter ────────────────── */
    ListNavScreenPresenterDeps_t int_config_deps = {0};
    int_config_deps.view = EezIntConfigurationScreenView_GetInterface();
    int_config_deps.router = router;
    int_config_deps.item_count = 7U;
    int_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_STATE;
    int_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    int_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    int_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_DAYS;
    int_config_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD;
    int_config_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_START;
    int_config_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PREDEFINED;
    int_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
    int_config_deps.guard_fn = relay_guard_fn;
    int_config_deps.guard_ctx = container->config_storage;
    int_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
    int_config_deps.on_guard_denied_fn = int_config_on_guard_denied_fn;
    int_config_deps.on_guard_denied_ctx = NULL;
    int_config_deps.pre_navigate_fn = int_config_pre_navigate_fn;
    (void)PresentationLayer_InitIntConfiguration(&int_config_deps);

    /* ── 5k. Int Configuration State screen presenter ───────────────── */
    IntConfigurationStateScreenPresenterDeps_t int_config_state_deps = {0};
    int_config_state_deps.view = EezIntConfigurationStateScreenView_GetInterface();
    int_config_state_deps.config_storage = container->config_storage;
    int_config_state_deps.router = router;
    int_config_state_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitIntConfigurationState(&int_config_state_deps);

    /* ── 5l. Int Configuration Start screen presenter ───────────────── */
    IntConfigurationStartScreenPresenterDeps_t int_config_start_deps = {0};
    int_config_start_deps.view = EezIntConfigurationStartScreenView_GetInterface();
    int_config_start_deps.config_storage = container->config_storage;
    int_config_start_deps.router = router;
    int_config_start_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitIntConfigurationStart(&int_config_start_deps);

    /* ── 5m. Int Configuration Predefined screen presenter ──────────── */
    IntConfigurationPredefinedScreenPresenterDeps_t int_config_predefined_deps = {0};
    int_config_predefined_deps.view = EezIntConfigurationPredefinedScreenView_GetInterface();
    int_config_predefined_deps.config_storage = container->config_storage;
    int_config_predefined_deps.router = router;
    int_config_predefined_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitIntConfigurationPredefined(&int_config_predefined_deps);

    /* ── 5n. Int Configuration Days screen presenter ────────────────── */
    IntConfigurationDaysScreenPresenterDeps_t int_config_days_deps = {0};
    int_config_days_deps.view = EezIntConfigurationDaysScreenView_GetInterface();
    int_config_days_deps.config_storage = container->config_storage;
    int_config_days_deps.router = router;
    int_config_days_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitIntConfigurationDays(&int_config_days_deps);

    /* ── 5o. Int Configuration Period screen presenter ──────────────── */
    IntConfigurationPeriodScreenPresenterDeps_t int_config_period_deps = {0};
    int_config_period_deps.view = EezIntConfigurationPeriodScreenView_GetInterface();
    int_config_period_deps.config_storage = container->config_storage;
    int_config_period_deps.router = router;
    int_config_period_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    int_config_period_deps.period_menu_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU;
    int_config_period_deps.set_period_menu_mode_fn = PresentationLayer_GetPeriodMenuSetModeFn();
    int_config_period_deps.set_period_menu_mode_ctx = PresentationLayer_GetPeriodMenuSetModeCtx();
    (void)PresentationLayer_InitIntConfigurationPeriod(&int_config_period_deps);

    /* ── 5p. Int Configuration Period Menu screen presenter ─────────── */
    IntConfigurationPeriodMenuScreenPresenterDeps_t int_config_period_menu_deps = {0};
    int_config_period_menu_deps.view = EezIntConfigurationPeriodMenuScreenView_GetInterface();
    int_config_period_menu_deps.config_storage = container->config_storage;
    int_config_period_menu_deps.router = router;
    int_config_period_menu_deps.period_roller_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD;
    int_config_period_menu_deps.config_input_screen_id = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    int_config_period_menu_deps.set_edit_context_fn = PresentationLayer_GetPeriodEditContextFn();
    int_config_period_menu_deps.set_edit_context_ctx = PresentationLayer_GetPeriodEditContextCtx();
    (void)PresentationLayer_InitIntConfigurationPeriodMenu(&int_config_period_menu_deps);

    /* ── 5q. Access Denied screen presenter ─────────────────────────── */
    AccessDeniedScreenPresenterDeps_t access_denied_deps = {0};
    access_denied_deps.view = EezAccessDeniedScreenView_GetInterface();
    access_denied_deps.router = router;
    access_denied_deps.buzzer = NULL; /* no buzzer on PC */
    access_denied_deps.return_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitAccessDenied(&access_denied_deps);

    /* ── 5r. Configuration Input screen presenter ───────────────────── */
    ConfigurationInputScreenPresenterDeps_t config_input_deps = {0};
    config_input_deps.view = EezConfigurationInputScreenView_GetInterface();
    config_input_deps.router = router;
    config_input_deps.config_storage = container->config_storage;
    config_input_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
    (void)PresentationLayer_InitConfigurationInput(&config_input_deps);

    /* ── 5s. GPS Configuration screen presenter ─────────────────────── */
    ListNavScreenPresenterDeps_t gps_config_deps = {0};
    gps_config_deps.view = EezGpsConfigurationScreenView_GetInterface();
    gps_config_deps.router = router;
    gps_config_deps.item_count = 4U;
    gps_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_GPS_CONFIGURATION_ANTENNA;
    gps_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    gps_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    gps_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    gps_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
    gps_config_deps.guard_fn = gps_guard_fn;
    gps_config_deps.guard_ctx = container->config_storage;
    gps_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
    gps_config_deps.on_guard_denied_fn = gps_config_on_guard_denied_fn;
    gps_config_deps.on_guard_denied_ctx = NULL;
    gps_config_deps.pre_navigate_fn = gps_config_pre_navigate_fn;
    (void)PresentationLayer_InitGpsConfiguration(&gps_config_deps);

    /* ── 5t. GPS Configuration Antenna screen presenter ─────────────── */
    GpsConfigurationAntennaScreenPresenterDeps_t gps_antenna_deps = {0};
    gps_antenna_deps.view = EezGpsConfigurationAntennaScreenView_GetInterface();
    gps_antenna_deps.config_storage = container->config_storage;
    gps_antenna_deps.router = router;
    gps_antenna_deps.back_screen_id = (uint8_t)SCREEN_ID_GPS_CONFIGURATION;
    (void)PresentationLayer_InitGpsConfigurationAntenna(&gps_antenna_deps);

    /* ── 5u. Contact Configuration screen presenter ──────────────────── */
    ListNavScreenPresenterDeps_t contact_config_deps = {0};
    contact_config_deps.view = EezContactConfigurationScreenView_GetInterface();
    contact_config_deps.router = router;
    contact_config_deps.item_count = 3U;
    contact_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION_TYPE;
    contact_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    contact_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    contact_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
    contact_config_deps.guard_fn = contact_guard_fn;
    contact_config_deps.guard_ctx = container->config_storage;
    contact_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
    contact_config_deps.on_guard_denied_fn = contact_config_on_guard_denied_fn;
    contact_config_deps.on_guard_denied_ctx = NULL;
    contact_config_deps.pre_navigate_fn = contact_config_pre_navigate_fn;
    (void)PresentationLayer_InitContactConfiguration(&contact_config_deps);

    /* ── 5v. Contact Configuration Type screen presenter ────────────── */
    ContactConfigurationTypeScreenPresenterDeps_t contact_type_deps = {0};
    contact_type_deps.view = EezContactConfigurationTypeScreenView_GetInterface();
    contact_type_deps.config_storage = container->config_storage;
    contact_type_deps.router = router;
    contact_type_deps.back_screen_id = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION;
    (void)PresentationLayer_InitContactConfigurationType(&contact_type_deps);

    /* ── 5w. General Configuration screen presenter ──────────────────── */
    ListNavScreenPresenterDeps_t general_config_deps = {0};
    general_config_deps.view = EezGeneralConfigurationScreenView_GetInterface();
    general_config_deps.router = router;
    general_config_deps.item_count = 6U;
    general_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    general_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    general_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION_ALARM;
    general_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_SEGURITY;
    general_config_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_SEGURITY;
    general_config_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_SEGURITY;
    general_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
    general_config_deps.pre_navigate_fn = general_config_pre_navigate_fn;
    general_config_deps.pre_navigate_ctx = container;
    (void)PresentationLayer_InitGeneralConfiguration(&general_config_deps);

    /* ── 5x. General Configuration Alarm screen presenter ───────────── */
    GeneralConfigurationAlarmScreenPresenterDeps_t general_config_alarm_deps = {0};
    general_config_alarm_deps.view = EezGeneralConfigurationAlarmScreenView_GetInterface();
    general_config_alarm_deps.config_storage = container->config_storage;
    general_config_alarm_deps.router = router;
    general_config_alarm_deps.back_screen_id = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION;
    (void)PresentationLayer_InitGeneralConfigurationAlarm(&general_config_alarm_deps);

    /* ── 5y. Admin Configuration screen presenter ───────────────────── */
    ListNavScreenPresenterDeps_t admin_config_deps = {0};
    admin_config_deps.view = EezAdminConfigurationScreenView_GetInterface();
    admin_config_deps.router = router;
    admin_config_deps.item_count = 2U;
    admin_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_ADMIN_OPERATION_MODE;
    admin_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
    admin_config_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
    admin_config_deps.pre_navigate_fn = admin_config_pre_navigate_fn;
    admin_config_deps.pre_navigate_ctx = NULL;
    (void)PresentationLayer_InitAdminConfiguration(&admin_config_deps);

    /* ── 5z. Admin Operation Mode screen presenter ───────────────────── */
    AdminOperationModeScreenPresenterDeps_t admin_op_mode_deps = {0};
    admin_op_mode_deps.view = EezAdminOperationModeScreenView_GetInterface();
    admin_op_mode_deps.config_storage = container->config_storage;
    admin_op_mode_deps.router = router;
    admin_op_mode_deps.back_screen_id = (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION;
    (void)PresentationLayer_InitAdminOperationMode(&admin_op_mode_deps);

    /* ── 6. Register screens in the EEZ router ──────────────────────── */
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_HOME,
                              PresentationLayer_GetHomeScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_SEGURITY,
                              PresentationLayer_GetSecurityScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_MENU,
                              PresentationLayer_GetMenuScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_SETTINGS,
                              PresentationLayer_GetSettingsScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_SYSTEM_INFORMATION,
                              PresentationLayer_GetSystemInformationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_WIFI_MODULE,
                              PresentationLayer_GetWifiModuleScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_WIFI_CONFIGURATION,
                              PresentationLayer_GetWifiConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INFORMATION_SHOW,
                              PresentationLayer_GetInformationShowScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_QR_INFO,
                              PresentationLayer_GetQrInfoScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION,
                              PresentationLayer_GetIntConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_STATE,
                              PresentationLayer_GetIntConfigurationStateScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_START,
                              PresentationLayer_GetIntConfigurationStartScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_PREDEFINED,
                              PresentationLayer_GetIntConfigurationPredefinedScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_DAYS,
                              PresentationLayer_GetIntConfigurationDaysScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD,
                              PresentationLayer_GetIntConfigurationPeriodScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU,
                              PresentationLayer_GetIntConfigurationPeriodMenuScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_GPS_CONFIGURATION,
                              PresentationLayer_GetGpsConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_GPS_CONFIGURATION_ANTENNA,
                              PresentationLayer_GetGpsConfigurationAntennaScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION_TYPE,
                              PresentationLayer_GetContactConfigurationTypeScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION,
                              PresentationLayer_GetContactConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION,
                              PresentationLayer_GetGeneralConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION_ALARM,
                              PresentationLayer_GetGeneralConfigurationAlarmScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION,
                              PresentationLayer_GetAdminConfigurationScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_ADMIN_OPERATION_MODE,
                              PresentationLayer_GetAdminOperationModeScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_ACCESSDENIED,
                              PresentationLayer_GetAccessDeniedScreen());
    EezScreenRouter_SetScreen(&container->eez_router, (uint8_t)SCREEN_ID_CONFIGURATION_INPUT,
                              PresentationLayer_GetConfigurationInputScreen());

    /* ── 7. Set initial screen — STARTUP has no presenter, boot animation
     *      runs before InputRouter routes any keys (same as real device). ── */
    EezScreenRouter_SetInitialScreen(&container->eez_router, (uint8_t)SCREEN_ID_STARTUP);

    container->ui_initialized = true;
    LOG_INFO(container->logger, "DI", "UI subsystem ready — all screens wired, initial=STARTUP");
    return ERR_OK;
}

/* ============================================================================
 * WIRING & EXECUTION
 * ========================================================================== */

Result_t DI_WireComponents(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_StartActiveObjects(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;

    if (container->ui_initialized)
    {
        LOG_INFO(container->logger, "DI", "Starting UiAO task...");
        Result_t res = UiAO_Start(&container->ui_ao);
        if (res != ERR_OK)
        {
            return res;
        }
        LOG_INFO(container->logger, "DI", "UiAO task started — scheduler will run UI loop");
    }
    return ERR_OK;
}
Result_t DI_StopActiveObjects(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}

/* ============================================================================
 * DE-INITIALISATION
 * ========================================================================== */

Result_t DI_DeinitNetworkSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitWifiSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitUISubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitLoggingSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitEventNotifierSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitBuzzerSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitHourmeterSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitDigitalInputSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitRelaySubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitTimeSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitGPSSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}
Result_t DI_DeinitStorageSubsystem(DependencyContainer_t *container)
{
    (void)container;
    return ERR_OK;
}

/* ============================================================================
 * GETTERS
 * ========================================================================== */

void *DI_GetTimeSource(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetBatteryMonitor(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;
    if (!container->battery_initialized)
        return NULL;
    return MockBQ27441Adapter_GetInterface(&container->battery_monitor);
}

void *DI_GetBatteryAdapterInstance(DependencyContainer_t *container)
{
    if (container == NULL)
        container = &s_container;
    if (!container->battery_initialized)
        return NULL;
    return &container->battery_monitor;
}
void *DI_GetDeviceIdentity(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetRelayController(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetGPSSource(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}

void *DI_GetDigitalInputSource(DependencyContainer_t *container)
{
    if (container == NULL)
        return NULL;
    return container->digital_input_source;
}

void *DI_GetModularConfigStorage(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetConfigStorage(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetBuzzerNotificationService(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetBuzzerControl(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}

void *DI_GetLogger(DependencyContainer_t *container)
{
    if (container == NULL)
        return NULL;
    return container->logger;
}

void *DI_GetEventNotifier(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetEventLogStorage(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetWifiTransport(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetWifiControl(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetWifiModule(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetWifiEnableService(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetHourmeter(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}
void *DI_GetTemperatureSensor(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}

void *DI_GetDisplay(DependencyContainer_t *container)
{
    (void)container;
    return NULL; /* Display managed by LVGL SDL driver, not via I_Display */
}

void *DI_GetBacklightService(DependencyContainer_t *container)
{
    (void)container;
    return NULL;
}

/* ---------------------------------------------------------------------------
 * PC-only getter: encoder interface from UiAO (after DI_InitUISubsystem).
 * ------------------------------------------------------------------------- */
IEncoder_t *DI_GetEncoderInterface_PC(void)
{
    return UiAO_GetEncoderInterface(&s_container.ui_ao);
}
