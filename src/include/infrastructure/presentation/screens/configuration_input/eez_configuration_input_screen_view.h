/**
 * @file eez_configuration_input_screen_view.h
 * @brief EEZ-backed concrete view for the Configuration Input (HH:MM:SS) screen.
 *
 * Returns a singleton IConfigurationInputScreenView_t that drives
 * objects.configuration_input_* widgets using lv_lock()/lv_unlock().
 *
 * @author Tecna Smart Lab
 */
#ifndef EEZ_CONFIGURATION_INPUT_SCREEN_VIEW_H
#define EEZ_CONFIGURATION_INPUT_SCREEN_VIEW_H

#include "presentation/interfaces/i_configuration_input_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Returns the singleton EEZ view for the Configuration Input screen.
     *
     * @return Non-NULL pointer to IConfigurationInputScreenView_t.
     */
    IConfigurationInputScreenView_t *EezConfigurationInputScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_CONFIGURATION_INPUT_SCREEN_VIEW_H */
