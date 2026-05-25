/**
 * @file int_configuration_start_screen_presenter.c
 * @brief Presenter for the Interrupt Configuration Start screen.
 *
 * @note This file intentionally does NOT include lvgl.h — all widget access
 *       is delegated to the view interface, keeping logic hardware-free and
 *       fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/int_configuration_start/int_configuration_start_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — IScreen_t vtable callbacks
 *============================================================================*/

/**
 * @brief Called when the screen becomes active.
 *
 * Loads RelayConfig_t.start_with_on, synchronises the switch widget, and
 * resets the "Saved" timer state.
 */
static void on_enter(IScreen_t *base)
{
    IntConfigurationStartScreenPresenter_t *self =
        (IntConfigurationStartScreenPresenter_t *)base;

    /* Reset "Saved" display state from a previous visit. */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (res == ERR_OK)
    {
        IIntConfigurationStartScreenView_SyncSwitch(
            self->view, (relay_cfg.start_with_on != 0U));
    }
    /* On load failure leave the switch unchanged (safe-fail). */

    IIntConfigurationStartScreenView_SetLabel(self->view, ">> Start with?");
}

static void on_exit(IScreen_t *base)
{
    (void)base;
}

/**
 * @brief Periodic tick — monitors "Saved!" timeout and auto-navigates back.
 */
static void on_update(IScreen_t *base)
{
    IntConfigurationStartScreenPresenter_t *self =
        (IntConfigurationStartScreenPresenter_t *)base;

    if ((self->show_saved != 0U) &&
        ((os_ticks_get() - self->saved_tick) >= CONFIG_START_SAVED_DISPLAY_MS))
    {
        self->show_saved = 0U;
        if (self->router != NULL)
        {
            (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
        }
    }
}

/**
 * @brief Navigate back without saving.
 */
static void on_back_pressed(IScreen_t *base)
{
    IntConfigurationStartScreenPresenter_t *self =
        (IntConfigurationStartScreenPresenter_t *)base;

    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

/*============================================================================*
 * PUBLIC — Init
 *============================================================================*/

Result_t IntConfigurationStartScreenPresenter_Init(
    IntConfigurationStartScreenPresenter_t *self,
    const IntConfigurationStartScreenPresenterDeps_t *deps)
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

    (void)memset(self, 0, sizeof(*self));

    self->base.OnEnter = on_enter;
    self->base.OnExit = on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL;

    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router; /* NULL is allowed — skips navigation */
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}

/*============================================================================*
 * PUBLIC — OnToggle
 *============================================================================*/

void IntConfigurationStartScreenPresenter_OnToggle(
    IntConfigurationStartScreenPresenter_t *self, bool start_with_on)
{
    if (self == NULL)
    {
        return;
    }

    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (res != ERR_OK)
    {
        /* Cannot load — abort to avoid saving stale data. */
        return;
    }

    relay_cfg.start_with_on = start_with_on ? 1U : 0U;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);

    /* Show "Saved!" and arm the auto-return timer. */
    IIntConfigurationStartScreenView_SetLabel(self->view, ">> Saved!");
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
}
