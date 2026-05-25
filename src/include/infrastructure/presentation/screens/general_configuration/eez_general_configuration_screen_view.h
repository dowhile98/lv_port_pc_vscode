/**
 * @file eez_general_configuration_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the General Configuration screen.
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#ifndef EEZ_GENERAL_CONFIGURATION_SCREEN_VIEW_H
#define EEZ_GENERAL_CONFIGURATION_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Return the singleton EEZ-backed view for the General Configuration screen. */
    IListNavScreenView_t *EezGeneralConfigurationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_GENERAL_CONFIGURATION_SCREEN_VIEW_H */
