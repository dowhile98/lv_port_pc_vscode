/**
 * @file eez_admin_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t — populates the Admin Configuration lv_list.
 *
 * ## Items
 *   0. Select operation mode
 *   1. Select end date
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2026
 */
#include "presentation/screens/admin_configuration/eez_admin_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define ADMIN_CONFIG_ITEMS 2U

static const char *const s_labels[ADMIN_CONFIG_ITEMS] = {
    "1. Select operation mode",
    "2. Select end date",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.admin_configuration_list,
                                s_labels,
                                ADMIN_CONFIG_ITEMS,
                                on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.admin_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezAdminConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
