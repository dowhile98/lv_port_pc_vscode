#ifndef I_GPS_INGESTOR_H
#define I_GPS_INGESTOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_types.h"

/**
 * @brief Interfaz para ingestión de datos GPS (buffer y procesamiento).
 */
typedef struct IGPSIngestor_Vtable
{
    Result_t (*ProcessRxBuffer)(void *self, const uint8_t *data, uint16_t len);
    Result_t (*Process)(void *self);
} IGPSIngestor_Vtable;

/**
 * @brief Handle polimórfico para un ingestor GPS.
 */
typedef struct IGPSIngestor
{
    const IGPSIngestor_Vtable *vtable;
    void *impl;
} IGPSIngestor;

static inline bool gps_ingestor_is_valid(const IGPSIngestor *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t GPS_Ingest_ProcessRxBuffer(IGPSIngestor *iface, const uint8_t *data, uint16_t len)
{
    if (!gps_ingestor_is_valid(iface) || data == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ProcessRxBuffer(iface->impl, data, len);
}

static inline Result_t GPS_Ingest_Process(IGPSIngestor *iface)
{
    if (!gps_ingestor_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Process(iface->impl);
}

#endif /* I_GPS_INGESTOR_H */
