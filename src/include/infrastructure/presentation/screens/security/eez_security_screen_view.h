/**
 * @file eez_security_screen_view.h
 * @brief LVGL/EEZ-backed concrete ISecurityScreenView_t.
 *
 * Single function: returns the singleton view instance.
 * Only `eez_security_screen_view.c` may include `lvgl.h`.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef EEZ_SECURITY_SCREEN_VIEW_H
#define EEZ_SECURITY_SCREEN_VIEW_H

#include "presentation/interfaces/i_security_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed ISecurityScreenView_t.
     * @return Pointer to static instance (never NULL).
     */
    ISecurityScreenView_t *EezSecurityScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_SECURITY_SCREEN_VIEW_H */
