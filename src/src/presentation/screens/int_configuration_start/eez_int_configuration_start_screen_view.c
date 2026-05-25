/**
 * @file eez_int_configuration_start_screen_view.c
 * @brief EEZ-backed IIntConfigurationStartScreenView_t implementation.
 *
 * This is the ONLY file in the int_configuration_start presenter path
 * allowed to include lvgl.h.
 *
 * ## Thread safety
 * Every lv_* call is wrapped in lv_lock()/lv_unlock() — safe from any thread.
 *
 * ## Widget mapping
 *   objects.int_configuration_start_value → lv_switch → SyncSwitch(bool)
 *   objects.int_configuration_start_label → lv_label  → SetLabel(const char *)
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/int_configuration_start/eez_int_configuration_start_screen_view.h"
#include "presentation/interfaces/i_int_configuration_start_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

/**
 * @brief Synchronise the switch widget to the given start_with_on state.
 *
 * @param[in] self          View instance (unused — stateless singleton).
 * @param[in] start_with_on true → switch ON, false → switch OFF.
 */
static void sync_switch(IIntConfigurationStartScreenView_t *self, bool start_with_on)
{
    (void)self;
    lv_lock();
    if (start_with_on)
    {
        lv_obj_add_state(objects.int_configuration_start_value, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(objects.int_configuration_start_value, LV_STATE_CHECKED);
    }
    lv_unlock();
}

/**
 * @brief Update the label text.
 *
 * @param[in] self  View instance (unused — stateless singleton).
 * @param[in] text  Null-terminated string to display.
 */
static void set_label(IIntConfigurationStartScreenView_t *self, const char *text)
{
    (void)self;
    if (text == NULL)
    {
        return;
    }
    lv_lock();
    lv_label_set_text(objects.int_configuration_start_label, text);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IIntConfigurationStartScreenView_t s_view = {
    .SyncSwitch = sync_switch,
    .SetLabel = set_label,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

/**
 * @brief Return the singleton EEZ view for the Int Config Start screen.
 */
IIntConfigurationStartScreenView_t *EezIntConfigurationStartScreenView_GetInterface(void)
{
    return &s_view;
}
