/**
 * @file contact_configuration_type_screen_presenter.c
 * @brief Presenter for the Contact Configuration Type selection screen.
 *
 * @note Intentionally does NOT include lvgl.h — all widget access is
 *       delegated to IContactConfigurationTypeScreenView_t.
 *
 * ## Flow
 *   OnEnter  → load RelayConfig_t → rebuild list (current type focused).
 *   OnSelect → load config → set contact_type → save → show feedback → arm timer.
 *   OnUpdate → wait CONFIG_CONTACT_TYPE_SAVED_DISPLAY_MS → navigate back.
 *   OnBack   → navigate back without changes.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/contact_configuration_type/contact_configuration_type_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — constant labels
 *============================================================================*/

#define CONTACT_TYPE_ITEMS_COUNT 2U

static const char *const s_contact_labels[CONTACT_TYPE_ITEMS_COUNT] = {
    "Normally open",   /* id=0, RELAY_TYPE_NO, contact_type == 0 */
    "Normally closed", /* id=1, RELAY_TYPE_NC, contact_type == 1 */
};

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(ContactConfigurationTypeScreenPresenter_t *self, uint8_t screen_id)
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
 * @param[in] item_id  ContactTypeSelectItemId_t: 0=Normally Open, 1=Normally Closed.
 */
static void on_item_selected(void *ctx, uint8_t item_id)
{
    ContactConfigurationTypeScreenPresenter_t *self =
        (ContactConfigurationTypeScreenPresenter_t *)ctx;
    if (self == NULL)
    {
        return;
    }
    if (item_id >= CONTACT_TYPE_ITEMS_COUNT)
    {
        return; /* guard against stale callbacks */
    }

    /* Load current config → update contact_type → save. */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    relay_cfg.contact_type = item_id;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);

    /* Cache the new value so OnEnter correctly re-focuses on re-entry. */
    self->contact_type = item_id;

    /* Show feedback label. */
    const char *name = s_contact_labels[item_id];
    IContactConfigurationTypeScreenView_SetLabel(self->view, name);

    /* Arm auto-return timer. */
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
}

/*============================================================================*
 * PRIVATE — list rebuild
 *============================================================================*/

static void rebuild_list(ContactConfigurationTypeScreenPresenter_t *self, uint8_t current_type)
{
    IContactConfigurationTypeScreenView_ClearList(self->view);

    for (uint8_t i = 0U; i < CONTACT_TYPE_ITEMS_COUNT; i++)
    {
        IContactConfigurationTypeScreenView_AddListItem(
            self->view,
            i,
            s_contact_labels[i],
            (i == current_type), /* focus if current selection */
            on_item_selected,
            self);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    ContactConfigurationTypeScreenPresenter_t *self =
        (ContactConfigurationTypeScreenPresenter_t *)base;

    /* Reset save feedback state. */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    /* Load current contact type. */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    self->contact_type = (res == ERR_OK) ? relay_cfg.contact_type : 0U;

    /* Clear the feedback label. */
    IContactConfigurationTypeScreenView_SetLabel(self->view, ">>Select the contact type");

    /* Build and focus list. */
    rebuild_list(self, self->contact_type);
}

static void on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    ContactConfigurationTypeScreenPresenter_t *self =
        (ContactConfigurationTypeScreenPresenter_t *)base;

    if (self->show_saved == 0U)
    {
        return;
    }

    const uint32_t elapsed = os_ticks_get() - self->saved_tick;
    if (elapsed >= CONFIG_CONTACT_TYPE_SAVED_DISPLAY_MS)
    {
        self->show_saved = 0U;
        navigate_to(self, self->back_screen_id);
    }
}

static void on_back_pressed(IScreen_t *base)
{
    ContactConfigurationTypeScreenPresenter_t *self =
        (ContactConfigurationTypeScreenPresenter_t *)base;
    navigate_to(self, self->back_screen_id);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t ContactConfigurationTypeScreenPresenter_Init(
    ContactConfigurationTypeScreenPresenter_t *self,
    const ContactConfigurationTypeScreenPresenterDeps_t *deps)
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
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}
