/**
 * @file eez_access_denied_screen_view.c
 * @brief EEZ-backed IAccessDeniedScreenView_t implementation.
 *
 * This is the ONLY file in the access_denied presenter path
 * that is allowed to include lvgl.h.
 *
 * ## Thread safety
 * lv_lock()/lv_unlock() guards every lv_* call — safe from any thread.
 *
 * ## Widget mapping
 *   objects.accessdenied_label → lv_label → SetMessage(const char *msg)
 *
 * @author Tecna Smart Lab
 */
#include "presentation/screens/access_denied/eez_access_denied_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

/**
 * @brief Write the denied message to the accessdenied_label widget.
 *
 * @param[in] self  View instance (unused — stateless singleton).
 * @param[in] msg   Null-terminated message string (e.g. "Access denied").
 */
static void set_message(IAccessDeniedScreenView_t *self, const char *msg)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.accessdenied_label, msg != NULL ? msg : "");
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IAccessDeniedScreenView_t s_view = {
    .SetMessage = set_message,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

/**
 * @copydoc EezAccessDeniedScreenView_GetInterface
 */
IAccessDeniedScreenView_t *EezAccessDeniedScreenView_GetInterface(void)
{
    return &s_view;
}
