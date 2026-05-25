/**
 * @file license_service.h
 * @brief Domain service for license management (Free / Rental modes).
 *
 * Implements ILicenseStatus_t.  Reads SuperUserConfig_t.license_key and
 * expiration_date from IConfigStorage, compares against the current
 * ITimeSource time, and exposes the result as a LicenseStatus_t.
 *
 * ## Fail-safe policy
 * - Storage load fails → treat as FREE (device never permanently locked).
 * - Time source fails  → treat as RENTAL_ACTIVE (fail open).
 *
 * ## Usage
 * ```c
 * LicenseService_t          svc;
 * LicenseServiceConfig_t    cfg = {
 *     .config_storage = DI_GetConfigStorage(&container),
 *     .time_source    = DI_GetTimeSource(&container),
 * };
 * LicenseService_Init(&svc, &cfg);
 *
 * // On each System_Update():
 * ILicenseStatus *lic = LicenseService_GetInterface(&svc);
 * if (LicenseStatus_IsBlocked(lic)) { ... enforce ... }
 * ```
 *
 * @note NO #include "lvgl.h" — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef LICENSE_SERVICE_H
#define LICENSE_SERVICE_H

#include "interfaces/i_license_status.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_time_source.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Configuration bundle injected into LicenseService_Init().
     */
    typedef struct LicenseServiceConfig_t
    {
        IConfigStorage *config_storage; /**< Required — source of SuperUserConfig_t. */
        ITimeSource *time_source;       /**< Required — current date/time for expiry check. */
    } LicenseServiceConfig_t;

    /**
     * @brief LicenseService concrete instance.
     *
     * Implements ILicenseStatus_t via a static vtable.
     * Statically allocated — no malloc.
     */
    typedef struct LicenseService_t
    {
        ILicenseStatus iface; /**< Must be first — returned by GetInterface(). */
        IConfigStorage *config_storage;
        ITimeSource *time_source;
    } LicenseService_t;

    /**
     * @brief Initialise the LicenseService.
     *
     * @param[out] self  Instance (must not be NULL).
     * @param[in]  cfg   Config bundle (must not be NULL; config_storage must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, cfg, or cfg->config_storage is NULL.
     */
    Result_t LicenseService_Init(LicenseService_t *self, const LicenseServiceConfig_t *cfg);

    /**
     * @brief Returns the ILicenseStatus interface pointer.
     *
     * @param[in] self  Initialised instance (must not be NULL).
     * @return Pointer to the interface, or NULL if self is NULL.
     */
    ILicenseStatus *LicenseService_GetInterface(LicenseService_t *self);

    /**
     * @brief Direct status query (no interface indirection needed).
     *
     * @param[in]  self  Instance (must not be NULL).
     * @param[out] out   Filled status (must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self or out is NULL.
     */
    Result_t LicenseService_GetStatus(LicenseService_t *self, LicenseStatus_t *out);

#ifdef __cplusplus
}
#endif

#endif /* LICENSE_SERVICE_H */
