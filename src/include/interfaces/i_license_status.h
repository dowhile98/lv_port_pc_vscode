/**
 * @file i_license_status.h
 * @brief Domain interface for querying device license status.
 *
 * Abstracts the two operating modes:
 *   LICENSE_MODE_FREE          — no restrictions, no expiration check.
 *   LICENSE_MODE_RENTAL_ACTIVE — rental mode, license still valid.
 *   LICENSE_MODE_RENTAL_EXPIRED — rental mode, license has expired.
 *
 * @note NO hardware includes — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef I_LICENSE_STATUS_H
#define I_LICENSE_STATUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── License mode enum ──────────────────────────────────────────────────── */

    /**
     * @brief Operating license mode.
     */
    typedef enum LicenseMode
    {
        LICENSE_MODE_FREE = 0U,           /**< Free mode — no expiration restrictions. */
        LICENSE_MODE_RENTAL_ACTIVE = 1U,  /**< Rental mode — license valid. */
        LICENSE_MODE_RENTAL_EXPIRED = 2U, /**< Rental mode — license expired. */
    } LicenseMode_t;

    /**
     * @brief Snapshot of the current license status.
     */
    typedef struct LicenseStatus
    {
        LicenseMode_t mode;     /**< Current mode. */
        int32_t days_remaining; /**< Days until expiration (>0 = valid, <=0 = expired).
                                 *   Meaningless (0) when mode == LICENSE_MODE_FREE. */
    } LicenseStatus_t;

    /* ── Interface vtable ───────────────────────────────────────────────────── */

    typedef struct ILicenseStatus_Vtable
    {
        /**
         * @brief Returns the current license status snapshot.
         * @param[in]  self    Implementation pointer (must not be NULL).
         * @param[out] out     Output struct (must not be NULL).
         * @return ERR_OK, ERR_NULL_POINTER.
         */
        Result_t (*GetStatus)(void *self, LicenseStatus_t *out);

        /**
         * @brief Returns true when the license is expired and the device must be blocked.
         * @param[in] self Implementation pointer.
         * @return true if rental mode AND expired; false in all other cases.
         */
        bool (*IsBlocked)(void *self);

    } ILicenseStatus_Vtable;

    /**
     * @brief Interface instance.
     */
    typedef struct ILicenseStatus
    {
        const ILicenseStatus_Vtable *vtable;
        void *impl;
    } ILicenseStatus;

    /* ── NULL-safe dispatch helpers ─────────────────────────────────────────── */

    static inline bool license_status_is_valid(const ILicenseStatus *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
    }

    static inline Result_t LicenseStatus_GetStatus(ILicenseStatus *iface, LicenseStatus_t *out)
    {
        if (!license_status_is_valid(iface) || out == NULL)
            return ERR_NULL_POINTER;
        return iface->vtable->GetStatus(iface->impl, out);
    }

    static inline bool LicenseStatus_IsBlocked(ILicenseStatus *iface)
    {
        if (!license_status_is_valid(iface))
            return false;
        return iface->vtable->IsBlocked(iface->impl);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_LICENSE_STATUS_H */
