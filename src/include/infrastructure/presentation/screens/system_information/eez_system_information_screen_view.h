/**
 * @file eez_system_information_screen_view.h
 * @brief EEZ-backed IListNavScreenView_t for the System Information screen.
 */
#ifndef EEZ_SYSTEM_INFORMATION_SCREEN_VIEW_H
#define EEZ_SYSTEM_INFORMATION_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed IListNavScreenView_t.
     * @return Pointer to static vtable instance (never NULL).
     */
    IListNavScreenView_t *EezSystemInformationScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_SYSTEM_INFORMATION_SCREEN_VIEW_H */
