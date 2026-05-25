#pragma once
/**
 * @file mock_pc_adapters.h
 * @brief PC simulator stub adapters: ITimeSource, IGPSSource, IWifiStatusSource, IConfigStorage.
 */

#include "interfaces/i_time_source.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_wifi_status_source.h"
#include "interfaces/i_config_storage.h"

ITimeSource *MockTimeSource_GetInstance(void);
IGPSSource *MockGpsSource_GetInstance(void);
IWifiStatusSource_t *MockWifiStatusSource_GetInstance(void);
IConfigStorage *MockConfigStorage_GetInstance(void);
