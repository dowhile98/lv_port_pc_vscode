/**
 * @file i_time_sync_control.h
 * @brief Interfaz para control de sincronización de tiempo.
 *
 * Infrastructure Layer Interface - Permite que fuentes externas (GPS, PPS)
 * notifiquen eventos de sincronización al TimeAdapter.
 */

#ifndef I_TIME_SYNC_CONTROL_H
#define I_TIME_SYNC_CONTROL_H

#include "hal_types.h"
#include "common/date_time.h"
#include <stddef.h>
/**
 * @brief Vtable para control de sincronización.
 */
typedef struct ITimeSyncControl_Vtable
{
    /**
     * @brief Notifica un evento PPS (Pulse Per Second).
     *
     * **CRITICAL: This callback is executed in ISR context from GPSAdapter.**
     *
     * @param self Instancia de TimeAdapter.
     * @param gps_time Tiempo UTC del GPS (NO NULL).
     *                 GPSAdapter MUST guarantee fix_status == FIX_3D when passing time.
     *                 If NULL, it indicates a caller error.
     *
     * @return ERR_OK si sincronización exitosa.
     *
     * @note PROHIBIDO en ISR: mutex, malloc, blocking calls.
     * @note Contexto: Se ejecuta cada 1 segundo (PPS freq).
     * @note Latencia: Debe completar en <50μs para no bloquear ISR.
     *
     * @warning Comportamiento si gps_time == NULL:
     *          - TimeAdapter solo actualiza timestamp, no sincroniza RTC.
     *          - Indica que GPSAdapter perdió FIX o no está listo aún.
     */
    Result_t (*OnPPS)(void *self, const DateTime_t *gps_time);

    /**
     * @brief Fuerza una sincronización manual de tiempo.
     * @param self Instancia.
     * @param time Nuevo tiempo.
     */
    Result_t (*SyncTime)(void *self, const DateTime_t *time);
} ITimeSyncControl_Vtable;

/**
 * @brief Interfaz polimórfica de control de sincronización.
 */
typedef struct ITimeSyncControl
{
    const ITimeSyncControl_Vtable *vtable;
    void *impl;
} ITimeSyncControl;

static inline bool time_sync_control_is_valid(const ITimeSyncControl *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t TimeSyncControl_OnPPS(ITimeSyncControl *iface, const DateTime_t *gps_time)
{
    if (!time_sync_control_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->OnPPS(iface->impl, gps_time);
}

static inline Result_t TimeSyncControl_SyncTime(ITimeSyncControl *iface, const DateTime_t *time)
{
    if (!time_sync_control_is_valid(iface) || time == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->SyncTime(iface->impl, time);
}

#endif /* I_TIME_SYNC_CONTROL_H */
