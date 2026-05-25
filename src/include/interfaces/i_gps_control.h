/**
 * @file i_gps_control.h
 * @brief Interfaz para control de hardware GPS (reset, configuración).
 *
 * @note Separada de IGPSSource (lectura) e IGPSIngestor (procesamiento)
 *       para respetar SRP (Single Responsibility Principle).
 */

#ifndef I_GPS_CONTROL_H
#define I_GPS_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include <stddef.h>
/**
 * @brief Vtable para control de hardware GPS.
 */
typedef struct IGPSControl_Vtable
{
    /**
     * @brief Ejecuta secuencia de reset hardware del GPS (LOW → delay → HIGH).
     * @note Bloqueante (~1.1s). NO llamar desde ISR.
     * @param[in] self  Implementación concreta (GPSAdapter).
     * @return ERR_OK si reset ejecutado correctamente.
     */
    Result_t (*Reset)(void *self);

    /**
     * @brief Aplica configuración de hardware (selección antena).
     * @note Thread-safe. NO incluye reset (usar Reset() separadamente).
     * @param[in] self  Implementación concreta (GPSAdapter).
     * @return ERR_OK si configuración aplicada correctamente.
     */
    Result_t (*ApplyHardwareConfig)(void *self);

} IGPSControl_Vtable;

/**
 * @brief Handle polimórfico para control de GPS.
 */
typedef struct IGPSControl
{
    const IGPSControl_Vtable *vtable;
    void *impl;
} IGPSControl;

/* ===== Helper Functions ===== */

static inline bool gps_control_is_valid(const IGPSControl *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

/**
 * @brief Ejecuta reset hardware del GPS.
 * @note Bloqueante (~1.1s). Solo desde contexto de thread.
 */
static inline Result_t GPS_Control_Reset(IGPSControl *iface)
{
    if (!gps_control_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Reset(iface->impl);
}

/**
 * @brief Aplica configuración de hardware (antena).
 */
static inline Result_t GPS_Control_ApplyHardwareConfig(IGPSControl *iface)
{
    if (!gps_control_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ApplyHardwareConfig(iface->impl);
}

#endif /* I_GPS_CONTROL_H */
