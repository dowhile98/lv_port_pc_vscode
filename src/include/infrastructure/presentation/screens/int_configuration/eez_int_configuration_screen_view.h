/**
 * @file eez_int_configuration_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the Interrupt Configuration screen.
 * @author Tecna Smart Lab
 * @date   2 de Marzo 2026
 */
#ifndef EEZ_INT_CONFIGURATION_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Return the singleton EEZ-backed view for the Interrupt Configuration screen. */
    IListNavScreenView_t *EezIntConfigurationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_SCREEN_VIEW_H */
