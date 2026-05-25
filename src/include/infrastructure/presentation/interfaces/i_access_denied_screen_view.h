/**
 * @file i_access_denied_screen_view.h
 * @brief View interface for the AccessDenied modal screen.
 *
 * Implemented by EezAccessDeniedScreenView. Zero lvgl.h dependency here;
 * all lv_*() calls live in the EEZ view implementation.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_ACCESS_DENIED_SCREEN_VIEW_H
#define I_ACCESS_DENIED_SCREEN_VIEW_H

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief View contract for the AccessDenied screen.
     */
    typedef struct IAccessDeniedScreenView_t IAccessDeniedScreenView_t;

    struct IAccessDeniedScreenView_t
    {
        /**
         * @brief Update the denial message label.
         * @param[in] self  View instance.
         * @param[in] msg   Null-terminated message string (e.g. "Access denied").
         */
        void (*SetMessage)(IAccessDeniedScreenView_t *self, const char *msg);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IAccessDeniedScreenView_SetMessage(
        IAccessDeniedScreenView_t *v, const char *msg)
    {
        if (v && v->SetMessage)
        {
            v->SetMessage(v, msg);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_ACCESS_DENIED_SCREEN_VIEW_H */
