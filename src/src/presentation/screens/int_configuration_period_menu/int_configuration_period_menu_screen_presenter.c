/**
 * @file int_configuration_period_menu_screen_presenter.c
 * @brief Dynamic-list presenter for the Interrupt Configuration Period Menu.
 *
 * Reconstructs the lv_list on every mode/index change (ClearList + AddListItem).
 *
 * Single mode  : [On time][Off time]
 * Multi idx 0..MAX-2 : [On time][Off time][End date][▶ Next cycle]
 * Multi idx MAX-1    : [On time][Off time][End date][Stop at end?][◀ Prev cycle]
 *
 * OnBack at idx==0 → navigate to period_roller_screen_id
 * OnBack at idx>0  → idx--, refresh (no navigate)
 *
 * @note NO #include "lvgl.h" — intentional.
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/int_configuration_period_menu/int_configuration_period_menu_screen_presenter.h"
#include "presentation/interfaces/i_int_configuration_period_menu_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "i_config_storage.h"
#include "common/relay_types.h"
#include <string.h>
#include "lwprintf/lwprintf.h"
#include "osal/osal.h"
/*============================================================================*
 * PRIVATE — forward declaration
 *============================================================================*/
static void rebuild_view(IntConfigurationPeriodMenuScreenPresenter_t *self);

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(IntConfigurationPeriodMenuScreenPresenter_t *self, uint8_t screen_id)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, screen_id);
    }
}

/**
 * @brief Internal item-selected callback registered for every list item.
 *
 * The presenter passes itself as ctx and encodes item_id numerically so
 * that one callback handles all items.
 */
static void on_item_selected(void *ctx, uint8_t item_id)
{
    IntConfigurationPeriodMenuScreenPresenter_t *self =
        (IntConfigurationPeriodMenuScreenPresenter_t *)ctx;

    switch ((PeriodMenuItemId_t)item_id)
    {
    case PERIOD_MENU_ITEM_ON_TIME:
    case PERIOD_MENU_ITEM_OFF_TIME:
    case PERIOD_MENU_ITEM_END_DATE:
    {
        /* Notify configuration_input what to edit */
        if (self->set_edit_context_fn != NULL)
        {
            self->set_edit_context_fn(self->set_edit_context_ctx,
                                      item_id, /* PeriodEditParam_t maps 1-to-1 */
                                      self->current_index);
        }
        navigate_to(self, self->config_input_screen_id);
        break;
    }

    case PERIOD_MENU_ITEM_NEXT:
    {
        if (self->current_index < (uint8_t)(RELAY_MAX_CYCLES - 1U))
        {
            self->current_index++;
        }
        rebuild_view(self);
        break;
    }

    case PERIOD_MENU_ITEM_PREV:
    {
        if (self->current_index > 0U)
        {
            self->current_index--;
        }
        rebuild_view(self);
        break;
    }

    case PERIOD_MENU_ITEM_STOP_AT_END:
    {
        /* Load config, toggle stop_at_end, save, rebuild label, restore focus. */
        RelayConfig_t relay_cfg;
        (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
        (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
        relay_cfg.multicycle.stop_at_end = (relay_cfg.multicycle.stop_at_end != 0U) ? 0U : 1U;
        (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);
        /*delay to save*/
        os_thread_sleep(40);
        /* Rebuild list to update the label text. */
        rebuild_view(self);
        /* Restore focus to this item — rebuild destroys and recreates all buttons. */
        IIntConfigurationPeriodMenuScreenView_FocusItem(self->view, PERIOD_MENU_ITEM_STOP_AT_END);
        break;
    }

    default:
        break;
    }
}

/**
 * @brief Rebuild header + list from current mode and current_index.
 *
 * Called from on_enter (which always resets current_index first) and from
 * NEXT/PREV item handlers (which adjust current_index before calling).
 */
static void rebuild_view(IntConfigurationPeriodMenuScreenPresenter_t *self)
{

    /*current configuration*/
    RelayConfig_t relay_cfg = {0};

    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    /* --- Header --- */
    char hdr[32];
    if (self->mode == 0U)
    {
        (void)strncpy(hdr, "Single period", sizeof(hdr) - 1U);
    }
    else
    {
        (void)lwprintf_snprintf(hdr, sizeof(hdr), "Cycle %u/%u",
                                (unsigned)(self->current_index + 1U),
                                (unsigned)RELAY_MAX_CYCLES);
    }
    hdr[sizeof(hdr) - 1U] = '\0';
    IIntConfigurationPeriodMenuScreenView_SetHeader(self->view, hdr);

    /* --- List --- */
    IIntConfigurationPeriodMenuScreenView_ClearList(self->view);

    /*on time*/
    lwprintf_snprintf(hdr, sizeof(hdr), "ON Time: %0.3fs", (self->mode == 0) ? (float)relay_cfg.simple_cycle.ton / 1000 : (float)relay_cfg.multicycle.ton[self->current_index] / 1000);

    IIntConfigurationPeriodMenuScreenView_AddListItem(
        self->view, PERIOD_MENU_ITEM_ON_TIME, hdr, on_item_selected, self);
    /*off time*/
    lwprintf_snprintf(hdr, sizeof(hdr), "OFF Time: %0.3fs", (self->mode == 0) ? (float)relay_cfg.simple_cycle.toff / 1000 : (float)relay_cfg.multicycle.toff[self->current_index] / 1000);
    IIntConfigurationPeriodMenuScreenView_AddListItem(
        self->view, PERIOD_MENU_ITEM_OFF_TIME, hdr, on_item_selected, self);

    if (self->mode != 0U)
    {
        lwprintf_snprintf(hdr, sizeof(hdr), "End date: %02u/%02u/%04u", relay_cfg.multicycle.boundary_dates[self->current_index].month,
                          relay_cfg.multicycle.boundary_dates[self->current_index].day,
                          relay_cfg.multicycle.boundary_dates[self->current_index].year);
        IIntConfigurationPeriodMenuScreenView_AddListItem(
            self->view, PERIOD_MENU_ITEM_END_DATE, hdr, on_item_selected, self);

        if (self->current_index == (uint8_t)(RELAY_MAX_CYCLES - 1U))
        {
            lwprintf_snprintf(hdr, sizeof(hdr), "Stop at end? %s", relay_cfg.multicycle.stop_at_end ? "Yes" : "No");
            /* Last index: Stop at end + Prev */
            IIntConfigurationPeriodMenuScreenView_AddListItem(
                self->view, PERIOD_MENU_ITEM_STOP_AT_END, hdr,
                on_item_selected, self);
            IIntConfigurationPeriodMenuScreenView_AddListItem(
                self->view, PERIOD_MENU_ITEM_PREV, "< Prev cycle",
                on_item_selected, self);
        }
        else
        {
            /* Not last: show Prev (if idx > 0) then Next */
            if (self->current_index > 0U)
            {
                IIntConfigurationPeriodMenuScreenView_AddListItem(
                    self->view, PERIOD_MENU_ITEM_PREV, "< Prev cycle",
                    on_item_selected, self);
            }
            IIntConfigurationPeriodMenuScreenView_AddListItem(
                self->view, PERIOD_MENU_ITEM_NEXT, "> Next cycle",
                on_item_selected, self);
        }
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    IntConfigurationPeriodMenuScreenPresenter_t *self =
        (IntConfigurationPeriodMenuScreenPresenter_t *)base;

    /* Reset navigation state */
    self->current_index = 0U;

    /* Use mode pushed by the period presenter (set via SetMode) when available;
     * fall back to storage read only on the very first entry before any
     * selection has been made. */
    if (!self->mode_set)
    {
        RelayConfig_t relay_cfg;
        (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
        const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
        self->mode = (res == ERR_OK) ? relay_cfg.multicycle.enabled : 0U;
        self->mode_set = true;
    }

    rebuild_view(self);
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
    IntConfigurationPeriodMenuScreenPresenter_t *self =
        (IntConfigurationPeriodMenuScreenPresenter_t *)base;

    if (self->current_index == 0U)
    {
        navigate_to(self, self->period_roller_screen_id);
    }
    else
    {
        self->current_index--;
        rebuild_view(self);
    }
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t IntConfigurationPeriodMenuScreenPresenter_Init(
    IntConfigurationPeriodMenuScreenPresenter_t *self,
    const IntConfigurationPeriodMenuScreenPresenterDeps_t *deps)
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

    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->period_roller_screen_id = deps->period_roller_screen_id;
    self->config_input_screen_id = deps->config_input_screen_id;
    self->set_edit_context_fn = deps->set_edit_context_fn;
    self->set_edit_context_ctx = deps->set_edit_context_ctx;

    return ERR_OK;
}

void IntConfigurationPeriodMenuScreenPresenter_SetMode(
    IntConfigurationPeriodMenuScreenPresenter_t *self,
    uint8_t mode)
{
    if (self == NULL)
    {
        return;
    }
    self->mode = mode;
    self->mode_set = true;
}
