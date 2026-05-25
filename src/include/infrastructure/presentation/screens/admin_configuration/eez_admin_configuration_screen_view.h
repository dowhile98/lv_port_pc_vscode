/**
 * @file eez_admin_configuration_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the Admin Configuration screen.
 *
 * @note #include "lvgl.h" is FORBIDDEN here.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_ADMIN_CONFIGURATION_SCREEN_VIEW_H
#define EEZ_ADMIN_CONFIGURATION_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton IListNavScreenView_t backed by the
     *        EEZ-generated @c objects.admin_configuration_list widget.
     *
     * @return Non-NULL pointer to the static view instance.
     */
    IListNavScreenView_t *EezAdminConfigurationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_ADMIN_CONFIGURATION_SCREEN_VIEW_H */
