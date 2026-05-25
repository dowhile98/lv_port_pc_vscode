/**
 * @file eez_int_configuration_state_screen_view.h
 * @brief EEZ-backed concrete view for the Interrupt Configuration State screen.
 *
 * Returns a singleton IIntConfigurationStateScreenView_t that drives
 * objects.int_configuration_status using lv_lock()/lv_unlock().
 *
 * @author Tecna Smart Lab
 */
#ifndef EEZ_INT_CONFIGURATION_STATE_SCREEN_VIEW_H
#define EEZ_INT_CONFIGURATION_STATE_SCREEN_VIEW_H

#include "presentation/interfaces/i_int_configuration_state_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton EEZ view for the Int Config State screen.
     *
     * @return Non-NULL pointer to IIntConfigurationStateScreenView_t.
     */
    IIntConfigurationStateScreenView_t *EezIntConfigurationStateScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_INT_CONFIGURATION_STATE_SCREEN_VIEW_H */
