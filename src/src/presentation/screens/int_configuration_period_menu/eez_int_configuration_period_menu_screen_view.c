/**
 * @file eez_int_configuration_period_menu_screen_view.c
 * @brief EEZ-backed IIntConfigurationPeriodMenuScreenView_t implementation.
 *
 * ## Responsibilities
 * - `SetHeader(text)`      → update `int_configuration_period_menu_label`.
 * - `SetSubLabel1(text)`   → update `int_configuration_period_menu_label_1`.
 * - `SetSubLabel2(text)`   → update `int_configuration_period_menu_label_2`.
 * - `ClearList()`          → empty `int_configuration_period_menu_list`.
 * - `AddListItem(item_id, label, cb, ctx)` → append a styled button that
 *   fires `cb(ctx, item_id)` on LV_EVENT_RELEASED.
 *
 * ## Thread safety
 * Every `lv_*` call is wrapped in `lv_lock() / lv_unlock()` — safe from any
 * thread.
 *
 * ## Concurrency assumption
 * `s_active_cb` and `s_active_ctx` are module-level statics, safe because
 * only one period-menu screen is visible at a time and `ClearList()` destroys
 * old buttons before new ones are created.
 *
 * ## Allowed includes
 * This is the ONLY file in the int_configuration_period_menu path that may
 * include `lvgl.h`.
 *
 * ## Widget mapping (EEZ-generated names in screens.h)
 *   objects.int_configuration_period_menu_label   → main header  (SetHeader)
 *   objects.int_configuration_period_menu_label_1 → sub-label 1  (SetSubLabel1)
 *   objects.int_configuration_period_menu_label_2 → sub-label 2  (SetSubLabel2)
 *   objects.int_configuration_period_menu_list    → lv_list       (ClearList / AddListItem)
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/int_configuration_period_menu/eez_int_configuration_period_menu_screen_view.h"
#include "presentation/interfaces/i_int_configuration_period_menu_screen_view.h"
#include "ui/screens.h"
#include "ui/styles.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — active callback state
 *
 * All buttons added by one rebuild share the same callback pair.
 * ClearList() resets them before every rebuild.
 *============================================================================*/

static PeriodMenuItemCallback_t s_active_cb = NULL;
static void *s_active_ctx = NULL;

/** One slot per PeriodMenuItemId_t — filled during AddListItem, cleared by ClearList. */
#define PERIOD_MENU_ITEM_COUNT 6U
static lv_obj_t *s_item_btns[PERIOD_MENU_ITEM_COUNT];

/*============================================================================*
 * PRIVATE — LVGL event handler
 *============================================================================*/

/**
 * @brief Fired by LVGL when a list button receives LV_EVENT_RELEASED.
 *
 * Retrieves the PeriodMenuItemId_t from user_data and invokes the registered
 * callback with the correct (ctx, item_id) argument order.
 */
static void list_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }
    uint8_t item_id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (s_active_cb != NULL)
    {
        s_active_cb(s_active_ctx, item_id);
    }
}

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

static void set_header(IIntConfigurationPeriodMenuScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.int_configuration_period_menu_label, text);
    lv_unlock();
}

static void clear_list(IIntConfigurationPeriodMenuScreenView_t *self)
{
    (void)self;
    s_active_cb = NULL;
    s_active_ctx = NULL;

    for (uint8_t i = 0U; i < PERIOD_MENU_ITEM_COUNT; i++)
    {
        s_item_btns[i] = NULL;
    }

    lv_lock();
    lv_obj_clean(objects.int_configuration_period_menu_list);
    lv_unlock();
}

static void add_list_item(IIntConfigurationPeriodMenuScreenView_t *self,
                          uint8_t item_id,
                          const char *label,
                          PeriodMenuItemCallback_t cb,
                          void *ctx)
{
    (void)self;
    /* Update active callback pair — all buttons in one rebuild share them */
    s_active_cb = cb;
    s_active_ctx = ctx;

    lv_lock();
    lv_obj_t *btn = lv_list_add_button(
        objects.int_configuration_period_menu_list, NULL, label);
    add_style_list_button(btn);
    lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_RELEASED,
                        (void *)(uintptr_t)item_id);
    lv_unlock();

    /* Store button pointer so FocusItem can retrieve it by item_id. */
    if (item_id < PERIOD_MENU_ITEM_COUNT)
    {
        s_item_btns[item_id] = btn;
    }
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static void focus_item(IIntConfigurationPeriodMenuScreenView_t *self, uint8_t item_id)
{
    (void)self;
    if (item_id >= PERIOD_MENU_ITEM_COUNT)
    {
        return;
    }
    lv_obj_t *btn = s_item_btns[item_id];
    if (btn == NULL)
    {
        return;
    }
    lv_lock();
    lv_group_focus_obj(btn);
    lv_unlock();
}

static IIntConfigurationPeriodMenuScreenView_t s_view = {
    set_header,
    clear_list,
    add_list_item,
    focus_item,
};

/*============================================================================*
 * PUBLIC — interface getter
 *============================================================================*/

/**
 * @brief Return the singleton EEZ view interface.
 *
 * @return Pointer to the statically-allocated view instance.
 */
IIntConfigurationPeriodMenuScreenView_t *EezIntConfigurationPeriodMenuScreenView_GetInterface(void)
{
    return &s_view;
}
