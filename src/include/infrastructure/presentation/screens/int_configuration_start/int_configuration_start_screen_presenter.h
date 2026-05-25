/**
 * @file int_configuration_start_screen_presenter.h
 * @brief Presenter for the Interrupt Configuration Start screen.
 *
 * ## Responsibilities
 * - On `OnEnter`: reads `RelayConfig_t.start_with_on` from `IConfigStorage` and
 *   synchronises the view switch; sets the prompt label.
 * - On `OnToggle`: saves the new `start_with_on` value, shows "Saved!" label,
 *   starts an auto-return timer.
 * - On `OnUpdate`: when the "Saved" period elapses, navigates back automatically.
 * - On `OnBackPressed`: discards changes and navigates to `back_screen_id`.
 *
 * ## Architecture
 * - `IScreen_t base` is the FIRST field — C99 first-field cast to IScreen_t*.
 * - Zero `lvgl.h` — fully testable on PC via IIntConfigurationStartScreenView_t mock.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef INT_CONFIGURATION_START_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_START_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_int_configuration_start_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Duration the "Saved!" message is shown before auto-navigating back (ms). */
#define CONFIG_START_SAVED_DISPLAY_MS 1000U

    /**
     * @brief Injected dependency bundle for IntConfigurationStartScreenPresenter_Init().
     */
    typedef struct IntConfigurationStartScreenPresenterDeps_t
    {
        IIntConfigurationStartScreenView_t *view; /**< REQUIRED — widget updates.    */
        IConfigStorage *config_storage;           /**< REQUIRED — EEPROM read/write. */
        IScreenRouter_t *router;                  /**< OPTIONAL — navigation.        */
        uint8_t back_screen_id;                   /**< SCREEN_ID_INT_CONFIGURATION.  */
    } IntConfigurationStartScreenPresenterDeps_t;

    /**
     * @brief Presenter state for the Interrupt Configuration Start screen.
     *
     * @note `base` MUST be the first field (C99 §6.7.2.1 first-field cast rule).
     */
    typedef struct IntConfigurationStartScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — IScreen_t cast. */
        IIntConfigurationStartScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        uint8_t show_saved;  /**< Non-zero while "Saved!" is displayed. */
        uint32_t saved_tick; /**< os_ticks snapshot when save occurred.  */
    } IntConfigurationStartScreenPresenter_t;

    /**
     * @brief Initialise the presenter and wire the IScreen_t vtable.
     *
     * @param[out] self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL; view + config_storage required).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if any required pointer is NULL.
     */
    Result_t IntConfigurationStartScreenPresenter_Init(
        IntConfigurationStartScreenPresenter_t *self,
        const IntConfigurationStartScreenPresenterDeps_t *deps);

    /**
     * @brief Handle the encoder switch toggle event.
     *
     * Saves `start_with_on` to EEPROM, displays "Saved!", and arms the
     * auto-return timer.  Called from `ui_actions.c` on LV_EVENT_VALUE_CHANGED.
     *
     * @param[in] self          Presenter instance (must not be NULL).
     * @param[in] start_with_on New switch state (true = start ON, false = start OFF).
     */
    void IntConfigurationStartScreenPresenter_OnToggle(
        IntConfigurationStartScreenPresenter_t *self, bool start_with_on);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_START_SCREEN_PRESENTER_H */
