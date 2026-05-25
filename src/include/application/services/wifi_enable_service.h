/**
 * @file    wifi_enable_service.h
 * @brief   WiFi module power control service (enable/disable + timeout watchdog).
 *
 * Service layer implementation of IWifiModule interface. Manages WiFi module
 * physical power state via ESP32 reset control with configurable auto-disable
 * timeout. Saves state to EEPROM and provides buzzer feedback.
 *
 * Dependencies:
 * - IWifiTransport: Hardware reset control (WifiTransport_ResetESP32)
 * - IConfigStorage: EEPROM persistence (Load/SaveWifiConfig)
 * - IBuzzerControl: Audio feedback patterns
 *
 * Thread-safety: NOT thread-safe. Call from single thread (System_Update).
 * Concurrency: Enable/Disable include ~50ms blocking delays (not ISR-safe).
 *
 * Related: WIFI_ENABLE_DISABLE_DESIGN.md, i_wifi_module.h
 *
 * @author  TCS Team
 * @date    2026-04-11
 */

#ifndef WIFI_ENABLE_SERVICE_H
#define WIFI_ENABLE_SERVICE_H

#include "interfaces/i_wifi_module.h"
#include "interfaces/i_wifi_transport.h"
#include "interfaces/i_config_storage.h"
#include "domain/services/buzzer_notification_service.h"
#include "interfaces/i_logger.h"
#include "hal_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Configuration ───────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Configuration for WifiEnableService initialization.
     */
    typedef struct
    {
        IWifiTransport *transport;           /**< WiFi transport (ESP32 reset control). Must not be NULL. */
        IConfigStorage *config_storage;      /**< Config storage (EEPROM). Must not be NULL. */
        BuzzerNotificationService_t *buzzer; /**< Buzzer notification service (event-driven). Must not be NULL. */
        ILogger *logger;                     /**< Logger for debug output (optional, can be NULL). */
        /**
         * @brief Called whenever WiFi is disabled (manually or by timeout).
         * @note  Used by DI container to update WifiStatusAdapter (icon feedback).
         *        Application layer does NOT know about the adapter — pure callback injection.
         * @param[in] ctx  Caller-provided context (may be NULL).
         */
        void (*on_wifi_disabled)(void *ctx); /**< Optional callback: fires on disable + timeout auto-disable. */
        void *on_wifi_disabled_ctx;          /**< Context passed to on_wifi_disabled (may be NULL). */
    } WifiEnableServiceConfig_t;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Service State ───────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief WiFi Enable Service instance.
     * @note  Opaque to callers — use via IWifiModule interface.
     */
    typedef struct WifiEnableService_t
    {
        /* Dependencies */
        IWifiTransport *transport;
        IConfigStorage *config_storage;
        BuzzerNotificationService_t *buzzer;
        ILogger *logger; /**< Optional logger (can be NULL). */

        /* State */
        bool enabled;                 /**< true if WiFi module enabled (ESP32 running). */
        uint32_t enable_timestamp_ms; /**< os_ticks_get() when Enable() was called. */
        uint16_t timeout_minutes;     /**< Auto-disable timeout (0 = always on, 1-255 = minutes). */

        /* Callbacks */
        void (*on_wifi_disabled)(void *ctx); /**< Injected callback: fired on disable + timeout. */
        void *on_wifi_disabled_ctx;          /**< Context for on_wifi_disabled. */

        /* Interface instance */
        IWifiModule_t iface; /**< Public interface (vtable + impl pointer). */

    } WifiEnableService_t;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Public API ──────────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Initialize WiFi Enable Service.
     * @note   Call once during system startup (from DI_Container).
     *
     * @param[out] self    Service instance to initialize (must not be NULL).
     * @param[in]  config  Configuration (all pointers must be valid).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if self or config is NULL;
     *         ERR_INVALID_PARAM if any dependency in config is NULL.
     */
    Result_t WifiEnableService_Init(WifiEnableService_t *self, const WifiEnableServiceConfig_t *config);

    /**
     * @brief  Get IWifiModule interface for this service.
     * @note   Used by DI_Container to wire interface to callers.
     *
     * @param[in] self  Service instance (must not be NULL).
     *
     * @return Pointer to IWifiModule_t interface, or NULL if self is NULL.
     */
    IWifiModule_t *WifiEnableService_GetInterface(WifiEnableService_t *self);

    /**
     * @brief  Sync WiFi module state from persisted configuration at startup.
     * @note   Call once from control_task after initial startup delay.
     *         Loads wifi_enable and wifi_timeout_minutes from EEPROM and applies
     *         state to ESP32 reset line + internal service state without beeps/toggles.
     *
     * @param[in,out] self  Service instance (must not be NULL).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if self is NULL.
     */
    Result_t WifiEnableService_SyncStartupState(WifiEnableService_t *self);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_ENABLE_SERVICE_H */
