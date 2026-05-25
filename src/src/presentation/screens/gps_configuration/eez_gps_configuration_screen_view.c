/**
 * @file eez_gps_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t — populates the GPS Configuration lv_list.
 *
 * ## Items
 *   0. Select antenna
 *   1. Time offset
 *   2. UTC setting
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#include "presentation/screens/gps_configuration/eez_gps_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define GPS_CONFIG_ITEMS 4U

static const char *const s_labels[GPS_CONFIG_ITEMS] = {
    "1. Select antenna",
    "2. Time offset",
    "3. UTC setting",
    "4. Auto-switch timeout",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.gps_configuration_list,
                                s_labels,
                                GPS_CONFIG_ITEMS,
                                on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.gps_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezGpsConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
