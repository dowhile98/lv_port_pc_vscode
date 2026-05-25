/**
 * @file int_configuration_days_screen_presenter.h
 * @brief Weekday-mask editor presenter for the IntConfigurationDays screen.
 *
 * ## Responsibilities
 * - OnEnter: loads `RelayConfig_t.time_window.weekday_mask` from storage,
 *   initialises all 7 day checkboxes, and focuses day 0 (Monday).
 * - OnKeyEvent UP/DOWN: moves the focus cursor between the 7 day checkboxes.
 * - OnKeyEvent ENTER: toggles the focused day's bit in the working mask,
 *   saves the complete mask to storage immediately, shows ">> Saved!" for
 *   @c INT_CONFIG_DAYS_SAVED_MS milliseconds, then auto-returns to
 *   @c back_screen_id.  Keys are ignored while "Saved" is visible.
 * - OnBackPressed: discards the working mask and navigates to back_screen_id.
 *
 * ## Bit layout (`weekday_mask`)
 *   Bit 0 = Monday, Bit 1 = Tuesday, …, Bit 6 = Sunday.
 *
 * ## Architecture
 * - `IScreen_t base` is the FIRST field — C99 first-field cast.
 * - Zero `lvgl.h` — fully testable on PC via mock view.
 * - Router is optional (NULL → navigation silently skipped in tests).
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef INT_CONFIGURATION_DAYS_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_DAYS_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_int_configuration_days_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "i_modular_config_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Duration (ms) the ">> Saved!" label is displayed before auto-return. */
#define INT_CONFIG_DAYS_SAVED_MS 500U

    /**
     * @brief Dependency bundle for IntConfigurationDaysScreenPresenter_Init().
     */
    typedef struct IntConfigurationDaysScreenPresenterDeps_t
    {
        IIntConfigurationDaysScreenView_t *view; /**< REQUIRED.               */
        IConfigStorage *config_storage;           /**< REQUIRED — EEPROM read/write. */
        IScreenRouter_t *router;                 /**< OPTIONAL.       */
        uint8_t back_screen_id;                  /**< Navigate target. */
    } IntConfigurationDaysScreenPresenterDeps_t;

    /**
     * @brief Weekday-mask editor screen presenter.
     *
     * @note `base` MUST be the first field — C99 first-field cast to IScreen_t*.
     */
    typedef struct IntConfigurationDaysScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IIntConfigurationDaysScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        /* Runtime state */
        uint8_t working_mask; /**< Local copy of weekday_mask (bit 0=Mon … 6=Sun) */
        uint8_t show_saved;   /**< 1 = "Saved!" message active, clicks ignored.    */
        uint32_t saved_tick;  /**< Tick when save completed (auto-return timer).    */

    } IntConfigurationDaysScreenPresenter_t;

    /**
     * @brief Initialise the presenter and wire the IScreen_t vtable.
     *
     * @param[out] self  Presenter instance — must not be NULL.
     * @param[in]  deps  Dependency bundle — deps, view, and config_storage required.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, deps->view, or deps->config_storage is NULL.
     */
    Result_t IntConfigurationDaysScreenPresenter_Init(
        IntConfigurationDaysScreenPresenter_t *self,
        const IntConfigurationDaysScreenPresenterDeps_t *deps);

    /**
     * @brief Called when a day checkbox is clicked by the encoder.
     *
     * Invoked from presentation_layer when the LVGL LV_EVENT_CLICKED fires
     * on one of the seven checkbox widgets.  Updates working_mask, saves
     * RelayConfig_t to storage, and starts the ">> Saved!" timer.  Silently
     * ignored while show_saved == 1 (ghost-click prevention).
     *
     * @param[in] self     Presenter instance — must not be NULL.
     * @param[in] day_idx  0 = Monday … 6 = Sunday.
     * @param[in] checked  New checkbox state after the click.
     */
    void IntConfigurationDaysScreenPresenter_OnDayClicked(
        IntConfigurationDaysScreenPresenter_t *self,
        uint8_t day_idx,
        bool checked);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_DAYS_SCREEN_PRESENTER_H */
