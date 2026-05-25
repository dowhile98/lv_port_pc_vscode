/**
 * @file home_screen_presenter.h
 * @brief Presenter for the Home (operational) screen.
 *
 * Implements IScreen_t — lifecycle managed by the Screen Router / DI container.
 *
 * Injected dependencies:
 *   - IHomeScreenView_t         → widget updates (lv_lock owned by view)
 *   - ITimeSource               → home_time (every OnUpdate)
 *   - IGPSSource                → home_gps_status blink + antenna icon
 *   - IWifiStatusSource_t       → home_wifi_status (every OnUpdate)
 *   - IConfigStorage            → relay + gps config fields (OnEnter + on event)
 *   - IConfigChangeNotifier_t * → subscribe to config changes (optional, NULL in tests)
 *   - IRelayController *        → subscribe to state changes (optional, NULL in tests)
 *   - os_event_flags_t          → ISR-safe wake signal for control task (NULL in tests)
 *
 * ## Event-driven update strategy (ACTION-030)
 *
 * Instead of polling config every N calls, the presenter subscribes to two
 * push sources in OnEnter and unsubscribes in OnExit:
 *
 *   StorageCoordinatorAO thread fires ConfigChangedCallback_t
 *     → sets pending.config_changed, signals event_flags (HOME_EVENT_CONFIG_CHANGED)
 *
 *   Timer OC ISR fires RelayStateChangeCallback_t
 *     → writes pending.relay_new_state, sets pending.relay_changed,
 *       signals event_flags (HOME_EVENT_RELAY_CHANGED) — ISR-safe
 *
 * OnUpdate drains both flags; control task wakes early via os_event_flags_get.
 * If event_flags is NULL (PC tests), the mechanism degrades gracefully.
 *
 * @note Zero LVGL includes — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef HOME_SCREEN_PRESENTER_H
#define HOME_SCREEN_PRESENTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_home_screen_view.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_wifi_status_source.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_battery_monitor.h"
#include "interfaces/i_digital_input_source.h"
#include "presentation/interfaces/i_screen_router.h"
#include "common/relay_types.h"
#include "application/services/gps_antenna_auto_switch_service.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Event flag bit — relay contact state changed (fired from ISR or RelayAO). */
#define HOME_EVENT_RELAY_CHANGED (1U << 0U)
    /** @brief Event flag bit — SystemConfig committed to AO cache (fired from StorageCoordinatorAO). */
#define HOME_EVENT_CONFIG_CHANGED (1U << 1U)
    /** @brief Mask for all Home screen event bits. */
#define HOME_EVENT_ALL (HOME_EVENT_RELAY_CHANGED | HOME_EVENT_CONFIG_CHANGED)

    /**
     * @brief Volatile pending event fields consumed by OnUpdate.
     *
     * Written by ISR-context callbacks (relay) and AO-thread callbacks (config).
     * Read and cleared atomically in OnUpdate (single-reader, single-writer pattern).
     *
     * @note relay_new_state is a single-byte enum — assignment is atomic on ARM Cortex-M.
     */
    typedef struct HomePendingEvents_t
    {
        volatile bool config_changed;                 /**< Config written to AO cache. */
        volatile bool relay_changed;                  /**< Relay contact state changed. */
        volatile RelayContactState_t relay_new_state; /**< New relay state (valid when relay_changed). */
    } HomePendingEvents_t;

    /**
     * @brief Dependency bundle for HomeScreenPresenter_Init.
     *
     * All pointer fields are used as follows:
     *   - view, time_source, gps_source, wifi_status, config_storage: REQUIRED (ERR_NULL_POINTER if NULL).
     *   - config_change_notifier: OPTIONAL. NULL disables config-change subscription.
     *   - relay_controller: OPTIONAL. NULL disables relay-state subscription.
     *   - event_flags: OPTIONAL. NULL disables control-task early-wake signalling.
     *   - router: OPTIONAL. NULL disables Home→Security ENTER navigation.
     *   - security_screen_id: Ignored when router is NULL.
     *   - update_period_ms: REQUIRED (> 0).
     *
     * @note encoder removed (ACTION-031+): ENTER navigation is now dispatched via
     *       InputRouter→IScreen_OnKeyEvent(SCREEN_KEY_ENTER), not by polling IEncoder_t.
     */
    typedef struct HomeScreenPresenterDeps_t
    {
        IHomeScreenView_t *view;                                /**< Widget update interface (never NULL). */
        ITimeSource *time_source;                               /**< RTC/GPS time (never NULL). */
        IGPSSource *gps_source;                                 /**< GPS fix status (never NULL). */
        IWifiStatusSource_t *wifi_status;                       /**< STA connected query (never NULL). */
        IConfigStorage *config_storage;                         /**< RAM cache adapter (never NULL). */
                                                                //        IConfigChangeNotifier_t *config_change_notifier; /**< Subscribe to config changes (NULL = no subscription). */
        IBatteryMonitor *battery_monitor;                       /**< Fuel gauge (NULL = battery display disabled). */
        IRelayController *relay_controller;                     /**< Subscribe to relay state changes (NULL = no subscription). */
        os_event_flags_t event_flags;                           /**< ISR-safe wake signal (NULL = no RTOS, PC tests). */
        IScreenRouter_t *router;                                /**< Navigate to security screen on ENTER (NULL = disabled). */
        uint8_t security_screen_id;                             /**< SCREEN_ID_SEGURITY (4). Ignored if router is NULL. */
        uint32_t update_period_ms;                              /**< OnUpdate call period (ms, >0). Used for GPS blink threshold. */
        IDigitalInputSource *di_source;                         /**< Alarm overtemp polling (NULL = alarm disabled). */
        uint8_t *pending_info_item_ptr;                         /**< PresentationLayer_GetPendingInfoItemPtr() — alarm navigation. */
        uint8_t *info_back_screen_ptr;                          /**< PresentationLayer_GetInfoBackScreenPtr()  — alarm navigation. */
        uint8_t alarm_screen_id;                                /**< SCREEN_ID_INFORMATION_SHOW (24). Used for alarm navigation. */
        GpsAntennaAutoSwitchService_t *gps_auto_switch_service; /**< GPS antenna auto-switch service (NULL = feature disabled). */
    } HomeScreenPresenterDeps_t;

    /**
     * @brief Home screen presenter state.
     *
     * @note base MUST be the first field. C99 §6.7.2.1 guarantees the address
     *       of the first member equals the struct address:
     *       (IScreen_t *)&presenter  ↔  &presenter
     */
    typedef struct HomeScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — C99 first-field cast. */
        /* Dependencies */
        IHomeScreenView_t *view;
        ITimeSource *time_source;
        IGPSSource *gps_source;
        IWifiStatusSource_t *wifi_status;
        IConfigStorage *config_storage;
        IBatteryMonitor *battery_monitor;   /**< NULL = battery display disabled. */
        IRelayController *relay_controller; /**< NULL = no subscription. */
        os_event_flags_t event_flags;       /**< NULL = no RTOS wake. */
        IScreenRouter_t *router;            /**< NULL = no ENTER navigation. */
        uint8_t security_screen_id;         /**< SCREEN_ID_SEGURITY. */
        uint32_t update_period_ms;          /**< Injected from deps; >0 guaranteed by Init. */
        /* Private cached state */
        uint8_t s_gps_blink_tick;     /**< 0/1 — current blink frame. */
        uint32_t s_gps_blink_counter; /**< Counts OnUpdate calls; resets at GPS blink threshold. */
        /* Event-driven pending state */
        HomePendingEvents_t pending; /**< Written by callbacks (ISR/AO), read by OnUpdate. */
        /* High-temperature alarm state */
        IDigitalInputSource *di_source;     /**< NULL = alarm polling disabled. */
        uint8_t *pending_info_item_ptr;     /**< Shared ptr — written before alarm nav. */
        uint8_t *info_back_screen_ptr;      /**< Shared ptr — written before alarm nav. */
        uint8_t alarm_screen_id;            /**< SCREEN_ID_INFORMATION_SHOW. */
        uint32_t alarm_last_nav_ticks;      /**< os_ticks_get() at last alarm navigation. */
        bool alarm_nav_active;              /**< true = info_show opened by alarm. */
        bool alarm_back_inhibit;            /**< true = user navigated back; inhibit 10 s. */
        uint32_t alarm_inhibit_start_ticks; /**< os_ticks_get() when inhibit started. */
        /* Periodic config polling (fallback when IConfigChangeNotifier is not wired) */
        /* GPS antenna auto-switch service */
        GpsAntennaAutoSwitchService_t *gps_auto_switch_service; /**< NULL = feature disabled. */
        uint32_t config_poll_ticks;                             /**< os_ticks_get() at last config re-read. */
    } HomeScreenPresenter_t;

    /**
     * @brief Initialise with injected dependencies.
     *
     * Wires the IScreen_t vtable and stores all injected dependencies.
     * Does NOT load config (deferred to OnEnter to avoid blocking Init).
     *
     * @param[in,out] self  Presenter instance (must not be NULL).
     * @param[in]     deps  Dependency bundle (must not be NULL; required fields must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, deps, or any REQUIRED deps field is NULL.
     * @return ERR_INVALID_PARAM if update_period_ms is 0.
     */
    Result_t HomeScreenPresenter_Init(HomeScreenPresenter_t *self,
                                      const HomeScreenPresenterDeps_t *deps);

    /* IScreen_t method implementations — exposed for testing; called via base vtable */
    void HomeScreenPresenter_OnEnter(IScreen_t *self);
    void HomeScreenPresenter_OnExit(IScreen_t *self);
    void HomeScreenPresenter_OnUpdate(IScreen_t *self);

    /**
     * @brief OnKeyEvent — ENTER navigates to security screen.
     *
     * Wired as IScreen_t::OnKeyEvent. Called by InputRouter_t on
     * DI_ID_BUTTON_ENTER / DI_EVENT_PRESS (no polling, no edge detection needed).
     *
     * @param[in] self   IScreen_t base pointer.
     * @param[in] key    ScreenKey_t value (uint8_t).
     * @param[in] event  ScreenKeyEvent_t value (uint8_t); reserved for future use.
     */
    void HomeScreenPresenter_OnKeyEvent(IScreen_t *self, uint8_t key, uint8_t event);

    /**
     * @brief OnBackPressed — intentional no-op.
     *
     * Home is the root screen. OnBackPressed is NOT NULL to document that the
     * decision is deliberate (future: power-off confirmation dialog).
     */
    void HomeScreenPresenter_OnBackPressed(IScreen_t *self);

#ifdef __cplusplus
}
#endif

#endif /* HOME_SCREEN_PRESENTER_H */
