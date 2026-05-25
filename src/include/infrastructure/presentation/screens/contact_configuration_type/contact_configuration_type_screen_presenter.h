/**
 * @file contact_configuration_type_screen_presenter.h
 * @brief Presenter for the Contact Configuration Type selection screen.
 *
 * Shows a lv_list with options "Normally open" / "Normally closed".
 * Item selection fires a callback → saves RelayConfig_t.contact_type and
 * shows a feedback label, then auto-returns to back_screen_id after
 * CONFIG_CONTACT_TYPE_SAVED_DISPLAY_MS milliseconds.
 *
 * ## Lifecycle
 *   OnEnter   → loads RelayConfig_t, rebuilds list with current type focused
 *   Item tap  → saves contact_type, shows feedback label, arms delay timer
 *   OnUpdate  → when timer expires, navigates to back_screen_id
 *   OnBack    → navigates to back_screen_id without saving
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef CONTACT_CONFIGURATION_TYPE_SCREEN_PRESENTER_H
#define CONTACT_CONFIGURATION_TYPE_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_contact_configuration_type_screen_view.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Duration (ms) to display feedback label before auto-returning. */
#define CONFIG_CONTACT_TYPE_SAVED_DISPLAY_MS 1500U

    /* ------------------------------------------------------------------ */
    /* Dependency bundle                                                   */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Dependencies required to initialise the contact type selection presenter.
     */
    typedef struct ContactConfigurationTypeScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IContactConfigurationTypeScreenView_t *view;

        /** Config storage for loading/saving RelayConfig_t (required). */
        IConfigStorage *config_storage;

        /** Screen router.  NULL → navigation silently skipped (test-time). */
        IScreenRouter_t *router;

        /** Screen ID to navigate to on back or after save delay (SCREEN_ID_CONTACT_CONFIGURATION). */
        uint8_t back_screen_id;

    } ContactConfigurationTypeScreenPresenterDeps_t;

    /* ------------------------------------------------------------------ */
    /* Presenter struct                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Contact Configuration Type selection presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct ContactConfigurationTypeScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IContactConfigurationTypeScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        /** Cached contact_type read from RelayConfig_t on OnEnter. */
        uint8_t contact_type;

        /**
         * @brief Set to 1 after a successful save; cleared on OnEnter.
         * When set, OnUpdate polls the timer and navigates back on expiry.
         */
        uint8_t show_saved;

        /** Tick value captured at the moment of save (via os_ticks_get()). */
        uint32_t saved_tick;

    } ContactConfigurationTypeScreenPresenter_t;

    /* ------------------------------------------------------------------ */
    /* Public API                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the Contact Configuration Type selection presenter.
     *
     * @param[in]  self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL; view and
     *                   config_storage must not be NULL).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if any required arg is NULL.
     */
    Result_t ContactConfigurationTypeScreenPresenter_Init(
        ContactConfigurationTypeScreenPresenter_t *self,
        const ContactConfigurationTypeScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* CONTACT_CONFIGURATION_TYPE_SCREEN_PRESENTER_H */
