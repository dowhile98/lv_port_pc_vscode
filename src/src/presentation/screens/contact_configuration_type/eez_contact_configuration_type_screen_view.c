/**
 * @file eez_contact_configuration_type_screen_view.c
 * @brief EEZ-backed IContactConfigurationTypeScreenView_t implementation.
 *
 * ## Responsibilities
 * - `ClearList()`   → removes all buttons from `contact_configuration_type_list`.
 * - `AddListItem(item_id, label, selected, cb, ctx)`
 *     → appends one button to the list; if `selected == true` the button
 *       receives encoder focus (lv_group_focus_obj) to mark the current choice.
 *     → registers an LV_EVENT_RELEASED handler that fires `cb(ctx, item_id)`.
 * - `SetLabel(text)` → updates `contact_configuration_type_label`.
 *
 * ## Thread safety
 * Every `lv_*` call is wrapped in `lv_lock() / lv_unlock()` — safe from any thread.
 *
 * ## Allowed includes
 * This is the ONLY file in the contact_configuration_type path that may include
 * `lvgl.h`.
 *
 * ## Widget mapping (EEZ-generated names in screens.h)
 *   objects.contact_configuration_type_list  → lv_list
 *   objects.contact_configuration_type_label → lv_label (feedback)
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/contact_configuration_type/eez_contact_configuration_type_screen_view.h"
#include "presentation/interfaces/i_contact_configuration_type_screen_view.h"
#include "ui/screens.h"
#include "ui/styles.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — list item event forwarder
 *============================================================================*/

/**
 * @brief Internal struct stored in each list button's user_data.
 *
 * Holds the callback + context + item_id so LV_EVENT_RELEASED can invoke
 * the presenter callback without any global state.
 */
typedef struct
{
    ContactTypeSelectItemCallback_t cb;
    void *ctx;
    uint8_t item_id;
} ContactTypeListItemData_t;

/* Pool of item-data structs — one per possible list button (2 items). */
#define CONTACT_TYPE_LIST_MAX_ITEMS 2U
static ContactTypeListItemData_t s_item_data[CONTACT_TYPE_LIST_MAX_ITEMS];
static uint8_t s_item_count = 0U;

static void list_item_event_cb(lv_event_t *e)
{
    const ContactTypeListItemData_t *d =
        (const ContactTypeListItemData_t *)lv_event_get_user_data(e);
    if (d != NULL && d->cb != NULL)
    {
        d->cb(d->ctx, d->item_id);
    }
}

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

static void clear_list(IContactConfigurationTypeScreenView_t *self)
{
    (void)self;
    lv_lock();
    lv_obj_clean(objects.contact_configuration_type_list);
    s_item_count = 0U;
    lv_unlock();
}

static void add_list_item(IContactConfigurationTypeScreenView_t *self,
                          uint8_t item_id,
                          const char *label,
                          bool selected,
                          ContactTypeSelectItemCallback_t cb,
                          void *ctx)
{
    (void)self;
    if (s_item_count >= CONTACT_TYPE_LIST_MAX_ITEMS)
    {
        return;
    }

    ContactTypeListItemData_t *d = &s_item_data[s_item_count];
    d->cb = cb;
    d->ctx = ctx;
    d->item_id = item_id;
    s_item_count++;

    lv_lock();
    lv_obj_t *btn = lv_list_add_btn(objects.contact_configuration_type_list, NULL, label);
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

static void set_label(IContactConfigurationTypeScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.contact_configuration_type_label,
                      (text != NULL) ? text : "");
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IContactConfigurationTypeScreenView_t s_view = {
    .ClearList = clear_list,
    .AddListItem = add_list_item,
    .SetLabel = set_label,
};

/*============================================================================*
 * PUBLIC — interface getter
 *============================================================================*/

/**
 * @brief Return the singleton EEZ view interface.
 *
 * @return Pointer to the statically-allocated view instance.
 */
IContactConfigurationTypeScreenView_t *EezContactConfigurationTypeScreenView_GetInterface(void)
{
    return &s_view;
}
