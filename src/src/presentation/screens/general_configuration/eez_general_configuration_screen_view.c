/**
 * @file eez_general_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t — populates the General Configuration lv_list.
 *
 * ## Items
 *   0. Screen ON time
 *   1. Buzzer sound time
 *   2. Alarm configuration
 *   3. Delete historical events
 *   4. Restore configuration
 *   5. Reset hourmeter
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#include "presentation/screens/general_configuration/eez_general_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define GENERAL_CONFIG_ITEMS 6U

static const char *const s_labels[GENERAL_CONFIG_ITEMS] = {
    "1. Screen ON time",
    "2. Buzzer sound time",
    "3. Alarm configuration",
    "4. Delete historical events",
    "5. Restore configuration",
    "6. Reset hourmeter",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.general_configuration_list,
                                s_labels,
                                GENERAL_CONFIG_ITEMS,
                                on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.general_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezGeneralConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
