/**
 * @file system_init.c
 * @brief System Initialization Wrapper (FASE 2 - DI Container)
 * @version 2.0.0
 * @date 2026-02-03
 *
 * @details
 * Wrapper simplificado que delega toda la inicialización al DI Container.
 * Reducido de 534 líneas a <50 líneas siguiendo principios SOLID.
 *
 * @note ANTES (Fase 1): God Function de 320 líneas con todo hardcoded
 * @note AHORA (Fase 2): Wrapper de <50 líneas usando DI Container
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "infrastructure/startup/system_init.h"
#include "infrastructure/di/dependency_container.h"
#include "infrastructure/osal/osal.h"
#include "application/tasks/control_task.h"
#include "presentation/presentation_layer.h"				/* PresentationLayer_Update() — ACTION-029 */
#include "application/services/display_backlight_service.h" /* Fase 4 */
#include "application/services/led_status_service.h"
#include "bsp/stm32u5/bsp_init.h"
#include "main.h"
#include "gpio.h"
#include "os_port.h"
#include "infrastructure/startup/system_update_services.h"
/*============================================================================*
 * STATIC INSTANCE
 *============================================================================*/

/**
 * @brief Global DI Container (singleton pattern).
 * @note Pre-allocated statically to avoid dynamic memory.
 */
static DependencyContainer_t g_system_container;


/*============================================================================*
 * PUBLIC API
 *============================================================================*/

/**
 * @brief Initialize all system components with dependency injection.
 * @note Called from App_ThreadX_Init() before kernel start.
 * @note Order: BSP → DI Container → Subsystems → Wire → Start AOs → Control Task
 */
Result_t System_Init(void)
{
	Result_t res;

	/* ===== 1. Initialize BSP (HAL Interfaces) ===== */
	res = BSP_Init();
	if (res != ERR_OK)
		return res;

	const BSP_Interfaces_t *bsp = BSP_GetInterfaces();

	/* ===== 2. Initialize DI Container with BSP ===== */
	res = DI_Container_Init(&g_system_container, bsp);
	if (res != ERR_OK)
		return res;

	/* ===== 3. Initialize Subsystems in Order ===== */

	/* 3.0 External Flash (OctoSPI Memory-Mapped) — must be first: LVGL assets live here */
	res = DI_InitExtFlashSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* 3.1 Logger (UART/ITM) - NO requiere Storage */
	res = DI_InitLoggingSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* 3.2 Storage (EEPROM + EventLogAO + ConfigStorage) - puede usar Logger */
	res = DI_InitStorageSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* 3.3 Event Notifier (EEPROM events) - requiere Storage + Logger */
	res = DI_InitEventNotifierSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitGPSSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitTimeSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitBatteryMonitorSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitDeviceIdentitySubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitLicenseSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitUpdateSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitRelaySubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitDigitalInputSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	res = DI_InitBuzzerSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* LED status indicator (led_status[0]) — non-fatal */
	(void)DI_InitLedStatusSubsystem(&g_system_container);

	/* Hourmeter AO (requiere storage + gps + relay) */
	res = DI_InitHourmeterSubsystem(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* Internal MCU temperature sensor (ADC1) — non-fatal */
	(void)DI_InitTemperatureSensorSubsystem(&g_system_container);

	/* USB RNDIS hardware — debe llamarse ANTES de DI_InitWifiSubsystem para que
	 * DI_InitNetworkSubsystem() pueda incluir el puerto RNDIS en el NetworkAO. */
	res = DI_InitUsbRndisSubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		res = ERR_OK; /* Non-fatal: sistema opera sin USB RNDIS */
	}

	/* WiFi hardware (ESP-Hosted SPI + WifiControlAdapter) */
	res = DI_InitWifiSubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		res = ERR_OK; /* Non-fatal: sistema opera sin WiFi */
	}

	/* WiFi Enable Service (WiFi module power control) */
	res = DI_InitWifiEnableSubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		res = ERR_OK; /* Non-fatal: sistema opera sin control de encendido WiFi */
	}

	/* Side button multi-function service (SSR3-V1 path) */
	res = DI_InitSideButtonSubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		res = ERR_OK; /* Non-fatal: sistema opera sin side button avanzado */
	}

	/* Network stack + puertos + NetworkAO + HTTP (requiere wifi_initialized) */
	res = DI_InitNetworkSubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		res = ERR_OK; /* Non-fatal: sistema opera sin red */
	}

	/* UI requiere digital_input_initialized — se llama después de DigitalInput */
	res = DI_InitUISubsystem(&g_system_container);
	if (res != ERR_OK)
	{
		/* Non-fatal: el sistema opera sin display */
		res = ERR_OK;
	}
	/* ===== 4. Wire Components (Callbacks + Observers) ===== */
	res = DI_WireComponents(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* ===== 5. Start All Active Objects ===== */
	res = DI_StartActiveObjects(&g_system_container);
	if (res != ERR_OK)
		return res;

	/* ===== 6. Initialize Control Task (Main Loop) ===== */
	ControlTaskDeps_t ctrl_deps = {
		.wifi_service = DI_GetWifiEnableService(&g_system_container)};
	res = ControlTask_Init(&ctrl_deps);
	if (res != ERR_OK)
		return res;

	/* ===== 7. Hardware Initialization (GPS Power & display power) ===== */
	GPIO_WritePin(bsp->gpio, PENABLE2_GPIO_Port, PENABLE2_Pin, I_GPIO_STATE_HIGH);
	GPIO_WritePin(bsp->gpio, PENABLE1_GPIO_Port, PENABLE1_Pin, I_GPIO_STATE_HIGH);

	// enable tx/rx RS-485 for cicx1
	const BoardProfile_t *profile = BSP_GetBoardProfile();

	if (profile->rs485.enabled)
	{
		GPIO_WritePin(bsp->gpio, profile->rs485.rs485_rx_en_port, profile->rs485.rs485_rx_en_pin, I_GPIO_STATE_LOW);
		GPIO_WritePin(bsp->gpio, profile->rs485.rs485_tx_en_port, profile->rs485.rs485_tx_en_pin, I_GPIO_STATE_HIGH);
	}

	/*boot sound buzzer */

	BuzzerNotificationService_NotifyEvent(
		DI_GetBuzzerNotificationService(&g_system_container),
		BUZZER_EVENT_BOOT_COMPLETE);

	/* ===== 8. Signal presentation layer: all systems ready ===== */
	/* This MUST be the last statement before return ERR_OK.
	 * If any subsystem above returned early with an error, this line is
	 * never reached and PresentationLayer_IsSystemReady() returns false
	 * → Overview screen shows "System Error". */
	PresentationLayer_SetSystemReady(true);

	return ERR_OK;
}

/**
 * @brief Periodic update task (100ms loop).
 * @note Called from a low-priority maintenance thread.
 */
void System_Update(void)
{

	/* TimeAdapter state machine (GPS timeout detection) */
	ITimeSource *time_source = DI_GetTimeSource(&g_system_container);
	if (time_source)
	{
		TimeAdapter_Update(time_source->impl); /* FIXME: Usar interfaz correcta */
	}

	/* RelayAdapter update (GPS disconnect and config change detection) */
	IRelayController *relay_ctrl = DI_GetRelayController(&g_system_container);
	if (relay_ctrl)
	{
		RelayController_Update(relay_ctrl);
	}

	/* Battery Monitor update (BQ27441 polling) */
	IBatteryMonitor *battery = DI_GetBatteryMonitor(&g_system_container);
	if (battery)
	{
		/* Use concrete adapter update for polling; interface has no Update method */
		BQ27441Adapter_Update((BQ27441Adapter_t *)battery->impl);
	}

	/* Active screen refresh — screen-agnostic via ScreenRouter (ACTION-029) */
	if (DisplayBacklightService_IsAwake(DI_GetBacklightService(&g_system_container)) != false)
	{
		PresentationLayer_Update();
	}

	/* LED status indicator update (blink state machine) */
	if (g_system_container.led_status_initialized)
	{
		LedStatusService_Update(&g_system_container.led_status_service);
	}

	/* Backlight timeout service — Fase 4 */
	DisplayBacklightService_Update(DI_GetBacklightService(&g_system_container));

	/* Optional services (WiFi watchdog + side button FSM + GPS antenna auto-switch). */
	System_UpdateOptionalServices(
		g_system_container.wifi_enable_service_initialized,
		g_system_container.wifi_module,
		g_system_container.side_button_initialized,
		&g_system_container.side_button_service,
		g_system_container.gps_initialized,
		&g_system_container.gps_antenna_auto_switch_service);
}

/**
 * @brief Get DI Container instance for external access.
 * @return Pointer to the global container.
 * @note Use getters (DI_GetTimeSource, etc.) instead of accessing directly.
 */
DependencyContainer_t *System_GetContainer(void)
{
	return &g_system_container;
}
