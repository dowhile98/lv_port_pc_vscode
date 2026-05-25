/**
 * @file i_time_source.h
 * @brief Interfaz de fuente de tiempo (GPS, RTC, NTP, etc.).
 *
 * Domain Layer Interface - Abstracción de sincronización temporal.
 */

#ifndef I_TIME_SOURCE_H
#define I_TIME_SOURCE_H

#include <stdbool.h>
#include "hal_types.h"
#include "common/date_time.h"
#include <stddef.h>
/**
 * @brief Vtable para fuentes de tiempo.
 */
typedef struct ITimeSource_Vtable
{
    Result_t (*GetTime)(void *self, DateTime_t *out_time);
    Result_t (*SetTime)(void *self, const DateTime_t *time);
    bool (*IsSynchronized)(void *self);
} ITimeSource_Vtable;

/**
 * @brief Interfaz polimórfica de fuente de tiempo.
 */
typedef struct ITimeSource
{
    const ITimeSource_Vtable *vtable;
    void *impl;
} ITimeSource;

static inline bool time_source_is_valid(const ITimeSource *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t TimeSource_GetTime(ITimeSource *iface, DateTime_t *out_time)
{
    if (!time_source_is_valid(iface) || out_time == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->GetTime(iface->impl, out_time);
}

static inline Result_t TimeSource_SetTime(ITimeSource *iface, const DateTime_t *time)
{
    if (!time_source_is_valid(iface) || time == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->SetTime(iface->impl, time);
}

static inline bool TimeSource_IsSynchronized(ITimeSource *iface)
{
    if (!time_source_is_valid(iface))
    {
        return false;
    }
    return iface->vtable->IsSynchronized(iface->impl);
}

#endif /* I_TIME_SOURCE_H */
