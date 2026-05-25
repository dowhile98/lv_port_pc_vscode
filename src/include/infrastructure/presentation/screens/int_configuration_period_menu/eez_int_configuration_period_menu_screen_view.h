/**
 * @file eez_int_configuration_period_menu_screen_view.h
 * @brief EEZ view getter for the IntConfigurationPeriodMenu screen.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_period_menu_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view for the period menu screen.
     *
     * @return Pointer to the static @c IIntConfigurationPeriodMenuScreenView_t instance.
     */
    IIntConfigurationPeriodMenuScreenView_t *EezIntConfigurationPeriodMenuScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_PERIOD_MENU_SCREEN_VIEW_H */
