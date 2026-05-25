/**
 * @file bsp_stm32u5_device_identity.c
 * @brief BSP Device Identity — STM32U5 96-bit Unique ID reader.
 *
 * ONLY this file is allowed to call HAL_GetUIDw0/1/2.
 * Formats the 12-byte UID as a 24-character upper-case hex string:
 *   "WWWWWWWWWWWWWWWWWWWWWWWW"  (Word2 || Word1 || Word0, each little-endian)
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */

/* ── ONLY BSP file allowed to include HAL for UID access ─────────────────── */
#include "stm32u5xx_hal.h"

#include "bsp_stm32u5_device_identity.h"
#include <string.h>
#include <stdint.h>
#include "lwprintf.h"
#include <inttypes.h>
/* ── Private vtable impl ──────────────────────────────────────────────────── */

static Result_t get_uid_string(void *self, char *buf, uint32_t buf_size)
{
    (void)self; /* UID is read from fixed ROM — no instance state needed */

    if (buf == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (buf_size < 25U)
    {
        return ERR_INVALID_PARAM;
    }

    uint32_t w0 = HAL_GetUIDw0();
    uint32_t w1 = HAL_GetUIDw1();
    uint32_t w2 = HAL_GetUIDw2();

    /* Format: W2W1W0 — 8 hex digits per word, upper-case, no separator */
    (void)lwprintf_snprintf(buf, (size_t)buf_size,
                   "%08" PRIX32 "%08" PRIX32 "%08" PRIX32,
                   w2, w1, w0);

    return ERR_OK;
}

/* ── Static vtable ────────────────────────────────────────────────────────── */

static const IDeviceIdentity_Vtable s_vtable = {
    .GetUidString = get_uid_string,
};

/* ── Public API ───────────────────────────────────────────────────────────── */

Result_t BspDeviceIdentity_Init(BspDeviceIdentity_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    self->iface.vtable = &s_vtable;
    self->iface.impl   = self; /* impl not used, but must be non-NULL per contract */
    return ERR_OK;
}

IDeviceIdentity *BspDeviceIdentity_GetInterface(BspDeviceIdentity_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}
