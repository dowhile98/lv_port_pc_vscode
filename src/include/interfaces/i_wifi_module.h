/**
 * @file    i_wifi_module.h
 * @brief   WiFi module power control interface (enable/disable + timeout).
 *
 * Abstraction for controlling WiFi module physical power state via ESP32 reset
 * control with configurable timeout auto-disable. Provides decoupled API for
 * Application and Presentation layers to manage WiFi module without knowing
 * hardware details (IWifiTransport) or config storage (IConfigStorage).
 *
 * Orthogonal to i_wifi_control.h (which handles network operations like
 * STA connect, AP start). This interface is about module power lifecycle.
 *
 * @note   Thread-safe via OSAL mutex (implementation-dependent).
 * @note   Enable/Disable operations call WifiTransport_ResetESP32() which
 *         includes hardware delays (~50ms). Call from non-ISR context only.
 *
 * Related: WIFI_ENABLE_DISABLE_DESIGN.md, i_wifi_transport.h, wifi_types.h
 *
 * @author  TCS Team
 * @date    2026-04-11
 */

#ifndef I_WIFI_MODULE_H
#define I_WIFI_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── V-Table Definition ──────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief V-Table for IWifiModule interface.
     * @note  All function pointers take `void *self` as first parameter.
     */
    typedef struct IWifiModule_Vtable
    {
        /**
         * @brief  Enable WiFi module (de-assert ESP32 reset, start module).
         * @note   NOT ISR-safe (calls hard_delay ~50ms for ESP32 boot).
         *         Saves wifi_enable=1 to EEPROM and starts timeout timer.
         *         Triggers buzzer feedback (2 beeps).
         *
         * @param[in] self  Implementation instance (must not be NULL).
         *
         * @return ERR_OK on success; ERR_NULL_POINTER if self is NULL;
         *         ERR_ERROR if transport reset operation failed.
         */
        Result_t (*Enable)(void *self);

        /**
         * @brief  Disable WiFi module (assert ESP32 reset, hold in reset state).
         * @note   NOT ISR-safe (calls hard_delay).
         *         Saves wifi_enable=0 to EEPROM and stops timeout timer.
         *         Triggers buzzer feedback (1 long beep).
         *
         * @param[in] self  Implementation instance (must not be NULL).
         *
         * @return ERR_OK on success; ERR_NULL_POINTER if self is NULL;
         *         ERR_ERROR if transport reset operation failed.
         */
        Result_t (*Disable)(void *self);

        /**
         * @brief  Check if WiFi module is currently enabled.
         * @note   Thread-safe; ISR-safe (read-only state query).
         *
         * @param[in] self  Implementation instance (must not be NULL).
         *
         * @return true if WiFi enabled (ESP32 running), false if disabled or self is NULL.
         */
        bool (*IsEnabled)(const void *self);

        /**
         * @brief  Update timeout watchdog (call periodically from System_Update).
         * @note   Thread-safe; NOT ISR-safe (may call Disable() internally).
         *         If timeout elapsed (wifi_timeout_minutes > 0 and time exceeded),
         *         automatically calls Disable() and triggers buzzer feedback (3 beeps).
         *         Typical call period: ~100ms.
         *
         * @param[in] self  Implementation instance (must not be NULL).
         *
         * @return ERR_OK on success or no timeout; ERR_NULL_POINTER if self is NULL;
         *         ERR_TIMEOUT if auto-disable was triggered this call.
         */
        Result_t (*Update)(void *self);

    } IWifiModule_Vtable;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Interface Instance ──────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Polymorphic interface handle for WiFi module control.
     *
     * Callers (Presenters, Services, SideButtonService) hold a pointer to this
     * struct and call via inline wrappers below. Concrete implementation
     * (WifiEnableService) fills `.vtable` and `.impl` during Init().
     */
    typedef struct IWifiModule
    {
        const IWifiModule_Vtable *vtable; /**< Points to the concrete vtable (never NULL). */
        void *impl;                       /**< Points to the concrete instance (never NULL). */
    } IWifiModule_t;

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Validity Helper ─────────────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Check that a IWifiModule handle is fully initialised.
     * @return true if all mandatory function pointers and impl are non-NULL.
     */
    static inline bool WifiModule_IsValid(const IWifiModule_t *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->vtable->Enable != NULL) && (iface->vtable->Disable != NULL) && (iface->vtable->IsEnabled != NULL) && (iface->vtable->Update != NULL) && (iface->impl != NULL);
    }

    /* ══════════════════════════════════════════════════════════════════════════ */
    /* ── Inline Wrapper Functions ────────────────────────────────────────────── */
    /* ══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Enable WiFi module (de-assert ESP32 reset).
     * @note   NOT ISR-safe. Includes ~50ms hardware delay for ESP32 boot.
     *
     * @param[in] iface  WiFi module interface (IWifiModule_t *).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if iface invalid;
     *         ERR_ERROR if transport operation failed.
     */
    static inline Result_t WifiModule_Enable(IWifiModule_t *iface)
    {
        if (!WifiModule_IsValid(iface))
            return ERR_NULL_POINTER;

        return iface->vtable->Enable(iface->impl);
    }

    /**
     * @brief  Disable WiFi module (assert ESP32 reset).
     * @note   NOT ISR-safe. Includes hardware delay.
     *
     * @param[in] iface  WiFi module interface (IWifiModule_t *).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if iface invalid;
     *         ERR_ERROR if transport operation failed.
     */
    static inline Result_t WifiModule_Disable(IWifiModule_t *iface)
    {
        if (!WifiModule_IsValid(iface))
            return ERR_NULL_POINTER;

        return iface->vtable->Disable(iface->impl);
    }

    /**
     * @brief  Check if WiFi module is currently enabled.
     * @note   Thread-safe; ISR-safe.
     *
     * @param[in] iface  WiFi module interface (const IWifiModule_t *).
     *
     * @return true if WiFi enabled, false if disabled or iface invalid.
     */
    static inline bool WifiModule_IsEnabled(const IWifiModule_t *iface)
    {
        if (!WifiModule_IsValid(iface))
            return false;

        return iface->vtable->IsEnabled(iface->impl);
    }

    /**
     * @brief  Update timeout watchdog (periodic call from System_Update).
     * @note   Thread-safe; NOT ISR-safe.
     *         Call period: ~100ms typical.
     *
     * @param[in] iface  WiFi module interface (IWifiModule_t *).
     *
     * @return ERR_OK on success; ERR_NULL_POINTER if iface invalid;
     *         ERR_TIMEOUT if auto-disable triggered.
     */
    static inline Result_t WifiModule_Update(IWifiModule_t *iface)
    {
        if (!WifiModule_IsValid(iface))
            return ERR_NULL_POINTER;

        return iface->vtable->Update(iface->impl);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_WIFI_MODULE_H */
