/**
 * @file eez_access_denied_screen_view.h
 * @brief EEZ-backed concrete view for the Access Denied modal screen.
 *
 * Returns a singleton IAccessDeniedScreenView_t that drives
 * objects.accessdenied_label using lv_lock()/lv_unlock().
 *
 * @author Tecna Smart Lab
 */
#ifndef EEZ_ACCESS_DENIED_SCREEN_VIEW_H
#define EEZ_ACCESS_DENIED_SCREEN_VIEW_H

#include "presentation/interfaces/i_access_denied_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton EEZ view for the Access Denied screen.
     *
     * @return Non-NULL pointer to IAccessDeniedScreenView_t.
     */
    IAccessDeniedScreenView_t *EezAccessDeniedScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_ACCESS_DENIED_SCREEN_VIEW_H */
