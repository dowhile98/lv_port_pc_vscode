/**
 * @file gps_configuration_antenna_screen_presenter.h
 * @brief Presenter for the GPS Configuration Antenna selection screen.
 *
 * Shows a lv_list with options "Internal antenna" / "External antenna".
 * Item selection fires a callback → saves GPSConfig_t.antenna_type and
 * shows a feedback label, then auto-returns to back_screen_id after
 * CONFIG_ANTENNA_SAVED_DISPLAY_MS milliseconds.
 *
 * ## Lifecycle
 *   OnEnter   → loads GPSConfig_t, rebuilds list with current antenna focused
 *   Item tap  → saves antenna_type, shows "Saved!" label, arms delay timer
 *   OnUpdate  → when timer expires, navigates to back_screen_id
 *   OnBack    → navigates to back_screen_id (GPS_CONFIGURATION) without saving
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef GPS_CONFIGURATION_ANTENNA_SCREEN_PRESENTER_H
#define GPS_CONFIGURATION_ANTENNA_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_gps_configuration_antenna_screen_view.h"
#include "interfaces/i_config_storage.h"
#include "common/gps_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Duration (ms) to display "Saved!" label before auto-returning. */
#define CONFIG_ANTENNA_SAVED_DISPLAY_MS 1500U

    /* ------------------------------------------------------------------ */
    /* Dependency bundle                                                   */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Dependencies required to initialise the antenna selection presenter.
     */
    typedef struct GpsConfigurationAntennaScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IGpsConfigurationAntennaScreenView_t *view;

        /** Config storage for loading/saving GPSConfig_t (required). */
        IConfigStorage *config_storage;

        /** Screen router.  NULL → navigation silently skipped (test-time). */
        IScreenRouter_t *router;

        /** Screen ID to navigate to on back or after save delay (SCREEN_ID_GPS_CONFIGURATION). */
        uint8_t back_screen_id;

    } GpsConfigurationAntennaScreenPresenterDeps_t;

    /* ------------------------------------------------------------------ */
    /* Presenter struct                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * @brief GPS Configuration Antenna selection presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct GpsConfigurationAntennaScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IGpsConfigurationAntennaScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        /** Cached antenna_type read from GPSConfig_t on OnEnter. */
        uint8_t antenna_type;

        /**
         * @brief Set to 1 after a successful save; cleared on OnEnter.
         * When set, OnUpdate polls the timer and navigates back on expiry.
         */
        uint8_t show_saved;

        /** Tick value captured at the moment of save (via os_ticks_get()). */
        uint32_t saved_tick;

    } GpsConfigurationAntennaScreenPresenter_t;

    /* ------------------------------------------------------------------ */
    /* Public API                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the GPS Configuration Antenna selection presenter.
     *
     * @param[out] self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL; view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t GpsConfigurationAntennaScreenPresenter_Init(
        GpsConfigurationAntennaScreenPresenter_t *self,
        const GpsConfigurationAntennaScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* GPS_CONFIGURATION_ANTENNA_SCREEN_PRESENTER_H */
