/**
 * @file i_list_nav_screen_view.h
 * @brief Generic view contract for any lv_list-based navigation screen.
 *
 * Used by all configuration sub-screens (int_configuration, gps_configuration,
 * contact_configuration, general_configuration) which share the same presenter
 * logic but each target a different EEZ lv_list widget.
 *
 * @note Zero #include "lvgl.h" — fully PC-buildable.
 *
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#ifndef I_LIST_NAV_SCREEN_VIEW_H
#define I_LIST_NAV_SCREEN_VIEW_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct IListNavScreenView_t;

    /**
     * @brief Fired by the view when the user releases a list button.
     *
     * @param[in] item_idx  Zero-based index of the activated item.
     * @param[in] context   Opaque pointer supplied to InitItems().
     */
    typedef void (*ListNavItemCallback_t)(uint8_t item_idx, void *context);

    /**
     * @brief Generic view interface for lv_list navigation screens.
     */
    typedef struct IListNavScreenView_t
    {
        /**
         * @brief Populate the list widget and register the selection callback.
         *
         * Clears any items from a previous screen-enter, then adds the fixed
         * set of entries. Each item fires @p on_selected with its zero-based
         * index when released.
         *
         * @note lv_lock() is handled internally — safe from any thread.
         *
         * @param[in] self        View instance.
         * @param[in] on_selected Callback fired on item activation.
         * @param[in] context     Opaque pointer forwarded to @p on_selected.
         */
        void (*InitItems)(struct IListNavScreenView_t *self,
                          ListNavItemCallback_t on_selected,
                          void *context);

        /**
         * @brief Move encoder/keyboard focus to the button at @p idx.
         *
         * Called by the presenter after InitItems() to restore the last
         * selected position when re-entering a screen.
         *
         * @note lv_lock() is handled internally — safe from any thread.
         * @note NULL = not implemented (focus stays at first item).
         *
         * @param[in] self  View instance.
         * @param[in] idx   Zero-based index of the item to focus.
         */
        void (*SetFocusedItem)(struct IListNavScreenView_t *self, uint8_t idx);
    } IListNavScreenView_t;

    /**
     * @brief NULL-safe dispatch: populate list items.
     */
    static inline void IListNavScreenView_InitItems(IListNavScreenView_t *self,
                                                    ListNavItemCallback_t on_selected,
                                                    void *context)
    {
        if (self != NULL && self->InitItems != NULL)
        {
            self->InitItems(self, on_selected, context);
        }
    }

    /**
     * @brief NULL-safe dispatch: focus a list item by index.
     */
    static inline void IListNavScreenView_SetFocusedItem(IListNavScreenView_t *self,
                                                         uint8_t idx)
    {
        if (self != NULL && self->SetFocusedItem != NULL)
        {
            self->SetFocusedItem(self, idx);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_LIST_NAV_SCREEN_VIEW_H */
