/**
 * @file home_screen_presenter.c
 * @brief Home screen presenter — display logic only (ACTION-030).
 *
 * Data sources and update strategy:
 *   - home_time         : ITimeSource, every OnUpdate (always live)
 *   - home_wifi_status  : IWifiStatusSource_t, every OnUpdate (always live)
 *   - home_gps_status   : IGPSSource::GetFixStatus, every OnUpdate; counter-based 200ms blink
 *   - home_relay_state  : EVENT-DRIVEN — RelayStateChangeCallback_t fires from ISR/RelayAO
 *   - home_antenna_icon + config fields : loaded on OnEnter; EVENT-DRIVEN refresh via
 *                         ConfigChangedCallback_t — fires from StorageCoordinatorAO thread
 *
 * ## Event-driven mechanism (ACTION-030)
 *
 * OnEnter subscribes to two push sources:
 *   1. IConfigChangeNotifier  → on_config_changed()  fires from AO thread
 *   2. IRelayController        → on_relay_state_changed() fires from ISR/RelayAO
 *
 * Both callbacks write to HomePendingEvents_t.volatile fields and signal event_flags
 * (if wired). OnUpdate drains the flags and calls the appropriate view setters.
 * OnExit unsubscribes both callbacks.
 *
 * If event_flags is NULL (PC tests), control-task waking is silently skipped;
 * presenter still processes pending flags in all OnUpdate calls.
 *
 * @note NO #include "lvgl.h" — intentional. Must remain LVGL-free for PC testability.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/home/home_screen_presenter.h"
#include "infrastructure/presentation/presentation_layer.h"
#include "interfaces/i_battery_monitor.h"
#include "interfaces/i_digital_input_source.h"
#include "common/general_types.h"
#include "infrastructure/osal/osal.h"
#include <string.h>
#include "lwprintf/lwprintf.h" /* lwprintf_snprintf */
#include <inttypes.h>          /* PRIu32 */

/** @brief High-temp alarm item index in InformationShowScreenPresenter. */
#define INFORMATION_SHOW_ITEM_ALARM 14U

/** @brief Half-period of the GPS offline blink animation (ms). Full period = 2× this value. */
#define GPS_BLINK_HALF_PERIOD_MS 200U

/** @brief Period (ms) between forced config re-reads in on_update.
 *  Ensures changes from the web server or other sources are reflected on screen
 *  even when IConfigChangeNotifier is not wired (v2 storage adapter limitation). */
#define CONFIG_POLL_PERIOD_MS 500U

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

/**
 * @brief Populate all config-derived fields from self->s_config.
 * @param[in] self  Presenter instance (must not be NULL).
 */
static void paint_config_fields(HomeScreenPresenter_t *self)
{
    RelayConfig_t relay_cfg = {0};
    GPSConfig_t gps_cfg = {0};

    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &gps_cfg);

    const RelayConfig_t *r = &relay_cfg;

    IHomeScreenView_SetCycleStatus(self->view, r->enabled ? "enabled" : "disabled");

    IHomeScreenView_SetContactType(self->view,
                                   (r->contact_type == RELAY_TYPE_NC) ? "Normally closed" : "Normally open");

    char buf[32];
    (void)lwprintf_snprintf(buf, sizeof(buf), "%" PRIu32 " ms", r->simple_cycle.ton);
    IHomeScreenView_SetOnTime(self->view, buf);
    (void)lwprintf_snprintf(buf, sizeof(buf), "%" PRIu32 " ms", r->simple_cycle.toff);
    IHomeScreenView_SetOffTime(self->view, buf);

    (void)lwprintf_snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                            (unsigned)r->time_window.start_time.hour,
                            (unsigned)r->time_window.start_time.minute,
                            (unsigned)r->time_window.start_time.second);
    IHomeScreenView_SetStartTime(self->view, buf);

    (void)lwprintf_snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                            (unsigned)r->time_window.stop_time.hour,
                            (unsigned)r->time_window.stop_time.minute,
                            (unsigned)r->time_window.stop_time.second);
    IHomeScreenView_SetStopTime(self->view, buf);

    IHomeScreenView_SetStartOn(self->view, r->start_with_on ? "yes" : "no");

    /* Determine antenna icon based on config + auto-switch service state */
    HomeAntennaIconId_t antenna_icon;
    if (self->gps_auto_switch_service != NULL)
    {
        GpsAntennaAutoSwitchStatus_t switch_status;
        if (GpsAntennaAutoSwitchService_GetStatus(self->gps_auto_switch_service, &switch_status) == ERR_OK)
        {
            if (switch_status.state == GPS_AUTO_SWITCH_STATE_COOLDOWN)
            {
                /* Both antennas failed → show FAILED icon */
                antenna_icon = HOME_ANTENNA_ICON_FAILED;
            }
            else
            {
                /* Normal operation → show current antenna type */
                antenna_icon = (gps_cfg.antenna_type == GPS_ANTENNA_EXTERNAL)
                                   ? HOME_ANTENNA_ICON_EXTERNAL
                                   : HOME_ANTENNA_ICON_INTERNAL;
            }
        }
        else
        {
            /* Service error → fallback to config value */
            antenna_icon = (gps_cfg.antenna_type == GPS_ANTENNA_EXTERNAL)
                               ? HOME_ANTENNA_ICON_EXTERNAL
                               : HOME_ANTENNA_ICON_INTERNAL;
        }
    }
    else
    {
        /* Service not available → use config value */
        antenna_icon = (gps_cfg.antenna_type == GPS_ANTENNA_EXTERNAL)
                           ? HOME_ANTENNA_ICON_EXTERNAL
                           : HOME_ANTENNA_ICON_INTERNAL;
    }

    IHomeScreenView_SetAntennaIcon(self->view, antenna_icon);
}

/*============================================================================*
 * PRIVATE — event callbacks (registered in OnEnter, unregistered in OnExit)
 *============================================================================*/

/**
 * @brief Called by StorageCoordinatorAO when new config is committed to RAM cache.
 *
 * Runs in StorageCoordinatorAO THREAD context — NOT an ISR.
 * Sets the config_changed pending flag and signals the control task event_flags.
 *
 * @param[in] ctx        HomeScreenPresenter_t * (registered as context).
 * @param[in] new_config New configuration (not used here — re-read from cache in OnUpdate).
 */
static void __attribute__((unused)) on_config_changed(void *ctx, const void *new_config)
{
    (void)new_config; /* Will re-read from IConfigStorage (RAM cache) in OnUpdate */
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)ctx;
    self->pending.config_changed = true;
    if (self->event_flags != NULL)
    {
        (void)os_event_flags_set(self->event_flags, HOME_EVENT_CONFIG_CHANGED, OS_FLAGS_OR);
    }
}

/**
 * @brief Called when the relay contact state changes.
 *
 * Runs in Timer OC ISR context or RelayAO thread — MUST be ISR-safe.
 * Only performs volatile writes and ISR-safe os_event_flags_set (no blocking).
 *
 * @param[in] ctx        HomeScreenPresenter_t * (registered as context).
 * @param[in] old_state  Previous contact state (unused — only new state shown).
 * @param[in] new_state  New contact state (RELAY_CONTACT_OPEN / RELAY_CONTACT_CLOSED).
 * @param[in] timestamp  Event timestamp in ms (unused by presenter).
 */
static void on_relay_state_changed(void *ctx,
                                   RelayContactState_t old_state,
                                   RelayContactState_t new_state,
                                   uint32_t timestamp)
{
    (void)old_state;
    (void)timestamp;
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)ctx;
    self->pending.relay_new_state = new_state; /* single-byte enum — atomic on ARM */
    self->pending.relay_changed = true;
    if (self->event_flags != NULL)
    {
        (void)os_event_flags_set(self->event_flags, HOME_EVENT_RELAY_CHANGED, OS_FLAGS_OR);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

/**
 * @brief Called when the Home screen begins loading.
 *
 * Loads config once, paints all static fields, subscribes to push sources.
 * Falls back to safe defaults if config storage fails.
 */
static void on_enter(IScreen_t *base)
{
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)base;

    /* Clear all pending events from any previous session */
    self->pending.config_changed = false;
    self->pending.relay_changed = false;
    self->pending.relay_new_state = RELAY_CONTACT_OPEN;

    /* ── Alarm inhibit: detect return-from-alarm via back press ── */
    if (self->alarm_nav_active)
    {
        self->alarm_back_inhibit = true;
        self->alarm_inhibit_start_ticks = os_ticks_get();
        self->alarm_nav_active = false;
    }

    /* Initial time placeholder */
    IHomeScreenView_SetTime(self->view, "--:--:--");

    /* WiFi: seed view with current status (same source as on_update) */
    IHomeScreenView_SetWifiStatus(self->view,
                                  WifiStatusSource_IsConnected(self->wifi_status));

    /* Battery: no sensor yet */
    IHomeScreenView_SetBatteryLevel(self->view, -1);

    /* Load config fields (modular v2.0) */
    paint_config_fields(self);
    self->config_poll_ticks = os_ticks_get(); /* arm the periodic timer from now */

    /*TODO: Subscribe to config changes (optional — NULL = no subscription in tests) */

    /* Subscribe to relay state changes (optional — NULL = no subscription in tests) */
    if (self->relay_controller != NULL)
    {
        (void)RelayController_RegisterStateChangeCallback(
            self->relay_controller, on_relay_state_changed, self);
    }

    /* Reset GPS blink counters */
    self->s_gps_blink_tick = 0u;
    self->s_gps_blink_counter = 0u;
}

/**
 * @brief Called when the Home screen unloads.
 *
 * Unregisters all callbacks to prevent dangling references.
 */
static void screen_on_exit(IScreen_t *base)
{
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)base;

    //    if (self->config_change_notifier != NULL)
    //    {
    ////        (void)IConfigChangeNotifier_Unsubscribe(self->config_change_notifier);
    //    }

    if (self->relay_controller != NULL)
    {
        (void)RelayController_UnregisterStateChangeCallback(self->relay_controller);
    }
}

/**
 * @brief Refresh live data — called every 100 ms from Control thread.
 *
 * 1. Always: time, WiFi status, GPS blink.
 * 2. On pending.relay_changed: update relay state indicator.
 * 3. On pending.config_changed: reload config from RAM cache, repaint config fields.
 *
 * All sources fail gracefully (display left unchanged on error).
 */
static void on_update(IScreen_t *base)
{
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)base;
    uint32_t ticks = 0;
    // verify time window
    static uint32_t update_ticks = 0;

    if ((os_ticks_get() - update_ticks) < 100)
    {
        return;
    }

    update_ticks = os_ticks_get();
    /* --- Time (every call) --- */
    DateTime_t now;
    if (TimeSource_GetTime(self->time_source, &now) == ERR_OK)
    {
        char buf[16];
        (void)lwprintf_snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                                (unsigned)now.hour,
                                (unsigned)now.minute,
                                (unsigned)now.second);
        IHomeScreenView_SetTime(self->view, buf);
    }

    /* --- WiFi status (every call) --- */
    IHomeScreenView_SetWifiStatus(self->view,
                                  WifiStatusSource_IsConnected(self->wifi_status));

    /* --- GPS blink / online (counter-based, period = 2×GPS_BLINK_HALF_PERIOD_MS) --- */
    {
        GPSFixStatus_t fix = GPS_FIX_NONE;
        (void)GPS_Source_GetFixStatus(self->gps_source, &fix);

        if (fix == GPS_FIX_3D)
        {
            self->s_gps_blink_counter = 0u;
            IHomeScreenView_SetGpsIcon(self->view, HOME_GPS_ICON_ONLINE);
        }
        else
        {
            /*get system ticks*/
            ticks = os_ticks_get();
            /*verify*/
            if ((ticks - self->s_gps_blink_counter) >= (self->update_period_ms))
            {
                self->s_gps_blink_counter = ticks;
                self->s_gps_blink_tick ^= 1u;

                // update icon
                IHomeScreenView_SetGpsIcon(self->view,
                                           (self->s_gps_blink_tick == 0u)
                                               ? HOME_GPS_ICON_OFFLINE_1
                                               : HOME_GPS_ICON_OFFLINE_2);
            }
        }
    }

    /* --- Relay state (event-driven: only when callback has fired) --- */
    if (self->pending.relay_changed)
    {
        self->pending.relay_changed = false; /* clear before read to avoid race */
        IHomeScreenView_SetRelayState(self->view,
                                      (self->pending.relay_new_state == RELAY_CONTACT_CLOSED));
    }

    /* --- Config refresh ---------------------------------------------------
     * Two paths trigger a repaint:
     *   1. Event-driven (future): pending.config_changed set by
     *      IConfigChangeNotifier callback when eventually wired.
     *   2. Periodic fallback: re-read every CONFIG_POLL_PERIOD_MS so that
     *      changes from the web server or any other source are reflected
     *      without needing a push notification.
     * --------------------------------------------------------------------- */
    {
        uint32_t now_cfg = os_ticks_get();
        bool do_refresh = self->pending.config_changed;
        if (!do_refresh &&
            (now_cfg - self->config_poll_ticks) >= CONFIG_POLL_PERIOD_MS)
        {
            do_refresh = true;
        }
        if (do_refresh)
        {
            self->pending.config_changed = false; /* clear before re-read */
            self->config_poll_ticks = now_cfg;
            paint_config_fields(self);
        }
    }

    /* --- Battery level (every call — GetData is a fast I2C read) --- */
    if (self->battery_monitor != NULL)
    {
        BatteryData_t bat = {0};
        if (BatteryMonitor_GetData(self->battery_monitor, &bat) == ERR_OK)
        {
            IHomeScreenView_SetBatteryLevel(self->view, (int32_t)bat.state_of_charge_percent);
        }
    }

    /* ── High-temp alarm polling ──────────────────────────────── */
    if (self->di_source != NULL && self->router != NULL &&
        self->pending_info_item_ptr != NULL && self->info_back_screen_ptr != NULL)
    {
        GeneralConfig_t gcfg = {0};
        if (ConfigStorage_LoadGeneralConfig(self->config_storage, &gcfg) == ERR_OK &&
            gcfg.buzzer_high_temp_alarm != 0U)
        {
            uint32_t now_ms = os_ticks_get();

            /* Handle back-inhibit: clear when 10 s have elapsed */
            if (self->alarm_back_inhibit &&
                (now_ms - self->alarm_inhibit_start_ticks) >= 10000U)
            {
                self->alarm_back_inhibit = false;
            }

            bool overtemp_active = false;
            (void)DigitalInputSource_ReadInput(self->di_source,
                                               DI_ID_ALARM_OVERTEMP, &overtemp_active);
            if (overtemp_active && !self->alarm_back_inhibit)
            {
                bool first_detect = !self->alarm_nav_active;
                bool repeat_nav = self->alarm_nav_active &&
                                  ((now_ms - self->alarm_last_nav_ticks) >= 10000U);
                if (first_detect || repeat_nav)
                {
                    *self->pending_info_item_ptr = (uint8_t)INFORMATION_SHOW_ITEM_ALARM;
                    *self->info_back_screen_ptr = (uint8_t)3U; /* SCREEN_ID_HOME */
                    (void)IScreenRouter_NavigateTo(self->router, self->alarm_screen_id);
                    self->alarm_nav_active = true;
                    self->alarm_last_nav_ticks = now_ms;
                }
            }
            else if (!overtemp_active && self->alarm_nav_active)
            {
                (void)IScreenRouter_NavigateTo(self->router, (uint8_t)3U); /* SCREEN_ID_HOME */
                self->alarm_nav_active = false;
            }
        }
    }
}

/**
 * @brief ENTER key → navigate to security screen.
 *
 * Called by InputRouter on DI_ID_BUTTON_ENTER / DI_EVENT_PRESS.
 * No polling, no edge detection (InputRouter delivers one event per key press).
 */
static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    (void)event; /* reserved for future KEEPALIVE / long-press handling */
    HomeScreenPresenter_t *self = (HomeScreenPresenter_t *)base;
    if (key == (uint8_t)SCREEN_KEY_ENTER && self->router != NULL)
    {
        /* Reset security context to default before navigating (clears stale state) */
        PresentationLayer_ResetSecurityToDefault();
        (void)IScreenRouter_NavigateTo(self->router, self->security_screen_id);
    }
}

/**
 * @brief Back-press handler — intentional no-op (root screen).
 */
static void on_back_pressed(IScreen_t *base)
{
    (void)base;
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t HomeScreenPresenter_Init(HomeScreenPresenter_t *self,
                                  const HomeScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    /* Required fields */
    if (deps->view == NULL || deps->time_source == NULL ||
        deps->gps_source == NULL || deps->wifi_status == NULL ||
        deps->config_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->update_period_ms == 0u)
    {
        return ERR_INVALID_PARAM;
    }

    memset(self, 0, sizeof(HomeScreenPresenter_t));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed; /* no-op, NOT NULL */
    self->base.OnKeyEvent = on_key_event;

    /* Required dependencies */
    self->view = deps->view;
    self->time_source = deps->time_source;
    self->gps_source = deps->gps_source;
    self->wifi_status = deps->wifi_status;
    self->config_storage = deps->config_storage;
    self->update_period_ms = deps->update_period_ms;

    /* Optional dependencies (NULL = feature disabled) */
    //    self->config_change_notifier = deps->config_change_notifier;
    self->battery_monitor = deps->battery_monitor;
    self->relay_controller = deps->relay_controller;
    self->event_flags = deps->event_flags;
    self->router = deps->router;
    self->security_screen_id = deps->security_screen_id;

    /* Alarm polling (optional — NULL = alarm disabled) */
    self->di_source = deps->di_source;
    self->pending_info_item_ptr = deps->pending_info_item_ptr;
    self->info_back_screen_ptr = deps->info_back_screen_ptr;
    self->alarm_screen_id = deps->alarm_screen_id;
    self->alarm_last_nav_ticks = 0U;
    self->alarm_nav_active = false;
    self->alarm_back_inhibit = false;
    self->alarm_inhibit_start_ticks = 0U;

    /* GPS antenna auto-switch service (optional — NULL = feature disabled) */
    self->gps_auto_switch_service = deps->gps_auto_switch_service;

    return ERR_OK;
}

/* Public delegates */
void HomeScreenPresenter_OnEnter(IScreen_t *s) { on_enter(s); }
void HomeScreenPresenter_OnExit(IScreen_t *s) { screen_on_exit(s); }
void HomeScreenPresenter_OnUpdate(IScreen_t *s) { on_update(s); }
void HomeScreenPresenter_OnBackPressed(IScreen_t *s) { on_back_pressed(s); }
void HomeScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event) { on_key_event(s, key, event); }
