/**
 * @file information_show_screen_presenter.h
 * @brief Unified presenter for the information_show screen.
 *
 * Consolidates Device Status, Interrupt Configuration, GPS Status and
 * Battery Status into a single screen (SCREEN_ID_INFORMATION_SHOW = 29).
 *
 * The active item is determined by *pending_item_ptr, set via
 * PresentationLayer_SetPendingInfoItem() through the ListNavScreenPresenter
 * pre_navigate_fn hook before the EEZ screen load event fires.
 *
 * ## Provider table (item index → title + content builder)
 *
 * | idx | Title                       | Domain interfaces            |
 * |-----|-----------------------------|------------------------------|
 * |  0  | Device Status               | time_source, relay, hourmeter|
 * |  1  | Interrupt Configuration     | config_storage, time_source  |
 * |  2  | GPS Status                  | config_storage, gps_source   |
 * |  3  | Battery Status              | battery                      |
 * |  4  | Batch                       | device_identity              |
 * |  5  | FW & HW Version             | (none — compile-time)        |
 * |  6  | License Status              | license_status               |
 * |  7  | (reserved)                  | (none)                       |
 * |  8  | AP Information              | config_storage               |
 * |  9  | STA Information             | config_storage, wifi_status, sta_port|
 * | 10  | Connect to AP               | config_storage               |
 * | 11  | Web Server (AP)             | config_storage               |
 * | 12  | Web Server (STA)            | config_storage               |
 * | 13  | Historical Events           | event_log_storage            |
 *
 * @note NO #include "lvgl.h" — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef INFORMATION_SHOW_SCREEN_PRESENTER_H
#define INFORMATION_SHOW_SCREEN_PRESENTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_information_show_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_battery_monitor.h"
#include "interfaces/i_device_identity.h"
#include "interfaces/i_license_status.h"
#include "interfaces/i_wifi_status_source.h"
#include "interfaces/i_network_port.h"
#include "interfaces/i_event_log_storage.h"
#include "interfaces/i_digital_input_source.h"
#include "application/activeobjects/hourmeter_ao.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Content buffer size (bytes) — large enough for the most verbose provider. */
#define INFORMATION_SHOW_CONTENT_BUF_SIZE 1024U

    /**
     * @brief Dependency bundle for InformationShowScreenPresenter_Init().
     *
     * All domain interface pointers are optional (NULL → graceful "--" fallback).
     * view and pending_item_ptr are required.
     */
    typedef struct InformationShowScreenPresenterDeps_t
    {
        IInformationShowScreenView_t *view; /**< View contract.  Required. */
        IScreenRouter_t *router;            /**< Back navigation. NULL OK in tests. */
        uint8_t back_screen_id;             /**< Screen ID on back press. */

        /**
         * @brief Pointer to the shared "pending item" byte owned by presentation_layer.c.
         *
         * Updated by ListNavScreenPresenter via pre_navigate_fn before EEZ routes to
         * SCREEN_ID_INFORMATION_SHOW.  Required — must not be NULL.
         */
        const uint8_t *pending_item_ptr;

        /* ── Domain interfaces (all optional) ─────────────────────────── */
        ITimeSource *time_source;         /**< Date/time (item 0, 1). */
        IRelayController *relay;          /**< Relay state & alarm (item 0). */
        HourmeterAO_t *hourmeter;         /**< Hourmeter counters (item 0). */
        IConfigStorage *config_storage;   /**< Relay & GPS config (item 1, 2). */
        IGPSSource *gps_source;           /**< Live GPS data (item 2). */
        IBatteryMonitor *battery;         /**< BQ27441 fuel gauge (item 3). */
        IDeviceIdentity *device_identity; /**< 96-bit UID reader (item 4). */
        ILicenseStatus *license_status;   /**< License mode / expiry (item 6). */
        IWifiStatusSource_t *wifi_status; /**< STA connected query (item 9). Optional. */
        INetworkPort_t *sta_port;         /**< Live STA IP/mask/gw (item 9). Optional. */
        INetworkPort_t *ap_port;          /**< Live AP  IP (items 8, 11). Optional. */

        /**
         * @brief Historical event log storage (item 13). Optional.
         */
        IEventLogStorage *event_log_storage;

        /**
         * @brief Digital input source for overtemp alarm polling (item 14). Optional.
         */
        IDigitalInputSource *di_source;

        /**
         * @brief Confirmation message text for item 15 (CONFIRMATION). Optional.
         *
         * Pointer to a static string owned by presentation_layer.c.
         * When item 15 is active, the presenter displays this text and
         * auto-dismisses after 1 second.
         */
        const char *pending_confirmation_msg;

        /**
         * @brief Pointer to dynamic back-screen ID (optional).
         *
         * When non-NULL, on_back_pressed() navigates to *back_screen_id_ptr
         * instead of back_screen_id.  Set via PresentationLayer_SetInfoBackScreen()
         * before each navigation to allow WiFi Module and System Information to
         * both share SCREEN_ID_INFORMATION_SHOW with correct back destinations.
         */
        const uint8_t *back_screen_id_ptr;
    } InformationShowScreenPresenterDeps_t;

    /**
     * @brief Unified information_show screen presenter.
     *
     * IScreen_t MUST be the first field (C99 first-field cast rule).
     */
    typedef struct InformationShowScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — lifecycle vtable. */
        IInformationShowScreenView_t *view;
        IScreenRouter_t *router;
        uint8_t back_screen_id;
        const uint8_t *pending_item_ptr;

        /* Domain interfaces */
        ITimeSource *time_source;
        IRelayController *relay;
        HourmeterAO_t *hourmeter;
        IConfigStorage *config_storage;
        IGPSSource *gps_source;
        IBatteryMonitor *battery;
        IDeviceIdentity *device_identity;     /**< 96-bit UID reader (item 4). */
        ILicenseStatus *license_status;       /**< License mode / expiry (item 6). */
        IWifiStatusSource_t *wifi_status;     /**< STA connected query (item 9). */
        INetworkPort_t *sta_port;             /**< Live STA IP/mask/gw (item 9). */
        INetworkPort_t *ap_port;              /**< Live AP  IP (items 8, 11). */
        IEventLogStorage *event_log_storage;  /**< Historical event log (item 13). */
        IDigitalInputSource *di_source;       /**< Overtemp alarm polling (item 14). */
        const char *pending_confirmation_msg; /**< Confirmation text for item 15. */
        const uint8_t *back_screen_id_ptr;    /**< Dynamic back-screen override. */

        bool active;                       /**< true while screen is visible. */
        uint16_t event_log_idx;            /**< Current event index for item 13 navigation. */
        uint32_t confirmation_start_ticks; /**< Ticks when item 15 entered (auto-dismiss). */
    } InformationShowScreenPresenter_t;

    /**
     * @brief Initialise the unified information_show presenter.
     *
     * @param[out] self  Presenter instance (must not be NULL).
     * @param[in]  deps  Dependency bundle (must not be NULL;
     *                   deps->view and deps->pending_item_ptr must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, deps, deps->view, or deps->pending_item_ptr is NULL.
     */
    Result_t InformationShowScreenPresenter_Init(InformationShowScreenPresenter_t *self,
                                                 const InformationShowScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* INFORMATION_SHOW_SCREEN_PRESENTER_H */
