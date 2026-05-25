/**
 * @file int_configuration_period_screen_presenter.h
 * @brief Presenter for the Interrupt Configuration Period list screen.
 *
 * Shows a lv_list with options Single / Multiple.
 * Item selection fires a callback → saves multicycle.enabled and
 * navigates to period_menu_screen_id.
 *
 * ## Lifecycle
 *   OnEnter   → loads RelayConfig_t, rebuilds list with active selection marked
 *   Item tap  → saves multicycle.enabled, navigates to period_menu_screen_id
 *   OnBack    → navigates to back_screen_id (INT_CONFIGURATION) without saving
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef INT_CONFIGURATION_PERIOD_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_PERIOD_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_int_configuration_period_screen_view.h"
#include "i_config_storage.h"
#include "common/relay_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------------------------------------------------------ */
    /* Dependency bundle                                                   */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Dependencies required to initialise the period list presenter.
     */
    typedef struct IntConfigurationPeriodScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IIntConfigurationPeriodScreenView_t *view;

        /** Config storage for loading/saving RelayConfig_t (required). */
        IConfigStorage *config_storage;

        /** Screen router.  NULL → navigation silently skipped (test-time). */
        IScreenRouter_t *router;

        /** Screen ID to navigate to on back (SCREEN_ID_INT_CONFIGURATION). */
        uint8_t back_screen_id;

        /** Screen ID to navigate to on confirm (SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU). */
        uint8_t period_menu_screen_id;

        /**
         * @brief Optional callback invoked BEFORE navigating to the period menu
         *        to push the selected mode (0=Single, 1=Multiple) so the menu
         *        presenter never needs a storage read on entry.
         *
         * @param ctx   @c set_period_menu_mode_ctx forwarded unchanged.
         * @param mode  0 = Single, 1 = Multiple.
         *
         * May be NULL (menu will fall back to storage read on first enter).
         */
        void (*set_period_menu_mode_fn)(void *ctx, uint8_t mode);

        /** Opaque context forwarded to @c set_period_menu_mode_fn. */
        void *set_period_menu_mode_ctx;

    } IntConfigurationPeriodScreenPresenterDeps_t;

    /* ------------------------------------------------------------------ */
    /* Presenter struct                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Interrupt Configuration Period list presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct IntConfigurationPeriodScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IIntConfigurationPeriodScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;
        uint8_t period_menu_screen_id;

        /** Callback to push mode to the period menu presenter before navigating. */
        void (*set_period_menu_mode_fn)(void *ctx, uint8_t mode);
        void *set_period_menu_mode_ctx;

        /**
         * @brief Cached value of multicycle.enabled (0=Single, 1=Multiple).
         *
         * Set on first OnEnter from storage, then updated immediately in
         * on_item_selected so that re-entry never reads a stale async-write cache.
         */
        uint8_t enabled;

        /** @brief False until the first OnEnter loads enabled from storage. */
        bool mode_loaded;

    } IntConfigurationPeriodScreenPresenter_t;

    /* ------------------------------------------------------------------ */
    /* Public API                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the period list presenter.
     *
     * Wires the IScreen_t vtable (OnEnter / OnExit / OnUpdate / OnBackPressed).
     *
     * @param[in] self  Presenter instance — must not be NULL.
     * @param[in] deps  Dependency bundle — deps, view, config_storage required.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, deps->view, or
     *                           deps->config_storage is NULL.
     */
    Result_t IntConfigurationPeriodScreenPresenter_Init(
        IntConfigurationPeriodScreenPresenter_t *self,
        const IntConfigurationPeriodScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_PERIOD_SCREEN_PRESENTER_H */
