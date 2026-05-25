/**
 * @file eez_contact_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t — populates the Contact Configuration lv_list.
 *
 * ## Items
 *   0. Contact type
 *   1. Compensation ON to OFF
 *   2. Compensation OFF to ON
 *
 * @note ONLY file in this screen path allowed to #include "lvgl.h".
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#include "presentation/screens/contact_configuration/eez_contact_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define CONTACT_CONFIG_ITEMS 3U

static const char *const s_labels[CONTACT_CONFIG_ITEMS] = {
    "1. Contact type",
    "2. Compensation ON to OFF",
    "3. Compensation OFF to ON",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.contact_configuration_list,
                                s_labels,
                                CONTACT_CONFIG_ITEMS,
                                on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.contact_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezContactConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
