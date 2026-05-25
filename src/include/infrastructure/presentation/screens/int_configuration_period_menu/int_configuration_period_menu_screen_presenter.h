/**
 * @file int_configuration_period_menu_screen_presenter.h
 * @brief Presenter for the Interrupt Configuration Period Menu screen.
 *
 * Dynamically renders an lv_list based on the current period mode and index.
 *
 * ## Modes
 *   SINGLE (multicycle.enabled == 0):
 *     List: [On time] [Off time]
 *
 *   MULTIPLE (multicycle.enabled == 1), index 0 .. MAX-2:
 *     List: [On time] [Off time] [End date] [▶ Next cycle]
 *
 *   MULTIPLE, index MAX-1:
 *     List: [On time] [Off time] [End date] [Stop at end?] [◀ Prev cycle]
 *
 * ## Navigation
 *   - On time / Off time / End date: calls set_edit_context_fn then navigates
 *     to config_input_screen_id (SCREEN_ID_CONFIGURATION_INPUT).
 *   - Next / Prev: in-place, no navigate.
 *   - OnBack at index 0: navigate to period_roller_screen_id.
 *   - OnBack at index > 0: decrement index, refresh list.
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef INT_CONFIGURATION_PERIOD_MENU_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_PERIOD_MENU_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_int_configuration_period_menu_screen_view.h"
#include "i_config_storage.h"
#include "common/relay_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------------------------------------------------------ */
    /* Dependency bundle                                                   */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Dependencies required to initialise the period menu presenter.
     */
    typedef struct IntConfigurationPeriodMenuScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IIntConfigurationPeriodMenuScreenView_t *view;

        /** Modular config storage for loading RelayConfig_t (required). */
        IConfigStorage *config_storage;

        /** Screen router.  NULL → navigation silently skipped (tests). */
        IScreenRouter_t *router;

        /**
         * @brief Screen ID to navigate to for back at index 0.
         *        Typically SCREEN_ID_INT_CONFIGURATION_PERIOD.
         */
        uint8_t period_roller_screen_id;

        /**
         * @brief Screen ID to navigate to when editing On/Off/End Date.
         *        Typically SCREEN_ID_CONFIGURATION_INPUT.
         */
        uint8_t config_input_screen_id;

        /**
         * @brief Optional callback invoked BEFORE navigating to
         *        config_input_screen_id to tell the configuration_input
         *        presenter what field+cycle to edit.
         *
         * @param ctx        @c set_edit_context_ctx forwarded unchanged.
         * @param param      @c PeriodEditParam_t value.
         * @param cycle_idx  current_index at the time of selection.
         *
         * May be NULL (navigation still occurs, but field context is unset).
         */
        void (*set_edit_context_fn)(void *ctx, uint8_t param, uint8_t cycle_idx);

        /** Opaque context forwarded to @c set_edit_context_fn. */
        void *set_edit_context_ctx;

    } IntConfigurationPeriodMenuScreenPresenterDeps_t;

    /* ------------------------------------------------------------------ */
    /* Presenter struct                                                    */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Interrupt Configuration Period Menu presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct IntConfigurationPeriodMenuScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IIntConfigurationPeriodMenuScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t period_roller_screen_id;
        uint8_t config_input_screen_id;
        void (*set_edit_context_fn)(void *ctx, uint8_t param, uint8_t cycle_idx);
        void *set_edit_context_ctx;

        /* Runtime state (reset on each OnEnter) */
        uint8_t current_index; /**< 0 .. RELAY_MAX_CYCLES - 1 */
        uint8_t mode;          /**< 0 = Single, 1 = Multiple  */

        /**
         * @brief True once SetMode() has been called by the period presenter.
         * When set, on_enter skips the ConfigStorage_LoadRelayConfig() call.
         */
        bool mode_set;

    } IntConfigurationPeriodMenuScreenPresenter_t;

    /* ------------------------------------------------------------------ */
    /* Public API                                                          */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the period menu presenter.
     *
     * @param[in] self  Presenter instance — must not be NULL.
     * @param[in] deps  Dependency bundle — deps, view, config_storage required.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, deps->view, or
     *                           deps->config_storage is NULL.
     */
    Result_t IntConfigurationPeriodMenuScreenPresenter_Init(
        IntConfigurationPeriodMenuScreenPresenter_t *self,
        const IntConfigurationPeriodMenuScreenPresenterDeps_t *deps);

    /**
     * @brief Push the period mode so on_enter never reads storage.
     *
     * Called by the period presenter's on_item_selected (via a callback)
     * immediately before navigating to this screen.
     *
     * @param[in] self  Presenter instance — must not be NULL.
     * @param[in] mode  0 = Single, 1 = Multiple.
     */
    void IntConfigurationPeriodMenuScreenPresenter_SetMode(
        IntConfigurationPeriodMenuScreenPresenter_t *self,
        uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_PERIOD_MENU_SCREEN_PRESENTER_H */
