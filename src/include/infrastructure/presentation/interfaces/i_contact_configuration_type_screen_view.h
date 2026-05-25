/**
 * @file i_contact_configuration_type_screen_view.h
 * @brief View interface for the Contact Configuration Type selection screen.
 *
 * Implemented by EezContactConfigurationTypeScreenView. Zero lvgl.h dependency.
 *
 * ## Widget mapping
 *   contact_configuration_type_list  → lv_list  (ClearList + AddListItem)
 *     Items: "Normally open" (id=0), "Normally closed" (id=1)
 *     Currently-active item is visually focused via the `selected` flag.
 *   contact_configuration_type_label → lv_label (SetLabel — feedback text)
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H
#define I_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H

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
     *
     * Matches RelayConfig_t.contact_type values directly.
     * RELAY_TYPE_NO = 0 (Normally Open), RELAY_TYPE_NC = 1 (Normally Closed).
     */
    typedef enum
    {
        CONTACT_TYPE_SELECT_NO = 0U, /**< Normally Open */
        CONTACT_TYPE_SELECT_NC = 1U, /**< Normally Closed */
    } ContactTypeSelectItemId_t;

    /**
     * @brief Callback type registered per list item via AddListItem.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  @c ContactTypeSelectItemId_t value of the selected item.
     */
    typedef void (*ContactTypeSelectItemCallback_t)(void *ctx, uint8_t item_id);

    /* ------------------------------------------------------------------ */
    /* Interface                                                           */
    /* ------------------------------------------------------------------ */

    typedef struct IContactConfigurationTypeScreenView_t IContactConfigurationTypeScreenView_t;

    struct IContactConfigurationTypeScreenView_t
    {
        /**
         * @brief Remove all items from the list.
         *
         * Called before rebuilding via AddListItem.
         */
        void (*ClearList)(IContactConfigurationTypeScreenView_t *self);

        /**
         * @brief Append one item to the selection list.
         *
         * @param[in] self      View instance.
         * @param[in] item_id   @c ContactTypeSelectItemId_t — passed verbatim to @p cb.
         * @param[in] label     Display string for this item.
         * @param[in] selected  true → set default encoder focus on this item.
         * @param[in] cb        Callback fired when the item is selected (ENTER/RELEASED).
         * @param[in] ctx       Opaque context forwarded to @p cb.
         */
        void (*AddListItem)(IContactConfigurationTypeScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            bool selected,
                            ContactTypeSelectItemCallback_t cb,
                            void *ctx);

        /**
         * @brief Update the feedback label text.
         *
         * @param[in] self  View instance.
         * @param[in] text  Text to display (NULL → empty string displayed).
         */
        void (*SetLabel)(IContactConfigurationTypeScreenView_t *self, const char *text);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IContactConfigurationTypeScreenView_ClearList(
        IContactConfigurationTypeScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IContactConfigurationTypeScreenView_AddListItem(
        IContactConfigurationTypeScreenView_t *v,
        uint8_t item_id,
        const char *label,
        bool selected,
        ContactTypeSelectItemCallback_t cb,
        void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, selected, cb, ctx);
        }
    }

    static inline void IContactConfigurationTypeScreenView_SetLabel(
        IContactConfigurationTypeScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetLabel != NULL)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_CONTACT_CONFIGURATION_TYPE_SCREEN_VIEW_H */
