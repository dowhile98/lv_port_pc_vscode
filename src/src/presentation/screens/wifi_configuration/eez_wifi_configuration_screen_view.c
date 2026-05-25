/**
 * @file eez_wifi_configuration_screen_view.c
 * @brief EEZ-backed IListNavScreenView_t for the WiFi Configuration list screen.
 *
 * Implements the 3-item WiFi Configuration menu:
 *   1. WiFi module toggle (state-aware text)
 *   2. WiFi auto-disable timeout
 *   3. More... (TBD)
 *
 * @note #include "lvgl.h" is intentional and expected in this file.
 */
#include "presentation/screens/wifi_configuration/eez_wifi_configuration_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include <string.h>
#include "ui/screens.h"
#include "lvgl.h"

#define WIFI_CONFIGURATION_ITEMS 3U

static char s_toggle_label[40] = "1. WiFi module: unknown";
static const char *const s_labels[WIFI_CONFIGURATION_ITEMS] = {
    s_toggle_label,
    "2. WiFi timeout",
    "3. More... (TBD)"};

void EezWifiConfigurationScreenView_SetWifiToggleState(bool wifi_available, bool wifi_enabled)
{
    if (!wifi_available)
    {
        (void)strncpy(s_toggle_label, "1. WiFi module: N/A", sizeof(s_toggle_label) - 1U);
    }
    else if (wifi_enabled)
    {
        (void)strncpy(s_toggle_label, "1. WiFi module: ON", sizeof(s_toggle_label) - 1U);
    }
    else
    {
        (void)strncpy(s_toggle_label, "1. WiFi module: OFF", sizeof(s_toggle_label) - 1U);
    }

    s_toggle_label[sizeof(s_toggle_label) - 1U] = '\0';
}

static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.wifi_configuration_list,
                                s_labels,
                                WIFI_CONFIGURATION_ITEMS,
                                (LvglListItemCallback_t)on_selected,
                                context);
}

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.wifi_configuration_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

IListNavScreenView_t *EezWifiConfigurationScreenView_GetInterface(void)
{
    return &s_vtable;
}
