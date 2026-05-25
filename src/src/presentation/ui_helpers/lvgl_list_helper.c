/**
 * @file lvgl_list_helper.c
 * @brief Shared utility — populates lv_list-based navigation menus.
 *
 * @see lvgl_list_helper.h for full API and concurrency notes.
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/styles.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — active callback state
 *
 * Safe because only one list-menu screen is visible at a time.
 * LvglListHelper_PopulateMenu() updates these before creating any button, so
 * all buttons created in one call share the same callback pair.
 *============================================================================*/

static LvglListItemCallback_t s_active_cb = NULL;
static void *s_active_context = NULL;

/*============================================================================*
 * PRIVATE — LVGL event handler
 *============================================================================*/

/**
 * @brief Fired by LVGL when any list button receives LV_EVENT_RELEASED.
 *
 * Retrieves the zero-based item index from user_data and invokes the
 * registered callback.  The LV_EVENT_RELEASED guard ensures we only react
 * to full press-release cycles (not hover or focus events).
 */
static void list_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_RELEASED)
    {
        return;
    }
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (s_active_cb != NULL)
    {
        s_active_cb(idx, s_active_context);
    }
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

void LvglListHelper_SetFocusedItem(lv_obj_t *list, uint8_t idx)
{
    lv_lock();
    uint32_t count = lv_obj_get_child_count(list);
    if ((uint32_t)idx < count)
    {
        lv_obj_t *btn = lv_obj_get_child(list, (int32_t)idx);
        if (btn != NULL)
        {
            lv_group_t *grp = lv_obj_get_group(btn);
            if (grp != NULL)
            {
                lv_group_focus_obj(btn);
            }
        }
    }
    lv_unlock();
}

void LvglListHelper_PopulateMenu(lv_obj_t *list,
                                 const char *const *labels,
                                 uint8_t count,
                                 LvglListItemCallback_t on_released,
                                 void *context)
{
    /* Store callback before acquiring the lock to minimise time inside lock */
    s_active_cb = on_released;
    s_active_context = context;

    lv_lock();

    /* Remove buttons left from the previous screen visit */
    lv_obj_clean(list);

    for (uint8_t i = 0U; i < count; i++)
    {

        lv_obj_t *btn = lv_list_add_button(list, NULL, labels[i]);

        add_style_list_button(btn);
        lv_obj_add_event_cb(btn, list_btn_event_cb, LV_EVENT_RELEASED,
                            (void *)(uintptr_t)i);
    }

    lv_unlock();
}
