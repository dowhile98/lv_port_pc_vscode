/**
 * @file eez_system_information_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t for System Information list.
 */
#include "presentation/screens/system_information/eez_system_information_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define SYSTEM_INFORMATION_ITEMS 8U

static const char *const s_labels[SYSTEM_INFORMATION_ITEMS] = {
    "1. Device Status",
    "2. Interruption configuration",
    "3. GPS Status",
    "4. Battery Status",
    "5. Batch",
    "6. Firmware/hardware version",
    "7. License status",
    "8. WiFi Module",
};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.system_information_list,
                                s_labels,
                                SYSTEM_INFORMATION_ITEMS,
                                (LvglListItemCallback_t)on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.system_information_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezSystemInformationScreenView_GetInterface(void)
{
    return &s_vtable;
}
