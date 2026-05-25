/**
 * @file i_battery_monitor.h
 * @brief Battery Monitor Interface - Abstract battery fuel gauge API
 *
 * Domain-layer interface for reading battery status (SOC, voltage, current, etc.)
 * Implementations: BQ27441Adapter (IC), MockBatteryMonitor (test)
 */

#ifndef I_BATTERY_MONITOR_H
#define I_BATTERY_MONITOR_H

#include "hal/hal_types.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Battery status flags (abstraction of chip-specific flags)
 */
typedef enum
{
    BATTERY_STATUS_DISCHARGING = (1 << 0),  /**< Battery discharging */
    BATTERY_STATUS_CHARGING = (1 << 1),     /**< Battery charging */
    BATTERY_STATUS_FULL = (1 << 2),         /**< Fully charged */
    BATTERY_STATUS_LOW_SOC = (1 << 3),      /**< SOC below threshold */
    BATTERY_STATUS_CRITICAL_SOC = (1 << 4), /**< SOC critically low */
    BATTERY_STATUS_OVER_TEMP = (1 << 5),    /**< Over-temperature */
    BATTERY_STATUS_UNDER_TEMP = (1 << 6),   /**< Under-temperature */
    BATTERY_STATUS_ERROR = (1 << 7),        /**< Communication error */
} BatteryStatus_t;

/**
 * @brief Battery measurements snapshot
 */
typedef struct
{
    uint16_t voltage_mv;              /**< Voltage in millivolts */
    int16_t current_ma;               /**< Current in milliamps (+ charging, - discharging) */
    uint16_t state_of_charge_percent; /**< State of Charge (0-100%) */
    uint16_t remaining_capacity_mah;  /**< Remaining capacity in mAh */
    uint16_t full_capacity_mah;       /**< Full charge capacity in mAh */
    int16_t temperature_decideg_c;    /**< Temperature in 0.1°C (-400 to +1250 → -40°C to 125°C) */
    int16_t average_power_mw;         /**< Average power in mW */
    uint8_t state_of_health_percent;  /**< State of Health (0-100%) — BQ27441 register 0x20 high byte */
    BatteryStatus_t status;           /**< Status flags bitmap */
} BatteryData_t;

/**
 * @brief V-Table for battery monitor operations
 */
typedef struct IBatteryMonitor_Vtable
{
    /**
     * @brief Get battery measurements snapshot
     * @param[in] self Implementation pointer
     * @param[out] data Battery data structure to fill
     * @return ERR_OK on success, ERR_ERROR if I2C fails, ERR_NULL_POINTER if params invalid
     */
    Result_t (*GetData)(void *self, BatteryData_t *data);

    /**
     * @brief Get battery voltage
     * @param[in] self Implementation pointer
     * @param[out] voltage_mv Voltage in millivolts
     * @return ERR_OK on success, error code otherwise
     */
    Result_t (*GetVoltage)(void *self, uint16_t *voltage_mv);

    /**
     * @brief Get state of charge
     * @param[in] self Implementation pointer
     * @param[out] soc_percent SOC percentage (0-100)
     * @return ERR_OK on success, error code otherwise
     */
    Result_t (*GetStateOfCharge)(void *self, uint16_t *soc_percent);

    /**
     * @brief Check if battery is healthy (voltage > threshold, SOC > critical, no errors)
     * @param[in] self Implementation pointer
     * @return true if healthy, false otherwise
     */
    bool (*IsHealthy)(void *self);
} IBatteryMonitor_Vtable;

/**
 * @brief Battery Monitor interface object
 */
typedef struct
{
    const IBatteryMonitor_Vtable *vtable;
    void *impl; /**< Opaque implementation pointer (BQ27441Adapter_t*) */
} IBatteryMonitor;

/* ===== Inline Wrappers ===== */

static inline Result_t BatteryMonitor_GetData(IBatteryMonitor *iface, BatteryData_t *data)
{
    if (!iface || !iface->vtable || !iface->vtable->GetData)
        return ERR_NULL_POINTER;
    return iface->vtable->GetData(iface->impl, data);
}

static inline Result_t BatteryMonitor_GetVoltage(IBatteryMonitor *iface, uint16_t *voltage_mv)
{
    if (!iface || !iface->vtable || !iface->vtable->GetVoltage)
        return ERR_NULL_POINTER;
    return iface->vtable->GetVoltage(iface->impl, voltage_mv);
}

static inline Result_t BatteryMonitor_GetStateOfCharge(IBatteryMonitor *iface, uint16_t *soc_percent)
{
    if (!iface || !iface->vtable || !iface->vtable->GetStateOfCharge)
        return ERR_NULL_POINTER;
    return iface->vtable->GetStateOfCharge(iface->impl, soc_percent);
}

static inline bool BatteryMonitor_IsHealthy(IBatteryMonitor *iface)
{
    if (!iface || !iface->vtable || !iface->vtable->IsHealthy)
        return false;
    return iface->vtable->IsHealthy(iface->impl);
}

#endif /* I_BATTERY_MONITOR_H */
