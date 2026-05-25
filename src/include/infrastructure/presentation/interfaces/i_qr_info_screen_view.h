/**
 * @file i_qr_info_screen_view.h
 * @brief View contract for the QR Info screen.
 *
 * Separates presenter logic from the LVGL/EEZ implementation.
 * NO #include "lvgl.h" — fully testable on PC with manual mocks.
 *
 * ## Widget mapping (EEZ objects)
 *   SetTitle()   → objects.qr_info_tittle  (lv_label — top title)
 *   SetContent() → objects.qr_info_content (lv_qrcode — QR widget)
 *   SetLabel()   → objects.qr_info_label   (lv_label — bottom description, e.g. URL or SSID)
 *
 * @note ISP: Only the operations the QrInfoScreenPresenter actually calls.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_QR_INFO_SCREEN_VIEW_H
#define I_QR_INFO_SCREEN_VIEW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── V-Table ───────────────────────────────────────────────────────────── */

    typedef struct IQrInfoScreenView_Vtable
    {
        /**
         * @brief Set the title label text.
         * @param[in] self   View context (not NULL).
         * @param[in] title  Null-terminated string (not NULL).
         */
        void (*SetTitle)(void *self, const char *title);

        /**
         * @brief Update the QR code widget with new data.
         * @param[in] self     View context (not NULL).
         * @param[in] qr_data  Null-terminated QR content string (not NULL).
         */
        void (*SetContent)(void *self, const char *qr_data);

        /**
         * @brief Set the bottom description label (explains what the QR shows).
         *
         * For WiFi QRs shows the SSID; for URL QRs shows the URL itself.
         *
         * @param[in] self   View context (not NULL).
         * @param[in] label  Null-terminated string (not NULL).
         */
        void (*SetLabel)(void *self, const char *label);
    } IQrInfoScreenView_Vtable;

    /* ── Interface handle ──────────────────────────────────────────────────── */

    typedef struct IQrInfoScreenView_t
    {
        const IQrInfoScreenView_Vtable *vtable;
        void *impl;
    } IQrInfoScreenView_t;

    /* ── NULL-safe inline dispatch helpers ────────────────────────────────── */

    static inline void IQrInfoScreenView_SetTitle(IQrInfoScreenView_t *v,
                                                  const char *title)
    {
        if (v != NULL && v->vtable != NULL && v->vtable->SetTitle != NULL)
        {
            v->vtable->SetTitle(v->impl, title);
        }
    }

    static inline void IQrInfoScreenView_SetContent(IQrInfoScreenView_t *v,
                                                    const char *qr_data)
    {
        if (v != NULL && v->vtable != NULL && v->vtable->SetContent != NULL)
        {
            v->vtable->SetContent(v->impl, qr_data);
        }
    }

    static inline void IQrInfoScreenView_SetLabel(IQrInfoScreenView_t *v,
                                                  const char *label)
    {
        if (v != NULL && v->vtable != NULL && v->vtable->SetLabel != NULL)
        {
            v->vtable->SetLabel(v->impl, label);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_QR_INFO_SCREEN_VIEW_H */
