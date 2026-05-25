/**
 * @file eez_int_configuration_period_screen_view.h
 * @brief EEZ view getter for the IntConfigurationPeriod roller screen.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_period_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed view for the period roller screen.
     *
     * @return Pointer to the static @c IIntConfigurationPeriodScreenView_t instance.
     */
    IIntConfigurationPeriodScreenView_t *EezIntConfigurationPeriodScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_PERIOD_SCREEN_VIEW_H */
