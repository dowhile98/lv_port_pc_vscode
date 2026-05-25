/**
 * @file i_security_screen_view.h
 * @brief View contract for the Security (password entry) screen — zero LVGL.
 *
 * Implemented by EezSecurityScreenView_t which wraps LVGL widget calls
 * under lv_lock()/lv_unlock(). The presenter depends only on this interface,
 * making it fully testable on PC without any LVGL dependency.
 *
 * ## Widget mapping (from EEZ screens.c)
 *   objects.security_title      → lv_label — SetTitle()
 *   objects.segurity_pswd_input → lv_label — SetPswdInput()
 *   objects.segurity_lock_icon  → lv_image — SetLockIcon() (lock/unlock asset)
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef I_SECURITY_SCREEN_VIEW_H
#define I_SECURITY_SCREEN_VIEW_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct ISecurityScreenView_t ISecurityScreenView_t;

    /**
     * @brief View contract for the Security screen — pure function-pointer vtable.
     */
    struct ISecurityScreenView_t
    {
        /**
         * @brief Update the password-input display label.
         *
         * Called after each UP/DOWN key press or buffer clear.
         * Shows "..." when empty, "*" × n when partially filled.
         *
         * @param[in] self  View instance.
         * @param[in] s     Null-terminated string (must not be NULL).
         */
        void (*SetPswdInput)(ISecurityScreenView_t *self, const char *s);

        /**
         * @brief Update the title / feedback label.
         *
         * Used for:
         *   - Initial prompt:  "--- ENTER KEY ---"
         *   - Valid feedback:  "** VALID ACCESS **"
         *   - Invalid feedback:"** INVALID ACCESS **"
         *
         * @param[in] self  View instance.
         * @param[in] s     Null-terminated string (must not be NULL).
         */
        void (*SetTitle)(ISecurityScreenView_t *self, const char *s);

        /**
         * @brief Switch the lock icon between locked and unlocked states.
         *
         * - unlocked == false → img_lock_icon   (default, shown on OnEnter)
         * - unlocked == true  → img_unlock_icon (shown on valid access)
         *
         * @param[in] self      View instance.
         * @param[in] unlocked  false = lock icon, true = unlock icon.
         */
        void (*SetLockIcon)(ISecurityScreenView_t *self, bool unlocked);
    };

    /* ── NULL-safe dispatch helpers ─────────────────────────────────────── */

    /**
     * @brief NULL-safe SetPswdInput dispatch.
     */
    static inline void ISecurityScreenView_SetPswdInput(ISecurityScreenView_t *v,
                                                        const char *s)
    {
        if (v && v->SetPswdInput)
        {
            v->SetPswdInput(v, s);
        }
    }

    /**
     * @brief NULL-safe SetTitle dispatch.
     */
    static inline void ISecurityScreenView_SetTitle(ISecurityScreenView_t *v,
                                                    const char *s)
    {
        if (v && v->SetTitle)
        {
            v->SetTitle(v, s);
        }
    }

    /**
     * @brief NULL-safe SetLockIcon dispatch.
     */
    static inline void ISecurityScreenView_SetLockIcon(ISecurityScreenView_t *v,
                                                       bool unlocked)
    {
        if (v && v->SetLockIcon)
        {
            v->SetLockIcon(v, unlocked);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_SECURITY_SCREEN_VIEW_H */
