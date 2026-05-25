/**
 * @file configuration_input_screen_presenter.h
 * @brief Generic HH:MM:SS time-field editor presenter.
 *
 * Edits a single @c DateTime_t (start_time or stop_time) from
 * @c TimeWindowConfig_t stored via @c IModularConfigStorage.
 *
 * ## State machine (field order)
 *   HH → MM → SS → (ENTER on SS) saves to EEPROM and navigates back.
 *
 * ## Timing / blink
 * - OnUpdate must be called at ~20 ms period from the Control AO thread.
 * - Active field blinks every 200 ms (every 10 OnUpdate ticks).
 *
 * ## Usage
 * ```c
 * ConfigurationInputScreenPresenter_t p;
 * ConfigurationInputScreenPresenterDeps_t d = { ... };
 * ConfigurationInputScreenPresenter_Init(&p, &d);
 * // Before navigating to this screen:
 * ConfigurationInputScreenPresenter_SetParam(&p, CONFIG_INPUT_PARAM_START_TIME);
 * ```
 *
 * @note IScreen_t MUST be the first field — C99 first-field cast used by
 *       ScreenRouter.
 * @note Zero lvgl.h — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef CONFIGURATION_INPUT_SCREEN_PRESENTER_H
#define CONFIGURATION_INPUT_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_configuration_input_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include "common/general_types.h"
#include "hal/hal_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Blink threshold: flip blink state every N OnUpdate calls. */
#define CONFIG_INPUT_BLINK_TICKS 200U
#define CONFIG_INPUT_LONG_PRESS_TICKS 500U
/** @brief How long (ms) the ">> Saved!" message is shown before auto-returning. */
#define CONFIG_INPUT_SAVED_DISPLAY_MS 800U

    /* Forward declaration — required by ConfigInputDriver_t function pointer signatures. */
    typedef struct ConfigurationInputScreenPresenter_t ConfigurationInputScreenPresenter_t;

    /**
     * @brief Generic value storage union.
     *        The active driver interprets the correct member.
     *        Extend here when adding a new parameter type.
     */
    typedef union ConfigInputEditValue_t
    {
        DateTime_t time;   /**< HH:MM:SS — start_time / stop_time drivers              */
        uint32_t millis;   /**< ms duration — on_time / off_time / ... drivers           */
        int8_t signed_int; /**< Signed scalar — GPS time_offset (-10..+10 s)             */
        uint8_t byte_val;  /**< Unsigned byte — UTC offset index (0..UTC_OFFSET_COUNT-1) */
    } ConfigInputEditValue_t;

    /**
     * @brief Per-parameter editor strategy (driver).
     *
     * Each @c ConfigInputParam_t value maps to one @c ConfigInputDriver_t entry
     * in the static @c s_drivers[] table inside configuration_input_screen_presenter.c.
     *
     * **Adding a new parameter = adding one new driver entry in s_drivers[].**
     * The generic presenter loop (on_enter / on_key_event) requires no changes (OCP).
     */
    typedef struct ConfigInputDriver_t
    {
        const char *label;   /**< Prompt label shown on screen (e.g. ">> Start time?")     */
        uint8_t field_count; /**< Sub-field count: 3 for HH:MM:SS, 1 for a scalar value.  */
        /** Load the current parameter value from storage into self->edit_value. */
        void (*load_fn)(ConfigurationInputScreenPresenter_t *self);
        /** Persist self->edit_value to storage (called on final ENTER). */
        void (*save_fn)(ConfigurationInputScreenPresenter_t *self);
        /** Format self->edit_value as a string and call IView::SetValue. */
        void (*format_fn)(ConfigurationInputScreenPresenter_t *self);
        /** Increment the sub-field at index self->current_field. */
        void (*increment_fn)(ConfigurationInputScreenPresenter_t *self);
        /** Decrement the sub-field at index self->current_field. */
        void (*decrement_fn)(ConfigurationInputScreenPresenter_t *self);
    } ConfigInputDriver_t;

    /**
     * @brief Identifies which value is being edited.
     */
    typedef enum
    {
        CONFIG_INPUT_PARAM_START_TIME = 0,                  /**< Editing TimeWindowConfig_t.start_time */
        CONFIG_INPUT_PARAM_STOP_TIME = 1,                   /**< Editing TimeWindowConfig_t.stop_time  */
        CONFIG_INPUT_PARAM_ON_TIME = 2,                     /**< Editing RelayConfig_t.multicycle.ton[cycle_idx]  (ms) */
        CONFIG_INPUT_PARAM_OFF_TIME = 3,                    /**< Editing RelayConfig_t.multicycle.toff[cycle_idx] (ms) */
        CONFIG_INPUT_PARAM_END_DATE = 4,                    /**< Editing RelayConfig_t.multicycle.boundary_dates[cycle_idx] (DD/MM) */
        CONFIG_INPUT_PARAM_GPS_TIME_OFFSET = 5,             /**< Editing GPSConfig_t.time_offset (-10..+10 s)             */
        CONFIG_INPUT_PARAM_UTC_OFFSET_INDEX = 6,            /**< Editing GPSConfig_t.utc_offset_index (0..UTC_OFFSET_COUNT-1) */
        CONFIG_INPUT_PARAM_TON_MARGIN_MS = 7,               /**< Editing RelayConfig_t.ton_margin_ms (0..50 ms) — Comp. ON→OFF */
        CONFIG_INPUT_PARAM_TOFF_MARGIN_MS = 8,              /**< Editing RelayConfig_t.toff_margin_ms (0..50 ms) — Comp. OFF→ON */
        CONFIG_INPUT_PARAM_SCREEN_TIMEOUT_S = 9,            /**< Editing GeneralConfig_t.screen_blacklight_timeout_ms (15..100 s, step 5 s) */
        CONFIG_INPUT_PARAM_BUZZER_ON_TIME_MS = 10,          /**< Editing GeneralConfig_t.buzzer_on_time_ms (40..100 ms, step 1 ms) */
        CONFIG_INPUT_PARAM_EXPIRATION_DATE = 11,            /**< Editing SuperUserConfig_t.expiration_date (DD/MM/YYYY → unix epoch at 23:59:59) */
        CONFIG_INPUT_PARAM_ANTENNA_SWITCH_TIMEOUT_MIN = 12, /**< Editing GPSConfig_t.antenna_switch_timeout_min (0=disabled, 1-60 min) */
        CONFIG_INPUT_PARAM_WIFI_TIMEOUT_MIN = 13,           /**< Editing WifiConfig_t.wifi_timeout_minutes (0=always on, 1-60 min) */
        /* Add new parameters above this line. */
        CONFIG_INPUT_PARAM_COUNT /**< Sentinel — equals the size of s_drivers[]. */
    } ConfigInputParam_t;

    /**
     * @brief Dependency bundle for ConfigurationInputScreenPresenter_Init().
     */
    typedef struct ConfigurationInputScreenPresenterDeps_t
    {
        /** EEZ-backed view.  Required — must not be NULL. */
        IConfigurationInputScreenView_t *view;

        /** Screen router.  NULL → navigation silently skipped (tests). */
        IScreenRouter_t *router;

        /**
         * @brief Modular config storage (Load/Save TimeWindowConfig_t).
         * Required — must not be NULL.
         */
        IConfigStorage *config_storage;

        /** Screen to navigate to on save (ENTER on SS) or OnBackPressed. */
        uint8_t back_screen_id;

    } ConfigurationInputScreenPresenterDeps_t;

    /**
     * @brief ConfigurationInput time-editor screen presenter.
     *
     * IScreen_t MUST be the first field — supports C99 first-field cast.
     */
    typedef struct ConfigurationInputScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */
        IConfigurationInputScreenView_t *view;
        IScreenRouter_t *router;
        IConfigStorage *config_storage;
        uint8_t back_screen_id;
        uint8_t cycle_idx; /**< Cycle index for multicycle on/off/end_date drivers (0..RELAY_MAX_CYCLES-1). */

        /* Strategy driver — assigned in Init(), overwritten on each OnEnter. */
        const ConfigInputDriver_t *driver; /**< Active parameter driver (never NULL after Init) */

        /* Runtime state */
        ConfigInputParam_t param_id;       /**< Selected parameter                              */
        ConfigInputEditValue_t edit_value; /**< Working copy (driver-interpreted)               */
        uint8_t current_field;             /**< 0 .. driver->field_count - 1                   */
        uint32_t blink_ticks;              /**< Tick of last blink flip                         */
        uint32_t up_press_tick;            /**< Tick when UP was last actioned (PRESS/repeat)   */
        uint32_t down_press_tick;          /**< Tick when DOWN was last actioned (PRESS/repeat) */
        uint8_t blink_on;                  /**< 1 = field shown, 0 = blanked                    */
        uint8_t up_held;                   /**< Pending UP icon release                         */
        uint8_t down_held;                 /**< Pending DOWN icon release                       */
        uint8_t show_saved;                /**< 1 = "Saved" message is active, keys ignored     */
        uint32_t saved_tick;               /**< Tick when save completed (for auto-return timer) */

    } ConfigurationInputScreenPresenter_t;

    /**
     * @brief Initialise the configuration input presenter.
     *
     * @param[in] self  Presenter instance — must not be NULL.
     * @param[in] deps  Dependency bundle — deps, view, and config_storage must
     *                  not be NULL.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, deps->view, or
     *                           deps->config_storage is NULL.
     */
    Result_t ConfigurationInputScreenPresenter_Init(
        ConfigurationInputScreenPresenter_t *self,
        const ConfigurationInputScreenPresenterDeps_t *deps);

    /**
     * @brief Select which DateTime_t will be edited on the next OnEnter.
     *
     * Must be called BEFORE navigating to this screen.
     * Typically invoked from @c PresentationLayer_SetConfigInputParam().
     *
     * @param[in] self   Presenter instance — must not be NULL.
     * @param[in] param  @c CONFIG_INPUT_PARAM_START_TIME or
     *                   @c CONFIG_INPUT_PARAM_STOP_TIME.
     */
    void ConfigurationInputScreenPresenter_SetParam(
        ConfigurationInputScreenPresenter_t *self,
        ConfigInputParam_t param);

    /**
     * @brief Set all editing context before navigating to this screen.
     *
     * Sets param_id, cycle_idx, and overrides back_screen_id so the
     * presenter returns to the correct screen after save/back.
     *
     * Must be called BEFORE navigating to this screen (typically via
     * the set_edit_context_fn callback registered in IntConfigurationPeriodMenu).
     *
     * @param[in] self          Presenter instance — must not be NULL.
     * @param[in] param         Which field to edit.
     * @param[in] cycle_idx     Which multicycle slot (0..RELAY_MAX_CYCLES-1).
     * @param[in] back_screen_id  Screen to return to after save or back.
     */
    void ConfigurationInputScreenPresenter_SetContext(
        ConfigurationInputScreenPresenter_t *self,
        ConfigInputParam_t param,
        uint8_t cycle_idx,
        uint8_t back_screen_id);

#ifdef __cplusplus
}
#endif

#endif /* CONFIGURATION_INPUT_SCREEN_PRESENTER_H */
