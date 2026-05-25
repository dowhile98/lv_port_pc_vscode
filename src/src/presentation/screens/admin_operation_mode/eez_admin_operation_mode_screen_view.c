/**
 * @file eez_admin_operation_mode_screen_view.c
 * @brief EEZ-backed IAdminOperationModeScreenView_t implementation.
 *
 * Widget mapping:
 *   objects.admin_operation_mode_list  → lv_list
 *   objects.admin_operation_mode_label → lv_label (feedback)
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/admin_operation_mode/eez_admin_operation_mode_screen_view.h"
#include "presentation/interfaces/i_admin_operation_mode_screen_view.h"
#include "ui/screens.h"
#include "ui/styles.h"
#include "lvgl.h"

#define ADMIN_OP_MODE_LIST_MAX_ITEMS 2U

typedef struct
{
    AdminOperationModeItemCallback_t cb;
    void *ctx;
    uint8_t item_id;
} AdminOpModeListItemData_t;

static AdminOpModeListItemData_t s_item_data[ADMIN_OP_MODE_LIST_MAX_ITEMS];
static uint8_t s_item_count = 0U;

static void list_item_event_cb(lv_event_t *e)
{
    const AdminOpModeListItemData_t *d =
        (const AdminOpModeListItemData_t *)lv_event_get_user_data(e);
    if (d != NULL && d->cb != NULL)
    {
        d->cb(d->ctx, d->item_id);
    }
}

static void clear_list(IAdminOperationModeScreenView_t *self)
{
    (void)self;
    lv_lock();
    lv_obj_clean(objects.admin_operation_mode_list);
    s_item_count = 0U;
    lv_unlock();
}

static void add_list_item(IAdminOperationModeScreenView_t *self,
                          uint8_t item_id,
                          const char *label,
                          bool selected,
                          AdminOperationModeItemCallback_t cb,
                          void *ctx)
{
    (void)self;
    if (s_item_count >= ADMIN_OP_MODE_LIST_MAX_ITEMS)
    {
        return;
    }
    AdminOpModeListItemData_t *d = &s_item_data[s_item_count];
    d->cb = cb;
    d->ctx = ctx;
    d->item_id = item_id;
    s_item_count++;

    lv_lock();
    lv_obj_t *btn = lv_list_add_btn(objects.admin_operation_mode_list, NULL, label);
    add_style_list_button(btn);
    if (selected)
    {
        lv_group_t *g = lv_group_get_default();
        if (g != NULL)
        {
            lv_group_focus_obj(btn);
        }
    }
    lv_obj_add_event_cb(btn, list_item_event_cb, LV_EVENT_RELEASED, (void *)d);
    lv_unlock();
}

static void set_label(IAdminOperationModeScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.admin_operation_mode_label, (text != NULL) ? text : "");
    lv_unlock();
}

static IAdminOperationModeScreenView_t s_view = {
    .ClearList = clear_list,
    .AddListItem = add_list_item,
    .SetLabel = set_label,
};

IAdminOperationModeScreenView_t *EezAdminOperationModeScreenView_GetInterface(void)
{
    return &s_view;
}
