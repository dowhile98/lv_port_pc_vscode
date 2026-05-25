/**
 * @file eez_admin_operation_mode_screen_view.h
 * @brief EEZ-backed IAdminOperationModeScreenView_t for the Admin Operation Mode screen.
 *
 * @note #include "lvgl.h" is FORBIDDEN here.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef EEZ_ADMIN_OPERATION_MODE_SCREEN_VIEW_H
#define EEZ_ADMIN_OPERATION_MODE_SCREEN_VIEW_H

#include "presentation/interfaces/i_admin_operation_mode_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    IAdminOperationModeScreenView_t *EezAdminOperationModeScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_ADMIN_OPERATION_MODE_SCREEN_VIEW_H */
