/**
 * @file eez_gps_configuration_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the GPS Configuration screen.
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#ifndef EEZ_GPS_CONFIGURATION_SCREEN_VIEW_H
#define EEZ_GPS_CONFIGURATION_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Return the singleton EEZ-backed view for the GPS Configuration screen. */
    IListNavScreenView_t *EezGpsConfigurationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_GPS_CONFIGURATION_SCREEN_VIEW_H */
