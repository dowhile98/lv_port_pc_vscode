/**
 * @file system_update_services.h
 * @brief Helpers for optional periodic service updates.
 */

#ifndef SYSTEM_UPDATE_SERVICES_H
#define SYSTEM_UPDATE_SERVICES_H

#include <stdbool.h>

#include "interfaces/i_wifi_module.h"
#include "application/services/side_button_service.h"
#include "application/services/gps_antenna_auto_switch_service.h"

#ifdef __cplusplus
extern "C"
{
#endif

	static inline void System_UpdateOptionalServices(bool wifi_enable_service_initialized,
													 IWifiModule_t *wifi_module,
													 bool side_button_initialized,
													 SideButtonService_t *side_button_service,
													 bool gps_initialized,
													 GpsAntennaAutoSwitchService_t *gps_auto_switch_service)
	{
		if (wifi_enable_service_initialized)
		{
			if (wifi_module != NULL)
			{
				(void)WifiModule_Update(wifi_module);
			}
		}

		if (side_button_initialized && (side_button_service != NULL))
		{
			(void)SideButtonService_Update(side_button_service);
		}

		if (gps_initialized && (gps_auto_switch_service != NULL))
		{
			(void)GpsAntennaAutoSwitchService_Update(gps_auto_switch_service);
		}
	}
#ifdef __cplusplus
	}
#endif

#endif /* SYSTEM_UPDATE_SERVICES_H */
