/**
 * @file eez_menu_screen_view.h
 * @brief EEZ-backed IMenuScreenView_t — header.
 *
 * @note Zero #include "lvgl.h" — safe to include from presenters and tests.
 *
 * @author Tecna Smart Lab
 * @date   28 de Febrero 2026
 */
#ifndef EEZ_MENU_SCREEN_VIEW_H
#define EEZ_MENU_SCREEN_VIEW_H

#include "presentation/interfaces/i_list_nav_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed IListNavScreenView_t for the main menu.
     * @return Pointer to static vtable instance (never NULL).
     */
    IListNavScreenView_t *EezMenuScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_MENU_SCREEN_VIEW_H */
