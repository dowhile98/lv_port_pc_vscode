/**
 * @file eez_qr_info_screen_view.h
 * @brief EEZ-backed IQrInfoScreenView_t for the QR Info screen.
 */
#ifndef EEZ_QR_INFO_SCREEN_VIEW_H
#define EEZ_QR_INFO_SCREEN_VIEW_H

#include "presentation/interfaces/i_qr_info_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Return the singleton EEZ-backed IQrInfoScreenView_t.
     * @return Pointer to static vtable instance (never NULL).
     */
    IQrInfoScreenView_t *EezQrInfoScreenView_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_QR_INFO_SCREEN_VIEW_H */
