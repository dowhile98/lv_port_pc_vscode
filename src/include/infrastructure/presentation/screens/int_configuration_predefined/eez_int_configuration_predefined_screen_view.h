/**
 * @file eez_int_configuration_predefined_screen_view.h
 * @brief EEZ-backed IIntConfigurationPredefinedScreenView_t — public getter.
 *
 * @note Zero #include "lvgl.h" — callers must not depend on LVGL symbols.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_predefined_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ view for the Predefined Cycles screen.
     *
     * @return Non-NULL pointer to the statically allocated view vtable.
     */
    IIntConfigurationPredefinedScreenView_t *
    EezIntConfigurationPredefinedScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H */
