/**
 * @file eez_int_configuration_start_screen_view.h
 * @brief EEZ-backed concrete view for the Interrupt Configuration Start screen.
 *
 * Returns a singleton IIntConfigurationStartScreenView_t that drives
 * objects.int_configuration_start_value (lv_switch) and
 * objects.int_configuration_start_label (lv_label) using lv_lock()/lv_unlock().
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_INT_CONFIGURATION_START_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_START_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_start_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton EEZ view for the Int Config Start screen.
     *
     * @return Non-NULL pointer to IIntConfigurationStartScreenView_t.
     */
    IIntConfigurationStartScreenView_t *EezIntConfigurationStartScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_START_SCREEN_VIEW_H */
