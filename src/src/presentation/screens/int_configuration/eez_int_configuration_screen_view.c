/**
 * @file eez_int_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t — populates the Interrupt Configuration lv_list.
 *
 * ## Items
 *   0. Interrupt state
 *   1. Start time
 *   2. Stop time
 *   3. Cycling days
 *   4. Period
 *   5. Cycle start
 *   6. Predefined Periods
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#include "presentation/screens/int_configuration/eez_int_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define INT_CONFIG_ITEMS 7U

static const char *const s_labels[INT_CONFIG_ITEMS] = {
    "1. Interrupt state",
    "2. Start time",
    "3. Stop time",
    "4. Cycling days",
    "5. Period",
    "6. Cycle start",
    "7. Predefined Periods",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.int_configuration_list,
                                s_labels,
                                INT_CONFIG_ITEMS,
                                on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.int_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezIntConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
