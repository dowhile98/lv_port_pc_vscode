/**
 * @file i_admin_operation_mode_screen_view.h
 * @brief View interface for the Admin Operation Mode selection screen.
 *
 * Implemented by EezAdminOperationModeScreenView. Zero lvgl.h dependency.
 *
 * ## Widget mapping
 *   admin_operation_mode_list  → lv_list  (ClearList + AddListItem)
 *     Items: "Free mode" (id=0), "Rent mode" (id=1)
 *   admin_operation_mode_label → lv_label (SetLabel — feedback text)
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_ADMIN_OPERATION_MODE_SCREEN_VIEW_H
#define I_ADMIN_OPERATION_MODE_SCREEN_VIEW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Callback type fired when the user selects a mode.
     *
     * @param[in] ctx      Opaque context (presenter pointer).
     * @param[in] item_id  0 = Free mode, 1 = Rent mode.
     */
    typedef void (*AdminOperationModeItemCallback_t)(void *ctx, uint8_t item_id);

    typedef struct IAdminOperationModeScreenView_t IAdminOperationModeScreenView_t;

    struct IAdminOperationModeScreenView_t
    {
        /** Remove all items from the selection list. */
        void (*ClearList)(IAdminOperationModeScreenView_t *self);

        /**
         * @brief Append one item to the selection list.
         *
         * @param[in] item_id   0 = Free, 1 = Rent (passed verbatim to cb).
         * @param[in] label     Display string.
         * @param[in] selected  true → set encoder focus (current choice).
         * @param[in] cb        Fired on RELEASED.
         * @param[in] ctx       Forwarded to cb.
         */
        void (*AddListItem)(IAdminOperationModeScreenView_t *self,
                            uint8_t item_id,
                            const char *label,
                            bool selected,
                            AdminOperationModeItemCallback_t cb,
                            void *ctx);

        /** Update the feedback label. NULL → empty string. */
        void (*SetLabel)(IAdminOperationModeScreenView_t *self, const char *text);
    };

    /* ---------- NULL-safe inline helpers ---------- */

    static inline void IAdminOperationModeScreenView_ClearList(
        IAdminOperationModeScreenView_t *v)
    {
        if (v != NULL && v->ClearList != NULL)
        {
            v->ClearList(v);
        }
    }

    static inline void IAdminOperationModeScreenView_AddListItem(
        IAdminOperationModeScreenView_t *v,
        uint8_t item_id,
        const char *label,
        bool selected,
        AdminOperationModeItemCallback_t cb,
        void *ctx)
    {
        if (v != NULL && v->AddListItem != NULL)
        {
            v->AddListItem(v, item_id, label, selected, cb, ctx);
        }
    }

    static inline void IAdminOperationModeScreenView_SetLabel(
        IAdminOperationModeScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetLabel != NULL)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_ADMIN_OPERATION_MODE_SCREEN_VIEW_H */
