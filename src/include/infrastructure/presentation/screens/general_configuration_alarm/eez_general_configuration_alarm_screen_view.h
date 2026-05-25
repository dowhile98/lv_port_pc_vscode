/**
 * @file eez_general_configuration_alarm_screen_view.h
 * @brief EEZ-backed IGeneralConfigurationAlarmScreenView_t getter.
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H
#define EEZ_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H

#include "presentation/interfaces/i_general_configuration_alarm_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view interface.
     *
     * @return Pointer to the statically-allocated view instance.
     */
    IGeneralConfigurationAlarmScreenView_t *EezGeneralConfigurationAlarmScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_GENERAL_CONFIGURATION_ALARM_SCREEN_VIEW_H */
