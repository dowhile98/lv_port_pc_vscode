/**
 * @file eez_menu_screen_view.c
 * @brief EEZ-backed IMenuScreenView_t — populates the main menu lv_list.
 *
 * ## Thread safety
 * All lv_* calls are delegated to LvglListHelper_PopulateMenu() which wraps
 * them in lv_lock()/lv_unlock().
 *
 * ## Widget mapping
 *   objects.menu_list → lv_list → InitItems() clears + adds 4 buttons
 *
 * ## Button event flow
 *   LV_EVENT_RELEASED on button ──► LvglListHelper internal cb
 *                                       └──► s_on_selected(item_idx, s_context)
 *                                                └──► MenuScreenPresenter::on_item_selected
 *                                                         └──► IScreenRouter_NavigateTo()
 *
 * @note This is the ONLY file in the menu presenter path allowed to #include "lvgl.h".
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#include "presentation/screens/menu/eez_menu_screen_view.h"
#include "presentation/ui_helpers/lvgl_list_helper.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — menu item labels
 *============================================================================*/

#define MENU_ITEMS 4U

static const char *const s_labels[MENU_ITEMS] = {
    "1. Settings",
    "2. System Information",
    "3. Historical Events",
    "4. Super User Menu",
};

/*============================================================================*
 * PRIVATE — InitItems implementation
 *============================================================================*/

/**
 * @brief Populate objects.menu_list with the 4 fixed menu items.
 *
 * Delegates entirely to LvglListHelper_PopulateMenu() which handles
 * lv_obj_clean, button creation, style application, and event wiring.
 */
static void init_items(IListNavScreenView_t *self,
                       ListNavItemCallback_t on_selected,
                       void *context)
{
    (void)self;
    LvglListHelper_PopulateMenu(objects.menu_list,
                                s_labels,
                                MENU_ITEMS,
                                (LvglListItemCallback_t)on_selected,
                                context);
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static void set_focused_item(IListNavScreenView_t *self, uint8_t idx)
{
    (void)self;
    LvglListHelper_SetFocusedItem(objects.menu_list, idx);
}

static IListNavScreenView_t s_vtable = {init_items, set_focused_item};

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

/**
 * @brief Return the singleton EEZ-backed IListNavScreenView_t for the main menu.
 * @return Pointer to static vtable instance (never NULL).
 */
IListNavScreenView_t *EezMenuScreenView_GetInterface(void)
{
    return &s_vtable;
}
