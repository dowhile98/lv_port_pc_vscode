#ifndef MOCK_BQ27441_ADAPTER_H
#define MOCK_BQ27441_ADAPTER_H

#include "interfaces/i_battery_monitor.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct MockBQ27441Adapter MockBQ27441Adapter_t;

struct MockBQ27441Adapter
{
    IBatteryMonitor iface;

    /* Configuration */
    uint16_t design_capacity_mah;
    uint16_t design_energy_mwh;
    uint16_t terminate_voltage_mv;

    /* Simulation state (evolves on each Update) */
    float remaining_capacity_mah;
    int16_t current_ma;
    bool is_charging;

    uint32_t last_update_tick;
    uint32_t (*get_tick_ms)(void);

    /* Cached values for IBatteryMonitor reads */
    uint16_t voltage_mv;
    uint16_t soc_percent;
    int16_t temperature_decideg_c;
    uint16_t full_capacity_mah;
    uint16_t remaining_capacity_mah_int;
    int16_t avg_power_mw;
    BatteryStatus_t status;

    bool initialized;
};

Result_t MockBQ27441Adapter_Init(MockBQ27441Adapter_t *self,
                                  uint16_t design_capacity_mah);
Result_t MockBQ27441Adapter_Update(MockBQ27441Adapter_t *self);
Result_t MockBQ27441Adapter_Deinit(MockBQ27441Adapter_t *self);
IBatteryMonitor *MockBQ27441Adapter_GetInterface(MockBQ27441Adapter_t *self);

void MockBQ27441Adapter_SetCharging(MockBQ27441Adapter_t *self, bool charging);

#endif /* MOCK_BQ27441_ADAPTER_H */
