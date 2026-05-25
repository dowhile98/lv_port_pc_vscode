/**
 * @file eez_wifi_configuration_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the WiFi Configuration screen.
 */
#ifndef EEZ_WIFI_CONFIGURATION_SCREEN_VIEW_H
#define EEZ_WIFI_CONFIGURATION_SCREEN_VIEW_H

#include <stdbool.h>
#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Update item 1 label to reflect WiFi module availability/state.
     *
     * @param[in] wifi_available  true if IWifiModule is available.
     * @param[in] wifi_enabled    true if module is currently enabled.
     */
    void EezWifiConfigurationScreenView_SetWifiToggleState(bool wifi_available, bool wifi_enabled);

    /**
     * @brief Return the singleton EEZ-backed IListNavScreenView_t for WiFi Configuration.
     * @return Pointer to static vtable instance (never NULL).
     */
    IListNavScreenView_t *EezWifiConfigurationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_WIFI_CONFIGURATION_SCREEN_VIEW_H */
