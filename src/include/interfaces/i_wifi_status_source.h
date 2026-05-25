/**
 * @file i_wifi_status_source.h
 * @brief Minimal ISP-compliant interface for querying WiFi STA connection status.
 *
 * @details
 * Follows ISP (Interface Segregation Principle): presenters that only need to
 * know "is WiFi connected?" should not depend on the full IWifiControl_t (1099 lines).
 * The DI container provides an adapter that reads from NetworkAO cached state.
 *
 * @note Intentionally zero dependencies beyond <stdbool.h>.
 * @note Thread-safety: implementation must be safe to call from Control AO thread.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */

#ifndef I_WIFI_STATUS_SOURCE_H
#define I_WIFI_STATUS_SOURCE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ===== V-Table ===== */

    typedef struct IWifiStatusSource_Vtable
    {
        /**
         * @brief Returns current STA connection status.
         * @param[in] impl  Implementation pointer (never NULL — checked by helper).
         * @return true if STA is connected to an AP, false otherwise.
         */
        bool (*IsConnected)(void *impl);
    } IWifiStatusSource_Vtable;

    /* ===== Interface instance ===== */

    typedef struct IWifiStatusSource_t
    {
        const IWifiStatusSource_Vtable *vtable;
        void *impl;
    } IWifiStatusSource_t;

    /* ===== Defensive helpers ===== */

    static inline bool wifi_status_source_is_valid(const IWifiStatusSource_t *s)
    {
        return (s != NULL) && (s->vtable != NULL) && (s->impl != NULL);
    }

    /**
     * @brief NULL-safe query: is WiFi STA connected?
     * @param[in] s  Interface instance (may be NULL — returns false).
     * @return true if connected, false if disconnected or interface is invalid.
     */
    static inline bool WifiStatusSource_IsConnected(const IWifiStatusSource_t *s)
    {
        if (!wifi_status_source_is_valid(s))
        {
            return false;
        }
        return s->vtable->IsConnected(s->impl);
    }

#ifdef __cplusplus
}
#endif
#endif /* I_WIFI_STATUS_SOURCE_H */
