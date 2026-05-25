/**
 * @file i_device_identity.h
 * @brief Device Identity Interface — abstract read-only access to the STM32 96-bit UID.
 *
 * Domain/Application layers use this interface to query the unique chip identifier
 * without depending on stm32u5xx_hal.h. The concrete implementation in BSP reads
 * HAL_GetUIDw0/1/2 and formats a hex string.
 *
 * @note Thread-safe: GetUidString reads factory-ROM registers (read-only, const after
 *       power-on) — no locking needed.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
#ifndef I_DEVICE_IDENTITY_H
#define I_DEVICE_IDENTITY_H

#include <stdint.h>
#include <stddef.h>
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Maximum length of the UID string including null terminator.
 *  96 bits = 24 hex chars + null = 25. We keep 32 for alignment. */
#define DEVICE_IDENTITY_UID_STR_LEN 32U

    /* ── 1. Vtable struct ─────────────────────────────────────────────────────── */

    /**
     * @brief V-Table for IDeviceIdentity.
     */
    typedef struct IDeviceIdentity_Vtable
    {
        /**
         * @brief Copy the null-terminated UID hex string into @p buf.
         *
         * Format: 24 upper-case hex characters, e.g. "1A2B3C4D5E6F7A8B9C0D1E2F"
         * (word2 || word1 || word0, big-endian per word, MSB first).
         *
         * @note Read-only operation — always succeeds if pointers are non-NULL.
         *
         * @param[in]  self      Implementation instance (must not be NULL).
         * @param[out] buf       Destination buffer (must not be NULL).
         * @param[in]  buf_size  Size of @p buf in bytes. Must be >= 25.
         *
         * @return ERR_OK            on success.
         * @return ERR_NULL_POINTER  if self or buf is NULL.
         * @return ERR_INVALID_PARAM if buf_size < 25.
         */
        Result_t (*GetUidString)(void *self, char *buf, uint32_t buf_size);

    } IDeviceIdentity_Vtable;

    /* ── 2. Interface instance struct ─────────────────────────────────────────── */

    /**
     * @brief Polymorphic interface handle for IDeviceIdentity.
     */
    typedef struct IDeviceIdentity
    {
        const IDeviceIdentity_Vtable *vtable; /**< Function pointer table. */
        void *impl;                           /**< Concrete implementation instance. */
    } IDeviceIdentity;

    /* ── 3. NULL-safe inline wrappers ─────────────────────────────────────────── */

    /**
     * @brief NULL-safe dispatch — GetUidString.
     *
     * Writes "--" into @p buf when the interface is unavailable.
     */
    static inline Result_t DeviceIdentity_GetUidString(IDeviceIdentity *iface,
                                                       char *buf,
                                                       uint32_t buf_size)
    {
        if (iface == NULL || iface->vtable == NULL || iface->vtable->GetUidString == NULL)
        {
            if (buf != NULL && buf_size >= 3U)
            {
                buf[0] = '-';
                buf[1] = '-';
                buf[2] = '\0';
            }
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetUidString(iface->impl, buf, buf_size);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_DEVICE_IDENTITY_H */
