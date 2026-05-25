/**
 * @file general_configuration_alarm_screen_presenter.h
 * @brief Presenter for the General Configuration Alarm selection screen.
 *
 * Shows a lv_list with options "Disable" / "Enable".
 * Item selection fires a callback → saves GeneralConfig_t.buzzer_high_temp_alarm
 * and shows a feedback label, then auto-returns to back_screen_id after
 * CONFIG_ALARM_SAVED_DISPLAY_MS milliseconds.
 *
 * ## Lifecycle
 *   OnEnter   → loads GeneralConfig_t, rebuilds list with current value focused
 *   Item tap  → saves buzzer_high_temp_alarm, shows feedback label, arms delay timer
 *   OnUpdate  → when timer expires, navigates to back_screen_id
 *   OnBack    → navigates to back_screen_id without saving
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef GENERAL_CONFIGURATION_ALARM_SCREEN_PRESENTER_H
#define GENERAL_CONFIGURATION_ALARM_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_general_configuration_alarm_screen_view.h"
#include "interfaces/i_config_storage.h"
#include "common/general_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Duration (ms) to display feedback label before auto-returning. */
#define CONFIG_ALARM_SAVED_DISPLAY_MS 1500U

    /* ------------------------------------------------------------------ */
    /* Dependency bundle                                                   */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Dependencies required to initialise the alarm selection presenter.
     */
    typedef struct GeneralConfigurationAlarmScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IGeneralConfigurationAlarmScreenView_t *view;

        /** Config storage for loading/saving GeneralConfig_t (required). */
        IConfigStorage *config_storage;

        /** Screen router.  NULL → navigation silently skipped (test-time). */
        IScreenRouter_t *router;

        /** Screen ID to navigate to on back or after save delay. */
        uint8_t back_screen_id;

    } GeneralConfigurationAlarmScreenPresenterDeps_t;

    /* ------------------------------------------------------------------ */
    /* Presenter struct                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * @brief General Configuration Alarm selection presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct GeneralConfigurationAlarmScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IGeneralConfigurationAlarmScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        /** Cached value read from GeneralConfig_t.buzzer_high_temp_alarm on OnEnter. */
        uint8_t alarm_enabled;

        /**
         * @brief Set to 1 after a successful save; cleared on OnEnter.
         * When set, OnUpdate polls the timer and navigates back on expiry.
         */
        uint8_t show_saved;

        /** Tick value captured at the moment of save (via os_ticks_get()). */
        uint32_t saved_tick;

    } GeneralConfigurationAlarmScreenPresenter_t;

    /* ------------------------------------------------------------------ */
    /* Public API                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the General Configuration Alarm selection presenter.
     *
     * @param[in]  self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL; view and
     *                   config_storage must not be NULL).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if any required arg is NULL.
     */
    Result_t GeneralConfigurationAlarmScreenPresenter_Init(
        GeneralConfigurationAlarmScreenPresenter_t *self,
        const GeneralConfigurationAlarmScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* GENERAL_CONFIGURATION_ALARM_SCREEN_PRESENTER_H */
