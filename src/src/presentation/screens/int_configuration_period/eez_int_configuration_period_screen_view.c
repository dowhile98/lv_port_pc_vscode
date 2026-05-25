/**
 * @file eez_int_configuration_period_screen_view.c
 * @brief EEZ-backed IIntConfigurationPeriodScreenView_t implementation.
 *
 * ## Responsibilities
 * - `ClearList()`        → removes all buttons from `int_configuration_period_list`.
 * - `AddListItem(item_id, label, selected, cb, ctx)`
 *     → appends one button to the list; if `selected == true` the button
 *       receives LV_STATE_CHECKED to visually mark the current choice.
 *     → registers an LV_EVENT_RELEASED handler that fires `cb(ctx, item_id)`.
 *
 * ## Thread safety
 * Every `lv_*` call is wrapped in `lv_lock() / lv_unlock()` — safe from any
 * thread.
 *
 * ## Allowed includes
 * This is the ONLY file in the int_configuration_period path that may include
 * `lvgl.h`.
 *
 * ## Widget mapping (EEZ-generated names in screens.h)
 *   objects.int_configuration_period_list → lv_list
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/int_configuration_period/eez_int_configuration_period_screen_view.h"
#include "presentation/interfaces/i_int_configuration_period_screen_view.h"
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
    PeriodSelectItemCallback_t cb;
    void *ctx;
    uint8_t item_id;
} PeriodListItemData_t;

/* Pool of item-data structs — one per possible list button (2 items). */
#define PERIOD_LIST_MAX_ITEMS 2U
static PeriodListItemData_t s_item_data[PERIOD_LIST_MAX_ITEMS];
static uint8_t s_item_count = 0U;

static void list_item_event_cb(lv_event_t *e)
{
    const PeriodListItemData_t *d =
        (const PeriodListItemData_t *)lv_event_get_user_data(e);
    if (d != NULL && d->cb != NULL)
    {
        d->cb(d->ctx, d->item_id);
    }
}

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

static void clear_list(IIntConfigurationPeriodScreenView_t *self)
{
    (void)self;
    lv_lock();
    lv_obj_clean(objects.int_configuration_period_list);
//    lv_obj_clean(objects.int_configuration_period_list);
    s_item_count = 0U;
    lv_unlock();
}

static void add_list_item(IIntConfigurationPeriodScreenView_t *self,
                          uint8_t item_id,
                          const char *label,
                          bool selected,
                          PeriodSelectItemCallback_t cb,
                          void *ctx)
{
    (void)self;
    if (s_item_count >= PERIOD_LIST_MAX_ITEMS)
    {
        return;
    }

    PeriodListItemData_t *d = &s_item_data[s_item_count];
    d->cb = cb;
    d->ctx = ctx;
    d->item_id = item_id;
    s_item_count++;

    lv_lock();
    lv_obj_t *btn = lv_list_add_btn(objects.int_configuration_period_list,
                                    NULL, label);
    add_style_list_button(btn);
    if (selected)
    {
//        lv_obj_add_state(btn, LV_STATE_CHECKED);
    	lv_group_t * g = lv_group_get_default();
    	if(g) {
    		lv_group_focus_obj(btn);
    	}
    }
    lv_obj_add_event_cb(btn, list_item_event_cb, LV_EVENT_RELEASED, (void *)d);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IIntConfigurationPeriodScreenView_t s_view = {
    .ClearList = clear_list,
    .AddListItem = add_list_item,
};

/*============================================================================*
 * PUBLIC — interface getter
 *============================================================================*/

/**
 * @brief Return the singleton EEZ view interface.
 *
 * @return Pointer to the statically-allocated view instance.
 */
IIntConfigurationPeriodScreenView_t *EezIntConfigurationPeriodScreenView_GetInterface(void)
{
    return &s_view;
}
