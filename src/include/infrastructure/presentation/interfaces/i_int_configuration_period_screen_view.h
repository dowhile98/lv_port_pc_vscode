/**
 * @file i_int_configuration_period_screen_view.h
 * @brief View interface for the Interrupt Configuration Period list screen.
 *
 * Implemented by EezIntConfigurationPeriodScreenView. Zero lvgl.h dependency.
 *
 * ## Widget mapping
 *   int_configuration_period_list → lv_list (ClearList + AddListItem)
 *     Items: "Single" (id=0), "Multiple" (id=1)
 *     Currently-active item is visually marked via the `selected` flag.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ------------------------------------------------------------------ */
    /* Identifiers                                                         */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Opaque item ID passed to the item-selected callback.
     */
    typedef enum
    {
        PERIOD_SELECT_SINGLE = 0U,   /**< Single interruption cycle  */
        PERIOD_SELECT_MULTIPLE = 1U, /**< Multiple cycles by date    */
    } PeriodSelectItemId_t;

    /**
     * @brief Callback type registered per list item via AddListItem.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  @c PeriodSelectItemId_t value of the selected item.
     */
    typedef void (*PeriodSelectItemCallback_t)(void *ctx, uint8_t item_id);

    /* ------------------------------------------------------------------ */
    /* Interface                                                           */
    /* ------------------------------------------------------------------ */

    typedef struct IIntConfigurationPeriodScreenView_t IIntConfigurationPeriodScreenView_t;

    struct IIntConfigurationPeriodScreenView_t
    {
        /**
         * @brief Remove all items from the list.
         *
         * Called before rebuilding via AddListItem.
         */
        void (*ClearList)(IIntConfigurationPeriodScreenView_t *self);

        /**
         * @brief Append one item to the selection list.
         *
         * @param[in] self      View instance.
         * @param[in] item_id   @c PeriodSelectItemId_t — passed verbatim to @p cb.
         * @param[in] label     Display string for this item.
         * @param[in] selected  true → visually mark as active choice.
         * @param[in] cb        Callback fired when the item is selected (ENTER).
         * @param[in] ctx       Opaque context forwarded to @p cb.
         */
        void (*AddListItem)(IIntConfigurationPeriodScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            bool selected,
                            PeriodSelectItemCallback_t cb,
                            void *ctx);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IIntConfigurationPeriodScreenView_ClearList(
        IIntConfigurationPeriodScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IIntConfigurationPeriodScreenView_AddListItem(
        IIntConfigurationPeriodScreenView_t *v,
        uint8_t item_id, const char *label, bool selected,
        PeriodSelectItemCallback_t cb, void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, selected, cb, ctx);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H */
