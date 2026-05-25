/**
 * @file i_int_configuration_period_menu_screen_view.h
 * @brief View interface for the Interrupt Configuration Period Menu screen.
 *
 * Implemented by EezIntConfigurationPeriodMenuScreenView. Zero lvgl.h dep.
 *
 * ## Widget mapping
 *   int_configuration_period_menu_label   → lv_label (SetHeader)
 *   int_configuration_period_menu_label_1 → lv_label (SetSubLabel1 — ton display)
 *   int_configuration_period_menu_label_2 → lv_label (SetSubLabel2 — toff display)
 *   int_configuration_period_menu_list    → lv_list  (ClearList + AddListItem)
 *
 * ## List reconstruction
 *   The presenter calls ClearList() then N × AddListItem() every time the
 *   mode or current_index changes.  The view does NOT decide which items appear.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------------------------------------------------------ */
    /* Identifiers                                                         */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Opaque item ID used when the view calls the callback.
     *
     * Values are passed as @p item_id to @c PeriodMenuItemCallback_t so the
     * presenter can dispatch without inspecting labels.
     */
    typedef enum
    {
        PERIOD_MENU_ITEM_ON_TIME = 0U,     /**< Navigate → edit ton                     */
        PERIOD_MENU_ITEM_OFF_TIME = 1U,    /**< Navigate → edit toff                    */
        PERIOD_MENU_ITEM_END_DATE = 2U,    /**< Navigate → edit boundary_dates[idx]      */
        PERIOD_MENU_ITEM_NEXT = 3U,        /**< In-place: current_index++               */
        PERIOD_MENU_ITEM_PREV = 4U,        /**< In-place: current_index--               */
        PERIOD_MENU_ITEM_STOP_AT_END = 5U, /**< Future: toggle stop_at_end flag         */
    } PeriodMenuItemId_t;

    /**
     * @brief Parameter type passed to set_edit_context_fn before navigating
     *        to the configuration_input screen.
     */
    typedef enum
    {
        PERIOD_EDIT_ON_TIME = 0U,  /**< Edit ton[cycle_idx]              (uint32_t ms) */
        PERIOD_EDIT_OFF_TIME = 1U, /**< Edit toff[cycle_idx]             (uint32_t ms) */
        PERIOD_EDIT_END_DATE = 2U, /**< Edit boundary_dates[cycle_idx]   (DateTime_t)  */
    } PeriodEditParam_t;

    /* ------------------------------------------------------------------ */
    /* Callback type                                                       */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Callback type registered per list item via AddListItem.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  @c PeriodMenuItemId_t value of the selected item.
     */
    typedef void (*PeriodMenuItemCallback_t)(void *ctx, uint8_t item_id);

    /* ------------------------------------------------------------------ */
    /* Interface                                                           */
    /* ------------------------------------------------------------------ */

    typedef struct IIntConfigurationPeriodMenuScreenView_t IIntConfigurationPeriodMenuScreenView_t;

    struct IIntConfigurationPeriodMenuScreenView_t
    {
        /**
         * @brief Update the main header label ("Single" or "Cycle N/4").
         */
        void (*SetHeader)(IIntConfigurationPeriodMenuScreenView_t *self, const char *text);

        /**
         * @brief Remove all items from the list.
         *
         * Called before rebuilding the list via AddListItem.
         */
        void (*ClearList)(IIntConfigurationPeriodMenuScreenView_t *self);

        /**
         * @brief Append one item to the list.
         *
         * @param[in] self     View instance.
         * @param[in] item_id  @c PeriodMenuItemId_t — passed verbatim to @p cb.
         * @param[in] label    Display string for this item.
         * @param[in] cb       Callback fired when the item is selected (ENTER).
         * @param[in] ctx      Opaque context forwarded to @p cb.
         */
        void (*AddListItem)(IIntConfigurationPeriodMenuScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            PeriodMenuItemCallback_t cb,
                            void *ctx);

        /**
         * @brief Move encoder/keyboard focus to the list item identified by
         *        @p item_id.
         *
         * Must be called AFTER the list has been rebuilt (after AddListItem
         * calls).  No-op if @p item_id was not added in the last rebuild.
         *
         * @param[in] self     View instance.
         * @param[in] item_id  @c PeriodMenuItemId_t of the item to focus.
         */
        void (*FocusItem)(IIntConfigurationPeriodMenuScreenView_t *self,
                          uint8_t item_id);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IIntConfigurationPeriodMenuScreenView_SetHeader(
        IIntConfigurationPeriodMenuScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetHeader != NULL)
        {
            v->SetHeader(v, text);
        }
    }

    static inline void IIntConfigurationPeriodMenuScreenView_ClearList(
        IIntConfigurationPeriodMenuScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IIntConfigurationPeriodMenuScreenView_AddListItem(
        IIntConfigurationPeriodMenuScreenView_t *v,
        uint8_t item_id, const char *label,
        PeriodMenuItemCallback_t cb, void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, cb, ctx);
        }
    }

    static inline void IIntConfigurationPeriodMenuScreenView_FocusItem(
        IIntConfigurationPeriodMenuScreenView_t *v, uint8_t item_id)
    {
        if (v != NULL && v->FocusItem != NULL)
        {
            v->FocusItem(v, item_id);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H */
