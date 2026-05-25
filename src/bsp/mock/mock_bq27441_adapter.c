#include "mock_bq27441_adapter.h"
#include "infrastructure/osal/osal.h"
#include <string.h>
#include <stdlib.h>

/* ==========================================================================
 *  Simulation constants
 * ========================================================================== */

#define FULL_CHARGE_MV       4200
#define NOMINAL_VOLTAGE_MV   3700
#define EMPTY_VOLTAGE_MV     3000
#define DISCHARGE_CURRENT_MA 300
#define CHARGE_CURRENT_MA    300
#define NOMINAL_TEMP_C       25

/* Li-ion discharge curve breakpoints (SOC% → mV)
 *   Used for linear interpolation — avoids fp math library. */
static const uint16_t s_discharge_curve[][2] = {
    {100, 4200},
    {90,  4080},
    {75,  3900},
    {60,  3780},
    {50,  3700},
    {40,  3620},
    {30,  3540},
    {20,  3440},
    {15,  3360},
    {10,  3200},
    {5,   3100},
    {0,   3000},
};
#define CURVE_POINTS (sizeof s_discharge_curve / sizeof s_discharge_curve[0])

/* Simple LCG pseudo-random for noise (deterministic, no stdlib rand seed issues) */
static uint32_t s_rng_state = 42;
static int16_t mock_rand_range(int16_t range)
{
    s_rng_state = s_rng_state * 1103515245U + 12345U;
    return (int16_t)((s_rng_state >> 16) % (uint32_t)(range * 2 + 1)) - range;
}

/* ==========================================================================
 *  Internal helpers
 * ========================================================================== */

static uint16_t interpolate_voltage(uint16_t soc_percent)
{
    if (soc_percent >= 100) return FULL_CHARGE_MV;
    if (soc_percent == 0)   return EMPTY_VOLTAGE_MV;

    for (size_t i = 0; i < CURVE_POINTS - 1; i++)
    {
        uint16_t hi_soc = s_discharge_curve[i][0];
        uint16_t hi_mv  = s_discharge_curve[i][1];
        uint16_t lo_soc = s_discharge_curve[i + 1][0];
        uint16_t lo_mv  = s_discharge_curve[i + 1][1];

        if (soc_percent <= hi_soc && soc_percent >= lo_soc)
        {
            uint16_t soc_range = hi_soc - lo_soc;
            uint16_t mv_range  = hi_mv  - lo_mv;
            uint16_t offset    = (uint16_t)((uint32_t)(soc_percent - lo_soc)
                                          * mv_range / soc_range);
            return lo_mv + offset;
        }
    }
    return EMPTY_VOLTAGE_MV;
}

/* ==========================================================================
 *  IBatteryMonitor vtable implementations
 * ========================================================================== */

static Result_t mock_get_data(void *self, BatteryData_t *data)
{
    MockBQ27441Adapter_t *m = (MockBQ27441Adapter_t *)self;
    if (!m || !data || !m->initialized)
        return ERR_NULL_POINTER;

    data->voltage_mv              = m->voltage_mv;
    data->current_ma              = m->current_ma;
    data->state_of_charge_percent = m->soc_percent;
    data->remaining_capacity_mah  = m->remaining_capacity_mah_int;
    data->full_capacity_mah       = m->full_capacity_mah;
    data->temperature_decideg_c   = m->temperature_decideg_c;
    data->average_power_mw        = m->avg_power_mw;
    data->state_of_health_percent = 100;
    data->status                  = m->status;
    return ERR_OK;
}

static Result_t mock_get_voltage(void *self, uint16_t *voltage_mv)
{
    MockBQ27441Adapter_t *m = (MockBQ27441Adapter_t *)self;
    if (!m || !voltage_mv || !m->initialized)
        return ERR_NULL_POINTER;
    *voltage_mv = m->voltage_mv;
    return ERR_OK;
}

static Result_t mock_get_soc(void *self, uint16_t *soc)
{
    MockBQ27441Adapter_t *m = (MockBQ27441Adapter_t *)self;
    if (!m || !soc || !m->initialized)
        return ERR_NULL_POINTER;
    *soc = m->soc_percent;
    return ERR_OK;
}

static bool mock_is_healthy(void *self)
{
    MockBQ27441Adapter_t *m = (MockBQ27441Adapter_t *)self;
    if (!m || !m->initialized)
        return false;
    return (m->voltage_mv > 3000 &&
            m->soc_percent > 5 &&
            !(m->status & BATTERY_STATUS_ERROR));
}

static const IBatteryMonitor_Vtable s_vtable = {
    .GetData          = mock_get_data,
    .GetVoltage       = mock_get_voltage,
    .GetStateOfCharge = mock_get_soc,
    .IsHealthy        = mock_is_healthy,
};

/* ==========================================================================
 *  Public API
 * ========================================================================== */

Result_t MockBQ27441Adapter_Init(MockBQ27441Adapter_t *self,
                                  uint16_t design_capacity_mah)
{
    if (!self)
        return ERR_NULL_POINTER;

    memset(self, 0, sizeof(*self));
    self->iface.vtable = &s_vtable;
    self->iface.impl   = self;

    self->design_capacity_mah = design_capacity_mah;
    self->design_energy_mwh   = (uint16_t)((uint32_t)design_capacity_mah
                                          * NOMINAL_VOLTAGE_MV / 1000);
    self->terminate_voltage_mv = EMPTY_VOLTAGE_MV;

    /* Start with a full battery */
    self->remaining_capacity_mah = (float)design_capacity_mah;
    self->full_capacity_mah      = design_capacity_mah;
    self->current_ma             = -(int16_t)DISCHARGE_CURRENT_MA;
    self->is_charging            = false;
    self->soc_percent            = 100;
    self->voltage_mv             = FULL_CHARGE_MV;
    self->temperature_decideg_c  = NOMINAL_TEMP_C * 10;
    self->avg_power_mw           = -(int16_t)((uint32_t)FULL_CHARGE_MV
                                             * DISCHARGE_CURRENT_MA / 1000);
    self->status                 = BATTERY_STATUS_DISCHARGING;
    self->get_tick_ms            = os_ticks_get;
    self->initialized            = true;

    return ERR_OK;
}

Result_t MockBQ27441Adapter_Update(MockBQ27441Adapter_t *self)
{
    if (!self || !self->initialized)
        return ERR_NULL_POINTER;

    uint32_t now = self->get_tick_ms();
    if (self->last_update_tick == 0)
    {
        self->last_update_tick = now;
        return ERR_OK;
    }

    uint32_t elapsed_ms = now - self->last_update_tick;
    if (elapsed_ms == 0)
        return ERR_OK;

    self->last_update_tick = now;

    /* Charge / discharge by elapsed time */
    float delta_mah;
    if (self->is_charging)
    {
        delta_mah = (float)CHARGE_CURRENT_MA * (float)elapsed_ms / 3600000.0f;
        self->remaining_capacity_mah += delta_mah;
        if (self->remaining_capacity_mah >= (float)self->design_capacity_mah)
        {
            self->remaining_capacity_mah = (float)self->design_capacity_mah;
            self->is_charging            = false;
        }
    }
    else
    {
        delta_mah = (float)DISCHARGE_CURRENT_MA * (float)elapsed_ms / 3600000.0f;
        if (delta_mah >= self->remaining_capacity_mah)
            self->remaining_capacity_mah = 0.0f;
        else
            self->remaining_capacity_mah -= delta_mah;
    }

    /* Update derived SOC */
    self->soc_percent = (uint16_t)(self->remaining_capacity_mah
                                  / (float)self->design_capacity_mah * 100.0f);
    if (self->soc_percent > 100)
        self->soc_percent = 100;

    /* Voltage from discharge curve + noise */
    self->voltage_mv = interpolate_voltage(self->soc_percent);
    {
        int16_t noise = mock_rand_range(10); /* ±10 mV */
        int32_t v     = (int32_t)self->voltage_mv + noise;
        if (v < 2900) v = 2900;
        if (v > 4250) v = 4250;
        self->voltage_mv = (uint16_t)v;
    }

    /* Current */
    if (self->is_charging)
        self->current_ma = CHARGE_CURRENT_MA + mock_rand_range(25);
    else if (self->soc_percent > 0)
        self->current_ma = -(int16_t)(DISCHARGE_CURRENT_MA + mock_rand_range(25));
    else
        self->current_ma = 0;

    /* Temperature: ~25°C ± 3°C (250 ± 30 decidegrees) */
    self->temperature_decideg_c = 250 + mock_rand_range(30);

    /* Average power = V × I */
    {
        int32_t p = (int32_t)self->voltage_mv * (int32_t)self->current_ma / 1000;
        self->avg_power_mw = (int16_t)p;
    }

    /* Status flags */
    BatteryStatus_t st = 0;
    if (!self->is_charging && self->soc_percent > 0)
        st |= BATTERY_STATUS_DISCHARGING;
    if (self->is_charging)
        st |= BATTERY_STATUS_CHARGING;
    if (self->soc_percent >= 100)
        st |= BATTERY_STATUS_FULL;
    if (self->soc_percent <= 5)
        st |= BATTERY_STATUS_CRITICAL_SOC;
    else if (self->soc_percent <= 10)
        st |= BATTERY_STATUS_LOW_SOC;
    if (self->voltage_mv < 2900 || self->soc_percent == 0)
        st |= BATTERY_STATUS_ERROR;
    self->status = st;

    /* Sync integer capacity for IBatteryMonitor reads */
    self->remaining_capacity_mah_int = (uint16_t)self->remaining_capacity_mah;
    self->full_capacity_mah          = self->design_capacity_mah;

    return ERR_OK;
}

Result_t MockBQ27441Adapter_Deinit(MockBQ27441Adapter_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    self->initialized = false;
    return ERR_OK;
}

IBatteryMonitor *MockBQ27441Adapter_GetInterface(MockBQ27441Adapter_t *self)
{
    if (!self || !self->initialized)
        return NULL;
    return &self->iface;
}

void MockBQ27441Adapter_SetCharging(MockBQ27441Adapter_t *self, bool charging)
{
    if (!self)
        return;
    self->is_charging = charging;
}
