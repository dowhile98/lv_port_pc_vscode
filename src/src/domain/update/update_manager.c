/**
 * @file  update_manager.c
 * @brief Implementación del Facade UpdateManager — delega a FirmwareUpdateStrategy
 *        o ExternalLoaderStrategy según el tipo recibido en Begin().
 *
 * @note  No incluye stm32u5xx_hal.h. El mutex es OSAL (os_mutex_t, opaco).
 */

#include "domain/update/update_manager.h"
#include "infrastructure/osal/osal.h" /* os_mutex_acquire / release */
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Private helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static UpdateManager_t *cast_self(void *raw)
{
    return (UpdateManager_t *)raw;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * V-Table implementations
 * ═══════════════════════════════════════════════════════════════════════════ */

static Result_t mgr_begin(void *raw, UpdateType_t type)
{
    UpdateManager_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Protección de concurrencia — no bloquear, retornar ERR_BUSY si ocupado */
    if (os_mutex_acquire(self->mutex, OS_NO_WAIT) != ERR_OK)
        return ERR_BUSY;

    if (self->is_busy)
    {
        os_mutex_release(self->mutex);
        return ERR_BUSY;
    }

    /* Seleccionar la estrategia correcta */
    IUpdateManager_t *strategy = NULL;
    if (type == UPDATE_TYPE_FIRMWARE)
        strategy = self->fw_strategy;
    else if (type == UPDATE_TYPE_EXTERNAL_LOADER)
        strategy = self->ext_strategy;
    else
    {
        os_mutex_release(self->mutex);
        return ERR_INVALID_PARAM;
    }

    /* Delegar Begin a la estrategia — si falla, no marcar busy */
    Result_t res = IUpdateManager_Begin(strategy, type);
    if (res != ERR_OK)
    {
        os_mutex_release(self->mutex);
        return res;
    }

    self->active = strategy;
    self->is_busy = true;
    os_mutex_release(self->mutex);
    return ERR_OK;
}

static Result_t mgr_write(void *raw, const uint8_t *buffer, size_t len)
{
    UpdateManager_t *self = cast_self(raw);
    if (self == NULL || buffer == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_busy || self->active == NULL)
        return ERR_INVALID_STATE;

    return IUpdateManager_Write(self->active, buffer, len);
}

static Result_t mgr_end(void *raw)
{
    UpdateManager_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_busy || self->active == NULL)
        return ERR_INVALID_STATE;

    Result_t res = IUpdateManager_End(self->active);

    /* Liberar el lock — active se mantiene para Reboot() */
    self->is_busy = false;
    return res;
}

static Result_t mgr_abort(void *raw)
{
    UpdateManager_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_busy || self->active == NULL)
        return ERR_INVALID_STATE;

    Result_t res = IUpdateManager_Abort(self->active);
    self->is_busy = false;
    return res;
}

static Result_t mgr_reboot(void *raw)
{
    UpdateManager_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* active se conserva después de End() para permitir Reboot() */
    if (self->active == NULL)
        return ERR_INVALID_STATE;

    return IUpdateManager_Reboot(self->active);
}

/* ── V-Table estática ─────────────────────────────────────────────────────── */

static const IUpdateManager_Vtable_t s_mgr_vtable = {
    .Begin = mgr_begin,
    .Write = mgr_write,
    .End = mgr_end,
    .Abort = mgr_abort,
    .Reboot = mgr_reboot,
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

Result_t UpdateManager_Init(UpdateManager_t *self,
                            const UpdateManagerConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;
    if (config->fw_strategy == NULL)
        return ERR_NULL_POINTER;
    if (config->ext_strategy == NULL)
        return ERR_NULL_POINTER;
    if (config->mutex == NULL)
        return ERR_INVALID_PARAM;

    self->fw_strategy = config->fw_strategy;
    self->ext_strategy = config->ext_strategy;
    self->mutex = config->mutex;
    self->active = NULL;
    self->is_busy = false;

    self->iface.vtable = &s_mgr_vtable;
    self->iface.impl = self;

    return ERR_OK;
}

IUpdateManager_t *UpdateManager_GetInterface(UpdateManager_t *self)
{
    if (self == NULL)
        return NULL;
    return &self->iface;
}
