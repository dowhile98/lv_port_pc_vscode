/**
 * @file int_configuration_period_screen_presenter.c
 * @brief List-based presenter for the Interrupt Configuration Period screen.
 *
 * - OnEnter      : loads RelayConfig_t → rebuilds list, marks active choice
 * - Item selected: saves multicycle.enabled, navigates to period_menu_screen_id
 * - OnBackPressed: navigates to back_screen_id without saving
 *
 * @note NO #include "lvgl.h" — intentional.
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/int_configuration_period/int_configuration_period_screen_presenter.h"
#include "presentation/interfaces/i_int_configuration_period_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "i_config_storage.h"
#include "common/relay_types.h"
#include <string.h>

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(IntConfigurationPeriodScreenPresenter_t *self, uint8_t screen_id)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, screen_id);
    }
}

/*============================================================================*
 * PRIVATE — list item callback
 *============================================================================*/

/**
 * @brief Called by the EEZ view when the user selects a list item.
 *
 * @param[in] ctx      Presenter pointer (passed via AddListItem).
 * @param[in] item_id  PeriodSelectItemId_t: 0=Single, 1=Multiple.
 */
static void on_item_selected(void *ctx, uint8_t item_id)
{
    IntConfigurationPeriodScreenPresenter_t *self =
        (IntConfigurationPeriodScreenPresenter_t *)ctx;
    if (self == NULL)
    {
        return;
    }

    /* Cache the mode now so on_enter does NOT need to re-read storage on
     * re-entry — the async write may not have propagated to the read cache
     * by the time the screen is visited again. */
    self->enabled = (item_id == (uint8_t)PERIOD_SELECT_MULTIPLE) ? 1U : 0U;

    /* Push mode to the period menu presenter so it never reads storage. */
    if (self->set_period_menu_mode_fn != NULL)
    {
        self->set_period_menu_mode_fn(self->set_period_menu_mode_ctx, self->enabled);
    }

    /* Persist to storage (may complete asynchronously). */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    relay_cfg.multicycle.enabled = self->enabled;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);

    navigate_to(self, self->period_menu_screen_id);
}

/*============================================================================*
 * PRIVATE — list rebuild
 *============================================================================*/

static void rebuild_list(IntConfigurationPeriodScreenPresenter_t *self, uint8_t enabled)
{
    IIntConfigurationPeriodScreenView_ClearList(self->view);

    IIntConfigurationPeriodScreenView_AddListItem(
        self->view,
        (uint8_t)PERIOD_SELECT_SINGLE,
        "Single",
        (enabled == 0U), /* highlight if currently single */
        on_item_selected,
        self);

    IIntConfigurationPeriodScreenView_AddListItem(
        self->view,
        (uint8_t)PERIOD_SELECT_MULTIPLE,
        "Multiple",
        (enabled != 0U), /* highlight if currently multiple */
        on_item_selected,
        self);
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    IntConfigurationPeriodScreenPresenter_t *self =
        (IntConfigurationPeriodScreenPresenter_t *)base;

    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    self->enabled = (res == ERR_OK) ? relay_cfg.multicycle.enabled : 0U;
    self->mode_loaded = true;

    rebuild_list(self, self->enabled);
}

static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    (void)base;
}

static void on_back_pressed(IScreen_t *base)
{
    IntConfigurationPeriodScreenPresenter_t *self =
        (IntConfigurationPeriodScreenPresenter_t *)base;
    navigate_to(self, self->back_screen_id);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t IntConfigurationPeriodScreenPresenter_Init(
    IntConfigurationPeriodScreenPresenter_t *self,
    const IntConfigurationPeriodScreenPresenterDeps_t *deps)
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
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL;
    self->set_period_menu_mode_fn = deps->set_period_menu_mode_fn;
    self->set_period_menu_mode_ctx = deps->set_period_menu_mode_ctx;
    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;
    self->period_menu_screen_id = deps->period_menu_screen_id;

    return ERR_OK;
}
