/**
 * @file eez_wifi_module_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the WiFi Module screen.
 */
#ifndef EEZ_WIFI_MODULE_SCREEN_VIEW_H
#define EEZ_WIFI_MODULE_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed IListNavScreenView_t for WiFi Module.
     * @return Pointer to static vtable instance (never NULL).
     */
    IListNavScreenView_t *EezWifiModuleScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_WIFI_MODULE_SCREEN_VIEW_H */
