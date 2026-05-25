/**
 * @file eez_wifi_module_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t for the WiFi Module list screen.
 *
 * Implements the 5-item WiFi Module navigation menu:
 *   1. AP Information
 *   2. STA Information
 *   3. Connect to AP
 *   4. Web Server (AP)
 *   5. Web Server (STA)
 *
 * All five items navigate to SCREEN_ID_INFORMATION_SHOW with pending item
 * indices 8–12 set by wifi_module_pre_navigate_fn in dependency_container.c.
 *
 * @note #include "lvgl.h" is intentional and expected in this file.
 */
#include "presentation/screens/wifi_module/eez_wifi_module_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

#define WIFI_MODULE_ITEMS 8U

static const char *const s_labels[WIFI_MODULE_ITEMS] = {
    "1. AP Information",
    "2. STA Information",
    "3. Connect to AP",
    "4. Web Server (AP)",
    "5. Web Server (STA)",
    "6. Connect AP (QR)",
    "7. Web Server via AP (QR)",
    "8. Web Server via STA (QR)"};

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.wifi_module_list,
                                s_labels,
                                WIFI_MODULE_ITEMS,
                                (LvglListItemCallback_t)on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.wifi_module_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezWifiModuleScreenView_GetInterface(void)
{
    return &s_vtable;
}
