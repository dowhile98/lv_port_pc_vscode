/**
 * @file lvgl_list_helper.h
 * @brief Shared utility for building lv_list-based navigation menus.
 *
 * Provides a single function that clears an lv_list, adds styled buttons,
 * and wires LV_EVENT_RELEASED to a caller-supplied callback.
 *
 * ## Intended users
 * Only EEZ view files (`eez_xxx_screen_view.c`) — the files allowed to
 * include `lvgl.h`. Presenter and interface files must NOT include this header.
 *
 * ## Thread safety
 * `LvglListHelper_PopulateMenu()` acquires `lv_lock()/lv_unlock()` internally.
 * When called from the LVGL thread (action callbacks), lv_lock() is a no-op
 * in LVGL 9.x with owner-thread tracking.
 *
 * ## Concurrency assumption
 * The helper stores the active callback and context in module-level statics.
 * This is safe because only one list-menu screen is visible at a time and
 * `lv_obj_clean()` destroys old buttons (and their event handlers) before new
 * ones are created — there is no overlap between two sets of active buttons.
 *
 * @note This header includes `lvgl.h` — only include from infrastructure EEZ
 *       view translation units.
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#ifndef LVGL_LIST_HELPER_H
#define LVGL_LIST_HELPER_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Callback fired when the user releases a list button.
     *
     * @param[in] item_idx  Zero-based index of the activated item.
     * @param[in] context   Opaque pointer supplied to LvglListHelper_PopulateMenu().
     */
    typedef void (*LvglListItemCallback_t)(uint8_t item_idx, void *context);

    /**
     * @brief Populate an lv_list with styled navigation buttons.
     *
     * 1. Acquires lv_lock().
     * 2. Calls lv_obj_clean(list) to remove any children from the previous visit.
     * 3. For each label, creates a button via lv_list_add_button(), applies
     *    add_style_list_button(), and registers LV_EVENT_RELEASED → internal cb.
     * 4. Releases lv_unlock().
     *
     * When a button is released the internal cb calls
     * `on_released(item_idx, context)`.
     *
     * @param[in] list        lv_list container (must not be NULL).
     * @param[in] labels      Array of C-string pointers (must have >= @p count entries).
     * @param[in] count       Number of buttons to create.
     * @param[in] on_released Callback invoked when a button fires LV_EVENT_RELEASED.
     * @param[in] context     Opaque pointer forwarded to @p on_released.
     */
    void LvglListHelper_PopulateMenu(lv_obj_t *list,
                                     const char *const *labels,
                                     uint8_t count,
                                     LvglListItemCallback_t on_released,
                                     void *context);

    /**
     * @brief Move lv_group focus to the button at @p idx inside @p list.
     *
     * Safe to call from any thread — acquires lv_lock() internally.
     * If @p idx is out of range or the button has no group, the call is a no-op.
     *
     * @param[in] list  lv_list container (must not be NULL).
     * @param[in] idx   Zero-based index of the button to focus.
     */
    void LvglListHelper_SetFocusedItem(lv_obj_t *list, uint8_t idx);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_LIST_HELPER_H */
