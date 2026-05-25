#ifndef I_GPS_SOURCE_H
#define I_GPS_SOURCE_H

#include <stdbool.h>
#include "hal_types.h"
#include "common/gps_types.h"
#include <stddef.h>
/**
 * @brief Vtable para fuentes de datos GPS.
 */
typedef struct IGPSSource_Vtable
{
    Result_t (*GetPosition)(void *self, GPSPosition_t *out_position);
    Result_t (*GetTimeUTC)(void *self, DateTime_t *out_time);
    Result_t (*GetFixStatus)(void *self, GPSFixStatus_t *out_status);
    Result_t (*RegisterPPSCallback)(void *self, PPSCallback_t callback, void *context);
} IGPSSource_Vtable;

/**
 * @brief Interfaz polimórfica de fuente GPS.
 */
typedef struct IGPSSource
{
    const IGPSSource_Vtable *vtable;
    void *impl;
} IGPSSource;

static inline bool gps_source_is_valid(const IGPSSource *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t GPS_Source_GetPosition(IGPSSource *iface, GPSPosition_t *out_position)
{
    if (!gps_source_is_valid(iface) || out_position == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->GetPosition(iface->impl, out_position);
}

static inline Result_t GPS_Source_GetTimeUTC(IGPSSource *iface, DateTime_t *out_time)
{
    if (!gps_source_is_valid(iface) || out_time == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->GetTimeUTC(iface->impl, out_time);
}

static inline Result_t GPS_Source_GetFixStatus(IGPSSource *iface, GPSFixStatus_t *out_status)
{
    if (!gps_source_is_valid(iface) || out_status == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->GetFixStatus(iface->impl, out_status);
}

static inline Result_t GPS_Source_RegisterPPSCallback(IGPSSource *iface, PPSCallback_t callback, void *context)
{
    if (!gps_source_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterPPSCallback(iface->impl, callback, context);
}

#endif /* I_GPS_SOURCE_H */
