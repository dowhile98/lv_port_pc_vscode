/**
 * @file eez_settings_screen_view.h
 * @brief EEZ-backed ISettingsScreenView_t — singleton accessor.
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#ifndef EEZ_SETTINGS_SCREEN_VIEW_H
#define EEZ_SETTINGS_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed IListNavScreenView_t for the settings menu.
     * @return Pointer to static vtable instance (never NULL).
     */
    IListNavScreenView_t *EezSettingsScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_SETTINGS_SCREEN_VIEW_H */
