/**
 * @file eez_int_configuration_days_screen_view.c
 * @brief EEZ-backed IIntConfigurationDaysScreenView_t implementation.
 *
 * ## Responsibilities
 * - `SetDayChecked(day_idx, checked)` → add/remove `LV_STATE_CHECKED` on the
 *   corresponding checkbox widget (used only during OnEnter initialisation;
 *   the LVGL encoder group handles runtime toggle and focus).
 * - `SetLabel(text)` → update the `int_configuration_days_label` label widget.
 *
 * ## Thread safety
 * Every `lv_*` call is wrapped in `lv_lock() / lv_unlock()` — safe from any
 * thread.
 *
 * ## Allowed includes
 * This is the ONLY file in the int_configuration_days path that may include
 * `lvgl.h`.
 *
 * ## Widget mapping (EEZ-generated names in screens.h)
 *   objects.int_configuration_days_mon   → day_idx 0 (Monday)
 *   objects.int_configuration_days_tue   → day_idx 1 (Tuesday)
 *   objects.int_configuration_days_wed   → day_idx 2 (Wednesday)
 *   objects.int_configuration_days_thu   → day_idx 3 (Thursday)
 *   objects.int_configuration_days_fri   → day_idx 4 (Friday)
 *   objects.int_configuration_days_sat   → day_idx 5 (Saturday)
 *   objects.int_configuration_days_sun   → day_idx 6 (Sunday)
 *   objects.int_configuration_days_label → prompt / status label
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/int_configuration_days/eez_int_configuration_days_screen_view.h"
#include "presentation/interfaces/i_int_configuration_days_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

/**
 * @brief Return the checkbox widget for a given day index.
 *
 * @param[in] day_idx  0 = Monday … 6 = Sunday.
 * @return Pointer to the lv_obj_t, or NULL for an out-of-range index.
 */
static lv_obj_t *get_day_obj(uint8_t day_idx)
{
    switch (day_idx)
    {
    case 0U:
        return objects.int_configuration_days_mon;
    case 1U:
        return objects.int_configuration_days_tue;
    case 2U:
        return objects.int_configuration_days_wed;
    case 3U:
        return objects.int_configuration_days_thu;
    case 4U:
        return objects.int_configuration_days_fri;
    case 5U:
        return objects.int_configuration_days_sat;
    case 6U:
        return objects.int_configuration_days_sun;
    default:
        return NULL;
    }
}

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

/**
 * @brief Check or uncheck a single day checkbox.
 *
 * @param[in] self     View instance (unused — stateless singleton).
 * @param[in] day_idx  0 = Monday … 6 = Sunday.
 * @param[in] checked  true = checked, false = unchecked.
 */
static void set_day_checked(IIntConfigurationDaysScreenView_t *self,
                            uint8_t day_idx,
                            bool checked)
{
    (void)self;
    lv_obj_t *obj = get_day_obj(day_idx);
    if (obj == NULL)
    {
        return;
    }
    lv_lock();
    if (checked)
    {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
    }
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

/**
 * @brief Update the prompt / status label.
 *
 * @param[in] self  View instance (unused — stateless singleton).
 * @param[in] text  Null-terminated text (e.g. ">>Days interrupt?" or ">> Saved!").
 */
static void set_label(IIntConfigurationDaysScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.int_configuration_days_label, text != NULL ? text : "");
    lv_unlock();
}

static IIntConfigurationDaysScreenView_t s_view = {
    .SetDayChecked = set_day_checked,
    .SetLabel = set_label,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

IIntConfigurationDaysScreenView_t *EezIntConfigurationDaysScreenView_GetInterface(void)
{
    return &s_view;
}
