/**
 * @file eez_security_screen_view.c
 * @brief EEZ-backed ISecurityScreenView_t — wraps LVGL widgets on the Security screen.
 *
 * ## Thread safety
 * lv_lock()/lv_unlock() guards every lv_* call — safe from any thread.
 *
 * ## Widget mapping
 *   objects.security_title      → lv_label → SetTitle()
 *   objects.segurity_pswd_input → lv_label → SetPswdInput()
 *   objects.segurity_lock_icon  → lv_image → SetLockIcon() (img_lock_icon / img_unlock_icon)
 *
 * @note This is the ONLY file allowed to include lvgl.h in the security presenter path.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#include "presentation/screens/security/eez_security_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

extern const lv_image_dsc_t img_lock_icon;   /* ui/images/ui_image_lock_icon.c */
extern const lv_image_dsc_t img_unlock_icon; /* ui/images/ui_image_unlock_icon.c */

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

static void set_pswd_input(ISecurityScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.segurity_pswd_input, s);
    lv_unlock();
}

static void set_title(ISecurityScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.security_title, s);
    lv_unlock();
}

static void set_lock_icon(ISecurityScreenView_t *self, bool unlocked)
{
    (void)self;
    lv_lock();
    lv_image_set_src(objects.segurity_lock_icon,
                     unlocked ? (const void *)&img_unlock_icon
                              : (const void *)&img_lock_icon);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static ISecurityScreenView_t s_vtable = {
    set_pswd_input,
    set_title,
    set_lock_icon,
};

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

/**
 * @brief Return the singleton EEZ-backed ISecurityScreenView_t.
 * @return Pointer to static vtable instance (never NULL).
 */
ISecurityScreenView_t *EezSecurityScreenView_GetInterface(void)
{
    return &s_vtable;
}
