/**
 * @file   i_temperature_sensor.h
 * @brief  Abstract interface for temperature sensors.
 *
 * @details
 * Port contract between Application/Domain layers and any concrete temperature
 * sensor implementation (internal MCU sensor, external IC, etc.).
 * Depends only on stdint.h, stdbool.h, and hal_types.h — zero HAL headers.
 *
 * @note   Thread-safety: NOT ISR-safe. Call from a thread context only.
 *         Implementations may block while the ADC conversion completes.
 */

#ifndef I_TEMPERATURE_SENSOR_H
#define I_TEMPERATURE_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Vtable ───────────────────────────────────────────────────────────────── */

    /**
     * @brief V-Table for ITemperatureSensor.
     */
    typedef struct ITemperatureSensor_Vtable
    {
        /**
         * @brief  Read the sensor temperature in degrees Celsius.
         *
         * @note   Blocking: waits for ADC conversion to complete (polling).
         *         Not ISR-safe. Typical latency ≤ 1 ms.
         *
         * @param[in]  self          Implementation instance (must not be NULL).
         * @param[out] out_celsius   Result in °C (must not be NULL).
         *
         * @return ERR_OK            on success.
         * @return ERR_NULL_POINTER  if self or out_celsius is NULL.
         * @return ERR_TIMEOUT       if ADC conversion did not complete within
         *                           the implementation's timeout.
         * @return ERR_ERROR         on hardware error.
         */
        Result_t (*ReadTemperatureCelsius)(void *self, float *out_celsius);

    } ITemperatureSensor_Vtable;

    /* ── Interface handle ─────────────────────────────────────────────────────── */

    /**
     * @brief Polymorphic handle for a temperature sensor.
     *
     * Callers hold a pointer to this struct and call via the inline wrappers
     * below. Concrete implementations fill `.vtable` and `.impl` in their
     * `Init()` or `GetInterface()` function.
     */
    typedef struct ITemperatureSensor
    {
        const ITemperatureSensor_Vtable *vtable; /**< Concrete vtable (never NULL after init). */
        void *impl;                              /**< Concrete instance  (never NULL after init). */
    } ITemperatureSensor_t;

    /* ── Validity helper ──────────────────────────────────────────────────────── */

    /**
     * @brief Check that an ITemperatureSensor handle is fully initialised.
     * @return true  if vtable, its single function pointer, and impl are all non-NULL.
     */
    static inline bool TemperatureSensor_IsValid(const ITemperatureSensor_t *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->vtable->ReadTemperatureCelsius != NULL) && (iface->impl != NULL);
    }

    /* ── Inline wrapper ───────────────────────────────────────────────────────── */

    /**
     * @brief  Read the sensor temperature in degrees Celsius.
     *
     * @param[in]  iface        Interface handle (must not be NULL).
     * @param[out] out_celsius  Result in °C.
     *
     * @return ERR_NULL_POINTER if iface is invalid or out_celsius is NULL.
     * @return Result_t         from the concrete implementation otherwise.
     */
    static inline Result_t TemperatureSensor_Read(ITemperatureSensor_t *iface,
                                                  float *out_celsius)
    {
        if (!TemperatureSensor_IsValid(iface) || out_celsius == NULL)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->ReadTemperatureCelsius(iface->impl, out_celsius);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_TEMPERATURE_SENSOR_H */
