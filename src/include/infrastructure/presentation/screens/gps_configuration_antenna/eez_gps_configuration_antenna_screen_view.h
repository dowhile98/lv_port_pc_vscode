/**
 * @file eez_gps_configuration_antenna_screen_view.h
 * @brief EEZ-backed IGpsConfigurationAntennaScreenView_t — single getter symbol.
 *
 * @note #include "lvgl.h" is FORBIDDEN here.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H
#define EEZ_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H

#include "presentation/interfaces/i_gps_configuration_antenna_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view interface.
     *
     * Backed by @c objects.gps_configuration_antenna_list (lv_list) and
     * @c objects.gps_configuration_antenna_label (lv_label).
     *
     * @return Non-NULL pointer to the statically-allocated interface instance.
     */
    IGpsConfigurationAntennaScreenView_t *EezGpsConfigurationAntennaScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_GPS_CONFIGURATION_ANTENNA_SCREEN_VIEW_H */
