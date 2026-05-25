/**
 * @file list_nav_screen_presenter.c
 * @brief Generic list-navigation screen presenter implementation.
 *
 * @note NO #include "lvgl.h" — intentional. All LVGL access goes through
 *       the injected IListNavScreenView_t implementation.
 *
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/list_nav/list_nav_screen_presenter.h"
#include "presentation/interfaces/i_screen_router.h"
#include <string.h>

/*============================================================================*
 * PRIVATE — forward declaration
 *============================================================================*/

static void on_item_selected(uint8_t item_idx, void *context);

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    ListNavScreenPresenter_t *self = (ListNavScreenPresenter_t *)base;
    IListNavScreenView_InitItems(self->view, on_item_selected, self);
    IListNavScreenView_SetFocusedItem(self->view, self->last_selected_idx);
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
    ListNavScreenPresenter_t *self = (ListNavScreenPresenter_t *)base;
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    (void)base;
    (void)key;
    (void)event;
}

/*============================================================================*
 * PRIVATE — item-selected callback (called from LVGL thread)
 *============================================================================*/

static void on_item_selected(uint8_t item_idx, void *context)
{
    ListNavScreenPresenter_t *self = (ListNavScreenPresenter_t *)context;
    if (self == NULL || self->router == NULL)
    {
        return;
    }
    if (item_idx >= self->item_count)
    {
        return;
    }

    /* Remember the activated item so OnEnter can restore focus on return */
    self->last_selected_idx = item_idx;

    /* Optional guard: deny navigation when guard_fn returns false */
    if (self->guard_fn != NULL && !self->guard_fn(item_idx, self->guard_ctx))
    {
        if (self->on_guard_denied_fn != NULL)
        {
            self->on_guard_denied_fn(item_idx, self->on_guard_denied_ctx);
        }
        if (self->guard_denied_screen_id != 0U)
        {
            (void)IScreenRouter_NavigateTo(self->router, self->guard_denied_screen_id);
        }
        return;
    }

    /* Optional pre-navigate hook */
    if (self->pre_navigate_fn != NULL)
    {
        self->pre_navigate_fn(item_idx, self->pre_navigate_ctx);
    }

    uint8_t screen_id = self->item_screen_ids[item_idx];
    if (screen_id == 0U)
    {
        return; /* no destination configured for this item */
    }
    (void)IScreenRouter_NavigateTo(self->router, screen_id);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t ListNavScreenPresenter_Init(ListNavScreenPresenter_t *self,
                                     const ListNavScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->item_count == 0U || deps->item_count > LIST_NAV_MAX_ITEMS)
    {
        return ERR_INVALID_PARAM;
    }

    memset(self, 0, sizeof(ListNavScreenPresenter_t));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed; /* real function, NOT NULL */
    self->base.OnKeyEvent = on_key_event;

    self->view = deps->view;
    self->router = deps->router;
    self->item_count = deps->item_count;
    self->back_screen_id = deps->back_screen_id;

    for (uint8_t i = 0U; i < deps->item_count; i++)
    {
        self->item_screen_ids[i] = deps->item_screen_ids[i];
    }

    /* Optional guard / hook fields (NULL = disabled) */
    self->guard_fn = deps->guard_fn;
    self->guard_ctx = deps->guard_ctx;
    self->guard_denied_screen_id = deps->guard_denied_screen_id;
    self->on_guard_denied_fn = deps->on_guard_denied_fn;
    self->on_guard_denied_ctx = deps->on_guard_denied_ctx;
    self->pre_navigate_fn = deps->pre_navigate_fn;
    self->pre_navigate_ctx = deps->pre_navigate_ctx;

    return ERR_OK;
}

/* Public lifecycle delegates */
void ListNavScreenPresenter_OnEnter(IScreen_t *s) { on_enter(s); }
void ListNavScreenPresenter_OnExit(IScreen_t *s) { screen_on_exit(s); }
void ListNavScreenPresenter_OnUpdate(IScreen_t *s) { on_update(s); }
void ListNavScreenPresenter_OnBackPressed(IScreen_t *s) { on_back_pressed(s); }
void ListNavScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event)
{
    on_key_event(s, key, event);
}
