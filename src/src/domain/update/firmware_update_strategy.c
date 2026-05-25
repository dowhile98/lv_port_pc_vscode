/**
 * @file  firmware_update_strategy.c
 * @brief Implementación de FirmwareUpdateStrategy — wrapper portable sobre CycloneBOOT.
 *
 * @note  Este archivo NO incluye stm32u5xx_hal.h ni update/update.h.
 *        La dependencia en CycloneBOOT está completamente encapsulada
 *        en ICycloneBootOps_t, inyectada vía FirmwareUpdateStrategyConfig_t.
 */

#include "domain/update/firmware_update_strategy.h"
#include <stddef.h>

#define TAG "FW_UPDATE"

/* ═══════════════════════════════════════════════════════════════════════════
 * Private helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static FirmwareUpdateStrategy_t *cast_self(void *raw)
{
    return (FirmwareUpdateStrategy_t *)raw;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Private vtable implementations
 * ═══════════════════════════════════════════════════════════════════════════ */

static Result_t fw_begin(void *raw, UpdateType_t type)
{
    (void)type; /* Esta estrategia ignora el tipo — ya fue seleccionada por UpdateManager */

    FirmwareUpdateStrategy_t *self = cast_self(raw);
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Paso 1: deshabilitar XIP ANTES de cualquier operación de CycloneBOOT */
    Result_t res = EXT_FLASH_DisableMemoryMapped(self->flash_iface, self->flash_handle);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Paso 2: inicializar contexto CycloneBOOT */
    res = self->cboot_ops->vtable->Init(self->cboot_ops->ctx);
    if (res != ERR_OK)
    {
        /* Re-habilitar XIP — no dejar el sistema sin acceso a flash externa */
        EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
        return res;
    }

    self->is_active = true;
    return ERR_OK;
}

static Result_t fw_write(void *raw, const uint8_t *buffer, size_t len)
{
    FirmwareUpdateStrategy_t *self = cast_self(raw);
    if (self == NULL || buffer == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (len == 0U)
    {
        return ERR_INVALID_PARAM;
    }
    if (!self->is_active)
    {
        return ERR_INVALID_STATE;
    }

    return self->cboot_ops->vtable->Process(self->cboot_ops->ctx, buffer, len);
}

static Result_t fw_end(void *raw)
{
    FirmwareUpdateStrategy_t *self = cast_self(raw);
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (!self->is_active)
    {
        return ERR_INVALID_STATE;
    }

    /* Verificar firma — CycloneBOOT lee Update Slot completo y valida ECDSA */
    Result_t res = self->cboot_ops->vtable->Finalize(self->cboot_ops->ctx);

    /* Re-habilitar XIP siempre — OK o firma inválida */
    EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);

    self->is_active = false;

    if (res != ERR_OK)
    {
        return ERR_INVALID_SIGNATURE;
    }

    return ERR_OK;
}

static Result_t fw_abort(void *raw)
{
    FirmwareUpdateStrategy_t *self = cast_self(raw);
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Re-habilitar XIP incondicionalmente — seguro de llamar en cualquier estado */
    EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
    self->is_active = false;

    return ERR_OK;
}

static Result_t fw_reboot(void *raw)
{
    FirmwareUpdateStrategy_t *self = cast_self(raw);
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    return self->cboot_ops->vtable->Reboot(self->cboot_ops->ctx);
}

/* ── Vtable estática (compartida por todas las instancias) ──────────────── */

static const IUpdateManager_Vtable_t s_fw_vtable = {
    .Begin = fw_begin,
    .Write = fw_write,
    .End = fw_end,
    .Abort = fw_abort,
    .Reboot = fw_reboot,
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

Result_t FirmwareUpdateStrategy_Init(FirmwareUpdateStrategy_t *self,
                                     const FirmwareUpdateStrategyConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (config->flash_iface == NULL || config->cboot_ops == NULL)
    {
        return ERR_NULL_POINTER;
    }

    self->flash_iface = config->flash_iface;
    self->flash_handle = config->flash_handle;
    self->cboot_ops = config->cboot_ops;
    self->logger = config->logger;
    self->is_active = false;

    /* Conectar la vtable y el puntero a sí mismo */
    self->iface.vtable = &s_fw_vtable;
    self->iface.impl = self;

    return ERR_OK;
}

IUpdateManager_t *FirmwareUpdateStrategy_GetInterface(FirmwareUpdateStrategy_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}
