/**
 * @file storage_coordinator_adapter_v2.c
 * @brief Implementación del adapter IEventNotifier para EepromEventLogAdapter (direct).
 * @version 2.0.0
 * @date 3 marzo 2026
 */

#include "infrastructure/adapters/storage/storage_coordinator_adapter_v2.h"
#include "interfaces/i_event_log_storage.h"
#include "common/date_time.h"
#include <string.h>

/* ===== V-Table Implementation ===== */

/**
 * @brief Implementación de IEventNotifier::NotifyEvent.
 * @note Escribe directamente al IEventLogStorage (sin queue, sin coordinator AO).
 */
static Result_t storage_adapter_v2_notify_event_impl(
    void *impl,
    const DateTime_t *timestamp,
    EventType_t type,
    EventSeverity_t severity,
    uint16_t info,
    uint16_t params)
{
    if (impl == NULL)
    {
        return ERR_NULL_POINTER;
    }

    StorageCoordinatorAdapter_v2_t *self = (StorageCoordinatorAdapter_v2_t *)impl;

    if (self->event_log_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Validar que IEventLogStorage está bien inicializado */
    if (self->event_log_storage->vtable == NULL ||
        self->event_log_storage->vtable->Append == NULL)
    {
        return ERR_INVALID_STATE;
    }

    /* Construir EventLogEntry_t desde parámetros */
    EventLogEntry_t event;

    /* Convert DateTime_t to uint32_t Unix timestamp if provided, else use 0 */
    if (timestamp != NULL)
    {
        event.timestamp = DateTime_ToUnix(timestamp);
    }
    else
    {
        event.timestamp = 0; /* Use RTC current time (handler responsibility) */
    }

    event.type = (uint8_t)type;
    event.severity = (uint8_t)severity;
    event.info = (uint8_t)(info & 0xFF); /* Store lower 8 bits */
    event.params = (int16_t)params;      /* Cast to int16_t */
    event.checksum = 0;                  /* EepromEventLogAdapter calculates */

    /* Llamar directamente a IEventLogStorage::Append */
    return self->event_log_storage->vtable->Append(
        self->event_log_storage->impl,
        &event);
}

/* ===== V-Table estática ===== */

static const IEventNotifier_Vtable s_storage_adapter_v2_vtable = {
    .NotifyEvent = storage_adapter_v2_notify_event_impl};

/* ===== Public API ===== */

Result_t StorageCoordinatorAdapter_v2_Init(
    StorageCoordinatorAdapter_v2_t *self,
    IEventLogStorage *event_log_storage)
{
    if (self == NULL || event_log_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Validar que event_log_storage está inicializado */
    if (event_log_storage->vtable == NULL ||
        event_log_storage->vtable->Append == NULL)
    {
        return ERR_INVALID_PARAM;
    }

    /* Limpiar estructura */
    memset(self, 0, sizeof(StorageCoordinatorAdapter_v2_t));

    /* Almacenar referencia al event log storage (no ownership) */
    self->event_log_storage = event_log_storage;

    /* Setup V-Table */
    self->interface.vtable = &s_storage_adapter_v2_vtable;
    self->interface.impl = self;
    self->is_initialized = true;

    return ERR_OK;
}

IEventNotifier *StorageCoordinatorAdapter_v2_GetInterface(
    StorageCoordinatorAdapter_v2_t *self)
{
    if (self == NULL || !self->is_initialized)
    {
        return NULL;
    }

    return &self->interface;
}

Result_t StorageCoordinatorAdapter_v2_Deinit(StorageCoordinatorAdapter_v2_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Limpiar estructura (NO desincializar event_log_storage) */
    self->is_initialized = false;
    self->interface.vtable = NULL;
    self->interface.impl = NULL;
    self->event_log_storage = NULL;

    return ERR_OK;
}
