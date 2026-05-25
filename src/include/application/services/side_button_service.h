/**
 * @file side_button_service.h
 * @brief Multi-function button service for SSR3-V1 (no display boards).
 *
 * Detects button patterns and executes actions:
 * - 1 click → relay manual toggle
 * - 2 clicks → GPS reset
 * - 3 clicks → emergency relay toggle
 * - Long press 500ms → status report (buzzer sequence)
 * - Long press 2s → WiFi toggle
 * - Long press 15s → factory reset
 *
 * @note  Thread-safe (requires OSAL for timing).
 * @note  NOT ISR-safe (uses delays, config storage).
 * @note  Call SideButtonService_Update() from main loop (~100ms).
 *
 * Related: SIDE_BUTTON_SERVICE_DESIGN.md, DigitalInputAO, IWifiModule
 *
 * @author  TCS Team
 * @date    2026-04-11
 */

#ifndef SIDE_BUTTON_SERVICE_H
#define SIDE_BUTTON_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_wifi_module.h"
#include "interfaces/i_gps_control.h"
#include "domain/services/buzzer_notification_service.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_digital_input_source.h"
#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Configuration ────────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Configuration structure for SideButtonService initialization.
     *
     * All dependencies must be valid (non-NULL pointers).
     */
    typedef struct
    {
        IRelayController *relay;             /**< Relay control interface (Start/Stop/EmergencyStop) */
        IWifiModule_t *wifi_module;          /**< WiFi module power control (Enable/Disable) */
        IGPSControl *gps;                    /**< GPS control interface (Reset) */
        BuzzerNotificationService_t *buzzer; /**< Buzzer notification service (event-driven audio feedback) */
        IConfigStorage *config;              /**< Config storage (relay mode, factory reset) */
        ILogger *logger;                     /**< Logger for debug output (optional, can be NULL) */
    } SideButtonServiceConfig_t;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Service Instance ─────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief SideButtonService instance (opaque structure).
     *
     * Internal state managed by implementation. Do not access fields directly.
     */
    typedef struct SideButtonService
    {
        /* Dependencies */
        IRelayController *relay;
        IWifiModule_t *wifi_module;
        IGPSControl *gps;
        BuzzerNotificationService_t *buzzer;
        IConfigStorage *config;
        ILogger *logger; /**< Optional logger (can be NULL) */

        /* Pattern detection state machine */
        uint8_t click_count;               /**< Number of clicks in current window */
        uint32_t first_click_timestamp_ms; /**< Timestamp of first click in sequence */
        uint32_t press_start_timestamp_ms; /**< Timestamp when button was pressed */
        bool is_pressed;                   /**< Current button physical state */
        uint16_t multi_click_window_ms;    /**< Multi-click detection window (default: 1000ms) */

        /* Long press tracking */
        bool long_press_500ms_triggered;
        bool long_press_2s_triggered;
        bool long_press_15s_triggered;

        /* Callback function pointer (registered in DigitalInputAO) */
        void (*on_button_event)(void *context,
                                DigitalInputID_t id,
                                DigitalInputEvent_t event,
                                const DigitalInputEventData_t *data);

    } SideButtonService_t;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Public API ────────────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Initialize SideButtonService with dependencies.
     *
     * @param[in,out] service  Service instance (must not be NULL).
     * @param[in]     config   Configuration with all dependencies (must not be NULL).
     *
     * @return ERR_OK on success;
     *         ERR_NULL_POINTER if service or config is NULL;
     *         ERR_NULL_POINTER if any dependency in config is NULL.
     */
    Result_t SideButtonService_Init(SideButtonService_t *service, const SideButtonServiceConfig_t *config);

    /**
     * @brief Periodic update function (call from main loop or control task).
     *
     * Checks for:
     * - Multi-click window timeout → dispatch pattern
     * - Long press thresholds (500ms, 2s, 15s)
     *
     * @note  Thread-safe; NOT ISR-safe (may execute actions with delays).
     * @note  Recommended call period: ~100ms.
     *
     * @param[in,out] service  Service instance (must not be NULL).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if service is NULL.
     */
    Result_t SideButtonService_Update(SideButtonService_t *service);

    /**
     * @brief Register service with DigitalInputAO to receive BUTTON_SIDE events.
     *
     * @note  Call after DI_InitDigitalInputSubsystem() in DI Container wiring.
     *
     * @param[in,out] service    Service instance (must not be NULL).
     * @param[in]     input_source  Digital input source interface (must not be NULL).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if service or input_source is NULL.
     */
    Result_t SideButtonService_RegisterEvents(SideButtonService_t *service, IDigitalInputSource *input_source);

#ifdef __cplusplus
}
#endif

#endif /* SIDE_BUTTON_SERVICE_H */
