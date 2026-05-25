/**
 * @file int_configuration_state_screen_presenter.h
 * @brief Presenter for the Interrupt Configuration State screen (ACTION-032).
 *
 * ## Responsibilities
 * - On `OnEnter`: reads `RelayConfig_t.enabled` from `IConfigStorage` and
 *   updates the view's switch widget accordingly.
 * - On `OnToggle`: receives the new switch state from the UI event, persists
 *   the updated `RelayConfig_t.enabled` back to EEPROM via `IConfigStorage`.
 * - On `OnBackPressed`: navigates to `back_screen_id` (INT_CONFIGURATION).
 *
 * ## Architecture
 * - `IScreen_t base` is the FIRST field — C99 first-field cast to IScreen_t*.
 * - Zero `lvgl.h` — fully testable on PC via IIntConfigurationStateScreenView_t
 *   mock.
 * - Router is optional (NULL in PC tests — navigation silently skipped).
 *
 * @author Tecna Smart Lab
 */
#ifndef INT_CONFIGURATION_STATE_SCREEN_PRESENTER_H
#define INT_CONFIGURATION_STATE_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_int_configuration_state_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Injected dependency bundle for IntConfigurationStateScreenPresenter_Init().
     *
     * The router is optional — if NULL, navigation calls are silently skipped
     * (useful for PC unit tests).
     */
    typedef struct IntConfigurationStateScreenPresenterDeps_t
    {
        IIntConfigurationStateScreenView_t *view; /**< REQUIRED — widget updates.    */
        IConfigStorage *config_storage;           /**< REQUIRED — EEPROM read/write. */
        IScreenRouter_t *router;                  /**< OPTIONAL — navigation.        */
        uint8_t back_screen_id;                   /**< SCREEN_ID_INT_CONFIGURATION.  */
    } IntConfigurationStateScreenPresenterDeps_t;

    /**
     * @brief Presenter state for the Interrupt Configuration State screen.
     *
     * @note `base` MUST be the first field so that a pointer to this struct
     *       can be safely cast to `IScreen_t *` per C99 §6.7.2.1.
     */
    typedef struct IntConfigurationStateScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — IScreen_t cast. */
        IIntConfigurationStateScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;
    } IntConfigurationStateScreenPresenter_t;

    /**
     * @brief Initialise the presenter and wire the IScreen_t vtable.
     *
     * @param[out] self  Presenter instance to initialise (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL;
     *                   deps->view and deps->config_storage must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if `self` or `deps` or required deps are NULL.
     */
    Result_t IntConfigurationStateScreenPresenter_Init(
        IntConfigurationStateScreenPresenter_t *self,
        const IntConfigurationStateScreenPresenterDeps_t *deps);

    /**
     * @brief Handle the encoder switch toggle event.
     *
     * Called from ui_actions.c when LV_EVENT_VALUE_CHANGED fires on the switch.
     * Loads SystemConfig, updates `relay.enabled`, then saves back via
     * IConfigStorage (asynchronous EEPROM write queued to StorageCoordinatorAO).
     *
     * @param[in] self      Presenter instance (must not be NULL).
     * @param[in] new_state New switch state (true = ON / enabled).
     */
    void IntConfigurationStateScreenPresenter_OnToggle(
        IntConfigurationStateScreenPresenter_t *self, bool new_state);

#ifdef __cplusplus
}
#endif

#endif /* INT_CONFIGURATION_STATE_SCREEN_PRESENTER_H */
