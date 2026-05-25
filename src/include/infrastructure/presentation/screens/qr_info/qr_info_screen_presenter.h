/**
 * @file qr_info_screen_presenter.h
 * @brief Presenter for the QR Info screen (SCREEN_ID_QR_INFO = 26).
 *
 * Renders a QR code for WiFi credentials or the Web Server URL, depending on
 * *pending_item_ptr set by the ListNavScreenPresenter pre_navigate_fn hook.
 *
 * ## Provider table (item index → QR content)
 *
 * | idx | Title                   | QR data                    | Label              |
 * |-----|-------------------------|----------------------------|--------------------||
 * |  0  | Connect via AP (QR)     | WIFI:S:…;T:WPA;P:…;;       | SSID: <ap_ssid>    |
 * |  1  | Web Server (AP) QR      | http://<ap_ip>             | http://<ap_ip>     |
 * |  2  | Web Server (STA) QR     | http://<sta_ip_live>       | http://<sta_ip>    |
 *
 * @note NO #include "lvgl.h" — fully testable on PC.
 *
 * @author Tecna Smart Lab
 */
#ifndef QR_INFO_SCREEN_PRESENTER_H
#define QR_INFO_SCREEN_PRESENTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_qr_info_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_device_identity.h"
#include "interfaces/i_network_port.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief QR data buffer — large enough for WIFI:S:...;T:WPA;P:...;; strings. */
#define QR_INFO_DATA_BUF_SIZE 128U

    /**
     * @brief Dependency bundle for QrInfoScreenPresenter_Init().
     *
     * view and pending_item_ptr are required; all others are optional.
     */
    typedef struct QrInfoScreenPresenterDeps_t
    {
        IQrInfoScreenView_t *view; /**< View contract. Required. */
        IScreenRouter_t *router;   /**< Back navigation. NULL OK in tests. */
        uint8_t back_screen_id;    /**< Fallback screen ID on back press. */

        /**
         * @brief Pointer to the shared "pending QR item" byte owned by presentation_layer.c.
         *
         * Set by PresentationLayer_SetPendingQrItem() before the screen loads.
         * Required — must not be NULL.
         */
        const uint8_t *pending_item_ptr;

        /**
         * @brief Pointer to dynamic back-screen ID (optional).
         *
         * When non-NULL, on_back_pressed() navigates to *back_screen_id_ptr
         * instead of back_screen_id.
         */
        const uint8_t *back_screen_id_ptr;

        /* ── Domain interfaces (optional) ──────────────────────────────── */
        IConfigStorage *config_storage;   /**< WiFi AP SSID, password, IP. */
        IDeviceIdentity *device_identity; /**< 96-bit UID for TCS-xxxxxxxx SSID. */
        INetworkPort_t *sta_port;         /**< Live STA IP for provider 2. Optional. */
    } QrInfoScreenPresenterDeps_t;

    /**
     * @brief QR Info screen presenter.
     *
     * IScreen_t MUST be the first field (C99 first-field cast rule).
     */
    typedef struct QrInfoScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — lifecycle vtable. */
        IQrInfoScreenView_t *view;
        IScreenRouter_t *router;
        uint8_t back_screen_id;
        const uint8_t *pending_item_ptr;
        const uint8_t *back_screen_id_ptr;

        IConfigStorage *config_storage;
        IDeviceIdentity *device_identity;
        INetworkPort_t *sta_port; /**< Live STA IP for provider 2. */

        bool active; /**< true while screen is visible. */
    } QrInfoScreenPresenter_t;

    /**
     * @brief Initialise the QR Info presenter.
     *
     * @param[out] self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL;
     *                   deps->view and deps->pending_item_ptr must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, deps, deps->view, or deps->pending_item_ptr is NULL.
     */
    Result_t QrInfoScreenPresenter_Init(QrInfoScreenPresenter_t *self,
                                        const QrInfoScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* QR_INFO_SCREEN_PRESENTER_H */
