/**
 * @file int_configuration_predefined_screen_presenter.h
 * @brief Presenter for the Predefined Cycles selection screen.
 *
 * ## Responsibilities
 * - OnEnter  : populates the cycle list and resets display state.
 * - OnSelect : saves the selected cycle to RelayConfig_t.simple_cycle
 *              (ton + toff in ms), displays a "Saved!" label, arms
 *              the auto-return timer.
 * - OnUpdate : auto-navigates back once CONFIG_PREDEFINED_SAVED_DISPLAY_MS
 *              has elapsed after a successful save.
 * - OnBackPressed : navigates back without changes.
 *
 * ## Architecture
 * - `IScreen_t base` is the FIRST field — C99 first-field cast rule.
 * - Zero `lvgl.h` — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef INT_CONFIGURATION_PREDEFINED_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_PREDEFINED_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_int_configuration_predefined_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Duration the "Saved!" label is shown before auto-navigating back (ms). */
#define CONFIG_PREDEFINED_SAVED_DISPLAY_MS 1500U

    /**
     * @brief Injected dependency bundle for IntConfigurationPredefinedScreenPresenter_Init().
     */
    typedef struct IntConfigurationPredefinedScreenPresenterDeps_t
    {
        IIntConfigurationPredefinedScreenView_t *view; /**< REQUIRED — widget updates.    */
        IConfigStorage *config_storage;                /**< REQUIRED — EEPROM read/write. */
        IScreenRouter_t *router;                       /**< OPTIONAL — navigation.        */
        uint8_t back_screen_id;                        /**< Typically SCREEN_ID_INT_CONFIGURATION. */
    } IntConfigurationPredefinedScreenPresenterDeps_t;

    /**
     * @brief Presenter state for the Predefined Cycles screen.
     *
     * @note `base` MUST be the first field (C99 §6.7.2.1 first-field cast rule).
     */
    typedef struct IntConfigurationPredefinedScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — IScreen_t cast. */
        IIntConfigurationPredefinedScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        uint8_t show_saved;  /**< Non-zero while "Saved!" label is displayed. */
        uint32_t saved_tick; /**< os_ticks snapshot when save occurred.        */
    } IntConfigurationPredefinedScreenPresenter_t;

    /**
     * @brief Initialise the presenter and wire the IScreen_t vtable.
     *
     * @param[out] self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (view + config_storage required).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if any required pointer is NULL.
     */
    Result_t IntConfigurationPredefinedScreenPresenter_Init(
        IntConfigurationPredefinedScreenPresenter_t *self,
        const IntConfigurationPredefinedScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_PREDEFINED_SCREEN_PRESENTER_H */
