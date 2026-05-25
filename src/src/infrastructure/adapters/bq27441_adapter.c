#include "infrastructure/adapters/bq27441_adapter.h"
#include "infrastructure/osal/osal.h"
#include <string.h>

/* ========================================================================== */
/*                          BQ27441 Definitions                              */
/* ========================================================================== */

#define BQ27441_I2C_ADDR 0x55 << 1
#define BQ27441_I2C_TIMEOUT_MS 100
#define BQ27441_UNSEAL_KEY 0x8000
#define BQ27441_DEVICE_ID 0x0421

/* Standard Commands */
#define BQ27441_CMD_CONTROL 0x00
#define BQ27441_CMD_TEMP 0x02
#define BQ27441_CMD_VOLTAGE 0x04
#define BQ27441_CMD_FLAGS 0x06
#define BQ27441_CMD_NOM_CAPACITY 0x08
#define BQ27441_CMD_AVAIL_CAPACITY 0x0A
#define BQ27441_CMD_REM_CAPACITY 0x0C
#define BQ27441_CMD_FULL_CAPACITY 0x0E
#define BQ27441_CMD_AVG_CURRENT 0x10
#define BQ27441_CMD_STDBY_CURRENT 0x12
#define BQ27441_CMD_MAX_CURRENT 0x14
#define BQ27441_CMD_AVG_POWER 0x18
#define BQ27441_CMD_SOC 0x1C
#define BQ27441_CMD_INT_TEMP 0x1E
#define BQ27441_CMD_SOH 0x20

/* Control Sub-commands */
#define BQ27441_CTRL_STATUS 0x0000
#define BQ27441_CTRL_DEVICE_TYPE 0x0001
#define BQ27441_CTRL_FW_VERSION 0x0002
#define BQ27441_CTRL_CHEM_ID 0x0008
#define BQ27441_CTRL_BAT_INSERT 0x000C
#define BQ27441_CTRL_SET_HIBERNATE 0x0011
#define BQ27441_CTRL_SET_CFGUPDATE 0x0013
#define BQ27441_CTRL_SEALED 0x0020
#define BQ27441_CTRL_RESET 0x0041
#define BQ27441_CTRL_SOFT_RESET 0x0042
#define BQ27441_CTRL_EXIT_CFGUPDATE 0x0043

/* Control Status Bits */
#define BQ27441_STATUS_SS (1 << 13)      /* Sealed State */
#define BQ27441_STATUS_CALMODE (1 << 12) /* Calibration Mode */
#define BQ27441_STATUS_INITCOMP (1 << 7) /* Initialization Complete */
#define BQ27441_STATUS_SLEEP (1 << 4)    /* Sleep Mode */

/* Flags Bits */
#define BQ27441_FLAG_OT (1 << 15)
#define BQ27441_FLAG_UT (1 << 14)
#define BQ27441_FLAG_FC (1 << 9)
#define BQ27441_FLAG_CHG (1 << 8)
#define BQ27441_FLAG_CFGUPMODE (1 << 4)
#define BQ27441_FLAG_BAT_DET (1 << 3)
#define BQ27441_FLAG_SOC1 (1 << 2)
#define BQ27441_FLAG_SOCF (1 << 1)
#define BQ27441_FLAG_DSG (1 << 0)

/* Extended Data Commands */
#define BQ27441_EXT_DATACLASS 0x3E
#define BQ27441_EXT_DATABLOCK 0x3F
#define BQ27441_EXT_BLOCKDATA 0x40
#define BQ27441_EXT_CHECKSUM 0x60
#define BQ27441_EXT_CONTROL 0x61

/* Data Class IDs */
#define BQ27441_ID_STATE 82

/* ========================================================================== */
/*                          Private Declarations                              */
/* ========================================================================== */

static Result_t bq27441_get_data(void *self, BatteryData_t *data);
static Result_t bq27441_get_voltage(void *self, uint16_t *voltage_mv);
static Result_t bq27441_get_soc(void *self, uint16_t *soc_percent);
static bool bq27441_is_healthy(void *self);

static Result_t bq27441_read_word(BQ27441Adapter_t *self, uint8_t reg, uint16_t *out);
static Result_t bq27441_write_word(BQ27441Adapter_t *self, uint8_t reg, uint16_t value);
static Result_t bq27441_read_control_word(BQ27441Adapter_t *self, uint16_t subcommand, uint16_t *out);
static Result_t bq27441_execute_control_word(BQ27441Adapter_t *self, uint16_t subcommand);
static Result_t bq27441_write_extended_data(BQ27441Adapter_t *self, uint8_t class_id, uint8_t offset, uint8_t *data, uint8_t len);

static Result_t bq27441_unseal(BQ27441Adapter_t *self);
static Result_t bq27441_seal(BQ27441Adapter_t *self);
static Result_t bq27441_enter_config(BQ27441Adapter_t *self);
static Result_t bq27441_exit_config(BQ27441Adapter_t *self, bool do_soft_reset);
static Result_t bq27441_configure(BQ27441Adapter_t *self);

static BatteryStatus_t bq27441_map_flags(uint16_t flags, uint16_t soc_percent);

static const IBatteryMonitor_Vtable s_battery_monitor_vtable = {
    .GetData = bq27441_get_data,
    .GetVoltage = bq27441_get_voltage,
    .GetStateOfCharge = bq27441_get_soc,
    .IsHealthy = bq27441_is_healthy};

/* ========================================================================== */
/*                          Public Functions                                  */
/* ========================================================================== */

Result_t BQ27441Adapter_Init(BQ27441Adapter_t *self, const BQ27441AdapterConfig_t *config)
{
    if (!self || !config || !config->i2c)
        return ERR_NULL_POINTER;

    memset(self, 0, sizeof(*self));
    self->iface.vtable = &s_battery_monitor_vtable;
    self->iface.impl = self;
    self->i2c = config->i2c;
    self->i2c_handle = config->i2c_handle;
    self->logger = config->logger;
    self->update_interval_ms = config->update_interval_ms ? config->update_interval_ms : 500;
    self->design_capacity_mah = config->design_capacity_mah;
    self->design_energy_mwh = config->design_energy_mwh;
    self->terminate_voltage_mv = config->terminate_voltage_mv;
    self->taper_rate_10th_h = config->taper_rate_10th_h;
    self->enable_auto_hibernate = config->enable_auto_hibernate;
    self->get_tick_ms = config->get_tick_ms ? config->get_tick_ms : os_ticks_get;

    self->state = BQ27441_STATE_INIT_WAIT;
    self->is_initialized = true;

    return ERR_OK;
}

Result_t BQ27441Adapter_Deinit(BQ27441Adapter_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    self->is_initialized = false;
    return ERR_OK;
}

IBatteryMonitor *BQ27441Adapter_GetInterface(BQ27441Adapter_t *self)
{
    if (!self || !self->is_initialized)
        return NULL;
    return &self->iface;
}

Result_t BQ27441Adapter_Update(BQ27441Adapter_t *self)
{
    if (!self || !self->is_initialized)
        return ERR_NULL_POINTER;

    uint32_t now = self->get_tick_ms();

    /* Throttling */
    if (self->data_valid && (now - self->last_update_timestamp < self->update_interval_ms))
    {
        return ERR_OK;
    }

    /* State Machine */
    switch (self->state)
    {
    case BQ27441_STATE_INIT_WAIT:
    {
        uint16_t device_type = 0;
        if (bq27441_read_control_word(self, BQ27441_CTRL_DEVICE_TYPE, &device_type) == ERR_OK)
        {
            if (device_type == BQ27441_DEVICE_ID)
            {
                self->state = BQ27441_STATE_CONFIG;
            }
            else
            {
                /* Wrong device ID? */
                self->state = BQ27441_STATE_ERROR;
                return ERR_ERROR;
            }
        }
        else
        {
            /* Comm error */
            return ERR_ERROR;
        }
    }
    /* fall through */
    case BQ27441_STATE_CONFIG:
    {
        Result_t res = bq27441_configure(self);
        if (res == ERR_OK)
        {
            self->state = BQ27441_STATE_READY;
        }
        else
        {
            self->state = BQ27441_STATE_ERROR;
            return res;
        }
        break;
    }

    case BQ27441_STATE_READY:
    case BQ27441_STATE_ERROR:
        /* Proceed to read data */
        break;

    default:
        break;
    }

    /* Read all data fields */
    uint16_t voltage_mv = 0;
    uint16_t soc = 0;
    uint16_t flags = 0;
    int16_t current_ma = 0;
    uint16_t full_cap = 0;
    uint16_t rem_cap = 0;
    int16_t avg_power = 0;
    uint16_t temp_bat = 0;
    uint16_t soh_raw = 0;

    bool success = true;
    success &= (bq27441_read_word(self, BQ27441_CMD_VOLTAGE, &voltage_mv) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_SOC, &soc) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_FLAGS, &flags) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_AVG_CURRENT, (uint16_t *)&current_ma) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_FULL_CAPACITY, &full_cap) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_REM_CAPACITY, &rem_cap) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_AVG_POWER, (uint16_t *)&avg_power) == ERR_OK);
    success &= (bq27441_read_word(self, BQ27441_CMD_TEMP, &temp_bat) == ERR_OK);
    /* SOH is best-effort — chip may return 0 before first learning cycle */
    (void)bq27441_read_word(self, BQ27441_CMD_SOH, &soh_raw);

    if (!success)
    {
        self->state = BQ27441_STATE_ERROR;
        self->data_valid = false;
        self->cached_data.status = BATTERY_STATUS_ERROR;
        return ERR_ERROR;
    }

    self->cached_data.voltage_mv = voltage_mv;
    self->cached_data.state_of_charge_percent = soc;
    self->cached_data.status = bq27441_map_flags(flags, soc);
    self->cached_data.temperature_decideg_c = (int16_t)temp_bat - 2731; /* Kelvin to 0.1C */
    self->cached_data.average_power_mw = avg_power;
    self->cached_data.remaining_capacity_mah = rem_cap;
    self->cached_data.full_capacity_mah = full_cap;
    self->cached_data.current_ma = current_ma;
    self->cached_data.state_of_health_percent = (uint8_t)(soh_raw >> 8U); /* high byte = SoH % */

    self->data_valid = true;
    self->state = BQ27441_STATE_READY;
    self->last_update_timestamp = now;

    /* Optional: put chip into HIBERNATE to save power between polls */
    if (self->enable_auto_hibernate)
    {
        bq27441_execute_control_word(self, BQ27441_CTRL_SET_HIBERNATE);
    }

    return ERR_OK;
}

/* ========================================================================== */
/*                          Private Helpers                                   */
/* ========================================================================== */

static Result_t bq27441_read_word(BQ27441Adapter_t *self, uint8_t reg, uint16_t *out)
{
    uint8_t buf[2];
    Result_t res = I2C_Mem_Read(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, reg,
                                I_I2C_MEMADDR_SIZE_8BIT, buf, 2, BQ27441_I2C_TIMEOUT_MS);
    if (res == ERR_OK)
    {
        *out = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    }
    return res;
}

static Result_t bq27441_write_word(BQ27441Adapter_t *self, uint8_t reg, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)(value >> 8);
    return I2C_Mem_Write(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, reg,
                         I_I2C_MEMADDR_SIZE_8BIT, buf, 2, BQ27441_I2C_TIMEOUT_MS);
}

static Result_t bq27441_read_control_word(BQ27441Adapter_t *self, uint16_t subcommand, uint16_t *out)
{
    Result_t res = bq27441_write_word(self, BQ27441_CMD_CONTROL, subcommand);
    if (res != ERR_OK)
        return res;
    return bq27441_read_word(self, BQ27441_CMD_CONTROL, out);
}

static Result_t bq27441_execute_control_word(BQ27441Adapter_t *self, uint16_t subcommand)
{
    return bq27441_write_word(self, BQ27441_CMD_CONTROL, subcommand);
}

static Result_t bq27441_write_extended_data(BQ27441Adapter_t *self, uint8_t class_id, uint8_t offset, uint8_t *data, uint8_t len)
{
    if (len > 32)
        return ERR_INVALID_ARGS;

    /* 1. Enable Block Data Control */
    Result_t res = bq27441_write_word(self, BQ27441_EXT_CONTROL, 0x00);
    if (res != ERR_OK)
        return res;

    /* 2. Select Data Class */
    uint8_t class_buf = class_id;
    res = I2C_Mem_Write(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_DATACLASS,
                        I_I2C_MEMADDR_SIZE_8BIT, &class_buf, 1, BQ27441_I2C_TIMEOUT_MS);
    if (res != ERR_OK)
        return res;

    /* 3. Select Data Block */
    uint8_t block_buf = offset / 32;
    res = I2C_Mem_Write(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_DATABLOCK,
                        I_I2C_MEMADDR_SIZE_8BIT, &block_buf, 1, BQ27441_I2C_TIMEOUT_MS);
    if (res != ERR_OK)
        return res;

    /* 4. Read current 32-byte block */
    uint8_t block_data[32];
    res = I2C_Mem_Read(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_BLOCKDATA,
                       I_I2C_MEMADDR_SIZE_8BIT, block_data, 32, BQ27441_I2C_TIMEOUT_MS);
    if (res != ERR_OK)
        return res;

    /* 5. Read current checksum */
    uint8_t old_checksum;
    res = I2C_Mem_Read(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_CHECKSUM,
                       I_I2C_MEMADDR_SIZE_8BIT, &old_checksum, 1, BQ27441_I2C_TIMEOUT_MS);
    if (res != ERR_OK)
        return res;

    /* 6. Update block data with new content */
    memcpy(&block_data[offset % 32], data, len);

    /* 7. Calculate new checksum */
    uint16_t sum = 0;
    for (int i = 0; i < 32; i++)
    {
        sum += block_data[i];
    }
    uint8_t new_checksum = (uint8_t)(255 - (sum % 256));

    /* 8. Write updated block data */
    res = I2C_Mem_Write(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_BLOCKDATA + (offset % 32),
                        I_I2C_MEMADDR_SIZE_8BIT, data, len, BQ27441_I2C_TIMEOUT_MS);
    if (res != ERR_OK)
        return res;

    /* 9. Write new checksum */
    return I2C_Mem_Write(self->i2c, self->i2c_handle, BQ27441_I2C_ADDR, BQ27441_EXT_CHECKSUM,
                         I_I2C_MEMADDR_SIZE_8BIT, &new_checksum, 1, BQ27441_I2C_TIMEOUT_MS);
}

static Result_t bq27441_unseal(BQ27441Adapter_t *self)
{
    Result_t res = bq27441_execute_control_word(self, BQ27441_UNSEAL_KEY);
    if (res != ERR_OK)
        return res;
    return bq27441_execute_control_word(self, BQ27441_UNSEAL_KEY);
}

static Result_t bq27441_seal(BQ27441Adapter_t *self)
{
    return bq27441_execute_control_word(self, BQ27441_CTRL_SEALED);
}

static Result_t bq27441_enter_config(BQ27441Adapter_t *self)
{
    Result_t res = bq27441_execute_control_word(self, BQ27441_CTRL_SET_CFGUPDATE);
    if (res != ERR_OK)
        return res;

    /* Wait for CFGUPMODE flag */
    uint16_t flags = 0;
    uint32_t timeout = 10; /* 10 retries */
    while (timeout--)
    {
        if (bq27441_read_word(self, BQ27441_CMD_FLAGS, &flags) == ERR_OK)
        {
            if (flags & BQ27441_FLAG_CFGUPMODE)
                return ERR_OK;
        }
        os_thread_sleep(10);
    }
    return ERR_TIMEOUT;
}

static Result_t bq27441_exit_config(BQ27441Adapter_t *self, bool do_soft_reset)
{
    Result_t res;
    if (do_soft_reset)
    {
        res = bq27441_execute_control_word(self, BQ27441_CTRL_SOFT_RESET);
    }
    else
    {
        res = bq27441_execute_control_word(self, BQ27441_CTRL_EXIT_CFGUPDATE);
    }

    if (res != ERR_OK)
        return res;

    /* Wait for CFGUPMODE flag to clear */
    uint16_t flags = 0;
    uint32_t timeout = 10;
    while (timeout--)
    {
        if (bq27441_read_word(self, BQ27441_CMD_FLAGS, &flags) == ERR_OK)
        {
            if (!(flags & BQ27441_FLAG_CFGUPMODE))
                return ERR_OK;
        }
        os_thread_sleep(10);
    }
    return ERR_TIMEOUT;
}

static Result_t bq27441_configure(BQ27441Adapter_t *self)
{
    /* Check if unsealing is needed */
    uint16_t status = 0;
    if (bq27441_read_control_word(self, BQ27441_CTRL_STATUS, &status) != ERR_OK)
        return ERR_ERROR;

    bool was_sealed = (status & BQ27441_STATUS_SS);
    if (was_sealed)
    {
        bq27441_unseal(self);
    }

    if (bq27441_enter_config(self) != ERR_OK)
        return ERR_ERROR;

    /* Write Design Capacity (mAh) */
    uint16_t cap = self->design_capacity_mah;
    uint8_t cap_buf[2] = {(uint8_t)(cap >> 8), (uint8_t)(cap & 0xFF)};
    bq27441_write_extended_data(self, BQ27441_ID_STATE, 10, cap_buf, 2);

    /* Write Design Energy (mWh) */
    uint16_t energy = self->design_energy_mwh;
    uint8_t energy_buf[2] = {(uint8_t)(energy >> 8), (uint8_t)(energy & 0xFF)};
    bq27441_write_extended_data(self, BQ27441_ID_STATE, 12, energy_buf, 2);

    /* Write Terminate Voltage (mV) */
    uint16_t term_v = self->terminate_voltage_mv;
    uint8_t term_buf[2] = {(uint8_t)(term_v >> 8), (uint8_t)(term_v & 0xFF)};
    bq27441_write_extended_data(self, BQ27441_ID_STATE, 16, term_buf, 2);

    /* Write Taper Rate (0.1h) */
    uint16_t taper = self->taper_rate_10th_h;
    uint8_t taper_buf[2] = {(uint8_t)(taper >> 8), (uint8_t)(taper & 0xFF)};
    bq27441_write_extended_data(self, BQ27441_ID_STATE, 27, taper_buf, 2);

    bq27441_exit_config(self, true);

    if (was_sealed)
    {
        bq27441_seal(self);
    }

    return ERR_OK;
}

static BatteryStatus_t bq27441_map_flags(uint16_t flags, uint16_t soc_percent)
{
    BatteryStatus_t status = 0;
    if (flags & BQ27441_FLAG_CHG)
        status |= BATTERY_STATUS_CHARGING;
    if (flags & BQ27441_FLAG_DSG)
        status |= BATTERY_STATUS_DISCHARGING;
    if (flags & BQ27441_FLAG_FC)
        status |= BATTERY_STATUS_FULL;
    if (flags & BQ27441_FLAG_OT)
        status |= BATTERY_STATUS_OVER_TEMP;
    if (flags & BQ27441_FLAG_UT)
        status |= BATTERY_STATUS_UNDER_TEMP;

    if (soc_percent <= 5)
        status |= BATTERY_STATUS_CRITICAL_SOC;
    else if (soc_percent <= 10)
        status |= BATTERY_STATUS_LOW_SOC;

    return status;
}

static Result_t bq27441_get_data(void *self, BatteryData_t *data)
{
    BQ27441Adapter_t *adapter = (BQ27441Adapter_t *)self;
    if (!adapter || !data)
        return ERR_NULL_POINTER;
    if (!adapter->data_valid)
        return ERR_ERROR;
    memcpy(data, &adapter->cached_data, sizeof(BatteryData_t));
    return ERR_OK;
}

static Result_t bq27441_get_voltage(void *self, uint16_t *voltage_mv)
{
    BQ27441Adapter_t *adapter = (BQ27441Adapter_t *)self;
    if (!adapter || !voltage_mv)
        return ERR_NULL_POINTER;
    if (!adapter->data_valid)
        return ERR_ERROR;
    *voltage_mv = adapter->cached_data.voltage_mv;
    return ERR_OK;
}

static Result_t bq27441_get_soc(void *self, uint16_t *soc_percent)
{
    BQ27441Adapter_t *adapter = (BQ27441Adapter_t *)self;
    if (!adapter || !soc_percent)
        return ERR_NULL_POINTER;
    if (!adapter->data_valid)
        return ERR_ERROR;
    *soc_percent = adapter->cached_data.state_of_charge_percent;
    return ERR_OK;
}

static bool bq27441_is_healthy(void *self)
{
    BQ27441Adapter_t *adapter = (BQ27441Adapter_t *)self;
    if (!adapter || !adapter->data_valid)
        return false;
    return (adapter->cached_data.voltage_mv > 3000 &&
            adapter->cached_data.state_of_charge_percent > 5 &&
            !(adapter->cached_data.status & BATTERY_STATUS_ERROR));
}
