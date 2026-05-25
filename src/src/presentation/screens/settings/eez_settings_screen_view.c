/**
 * @file eez_settings_screen_view.c
 * @brief EEZ-backed ISettingsScreenView_t — populates the settings lv_list.
 *
 * ## Thread safety
 * All lv_* calls are delegated to LvglListHelper_PopulateMenu() which wraps
 * them in lv_lock()/lv_unlock().
 *
 * ## Widget mapping
 *   objects.settings_list → lv_list → InitItems() clears + adds buttons
 *
 * ## Button event flow
 *   LV_EVENT_RELEASED on button ──► LvglListHelper internal cb
 *                                       └──► s_on_selected(item_idx, s_context)
 *                                                └──► SettingsScreenPresenter::on_item_selected
 *                                                         └──► IScreenRouter_NavigateTo()
 *
 * @note This is the ONLY file in the settings presenter path allowed to
 *       #include "lvgl.h".
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#include "presentation/screens/settings/eez_settings_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — settings item labels
 *============================================================================*/

#define SETTINGS_ITEMS 5U

static const char *const s_labels[SETTINGS_ITEMS] = {
    "1. Interrupt configuration",
    "2. GPS configuration",
    "3. Contact configuration",
    "4. General configuration",
    "5. WiFi configuration",
};

/*============================================================================*
 * PRIVATE — InitItems implementation
 *============================================================================*/

/**
 * @brief Populate objects.settings_list with the fixed settings items.
 *
 * Delegates entirely to LvglListHelper_PopulateMenu() which handles
 * lv_obj_clean, button creation, style application, and event wiring.
 */
static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.settings_list,
                                s_labels,
                                SETTINGS_ITEMS,
                                (LvglListItemCallback_t)on_selected,
                                context);
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.settings_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

/**
 * @brief Return the singleton EEZ-backed IListNavScreenView_t for the settings menu.
 * @return Pointer to static vtable instance (never NULL).
 */
IListNavScreenView_t *EezSettingsScreenView_GetInterface(void)
{
    return &s_vtable;
}
