/**
 * @file eez_int_configuration_days_screen_view.h
 * @brief EEZ Studio LVGL view implementation getter for IntConfigurationDays.
 *
 * Single-function public API.  The returned singleton wires lv_checkbox state
 * and label updates to the EEZ-generated `objects.int_configuration_days_*`
 * widgets.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_days_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ view that drives LVGL widgets.
     *
     * @return Pointer to the static @c IIntConfigurationDaysScreenView_t.
     *         Never NULL.
     */
    IIntConfigurationDaysScreenView_t *EezIntConfigurationDaysScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H */
