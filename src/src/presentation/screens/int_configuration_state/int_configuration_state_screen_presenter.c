/**
 * @file int_configuration_state_screen_presenter.c
 * @brief Presenter for the Interrupt Configuration State screen implementation.
 *
 * @note This file intentionally does NOT include lvgl.h — all widget access
 *       is delegated to the view interface, keeping logic hardware-free and
 *       fully testable on PC.
 *
 * @author Tecna Smart Lab
 */
#include "presentation/screens/int_configuration_state/int_configuration_state_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "common/relay_types.h"
#include <stddef.h>

/*============================================================================*
 * PRIVATE — IScreen_t vtable callbacks
 *============================================================================*/

/**
 * @brief Called when the screen becomes active (LVGL thread / ui_actions).
 *
 * Reads the current relay enabled state from config storage and synchronises
 * the view's switch widget.
 *
 * @param[in] base  First field of IntConfigurationStateScreenPresenter_t.
 */
static void on_enter(IScreen_t *base)
{
    IntConfigurationStateScreenPresenter_t *self =
        (IntConfigurationStateScreenPresenter_t *)base;

    /* v2.0: Load only RelayConfig_t (102 bytes) instead of SystemConfig_t (387 bytes) */
    RelayConfig_t relay_cfg;
    Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (res == ERR_OK)
    {
        IIntConfigurationStateScreenView_SyncSwitch(self->view, relay_cfg.enabled);
    }
    /* On load failure, leave the switch unchanged (safe-fail). */
}

/**
 * @brief Called when the screen is about to be hidden.
 *
 * Nothing to clean up for this screen.
 *
 * @param[in] base  First field of IntConfigurationStateScreenPresenter_t.
 */
static void on_exit(IScreen_t *base)
{
    (void)base;
}

/**
 * @brief Periodic update — not required for this screen.
 *
 * @param[in] base  First field of IntConfigurationStateScreenPresenter_t.
 */
static void on_update(IScreen_t *base)
{
    (void)base;
}

/**
 * @brief Navigate back to the Interrupt Configuration list screen.
 *
 * @param[in] base  First field of IntConfigurationStateScreenPresenter_t.
 */
static void on_back_pressed(IScreen_t *base)
{
    IntConfigurationStateScreenPresenter_t *self =
        (IntConfigurationStateScreenPresenter_t *)base;

    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

/*============================================================================*
 * PUBLIC — Init
 *============================================================================*/

/**
 * @copydoc IntConfigurationStateScreenPresenter_Init
 */
Result_t IntConfigurationStateScreenPresenter_Init(
    IntConfigurationStateScreenPresenter_t *self,
    const IntConfigurationStateScreenPresenterDeps_t *deps)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->config_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    self->base.OnEnter = on_enter;
    self->base.OnExit = on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL; /* Not used for this screen */
    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router; /* NULL is allowed (skips navigation) */
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}

/*============================================================================*
 * PUBLIC — OnToggle
 *============================================================================*/

/**
 * @copydoc IntConfigurationStateScreenPresenter_OnToggle
 */
void IntConfigurationStateScreenPresenter_OnToggle(
    IntConfigurationStateScreenPresenter_t *self, bool new_state)
{
    if (self == NULL)
    {
        return;
    }
    if (!config_storage_is_valid(self->config_storage))
    {
        return;
    }

    /* v2.0: Load/Save only RelayConfig_t (102 bytes) instead of SystemConfig_t (387 bytes) */
    /* Benefit: 73% reduction in EEPROM wear, 1 write instead of 5 */
    RelayConfig_t relay_cfg;
    Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (res != ERR_OK)
    {
        /* Cannot load — abort to avoid saving stale data. */
        return;
    }

    relay_cfg.enabled = new_state;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);
}
