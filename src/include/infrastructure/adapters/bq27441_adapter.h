/**
 * @file bq27441_adapter.h
 * @brief BQ27441-G1A Fuel Gauge Adapter
 *
 * Infrastructure Layer - Implements IBatteryMonitor interface for BQ27441 IC
 */

#ifndef BQ27441_ADAPTER_H
#define BQ27441_ADAPTER_H

#include "interfaces/i_battery_monitor.h"
#include "interfaces/i_logger.h"
#include "infrastructure/osal/osal.h" // For os_ticks_get
#include "hal/interfaces/i_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Adapter configuration
 */
typedef struct
{
    I_I2C *i2c;                    /**< I2C HAL interface (BSP dependency) */
    I_I2C_Handle_t i2c_handle;     /**< I2C peripheral handle */
    ILogger *logger;               /**< Logger for diagnostics (optional) */
    uint32_t update_interval_ms;   /**< Update interval (default 500ms) */
    uint16_t design_capacity_mah;  /**< Battery design capacity (e.g., 2000mAh) */
    uint16_t design_energy_mwh;    /**< Battery design energy (e.g., 7400mWh) */
    uint16_t terminate_voltage_mv; /**< Terminate voltage (e.g., 3000mV) */
    uint16_t taper_rate_10th_h;    /**< Taper rate in 0.1h (e.g., 100 for 10h) */
    bool enable_auto_hibernate;    /**< Enable automatic HIBERNATE when idle (default false) */
    uint32_t (*get_tick_ms)(void); /**< Millisecond timestamp provider (os_ticks_get) */
} BQ27441AdapterConfig_t;

/**
 * @brief Adapter state
 */
typedef enum
{
    BQ27441_STATE_UNINIT = 0,
    BQ27441_STATE_INIT_WAIT, /**< Waiting for battery detection */
    BQ27441_STATE_CONFIG,    /**< Entering/writing configuration */
    BQ27441_STATE_READY,     /**< Normal operation */
    BQ27441_STATE_ERROR      /**< Persistent I2C error */
} BQ27441_State_t;

/**
 * @brief Adapter instance
 */
typedef struct
{
    IBatteryMonitor iface;     /**< Public interface */
    I_I2C *i2c;                /**< I2C HAL interface (BSP dependency) */
    I_I2C_Handle_t i2c_handle; /**< I2C peripheral handle */
    ILogger *logger;
    uint32_t (*get_tick_ms)(void); /**< Timestamp provider (defaults to os_ticks_get) */
    bool enable_auto_hibernate;    /**< Preserve config option */

    BQ27441_State_t state;
    uint32_t update_interval_ms;
    uint32_t last_update_timestamp;

    /* Cached data (updated every update_interval_ms) */
    BatteryData_t cached_data;
    bool data_valid;

    /* Config */
    uint16_t design_capacity_mah;
    uint16_t design_energy_mwh;
    uint16_t terminate_voltage_mv;
    uint16_t taper_rate_10th_h;

    bool is_initialized;
} BQ27441Adapter_t;

/**
 * @brief Initialize BQ27441 adapter
 */
Result_t BQ27441Adapter_Init(BQ27441Adapter_t *self, const BQ27441AdapterConfig_t *config);

/**
 * @brief Update adapter (called from System_Update every 100ms)
 * @note Internally throttles to update_interval_ms (default 500ms)
 */
Result_t BQ27441Adapter_Update(BQ27441Adapter_t *self);

/**
 * @brief De-initialize adapter
 */
Result_t BQ27441Adapter_Deinit(BQ27441Adapter_t *self);

/**
 * @brief Get IBatteryMonitor interface
 */
IBatteryMonitor *BQ27441Adapter_GetInterface(BQ27441Adapter_t *self);

#endif /* BQ27441_ADAPTER_H */
