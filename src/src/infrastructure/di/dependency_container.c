/**
 * @file dependency_container_impl.c
 * @brief Implementación completa del Dependency Injection Container (FASE 2)
 * @version 2.0.0
 * @date 2026-02-03
 *
 * @details
 * Migración completa desde system_init.c al DI Container.
 * Implementa todas las funciones DI_Init*Subsystem() para inicialización modular.
 *
 * @note FASE 2: Contenedor completo con todos los subsistemas del sistema.
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "infrastructure/di/dependency_container.h"
#include "common/task_priorities.h" /* ✅ Mapa centralizado de prioridades */
#include "common/relay_types.h"		/* ✅ Phase 4.9: RelayConfig_t for Observer callbacks */
#include "common/gps_types.h"		/* ✅ Phase 4.9: GPSConfig_t for Observer callbacks */
#include "infrastructure/osal/osal.h"
#include "interfaces/i_wifi_transport.h" /* ✅ WiFi Transport Interface (DIP) */
#include "bsp/components/eeprom/eeprom_m24m01e.h"
#include "bsp/components/eeprom/eeprom_at24cxx.h"
#include "bsp/stm32u5/bsp_logging_output.h"
#include "bsp/stm32u5/bsp_board_profile.h"					   /* ✅ Board Profile Pattern */
#include "bsp/stm32u5/bsp_init.h"							   /* ✅ Peripheral Handle Lookup */
#include "bsp/stm32u5/bsp_stm32u5_adc_temp.h"				   /* u2705 Internal MCU temperature sensor (ADC1) */
#include "app_azure_rtos.h"									   /* ✅ App_GetBytePool() */
#include "esp_hosted_os_port.h"								   /* ✅ ESP-Hosted Framework DI API */
#include "control.h"										   /* ✅ ESP-Hosted Control Path API */
#include "common.h"											   /* ✅ ESP-Hosted Transport Events */
#include "infrastructure/adapters/wifi/esp_wifi_nic_adapter.h" /* ✅ NicDriver DI injection */
#include "application/activeobjects/network_ao.h"			   /* ✅ NetworkAO (Fase N.5) */
#include "application/activeobjects/eventlog_ao.h"
#include "application/activeobjects/ui_ao.h"																/* ✅ UiAO (ACTION-023) */
#include "presentation/presentation_layer.h"																/* ✅ HomeScreenPresenter shim (ACTION-026) */
#include "application/services/display_backlight_service.h"													/* ✅ Fase 4: backlight timeout */
#include "presentation/screens/home/eez_home_screen_view.h"													/* ✅ EEZ view (ACTION-026) */
#include "presentation/screens/security/eez_security_screen_view.h"											/* ✅ EEZ security view (ACTION-031) */
#include "presentation/screens/menu/eez_menu_screen_view.h"													/* ✅ EEZ menu view (ACTION-032) */
#include "presentation/screens/settings/eez_settings_screen_view.h"											/* ✅ EEZ settings view */
#include "presentation/screens/system_information/eez_system_information_screen_view.h"						/* ✅ EEZ system information view */
#include "presentation/screens/wifi_module/eez_wifi_module_screen_view.h"									/* ✅ EEZ WiFi module view */
#include "presentation/screens/wifi_configuration/eez_wifi_configuration_screen_view.h"						/* ✅ EEZ WiFi configuration view */
#include "presentation/screens/admin_configuration/eez_admin_configuration_screen_view.h"					/* ✅ EEZ admin configuration view */
#include "presentation/screens/admin_operation_mode/eez_admin_operation_mode_screen_view.h"					/* ✅ EEZ admin operation mode view */
#include "presentation/screens/int_configuration/eez_int_configuration_screen_view.h"						/* ✅ EEZ int config view */
#include "presentation/screens/gps_configuration/eez_gps_configuration_screen_view.h"						/* ✅ EEZ GPS config view */
#include "presentation/screens/contact_configuration/eez_contact_configuration_screen_view.h"				/* ✅ EEZ contact config view */
#include "presentation/screens/general_configuration/eez_general_configuration_screen_view.h"				/* ✅ EEZ general config view */
#include "presentation/screens/int_configuration_state/eez_int_configuration_state_screen_view.h"			/* ✅ EEZ int config state view */
#include "presentation/screens/int_configuration_start/eez_int_configuration_start_screen_view.h"			/* EEZ int config start view */
#include "presentation/screens/int_configuration_predefined/eez_int_configuration_predefined_screen_view.h" /* EEZ predefined cycles view */
#include "presentation/screens/gps_configuration_antenna/eez_gps_configuration_antenna_screen_view.h"		/* EEZ GPS antenna view */
#include "presentation/screens/contact_configuration_type/eez_contact_configuration_type_screen_view.h"
#include "presentation/screens/general_configuration_alarm/eez_general_configuration_alarm_screen_view.h" /* ✅ EEZ general config alarm view */ /* EEZ contact type view */
#include "presentation/screens/access_denied/eez_access_denied_screen_view.h"																	 /* ✅ EEZ access denied view */
#include "presentation/screens/configuration_input/eez_configuration_input_screen_view.h"														 /* ✅ EEZ configuration input view */
#include "presentation/screens/int_configuration_days/eez_int_configuration_days_screen_view.h"													 /* ✅ EEZ int configuration days view */
#include "presentation/screens/int_configuration_period/eez_int_configuration_period_screen_view.h"												 /* ✅ EEZ int configuration period view */
#include "presentation/screens/int_configuration_period_menu/eez_int_configuration_period_menu_screen_view.h"									 /* ✅ EEZ int configuration period menu view */
#include "presentation/screens/int_configuration_period/int_configuration_period_screen_presenter.h"											 /* ✅ Period roller presenter (Phase 4.10) */
#include "presentation/screens/int_configuration_period_menu/int_configuration_period_menu_screen_presenter.h"									 /* ✅ Period menu presenter (Phase 4.10) */
#include "presentation/screens/gps_configuration_antenna/gps_configuration_antenna_screen_presenter.h"
#include "presentation/screens/information_show/information_show_screen_presenter.h"
#include "presentation/screens/information_show/eez_information_show_screen_view.h"
#include "presentation/screens/qr_info/eez_qr_info_screen_view.h"				   /* ✅ EEZ QR Info view */
#include "src/presentation/ui/screens.h"										   /* ✅ ScreensEnum (SCREEN_ID_*) for navigation (ACTION-031) */
#include "infrastructure/adapters/wifi/wifi_status_adapter.h"					   /* ✅ WifiStatusSource ISP adapter (ACTION-027) */
#include "infrastructure/adapters/storage/storage_coordinator_config_adapter_v2.h" /* ✅ v2 - IConfigStorage modular backend (Phase 4.6) */
#include "infrastructure/adapters/storage/storage_coordinator_adapter_v2.h"		   /* ✅ v2 - IEventNotifier → EventLogAO (Phase 4.5) */
#include "infrastructure/adapters/network/http_server_adapter.h"				   /* ✅ HttpServerAdapter (Fase N.5) */
#include "infrastructure/adapters/network/cyclone_network_stack_adapter.h"		   /* ✅ CycloneTCP stack adapter (Fase N.7) */
#include "infrastructure/adapters/network/cyclone_network_port_adapter.h"		   /* ✅ Paso 5: CycloneNetworkPortAdapter STA/AP */
#include "infrastructure/adapters/network/cyclone_usb_rndis_port_adapter.h"		   /* ✅ USB RNDIS gateway (netInterface[2]) */
#include "infrastructure/adapters/crypto/cyclone_crypto_verifier_adapter.h"		   /* ✅ OTA crypto verifier adapter */
#include "infrastructure/adapters/cyclone_boot_ext_flash_adapter.h"				   /* ✅ OTA external flash adapter */
#include "version.h"
/* USB Device Library & RNDIS class driver (BSP-level — only allowed in Composition Root) */
#include "usbd_core.h"
#include "drivers/usb_rndis/rndis_driver.h"
#include "drivers/usb_rndis/usbd_rndis.h"
#include "drivers/usb_rndis/usbd_desc.h"

#include <string.h>

/**
 * @note BSP Interfaces deben obtenerse vía BSP_GetInterfaces().
 * @note OSAL ya está inicializado en app_threadx.c antes de llamar a DI_Container_Init().
 */

/*============================================================================*
 * PRIVATE HELPERS
 *============================================================================*/

/**
 * @brief Limpia estructura del container
 */
static void DI_Container_Clear(DependencyContainer_t *container)
{
	if (container)
	{
		memset(container, 0, sizeof(DependencyContainer_t));
	}
}

/**
 * @brief Creates NetworkPortConfig_t for WiFi Station (netInterface[0]).
 *
 * @param[out] cfg   Output config structure (caller-provided).
 * @param[in]  wifi  WiFi configuration from storage cache.
 *
 * @note MUST be registered as port[1] AFTER AP, so mode is already set.
 */
static void di_create_sta_port_config(NetworkPortConfig_t *cfg, const WifiConfig_t *wifi);
/**
 * @brief Creates NetworkPortConfig_t for WiFi Access Point (netInterface[1]).
 *
 * @param[out] cfg   Output config structure (caller-provided).
 * @param[in]  wifi  WiFi configuration from storage cache.
 *
 * @note MUST be registered as port[0] in NetworkAO so it configures FIRST:
 *       AP calls SetMode(APSTA) + StartSoftAP(), then STA calls ConnectAP().
 */
static void di_create_ap_port_config(NetworkPortConfig_t *cfg, const WifiConfig_t *wifi);
/*============================================================================*
 * NOTE: Default configurations removed - ModularConfigStorageAdapter
 * uses its own internal static defaults. No external injection needed.
 * SystemConfig_t monolithic defaults eliminated (Phase 4.10 - modular migration).
 *============================================================================*/

/*============================================================================*
 * UART CALLBACKS (GPS)
 *============================================================================*/

/**
 * @brief UART RX Callback: Feeds both GPSAdapter (ring buffer) and GPSAo (event)
 * @note Executes in ISR context. Must be ISR-safe (<50μs).
 * @param context DependencyContainer_t* (container instance)
 * @param size Number of bytes received in this DMA event.
 */
static void DI_GPS_UART_RxCallback(void *context, uint16_t size)
{
	DependencyContainer_t *container = (DependencyContainer_t *)context;
	if (!container || !container->gps_initialized)
	{
		return;
	}

	I_UART *uart = container->bsp->uart;
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	UART_HandleTypeDef *gps_uart = BSP_GetUARTHandle(profile->peripheral_map.gps_uart_index);

	/* Feed GPSAdapter ring buffer (ISR-safe, no parsing) */
	GPSAdapter_ProcessRxBuffer(&container->gps_adapter, container->gps_uart_dma_buffer, size);

	/* Restart DMA reception */
	UART_ReceiveUntilIdle_Async(uart, gps_uart, container->gps_uart_dma_buffer, DI_GPS_UART_DMA_BUFFER_SIZE);

	/* Notify GPSAo thread (posts event to queue) */
	GPSAo_OnRxData(&container->gps_ao, container->gps_uart_dma_buffer, size);
}

/**
 * @brief UART Error Callback: Restart DMA reception
 */
static void DI_GPS_UART_ErrorCallback(void *context, uint16_t size)
{
	(void)size;
	DependencyContainer_t *container = (DependencyContainer_t *)context;
	if (!container || !container->gps_initialized)
	{
		return;
	}

	I_UART *uart = container->bsp->uart;
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	UART_HandleTypeDef *gps_uart = BSP_GetUARTHandle(profile->peripheral_map.gps_uart_index);

	/* Restart DMA reception on error */
	UART_ReceiveUntilIdle_Async(uart, gps_uart, container->gps_uart_dma_buffer, DI_GPS_UART_DMA_BUFFER_SIZE);
}

/*============================================================================*
 * PPS CALLBACKS (GPS → TimeAdapter, GPS → RelayAO)
 *============================================================================*/

/**
 * @brief PPS Callback Bridge: GPS → TimeAdapter
 * @note Executes in ISR context (EXTI). Must be <50μs.
 */
static void DI_PPS_To_TimeSync_Callback(void *context, const DateTime_t *gps_time)
{
	ITimeSyncControl *sync_ctrl = (ITimeSyncControl *)context;

	/* CRITICAL: Pass GPS time to TimeAdapter for RTC sync */
	TimeSyncControl_OnPPS(sync_ctrl, gps_time);
}

/**
 * @brief PPS Callback Bridge: GPS → RelayAO (Event Queue)
 * @note Executes in ISR context. Posts event to Active Object.
 */
static void DI_PPS_To_RelayAO_Callback(void *context, const DateTime_t *gps_time)
{
	RelayAO_t *ao = (RelayAO_t *)context;

	/* Post PPS event to Active Object queue (non-blocking) */
	RelayAO_Event_t event = {
		.type = RELAY_EVENT_PPS,
		.payload = (void *)gps_time /* Can be NULL, RelayAdapter reads from TimeSource */
	};

	RelayAO_PostEvent(ao, &event); /* OS_NO_WAIT for ISR safety */
}

/*============================================================================*
 * DIGITAL INPUT CALLBACKS (Alarm)
 *============================================================================*/

/**
 * @brief Callback para alarma temperatura → BuzzerService (beep periódico)
 * @note ACTION-016: Genera beep cada 4 segundos mientras alarma activa
 * @note Ejecuta en DigitalInputAO thread (20ms polling)
 * @note Genera beep SOLO si:
 *       - Config general.buzzer_high_temp_alarm habilitado
 *       - Evento es PRESS (inicial) o KEEPALIVE (cada 4s)
 *       - En KEEPALIVE: verifica keepalive_count >= 1 y debounce de 3.5s
 *
 * @param context Puntero a DependencyContainer_t (para acceder storage + buzzer)
 * @param input_id ID de entrada (validated by registration)
 * @param event Tipo de evento (PRESS, KEEPALIVE, RELEASE, etc.)
 * @param data Metadata del evento (timestamp_ms, keepalive_count, press_duration_ms)
 */
static void DI_OnOvertempAlarm_BuzzerCallback(
	void *context,
	DigitalInputID_t input_id,
	DigitalInputEvent_t event,
	const DigitalInputEventData_t *data)
{
	(void)input_id; /* Validated by registration */

	/* Static para tracking de último beep (debounce anti-duplicación) */
	static uint32_t s_last_beep_timestamp_ms = 0;
	static const uint32_t BEEP_MIN_INTERVAL_MS = 3500; /* 3.5s mínimo entre beeps */

	DependencyContainer_t *container = (DependencyContainer_t *)context;
	/* ✅ Phase 4.3.3: Updated to use storage_v2_initialized */
	if (container == NULL || !container->buzzer_initialized || !container->storage_v2_initialized)
	{
		return; /* Safety: context inválido o subsistemas no inicializados */
	}

	if (data == NULL)
	{
		return; /* Safety: metadata requerida para validación de timing */
	}

	BuzzerNotificationService_t *buzzer_service = &container->buzzer_notification;

	/* Solo responder a PRESS (inicial) y KEEPALIVE (periódico cada 4s) */
	if (event != DI_EVENT_PRESS && event != DI_EVENT_KEEPALIVE)
	{
		return; /* Ignorar RELEASE, CLICK, etc. */
	}

	/* Validación adicional para KEEPALIVE: debe tener keepalive_count >= 1 */
	if (event == DI_EVENT_KEEPALIVE && data->keepalive_count == 0)
	{
		return; /* KEEPALIVE inválido (lwbtn debería tener count >= 1) */
	}

	/* Anti-duplicación: verificar que hayan pasado al menos 3.5s desde último beep */
	uint32_t current_time = data->timestamp_ms;
	uint32_t elapsed_ms = (current_time >= s_last_beep_timestamp_ms)
							  ? (current_time - s_last_beep_timestamp_ms)
							  : BEEP_MIN_INTERVAL_MS; /* Overflow: permitir beep */

	if (elapsed_ms < BEEP_MIN_INTERVAL_MS)
	{
		return; /* Debounce: muy poco tiempo desde último beep */
	}

	/* Consultar config desde cache RAM (thread-safe, <1μs latency) */
	GeneralConfig_t general_config;
	IConfigStorage *config_storage = DI_GetConfigStorage(container);
	if (config_storage == NULL)
	{
		return; /* Storage no inicializado */
	}

	Result_t res = ConfigStorage_LoadGeneralConfig(config_storage, &general_config);

	if (res != ERR_OK)
	{
		return; /* Error leyendo config, no beep (safe fallback) */
	}

	if (!general_config.buzzer_high_temp_alarm)
	{
		return; /* Buzzer deshabilitado por configuración de usuario */
	}

	/* Notificar alarma crítica vía BuzzerNotificationService */
	/* El servicio maneja prioridad CRITICAL (siempre suena, ignora silent_mode) */
	BuzzerNotificationService_NotifyEvent(
		buzzer_service,
		BUZZER_EVENT_TEMP_ALARM_HIGH);

	/* Actualizar timestamp del último beep para debounce */
	s_last_beep_timestamp_ms = current_time;
}

/**
 * @brief Callback para botones UI → BuzzerService (feedback táctil)
 * @note Genera beep corto (50ms) al presionar botón de navegación
 * @note Ejecuta en DigitalInputAO thread (20ms polling)
 * @note Solo responde a DI_EVENT_PRESS (ignora RELEASE, CLICK, KEEPALIVE)
 *
 * @param context Puntero a DependencyContainer_t (para acceder buzzer_notification)
 * @param input_id ID de entrada (botón presionado)
 * @param event Tipo de evento (filtrado: solo PRESS)
 * @param data Metadata del evento (ignored para feedback simple)
 */
static void DI_OnButtonPress_BuzzerCallback(
	void *context,
	DigitalInputID_t input_id,
	DigitalInputEvent_t event,
	const DigitalInputEventData_t *data)
{
	(void)input_id; /* Todos los botones usan mismo beep */
	(void)data;		/* Metadata no necesaria para feedback simple */

	DependencyContainer_t *container = (DependencyContainer_t *)context;
	if (container == NULL || !container->buzzer_initialized)
	{
		return; /* Safety: buzzer subsystem no inicializado */
	}

	/* Solo responder a PRESS (inicial), ignorar otros eventos */
	if (event != DI_EVENT_PRESS)
	{
		return;
	}

	BuzzerNotificationService_t *buzzer_service = &container->buzzer_notification;

	/* Generar feedback táctil (beep corto) */
	/* BuzzerNotificationService mapea BUTTON_PRESS → 50ms LOW priority */
	BuzzerNotificationService_NotifyEvent(
		buzzer_service,
		BUZZER_EVENT_BUTTON_PRESS);
}

/**
 * @brief Callback for temperature alarm (ACTION-009, actualizado ACTION-012).
 * @note Executes in DigitalInputAO polling thread context (20ms).
 * @note Posts alarm event to RelayAO which forces contact open immediately.
 * @param context Pointer to RelayAO_t instance (passed at registration)
 * @param input_id Digital input ID (validated by registration)
 * @param event PRESS (alarm active) or RELEASE (alarm cleared)
 * @param data Metadata del evento (ACTION-012: ignorado para alarma)
 */
static void DI_OnOvertempAlarm_Callback(void *context,
										DigitalInputID_t input_id,
										DigitalInputEvent_t event,
										const DigitalInputEventData_t *data)
{
	(void)input_id; /* Validated by registration */
	(void)data;		/* Alarm no usa metadata */

	RelayAO_t *relay_ao = (RelayAO_t *)context;
	if (relay_ao == NULL)
		return;

	RelayAO_Event_t relay_event;

	if (event == DI_EVENT_PRESS)
	{
		/* Alarm activated → Force relay open */
		relay_event.type = RELAY_EVENT_ALARM_OVERTEMP;
	}
	else if (event == DI_EVENT_RELEASE)
	{
		/* Alarm cleared → Resume normal operation */
		relay_event.type = RELAY_EVENT_ALARM_CLEARED;
	}
	else
	{
		return; /* Ignore CLICK/LONG_PRESS/KEEPALIVE on alarm input */
	}

	relay_event.payload = NULL;
	RelayAO_PostEvent(relay_ao, &relay_event); /* Non-blocking, executed in polling thread */
}

/*============================================================================*
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

Result_t DI_Container_Init(DependencyContainer_t *container, const BSP_Interfaces_t *bsp)
{
	if (!container || !bsp)
	{
		return ERR_NULL_POINTER;
	}

	/* Limpiar estructura */
	DI_Container_Clear(container);

	/* Inyectar BSP */
	container->bsp = bsp;
	container->bsp_initialized = true;

	/**
	 * NOTA: OSAL ya está inicializado en app_threadx.c:70 (antes de llamar System_Init).
	 * No es necesario (ni correcto) llamar os_init() aquí nuevamente.
	 */

	container->is_initialized = true;

	return ERR_OK;
}

/*============================================================================*
 * SUBSYSTEM INITIALIZATION
 *============================================================================*/

/**
 * @brief Initializes the external OctoSPI Flash and enables Memory-Mapped mode.
 *
 * Must be called BEFORE any subsystem that reads from external flash (LVGL assets,
 * timezone LUT, etc.). No prerequisites beyond BSP_Init().
 *
 * @param[in,out] container  Container instance.
 * @return ERR_OK on success, ERR_ERROR if flash init or memory-map fails.
 */
Result_t DI_InitExtFlashSubsystem(DependencyContainer_t *container)
{
	if (container == NULL || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	const BSP_Interfaces_t *bsp = container->bsp;
	if (bsp == NULL || bsp->ext_flash == NULL)
	{
		return ERR_NULL_POINTER;
	}

	/* Obtain hardware handle from BSP board profile */
	void *handle = BSP_GetOctoSPIHandle(0);
	if (handle == NULL)
	{
		return ERR_ERROR;
	}

	/* Init: configure quad/octo DTR settings (empty config = driver defaults) */
	I_EXT_FLASH_Config_t config = {0};
	Result_t res = EXT_FLASH_Init(bsp->ext_flash, handle, &config);
	if (res != ERR_OK)
	{
		return res;
	}

	/* Switch to Memory-Mapped mode so OCTOSPI1 region is directly addressable */
	res = EXT_FLASH_EnableMemoryMapped(bsp->ext_flash, handle);
	if (res != ERR_OK)
	{
		return res;
	}

	container->ext_flash_handle = handle;
	container->ext_flash_initialized = true;
	return ERR_OK;
}

void *DI_GetExtFlashHandle(const DependencyContainer_t *container)
{
	if (container == NULL || !container->ext_flash_initialized)
	{
		return NULL;
	}
	return container->ext_flash_handle;
}

/* ─────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Initialize Storage Subsystem (v2 unified).
 *
 * Initializes:
 * - EEPROM driver (M24M01E)
 * - EventLogAO (Active Object with Pattern B, replaces legacy adapter)
 * - ModularConfigStorageAdapter (modular config handlers)
 * - StorageCoordinatorAO_v2 (async coordinator with cache)
 * - StorageCoordinatorConfigAdapter_v2 (IConfigStorage interface)
 *
 * @note Defaults come from modular config handlers (no external injection needed).
 * @note All consumers should use v2 APIs (StorageCoordinatorAO_v2_* or IConfigStorage).
 *
 * @param[in,out] container  Container instance (must be initialized via DI_Container_Init).
 *
 * @return ERR_OK on success.
 * @return ERR_INVALID_STATE if container not initialized.
 * @return ERR_INVALID_PARAM if Board Profile or I2C handle invalid.
 * @return ERR_ERROR on driver/adapter initialization failure.
 */
Result_t DI_InitStorageSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* Get Board Profile for hardware configuration */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	/* Check if EEPROM is available in this board variant */
	if (!profile->eeprom.enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "EEPROM disabled in board profile (variant: %s)", profile->variant_name);
		}
		container->storage_initialized = false;
		container->storage_v2_initialized = false;
		return ERR_OK; /* Success, but storage not initialized */
	}

	void *eeprom_i2c = BSP_GetI2CHandle(profile->peripheral_map.eeprom_i2c_index);
	if (!eeprom_i2c)
	{
		return ERR_INVALID_PARAM; /* Invalid I2C index in profile */
	}

	/* ===== 1. Initialize EEPROM Driver ===== */
	container->eeprom_driver = M24M01E_Create(container->bsp->i2c, eeprom_i2c, &profile->eeprom);
	if (container->eeprom_driver == NULL)
	{
		return ERR_ERROR;
	}

	/* ===== 2. Initialize Event Log Active Object ===== */
	EventLogAOConfig_t event_log_cfg = {
		.eeprom = container->eeprom_driver,
		.eeprom_handle = eeprom_i2c,
		.logger = DI_GetLogger(container),
	};
	res = EventLogAO_Init(&container->event_log_ao, &event_log_cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	/* ===== 3. Initialize Modular Config Storage Adapter ===== */
	/* Note: Adapter uses internal static defaults, no injection needed */
	ModularConfigStorageAdapterConfig_t adapter_cfg = {
		.eeprom = container->eeprom_driver,
		.eeprom_handle = eeprom_i2c};
	res = ModularConfigStorageAdapter_Init(&container->modular_config_storage, &adapter_cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	/* ===== 4. Initialize Storage Coordinator AO v2 ===== */
	StorageCoordinatorAOConfig_v2_t coord_cfg = {
		.config_storage = ModularConfigStorageAdapter_GetInterface(&container->modular_config_storage),
		.rate_limit_ms = 100, /* 100ms rate limiting for EEPROM wear protection */
		.logger = DI_GetLogger(container),
		.event_notifier = NULL /* Wired later in DI_WireComponents after EventNotifier init */};
	res = StorageCoordinatorAO_v2_Init(&container->storage_coordinator_v2, &coord_cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	/* ===== 5. Initialize Config Storage Adapter v2 (IConfigStorage interface) ===== */
	res = StorageCoordinatorConfigAdapter_v2_Init(
		&container->config_storage_adapter_v2,
		&container->storage_coordinator_v2);
	if (res != ERR_OK)
	{
		return res;
	}

	/* Mark storage subsystem as initialized (both v1 and v2 flags for backward compat) */
	container->storage_initialized = true;
	container->storage_v2_initialized = true;

	return ERR_OK;
}

Result_t DI_InitGPSSubsystem(DependencyContainer_t *container)
{
	/* ✅ Phase 4.3.1: Migrated to Storage v2 (modular GPSConfig_t) */
	if (!container || !container->storage_v2_initialized)
	{
		return ERR_INVALID_STATE; /* DI_InitStorageSubsystem() must be called first */
	}

	Result_t res;

	/* ✅ Obtener Board Profile para configuración de hardware */
	const BoardProfile_t *profile = BSP_GetBoardProfile();

	/* ===== 1. Load GPS Configuration from v2 Storage (modular) ===== */
	/* ✅ Phase 4.10: DIP Compliance - Use IConfigStorage interface */
	GPSConfig_t gps_config;
	IConfigStorage *config_storage = DI_GetConfigStorage(container);
	if (config_storage == NULL)
	{
		return ERR_INVALID_STATE; /* Storage no inicializado */
	}

	res = ConfigStorage_LoadGPSConfig(config_storage, &gps_config);
	if (res != ERR_OK)
	{
		return res; /* If load fails, v2 already returns defaults from handler */
	}

	/* ===== 2. Initialize GPSAdapter with loaded config ===== */
	GPSAdapterConfig_t gps_cfg = {
		.pps_minimum_valid = 5,

		/* ✅ ACTION-014: Inject UTC offset from EEPROM (v2 modular config) */
		.utc_offset_index = gps_config.utc_offset_index,
		.seconds_offset = gps_config.time_offset,

		/* ✅ ACTION-014: Inject antenna config from EEPROM (v2 modular config) */
		.antenna_type = gps_config.antenna_type,

		/* ✅ Hardware pins desde Board Profile */
		.gpio = container->bsp->gpio,
		.rf_ctrl1_port = profile->rf_ctrl1.port, /* ✅ Desde Board Profile */
		.rf_ctrl1_pin = profile->rf_ctrl1.pin,	 /* ✅ Desde Board Profile */
		.rf_ctrl2_port = profile->rf_ctrl2.port, /* ✅ Desde Board Profile */
		.rf_ctrl2_pin = profile->rf_ctrl2.pin,	 /* ✅ Desde Board Profile */
		.gps_rst_port = profile->gps_reset.port, /* ✅ Desde Board Profile */
		.gps_rst_pin = profile->gps_reset.pin};	 /* ✅ Desde Board Profile */

	res = GPSAdapter_Init(&container->gps_adapter, container->bsp->uart, container->bsp->exti, profile->gps_pps.pin, &gps_cfg);
	if (res != ERR_OK)
		return res;

	/* ===== 3. Initialize GPSAo (NMEA Processing Thread) ===== */
	GPSAoConfig_t gps_ao_cfg = {
		.stack_ptr = container->gps_ao_stack,
		.stack_size = DI_GPS_AO_STACK_SIZE,
		.priority = TASK_PRIO_GPS,
		.logger = DI_GetLogger(container),
		.event_notifier = DI_GetEventNotifier(container)};

	IGPSIngestor *gps_ingestor = GPSAdapter_GetIngestInterface(&container->gps_adapter);
	res = GPSAo_Init(&container->gps_ao, gps_ingestor, &gps_ao_cfg);
	if (res != ERR_OK)
		return res;

	container->gps_initialized = true;

	/* ===== 4. Configure GPS UART Reception (DMA + Idle Line) ===== */
	void *gps_uart = BSP_GetUARTHandle(profile->peripheral_map.gps_uart_index);
	if (!gps_uart)
	{
		return ERR_INVALID_PARAM; /* Índice UART inválido en profile */
	}

	res = UART_RegisterRxEventCallback(container->bsp->uart, gps_uart, DI_GPS_UART_RxCallback, container);
	if (res != ERR_OK)
		return res;

	res = UART_RegisterErrorCallback(container->bsp->uart, gps_uart, DI_GPS_UART_ErrorCallback, container);
	if (res != ERR_OK)
		return res;

	/* Start DMA reception */
	UART_ReceiveUntilIdle_Async(container->bsp->uart, gps_uart, container->gps_uart_dma_buffer, DI_GPS_UART_DMA_BUFFER_SIZE);
	if (res != ERR_OK)
		return res;

	/* ===== 5. Initialize GPS Antenna Auto-Switch Service ===== */
	GpsAntennaAutoSwitchServiceDeps_t auto_switch_deps = {
		.gps_source = GPSAdapter_GetInterface(&container->gps_adapter),
		.gps_control = GPSAdapter_GetControlInterface(&container->gps_adapter),
		.config_storage = config_storage};

	res = GpsAntennaAutoSwitchService_Init(&container->gps_antenna_auto_switch_service, &auto_switch_deps);
	if (res != ERR_OK)
		return res;

	return ERR_OK;
}

Result_t DI_InitTimeSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->gps_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* ✅ Obtener Board Profile para configuración de hardware */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	/* Obtener RTC handle dinámico */
	void *rtc_handle = BSP_GetRTCHandle(profile->peripheral_map.rtc_index);
	if (!rtc_handle)
	{
		return ERR_INVALID_PARAM; /* Índice RTC inválido en profile */
	}

	/* ===== Initialize TimeAdapter ===== */
	/* NOTE: rtc_write_interval_ms drives GPS→RTC correction frequency.
	 * LSI accuracy is ±1% (±320 Hz) → 36 s drift/h without resync.
	 * At 1h interval: max accumulated drift ≤ 36 s (acceptable with GPS active).
	 * Previous value was 86400000 (24h) which allowed up to 864 s drift.
	 * Hardware fix: add 32.768 kHz LSE crystal on PC14/PC15 for ±1.7 s/day. */
	TimeAdapterConfig_t time_cfg = {
		.rtc = container->bsp->rtc,
		.rtc_handle = rtc_handle, /* ✅ Dinámico desde profile */
		.gps = GPSAdapter_GetInterface(&container->gps_adapter),
		.pps_dispatcher = &container->pps_dispatcher,
		.pps_timeout_ms = 2000,
		.rtc_write_interval_ms = 3600000, /* 1 h — LSI ±1% needs periodic GPS correction */
		.event_notifier = DI_GetEventNotifier(container)};
	res = TimeAdapter_Init(&container->time_adapter, &time_cfg);
	if (res != ERR_OK)
		return res;

	container->time_initialized = true;

	return ERR_OK;
}

Result_t DI_InitBatteryMonitorSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->storage_v2_initialized)
	{
		return ERR_INVALID_STATE; /* Require config storage for defaults */
	}

	Result_t res;
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	if (!profile->battery.enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "Battery monitor disabled in board profile (variant: %s)", profile->variant_name);
		}
		container->battery_monitor_initialized = false;
		return ERR_OK; /* Success, but battery monitoring not initialized */
	}
	void *i2c_handle = BSP_GetI2CHandle(profile->peripheral_map.battery_i2c_index);
	if (!i2c_handle)
	{
		return ERR_INVALID_PARAM; /* Invalid I2C index in profile */
	}

	BQ27441AdapterConfig_t cfg = {
		.i2c = container->bsp->i2c,
		.i2c_handle = i2c_handle,
		.logger = DI_GetLogger(container),
		.update_interval_ms = 500,
		.design_capacity_mah = 1000,
		.design_energy_mwh = 3700,
		.terminate_voltage_mv = 3000,
		.taper_rate_10th_h = 100,
		.enable_auto_hibernate = false,
		.get_tick_ms = os_ticks_get};

	res = BQ27441Adapter_Init(&container->battery_monitor, &cfg);
	if (res != ERR_OK)
		return res;

	container->battery_monitor_initialized = true;
	return ERR_OK;
}

Result_t DI_InitDeviceIdentitySubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}

	Result_t res = BspDeviceIdentity_Init(&container->device_identity_bsp);
	if (res != ERR_OK)
	{
		return res;
	}

	container->device_identity_initialized = true;
	return ERR_OK;
}

Result_t DI_InitRelaySubsystem(DependencyContainer_t *container)
{
	/* ✅ Phase 4.3.2: Updated to require storage_v2_initialized */
	if (!container || !container->time_initialized || !container->storage_v2_initialized)
	{
		return ERR_INVALID_STATE; /* DI_InitTimeSubsystem() and DI_InitStorageSubsystem() required */
	}

	Result_t res;

	/* ===== 1. Obtener configuración de hardware (profile) ===== */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_ERROR; /* No debería pasar, BSP_GetBoardProfile() nunca es NULL */
	}

	/* ===== 2. Load Relay Configuration from v2 Storage (modular) ===== */
	/* ✅ Phase 4.3.2: Migrated to Storage v2 - RelayConfig includes time_window */
	/* ✅ Phase 4.10: DIP Compliance - Use IConfigStorage interface */
	RelayConfig_t relay_config;
	IConfigStorage *config_storage = DI_GetConfigStorage(container);
	if (config_storage == NULL)
	{
		return ERR_INVALID_STATE; /* Storage no inicializado */
	}

	res = ConfigStorage_LoadRelayConfig(config_storage, &relay_config);
	if (res != ERR_OK)
	{
		return res; /* If load fails, v2 already returns defaults from handler */
	}

	/* ===== 3. Initialize CycleScheduler with loaded config ===== */
	res = CycleScheduler_Init(&container->cycle_scheduler, &relay_config);
	if (res != ERR_OK)
		return res;

	/* ===== 4. Obtener Timer handle dinámico ===== */
	void *relay_timer = BSP_GetTimerHandle(profile->peripheral_map.relay_timer_index);
	if (!relay_timer)
	{
		return ERR_INVALID_PARAM; /* Índice Timer inválido en profile */
	}

	/* ===== 5. Initialize RelayAdapter con pins del profile ===== */
	RelayAdapterConfig_t relay_cfg = {
		.time_source = TimeAdapter_GetTimeSourceInterface(&container->time_adapter),
		.gpio = container->bsp->gpio,
		.timer = container->bsp->timer,
		.timer_handle = relay_timer, /* ✅ Dinámico desde profile */
		.cycle_scheduler = &container->cycle_scheduler,

		/* ✅ CORRECTO: Usar profile en vez de hardcodear pines */
		.relay_gpio_port = profile->relay_control.port,
		.relay_gpio_pin = profile->relay_control.pin,

		/* ✅ LEDs también desde profile (pueden no existir en V2) */
		.db4_gpio_port = profile->led_status[1].enabled ? profile->led_status[1].port : 0,
		.db4_gpio_pin = profile->led_status[1].enabled ? profile->led_status[1].pin : 0,
		.db5_gpio_port = profile->led_status[2].enabled ? profile->led_status[2].port : 0,
		.db5_gpio_pin = profile->led_status[2].enabled ? profile->led_status[2].pin : 0,

		/* ✅ v4.0: TimeWindowConfig is now integrated in RelayConfig */
		.window_config = relay_config.time_window,
		.update_interval_ms = 100};

	res = RelayAdapter_Init(&container->relay_adapter, &relay_cfg);
	if (res != ERR_OK)
		return res;

	/* ===== 6. Initialize RelayAO (Active Object) ===== */
	RelayAO_Config_t ao_cfg = {
		.relay_controller = RelayAdapter_GetInterface(&container->relay_adapter),
		.stack_ptr = container->relay_ao_stack,
		.stack_size = DI_RELAY_AO_STACK_SIZE,
		.queue_buf = container->relay_ao_queue_buf,
		.queue_buf_size = sizeof(container->relay_ao_queue_buf),
		.priority = TASK_PRIO_RELAY,
		.logger = DI_GetLogger(container),
		.event_notifier = DI_GetEventNotifier(container),
		.time_source = DI_GetTimeSource(container)};
	res = RelayAO_Init(&container->relay_ao, &ao_cfg);
	if (res != ERR_OK)
		return res;

	container->relay_initialized = true;

	return ERR_OK;
}

Result_t DI_InitDigitalInputSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->relay_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* ===== 1. Obtener profile de hardware ===== */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_ERROR;
	}

	/* ===== 2. Construir array dinámico de inputs habilitados ===== */
	static DigitalInputConfig_t digital_input_configs[BUTTON_MAX_COUNT + 1]; /* +1 para alarma */
	uint8_t active_count = 0;

	/* 2.1. Iterar botones del profile y agregar solo los habilitados */
	for (uint8_t i = 0; i < BUTTON_MAX_COUNT; i++)
	{
		if (profile->buttons[i].enabled)
		{
			digital_input_configs[active_count++] = (DigitalInputConfig_t){
				.id = (DigitalInputID_t)i, /* ButtonID_t mapea 1:1 a DigitalInputID_t */
				.type = DI_TYPE_BUTTON,
				.port = profile->buttons[i].port,
				.pin = profile->buttons[i].pin,
				.active_low = profile->buttons[i].active_low,
				.long_press_ms = profile->buttons[i].long_press_ms};
		}
	}

	/* 2.2. Agregar entrada de alarma si está habilitada */
	if (profile->alarm_input.enabled)
	{
		digital_input_configs[active_count++] = (DigitalInputConfig_t){
			.id = DI_ID_ALARM_OVERTEMP,
			.type = DI_TYPE_ALARM_INPUT,
			.port = profile->alarm_input.port,
			.pin = profile->alarm_input.pin,
			.active_low = profile->alarm_input.active_low,
			.long_press_ms = 0};
	}

	/* ===== 3. Validar que hay al menos un input ===== */
	if (active_count == 0)
	{
		/* No debería pasar, pero manejar gracefully */
		return ERR_INVALID_STATE;
	}

	/* ===== 4. Initialize DigitalInputAdapter con inputs activos ===== */
	DigitalInputAdapterConfig_t di_adapter_cfg = {
		.gpio = container->bsp->gpio,
		.inputs = digital_input_configs,
		.input_count = active_count, /* ✅ Solo botones que existen */
		.debounce_ms = 20,
	};

	if (profile->display.enabled) /* Backlight management en input source */
	{
		di_adapter_cfg.backlight_svc = &container->backlight_service;
	}

	res = DigitalInputAdapter_Init(&container->digital_input, &di_adapter_cfg);
	if (res != ERR_OK)
		return res;

	/* ===== 5. Initialize DigitalInputAO (Polling Thread) ===== */
	IDigitalInputSource *di_source = DigitalInputAdapter_GetInterface(&container->digital_input);

	DigitalInputAO_Config_t di_ao_cfg = {
		.input_source = di_source,
		.poll_interval_ms = 20,
		.priority = TASK_PRIO_DIGITAL_INPUT,
		.stack_ptr = container->digitalinput_ao_stack,
		.stack_size = DI_DIGITALINPUT_AO_STACK_SIZE,
		.logger = DI_GetLogger(container),
		.event_notifier = DI_GetEventNotifier(container)};

	res = DigitalInputAO_Init(&container->digital_input_ao, &di_ao_cfg);
	if (res != ERR_OK)
		return res;

	container->digital_input_initialized = true;

	return ERR_OK;
}

/*============================================================================*
 * UI SUBSYSTEM (ACTION-023) — private guard/hook helpers
 *============================================================================*/

/**
 * @brief Guard predicate for the Interrupt Configuration ListNav.
 *        Item 0 (Interrupt State toggle) is always accessible.
 *        Items 1-N are blocked when relay.enabled != 0.
 *        Fails open (returns true) on storage error so the user is never
 *        permanently locked out by a transient read failure.
 *
 * @param[in] item_idx  Zero-based index of the item the user tapped.
 * @param[in] ctx       Pointer to IModularConfigStorage.
 * @return true  if navigation is allowed, false if it must be denied.
 */
static bool relay_guard_fn(uint8_t item_idx, void *ctx)
{
	if (item_idx == 0U)
	{
		return true; /* Item 0: "1. Interrupt State" — never blocked */
	}
	IConfigStorage *storage = (IConfigStorage *)ctx;
	RelayConfig_t cfg;

	if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
	{
		return true; /* Fail open — allow access on storage error */
	}
	return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied hook for the Interrupt Configuration ListNav.
 *        Sets the "return screen" on the Access Denied presenter
 *        so that after the timeout the router goes back to INT_CONFIGURATION.
 */
static void int_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx;
	(void)ctx;
	PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_INT_CONFIGURATION);
	PresentationLayer_SetAccessDeniedMessage(NULL); /* NULL = use default "CYCLE IN PROGRESS" */
}

/**
 * @brief Guard predicate for the GPS Configuration ListNav.
 *        All items are blocked when the relay is enabled (running).
 *        Fails open (returns true) on storage error.
 *
 * @param[in] item_idx  Zero-based index of the tapped item (all guarded equally).
 * @param[in] ctx       Pointer to IConfigStorage.
 * @return true if navigation is allowed, false if denied.
 */
static bool gps_guard_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx; /* All GPS config items are guarded identically */
	IConfigStorage *storage = (IConfigStorage *)ctx;
	RelayConfig_t cfg;

	if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
	{
		return true; /* Fail open — allow access on storage error */
	}
	return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied hook for the GPS Configuration ListNav.
 *        Sets the return screen so that after the timeout the router
 *        goes back to GPS_CONFIGURATION.
 */
static void gps_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx;
	(void)ctx;
	PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_GPS_CONFIGURATION);
	PresentationLayer_SetAccessDeniedMessage(NULL); /* NULL = use default "CYCLE IN PROGRESS" */
}

/**
 * @brief Guard predicate for the Contact Configuration ListNav.
 *        All items are blocked when the relay is enabled (running).
 *        Fails open (returns true) on storage error.
 *
 * @param[in] item_idx  Zero-based index of the tapped item (all guarded equally).
 * @param[in] ctx       Pointer to IConfigStorage.
 * @return true if navigation is allowed, false if denied.
 */
static bool contact_guard_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx; /* All contact config items are guarded identically */
	IConfigStorage *storage = (IConfigStorage *)ctx;
	RelayConfig_t cfg;

	if (ConfigStorage_LoadRelayConfig(storage, &cfg) != ERR_OK)
	{
		return true; /* Fail open — allow access on storage error */
	}
	return (cfg.enabled == 0U);
}

/**
 * @brief on_guard_denied hook for the Contact Configuration ListNav.
 *        Sets the return screen so that after the timeout the router
 *        goes back to CONTACT_CONFIGURATION.
 */
static void contact_config_on_guard_denied_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx;
	(void)ctx;
	PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
	PresentationLayer_SetAccessDeniedMessage(NULL); /* NULL = use default "CYCLE IN PROGRESS" */
}

/**
 * @brief Guard predicate for Settings / Menu — blocked when the license is expired.
 *        Fails open (returns true) if the license service is unavailable.
 *
 * @param[in] item_idx  Zero-based index of the tapped item (all guarded equally).
 * @param[in] ctx       Pointer to ILicenseStatus.
 * @return true if navigation is allowed, false if license is expired.
 */
static bool license_guard_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx;
	if (ctx == NULL)
	{
		return true; /* Fail open — no license service wired */
	}
	ILicenseStatus *lic = (ILicenseStatus *)ctx;
	/* LicenseStatus_IsBlocked returns true only on RENTAL_EXPIRED */
	return !LicenseStatus_IsBlocked(lic);
}

/**
 * @brief on_guard_denied hook for Settings — shows license-expired message and
 *        returns to SCREEN_ID_MENU after the modal timeout.
 */
static void license_guard_denied_fn(uint8_t item_idx, void *ctx)
{
	(void)item_idx;
	(void)ctx;
	PresentationLayer_SetAccessDeniedReturnScreen((uint8_t)SCREEN_ID_MENU);
	PresentationLayer_SetAccessDeniedMessage(
		"-----------------------------------------------------\n"
		"LICENSE EXPIRED\n"
		"CONTACT DISTRIBUTOR TO RENEW\n"
		"-----------------------------------------------------");
}

/**
 * @brief pre_navigate hook for the Interrupt Configuration ListNav.
 *        Sets the ConfigurationInput parameter (Start or Stop time) before
 *        the router navigates to SCREEN_ID_CONFIGURATION_INPUT.
 *
 * @param[in] item_idx  1 = Start time, 2 = Stop time.
 * @param[in] ctx       Unused.
 */
/**
 * @brief pre_navigate_fn for system_information list — sets the pending info item
 *        index before EEZ routes to SCREEN_ID_INFORMATION_SHOW.
 *
 * @param[in] item_idx  Zero-based list item index (0=DeviceStatus, 1=IntConfig, ...).
 * @param[in] ctx       Unused.
 */
static void system_info_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	PresentationLayer_SetPendingInfoItem(item_idx);
	PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_SYSTEM_INFORMATION);
}

static void int_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	switch (item_idx)
	{
	case 1U:
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_START_TIME, 0U,
			(uint8_t)SCREEN_ID_INT_CONFIGURATION);
		break;
	case 2U:
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_STOP_TIME, 0U,
			(uint8_t)SCREEN_ID_INT_CONFIGURATION);
		break;
	default:
		break;
	}
}

/**
 * @brief pre_navigate hook for the GPS Configuration ListNav.
 *        Sets the ConfigurationInput parameter before the router navigates
 *        to SCREEN_ID_CONFIGURATION_INPUT.
 *
 * @param[in] item_idx  1 = Time offset.
 * @param[in] ctx       Unused.
 */
static void gps_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	switch (item_idx)
	{
	case 1U: /* 2. Time offset */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_GPS_TIME_OFFSET, 0U,
			(uint8_t)SCREEN_ID_GPS_CONFIGURATION);
		break;
	case 2U: /* 3. UTC setting */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_UTC_OFFSET_INDEX, 0U,
			(uint8_t)SCREEN_ID_GPS_CONFIGURATION);
		break;
	case 3U: /* 4. Auto-switch timeout */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_ANTENNA_SWITCH_TIMEOUT_MIN, 0U,
			(uint8_t)SCREEN_ID_GPS_CONFIGURATION);
		break;
	default:
		break;
	}
}

/**
 * @brief Pre-navigation hook for the Contact Configuration menu.
 *
 * Sets the config input context (param + back_screen) before navigating
 * to SCREEN_ID_CONFIGURATION_INPUT for items 1 (Comp. ON→OFF) and
 * 2 (Comp. OFF→ON).
 *
 * @param[in] item_idx  1 = ton_margin_ms, 2 = toff_margin_ms.
 * @param[in] ctx       Unused.
 */
static void contact_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	switch (item_idx)
	{
	case 1U: /* 2. Compensation ON -> OFF */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_TON_MARGIN_MS, 0U,
			(uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
		break;
	case 2U: /* 3. Compensation OFF -> ON */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_TOFF_MARGIN_MS, 0U,
			(uint8_t)SCREEN_ID_CONTACT_CONFIGURATION);
		break;
	default:
		break;
	}
}

/* ── Security passwords ── */

/** @brief Default password for normal configuration access: UP DOWN DOWN UP (4 keys). */
static const uint8_t s_default_password[] = {
	SEC_KEY_UP, SEC_KEY_DOWN, SEC_KEY_DOWN, SEC_KEY_UP};

/** @brief Super user password: DOWN UP DOWN DOWN UP DOWN (6 keys). */
static const uint8_t s_super_password[] = {
	SEC_KEY_DOWN, SEC_KEY_UP, SEC_KEY_DOWN,
	SEC_KEY_DOWN, SEC_KEY_UP, SEC_KEY_DOWN};

/** @brief Maintenance password for destructive operations: UP UP DOWN UP DOWN (5 keys). */
static const uint8_t s_maintenance_password[] = {
	SEC_KEY_UP, SEC_KEY_UP, SEC_KEY_DOWN,
	SEC_KEY_UP, SEC_KEY_DOWN};

/**
 * @brief Reset security context to default state (password + screens).
 *
 * Call this before configuring a specific security context to ensure
 * no stale state persists from previous failed/cancelled attempts.
 *
 * Default context:
 *   - Password: UP DOWN DOWN UP (4 keys)
 *   - Success screen: MENU
 *   - Cancel screen: HOME
 */
static inline void reset_security_context_to_default(void)
{
	PresentationLayer_SetSecurityContext(s_default_password,
										 (uint8_t)sizeof(s_default_password),
										 (uint8_t)SCREEN_ID_MENU,
										 (uint8_t)SCREEN_ID_HOME);
}

/* ── Destructive action helpers wired to SecurityScreenPresenter.on_success_action ── */

/**
 * @brief Clear the event log and show confirmation screen.
 *
 * Logs EVENT_TYPE_SYSTEM_EVENTS_CLEARED BEFORE clearing so the entry survives.
 * Then sets the confirmation message and navigates to SCREEN_ID_INFORMATION_SHOW (item 15).
 */
static void execute_clear_events(void *ctx)
{
	DependencyContainer_t *c = (DependencyContainer_t *)ctx;
	if (c == NULL)
	{
		return;
	}
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (EventNotifier_IsValid(notifier))
	{
		DateTime_t now = {0};
		ITimeSource *ts = DI_GetTimeSource(c);
		if (ts != NULL)
		{
			(void)TimeSource_GetTime(ts, &now);
		}
		(void)EventNotifier_NotifyEvent(notifier, &now,
										EVENT_TYPE_SYSTEM_EVENTS_CLEARED,
										EVENT_SEVERITY_WARNING, 0, 0);
	}
	IEventLogStorage *log = DI_GetEventLogStorage(c);
	(void)EventLogStorage_Clear(log);
	PresentationLayer_SetConfirmationMessage("Events cleared");
	PresentationLayer_SetPendingInfoItem(15U);
	PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

/**
 * @brief Restore factory configuration and show confirmation screen.
 */
static void execute_restore_config(void *ctx)
{
	DependencyContainer_t *c = (DependencyContainer_t *)ctx;
	if (c == NULL)
	{
		return;
	}
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (EventNotifier_IsValid(notifier))
	{
		DateTime_t now = {0};
		ITimeSource *ts = DI_GetTimeSource(c);
		if (ts != NULL)
		{
			(void)TimeSource_GetTime(ts, &now);
		}
		(void)EventNotifier_NotifyEvent(notifier, &now,
										EVENT_TYPE_SYSTEM_FACTORY_RESET,
										EVENT_SEVERITY_WARNING, 0, 0);
	}
	IConfigStorage *cfg = DI_GetConfigStorage(c);
	(void)ConfigStorage_ResetToDefaults(cfg);
	PresentationLayer_SetConfirmationMessage("Config restored");
	PresentationLayer_SetPendingInfoItem(15U);
	PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

/**
 * @brief Reset the hourmeter and show confirmation screen.
 */
static void execute_reset_hourmeter(void *ctx)
{
	DependencyContainer_t *c = (DependencyContainer_t *)ctx;
	if (c == NULL)
	{
		return;
	}
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (EventNotifier_IsValid(notifier))
	{
		DateTime_t now = {0};
		ITimeSource *ts = DI_GetTimeSource(c);
		if (ts != NULL)
		{
			(void)TimeSource_GetTime(ts, &now);
		}
		(void)EventNotifier_NotifyEvent(notifier, &now,
										EVENT_TYPE_SYSTEM_HOURMETER_RESET,
										EVENT_SEVERITY_INFO, 0, 0);
	}
	HourmeterAO_t *hm = DI_GetHourmeter(c);
	if (hm == NULL)
	{
		return;
	}
	(void)HourmeterAO_Reset(hm);
	PresentationLayer_SetConfirmationMessage("Hourmeter reset");
	PresentationLayer_SetPendingInfoItem(15U);
	PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
}

/**
 * @brief Pre-navigation hook for the General Configuration menu.
 *
 * Sets the config input context (param + back_screen) before navigating
 * to SCREEN_ID_CONFIGURATION_INPUT for items 0 (Screen ON time) and
 * 1 (Buzzer sound time).
 *
 * For items 3/4/5 (destructive actions), sets the security success action
 * callback + success/cancel screens before routing to SCREEN_ID_SEGURITY.
 *
 * @param[in] item_idx  0 = screen_blacklight_timeout_ms, 1 = buzzer_on_time_ms,
 *                      3 = delete events, 4 = restore config, 5 = reset hourmeter.
 * @param[in] ctx       Unused.
 */
static void general_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	switch (item_idx)
	{
	case 0U: /* 1. Screen ON time */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_SCREEN_TIMEOUT_S, 0U,
			(uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
		break;
	case 1U: /* 2. Buzzer sound time */
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_BUZZER_ON_TIME_MS, 0U,
			(uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
		break;
	case 3U: /* 4. Delete events → Security */
		/* Reset to default first to clear stale state */
		reset_security_context_to_default();
		/* Configure maintenance context */
		PresentationLayer_SetSecurityContext(s_maintenance_password,
											 (uint8_t)sizeof(s_maintenance_password),
											 (uint8_t)SCREEN_ID_INFORMATION_SHOW,
											 (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
		PresentationLayer_SetSecuritySuccessAction(execute_clear_events, ctx);
		break;
	case 4U: /* 5. Restore config → Security */
		/* Reset to default first to clear stale state */
		reset_security_context_to_default();
		/* Configure maintenance context */
		PresentationLayer_SetSecurityContext(s_maintenance_password,
											 (uint8_t)sizeof(s_maintenance_password),
											 (uint8_t)SCREEN_ID_INFORMATION_SHOW,
											 (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
		PresentationLayer_SetSecuritySuccessAction(execute_restore_config, ctx);
		break;
	case 5U: /* 6. Reset hourmeter → Security */
		/* Reset to default first to clear stale state */
		reset_security_context_to_default();
		/* Configure maintenance context */
		PresentationLayer_SetSecurityContext(s_maintenance_password,
											 (uint8_t)sizeof(s_maintenance_password),
											 (uint8_t)SCREEN_ID_INFORMATION_SHOW,
											 (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION);
		PresentationLayer_SetSecuritySuccessAction(execute_reset_hourmeter, ctx);
		break;
	default:
		break;
	}
}

/**
 * @brief pre_navigate_fn for wifi_module list — sets the pending info item
 *        index (8 + item_idx for WiFi providers) and sets the back screen to
 *        SCREEN_ID_WIFI_MODULE before routing to SCREEN_ID_INFORMATION_SHOW
 *        or SCREEN_ID_QR_INFO.
 *
 * WiFi provider mapping:
 *   item 0 → idx 8  — AP Information      → SCREEN_ID_INFORMATION_SHOW
 *   item 1 → idx 9  — STA Information     → SCREEN_ID_INFORMATION_SHOW
 *   item 2 → idx 10 — Connect to AP       → SCREEN_ID_INFORMATION_SHOW
 *   item 3 → idx 11 — Web Server (AP)     → SCREEN_ID_INFORMATION_SHOW
 *   item 4 → idx 12 — Web Server (STA)    → SCREEN_ID_INFORMATION_SHOW
 *   item 5 → idx 0  — Connect via AP (QR) → SCREEN_ID_QR_INFO
 *   item 6 → idx 1  — Web Server (QR)     → SCREEN_ID_QR_INFO
 *
 * @param[in] item_idx  Zero-based list item index (0..6).
 * @param[in] ctx       Unused.
 */
static void wifi_module_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	if (item_idx <= 4U)
	{
		PresentationLayer_SetPendingInfoItem((uint8_t)(8U + item_idx));
		PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_WIFI_MODULE);
	}
	else
	{
		PresentationLayer_SetPendingQrItem((uint8_t)(item_idx - 5U));
		PresentationLayer_SetQrBackScreen((uint8_t)SCREEN_ID_WIFI_MODULE);
	}
}

/**
 * @brief pre_navigate_fn for admin_configuration list.
 *
 * Item 1 (Select end date) → sets config input context before routing to
 * SCREEN_ID_CONFIGURATION_INPUT.
 */
static void admin_config_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	if (item_idx == 1U) /* 2. Select end date */
	{
		PresentationLayer_SetConfigInputContext(
			CONFIG_INPUT_PARAM_EXPIRATION_DATE, 0U,
			(uint8_t)SCREEN_ID_ADMIN_CONFIGURATION);
	}
}

/**
 * @brief Menu pre-navigate callback.
 *
 * Item 2 → Historical Events: sets pending info item to 13, back screen to MENU.
 */
static void menu_pre_navigate_fn(uint8_t item_idx, void *ctx)
{
	(void)ctx;
	if (item_idx == 2U) /* 3. Historical Events */
	{
		PresentationLayer_SetPendingInfoItem(13U);
		PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_MENU);
	}
	else if (item_idx == 3U) /* 4. Super User Menu */
	{
		/* Restore default context first to clear any stale state from previous Security usage */
		reset_security_context_to_default();
		/* Now configure super user context */
		PresentationLayer_SetSecurityContext(s_super_password,
											 (uint8_t)sizeof(s_super_password),
											 (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION,
											 (uint8_t)SCREEN_ID_MENU);
	}
}

/**
 * @copydoc DI_InitUISubsystem
 */
Result_t DI_InitUISubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}

	/* UI necesita el IDigitalInputSource del subsistema de entradas.
	 * DI_InitDigitalInputSubsystem() DEBE haberse llamado antes. */
	if (!container->digital_input_initialized)
	{
		return ERR_INVALID_STATE;
	}

	IDigitalInputSource *di_source = DI_GetDigitalInputSource(container);
	if (di_source == NULL)
	{
		return ERR_ERROR;
	}

	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	/* Check if display is available in this board variant */
	if (!profile->display.enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "Display/UI disabled in board profile (variant: %s)", profile->variant_name);
		}
		container->ui_initialized = false;
		return ERR_OK; /* Success, but UI not initialized */
	}

	UiAO_Config_t ui_cfg = {
		.di_source = di_source,
		.display_width = profile->display.width,   /* Single Source of Truth: BoardProfile */
		.display_height = profile->display.height, /* Single Source of Truth: BoardProfile */
	};

	Result_t res = UiAO_Init(&container->ui_ao, &ui_cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	/* ── 2. Display Backlight Service (Fase 4) ───────────────────────── */

	DisplayBacklightServiceConfig_t bl_cfg = {
		.display = container->ui_ao.display_if,
		.config_storage = DI_GetConfigStorage(container),
		.di_source = di_source,
		.poll_period_ms = 1000U,
	};
	(void)DisplayBacklightService_Init(&container->backlight_service, &bl_cfg);

	/* ── 3. Input Router — routes buttons to active screen ───────────── */

	InputRouterConfig_t ir_cfg = {
		.di_source = di_source,
	};
	res = InputRouter_Init(&container->input_router, &ir_cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	/* ── 4. EEZ Screen Router ─────────────────────────────────────────── */
	res = EezScreenRouter_Init(&container->eez_router, &container->input_router);
	if (res != ERR_OK)
	{
		return res;
	}

	IScreenRouter_t *router = EezScreenRouter_GetInterface(&container->eez_router);

	/* Wire the router into the PresentationLayer BEFORE Init so that
	 * PresentationLayer_Update() dispatches through the router from the start. */
	PresentationLayer_SetRouter(router);

	/* Route all presenter config I/O through StorageCoordinatorAO_v2:
	 *   - Reads  → RAM cache (O(1), no I2C, modular aggregation)
	 *   - Writes → async queue → debounce → EEPROM write modular (wear reduction)
	 */
	static StorageCoordinatorConfigAdapter_v2_t s_cfg_adapter_v2;
	StorageCoordinatorConfigAdapter_v2_Init(&s_cfg_adapter_v2,
											&container->storage_coordinator_v2);

	HomeScreenPresenterDeps_t deps = {0};
	deps.view = EezHomeScreenView_GetInterface();
	deps.time_source = DI_GetTimeSource(container);
	deps.gps_source = DI_GetGPSSource(container);
	deps.wifi_status = WifiStatusAdapter_GetInterface();
	deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	deps.update_period_ms = 300U; /* update 300 (ms) */
								  /* Optional deps (ACTION-030): push-based event sources. */
								  //	deps.config_change_notifier = NULL; /* ⚠️ Phase 4.6: v2 doesn't support observer pattern yet - TODO Phase 5 */
	deps.battery_monitor = DI_GetBatteryMonitor(container);
	deps.relay_controller = DI_GetRelayController(container);
	deps.event_flags = NULL; /* TODO: wire control-task event flags */

	/* ACTION-031+: inject router for ENTER→Security (input now via InputRouter/OnKeyEvent). */
	deps.router = router;
	deps.security_screen_id = (uint8_t)SCREEN_ID_SEGURITY;
	/* Alarm polling (Phase 3): overtemp DI + pending item/back-screen ptrs. */
	deps.di_source = DI_GetDigitalInputSource(container);
	deps.pending_info_item_ptr = (uint8_t *)PresentationLayer_GetPendingInfoItemPtr();
	deps.info_back_screen_ptr = (uint8_t *)PresentationLayer_GetInfoBackScreenPtr();
	deps.alarm_screen_id = (uint8_t)SCREEN_ID_INFORMATION_SHOW;
	/* GPS antenna auto-switch service (optional): show antenna failed icon when both antennas fail */
	deps.gps_auto_switch_service = &container->gps_antenna_auto_switch_service;
	/* Non-fatal: missing deps are caught inside HomeScreenPresenter_Init. */
	(void)PresentationLayer_Init(&deps);
	/* ── Security screen presenter (ACTION-031) ─────────────────────── */

	SecurityScreenPresenterDeps_t sec_deps = {0};
	sec_deps.view = EezSecurityScreenView_GetInterface();
	sec_deps.router = router; /* ACTION-031+: wired screen router     */
	sec_deps.password = s_default_password;
	sec_deps.password_len = sizeof(s_default_password);
	sec_deps.success_screen_id = (uint8_t)SCREEN_ID_MENU;
	sec_deps.cancel_screen_id = (uint8_t)SCREEN_ID_HOME;
	sec_deps.update_period_ms = 100U;
	(void)PresentationLayer_InitSecurity(&sec_deps);

	/* ── Menu screen presenter ─────────────────────────────────────────── */
	ListNavScreenPresenterDeps_t menu_deps = {0};
	menu_deps.view = EezMenuScreenView_GetInterface();
	menu_deps.router = router;
	menu_deps.item_count = 4U;
	/* TODO: replace with dedicated screen IDs once those screens exist */
	menu_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_SETTINGS;			  /* 1. Settings          */
	menu_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION; /* 2. System Information */
	menu_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW;	  /* 3. Historical Events  */
	menu_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_SEGURITY;			  /* 4. Super User Menu */
	menu_deps.back_screen_id = (uint8_t)SCREEN_ID_HOME;
	menu_deps.pre_navigate_fn = menu_pre_navigate_fn;
	menu_deps.pre_navigate_ctx = NULL;
	(void)PresentationLayer_InitMenu(&menu_deps);

	/* ── System Information screen presenter ───────────────────────────── */
	ListNavScreenPresenterDeps_t system_info_deps = {0};
	system_info_deps.view = EezSystemInformationScreenView_GetInterface();
	system_info_deps.router = router;
	system_info_deps.item_count = 8U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		system_info_deps.item_screen_ids[i] = 0U;
	}
	/* Destination screens pending implementation: keep as no-op (0U) for now. */
	system_info_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
	system_info_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 1. Device Status */
	system_info_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 2. Interrupt Configuration */
	system_info_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 3. GPS Status */
	system_info_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 4. Battery Status */
	system_info_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 5. Batch */
	system_info_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 6. FW & HW Version */
	system_info_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 7. License Status */
	system_info_deps.item_screen_ids[7] = (uint8_t)SCREEN_ID_WIFI_MODULE;	   /* 8. WiFi Module */
	system_info_deps.pre_navigate_fn = system_info_pre_navigate_fn;
	system_info_deps.pre_navigate_ctx = NULL;
	(void)PresentationLayer_InitSystemInformation(&system_info_deps);

	/* ── WiFi Module screen presenter ──────────────────────────────────── */
	ListNavScreenPresenterDeps_t wifi_module_deps = {0};
	wifi_module_deps.view = EezWifiModuleScreenView_GetInterface();
	wifi_module_deps.router = router;
	wifi_module_deps.item_count = 8U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		wifi_module_deps.item_screen_ids[i] = 0U;
	}
	wifi_module_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 1. AP Information  */
	wifi_module_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 2. STA Information */
	wifi_module_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 3. Connect to AP   */
	wifi_module_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 4. Web Server (AP) */
	wifi_module_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INFORMATION_SHOW; /* 5. Web Server (STA)*/
	wifi_module_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_QR_INFO;		   /* 6. Connect via AP (QR) */
	wifi_module_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_QR_INFO;		   /* 7. Web Server (QR)     */
	wifi_module_deps.item_screen_ids[7] = (uint8_t)SCREEN_ID_QR_INFO;		   /* 8. Web Server STA (QR)     */
	wifi_module_deps.back_screen_id = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
	wifi_module_deps.pre_navigate_fn = wifi_module_pre_navigate_fn;
	wifi_module_deps.pre_navigate_ctx = NULL;
	(void)PresentationLayer_InitWifiModule(&wifi_module_deps);

	/* ── Information Show screen presenter (unified: Dev Status, IntConfig, GPS, Battery) ─ */
	InformationShowScreenPresenterDeps_t info_show_deps = {0};
	info_show_deps.view = EezInformationShowScreenView_GetInterface();
	info_show_deps.router = router;
	info_show_deps.back_screen_id = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
	info_show_deps.pending_item_ptr = PresentationLayer_GetPendingInfoItemPtr();
	info_show_deps.back_screen_id_ptr = PresentationLayer_GetInfoBackScreenPtr();
	info_show_deps.time_source = DI_GetTimeSource(container);
	info_show_deps.relay = DI_GetRelayController(container);
	info_show_deps.hourmeter = DI_GetHourmeter(container);
	info_show_deps.config_storage = DI_GetConfigStorage(container);
	info_show_deps.gps_source = DI_GetGPSSource(container);
	info_show_deps.battery = DI_GetBatteryMonitor(container);
	info_show_deps.device_identity = DI_GetDeviceIdentity(container);
	info_show_deps.license_status = DI_GetLicenseStatus(container);
	info_show_deps.wifi_status = WifiStatusAdapter_GetInterface();
	info_show_deps.sta_port = CycloneNetworkPortAdapter_GetPortInterface(&container->sta_port_adapter);
	info_show_deps.ap_port = CycloneNetworkPortAdapter_GetPortInterface(&container->ap_port_adapter);
	info_show_deps.event_log_storage = DI_GetEventLogStorage(container);
	info_show_deps.di_source = DI_GetDigitalInputSource(container);
	info_show_deps.pending_confirmation_msg = PresentationLayer_GetConfirmationMessagePtr();
	(void)PresentationLayer_InitInformationShow(&info_show_deps);

	/* ── QR Info screen presenter ──────────────────────── */
	QrInfoScreenPresenterDeps_t qr_info_deps = {0};
	qr_info_deps.view = EezQrInfoScreenView_GetInterface();
	qr_info_deps.router = router;
	qr_info_deps.pending_item_ptr = PresentationLayer_GetPendingQrItemPtr();
	qr_info_deps.back_screen_id_ptr = PresentationLayer_GetQrBackScreenPtr();
	qr_info_deps.config_storage = DI_GetConfigStorage(container);
	qr_info_deps.device_identity = DI_GetDeviceIdentity(container);
	qr_info_deps.sta_port = CycloneNetworkPortAdapter_GetPortInterface(&container->sta_port_adapter);
	(void)PresentationLayer_InitQrInfo(&qr_info_deps);
	/* ── Settings screen presenter ──────────────────────────────────────── */
	ListNavScreenPresenterDeps_t settings_deps = {0};
	settings_deps.view = EezSettingsScreenView_GetInterface();
	settings_deps.router = router;
	settings_deps.item_count = 5U;
	settings_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INT_CONFIGURATION;	 /* interruption configuration */
	settings_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_GPS_CONFIGURATION;	 /* GPS configuration         */
	settings_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION; /* Contact configuration     */
	settings_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION; /* General configuration     */
	settings_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_WIFI_CONFIGURATION;	 /* WiFi configuration        */
	settings_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
	settings_deps.guard_fn = license_guard_fn;
	settings_deps.guard_ctx = DI_GetLicenseStatus(container);
	settings_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
	settings_deps.on_guard_denied_fn = license_guard_denied_fn;
	(void)PresentationLayer_InitSettings(&settings_deps);

	/* ── WiFi Configuration screen presenter ─────────────────────────── */
	WifiConfigurationScreenPresenterDeps_t wifi_config_deps = {0};
	wifi_config_deps.view = EezWifiConfigurationScreenView_GetInterface();
	wifi_config_deps.router = router;
	wifi_config_deps.wifi_module = DI_GetWifiModule(container);
	wifi_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
	(void)PresentationLayer_InitWifiConfiguration(&wifi_config_deps);

	/* ── Interrupt Configuration screen presenter ────────────────────── */
	ListNavScreenPresenterDeps_t int_config_deps = {0};
	int_config_deps.view = EezIntConfigurationScreenView_GetInterface();
	int_config_deps.router = router;
	int_config_deps.item_count = 7U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		int_config_deps.item_screen_ids[i] = 0U;
	}
	int_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_STATE;	  /* 1. Interrupt state */
	int_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		  /* 2. Start time      */
	int_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		  /* 3. Stop time       */
	int_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_DAYS;		  /* 4. day configuration */
	int_config_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD;	  /* 5. Period            */
	int_config_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_START;	  /* 6. Start configuration */
	int_config_deps.item_screen_ids[6] = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PREDEFINED; /* 7. Predefined configuration */
	int_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;

	/* Wire guard + hooks */
	int_config_deps.guard_fn = relay_guard_fn;
	int_config_deps.guard_ctx = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	;
	int_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
	int_config_deps.on_guard_denied_fn = int_config_on_guard_denied_fn;
	int_config_deps.on_guard_denied_ctx = NULL;
	int_config_deps.pre_navigate_fn = int_config_pre_navigate_fn;
	int_config_deps.pre_navigate_ctx = NULL;

	(void)PresentationLayer_InitIntConfiguration(&int_config_deps);

	/* ── Interrupt Configuration State screen presenter ──────────────────────────── */
	IntConfigurationStateScreenPresenterDeps_t int_config_state_deps = {0};
	int_config_state_deps.view = EezIntConfigurationStateScreenView_GetInterface();
	int_config_state_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2); /* ✅ Phase 4.6: v2 */
	int_config_state_deps.router = router;
	int_config_state_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	(void)PresentationLayer_InitIntConfigurationState(&int_config_state_deps);

	/* ── Interrupt Configuration Start screen presenter ──────────────────────────── */
	IntConfigurationStartScreenPresenterDeps_t int_config_start_deps = {0};
	int_config_start_deps.view = EezIntConfigurationStartScreenView_GetInterface();
	int_config_start_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	int_config_start_deps.router = router;
	int_config_start_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	(void)PresentationLayer_InitIntConfigurationStart(&int_config_start_deps);

	/* ── Interrupt Configuration Predefined Cycles screen presenter ──────────────────────── */
	IntConfigurationPredefinedScreenPresenterDeps_t int_config_predefined_deps = {0};
	int_config_predefined_deps.view = EezIntConfigurationPredefinedScreenView_GetInterface();
	int_config_predefined_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	int_config_predefined_deps.router = router;
	int_config_predefined_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	(void)PresentationLayer_InitIntConfigurationPredefined(&int_config_predefined_deps);

	/* ── Interrupt Configuration Days screen presenter ────────────────────────────────────────── */
	IntConfigurationDaysScreenPresenterDeps_t int_config_days_deps = {0};
	int_config_days_deps.view = EezIntConfigurationDaysScreenView_GetInterface();
	int_config_days_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	int_config_days_deps.router = router;
	int_config_days_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	(void)PresentationLayer_InitIntConfigurationDays(&int_config_days_deps);

	/* -- Interrupt Configuration Period (roller) screen presenter -- */
	IntConfigurationPeriodScreenPresenterDeps_t int_config_period_deps = {0};
	int_config_period_deps.view = EezIntConfigurationPeriodScreenView_GetInterface();
	int_config_period_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	int_config_period_deps.router = router;
	int_config_period_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	int_config_period_deps.period_menu_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU;
	int_config_period_deps.set_period_menu_mode_fn = PresentationLayer_GetPeriodMenuSetModeFn();
	int_config_period_deps.set_period_menu_mode_ctx = PresentationLayer_GetPeriodMenuSetModeCtx();

	(void)PresentationLayer_InitIntConfigurationPeriod(&int_config_period_deps);
	/* -- Interrupt Configuration Period Menu (dynamic list) screen presenter -- */
	IntConfigurationPeriodMenuScreenPresenterDeps_t int_config_period_menu_deps = {0};
	int_config_period_menu_deps.view = EezIntConfigurationPeriodMenuScreenView_GetInterface();
	int_config_period_menu_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	int_config_period_menu_deps.router = router;
	int_config_period_menu_deps.period_roller_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD;
	int_config_period_menu_deps.config_input_screen_id = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
	int_config_period_menu_deps.set_edit_context_fn = PresentationLayer_GetPeriodEditContextFn();
	int_config_period_menu_deps.set_edit_context_ctx = PresentationLayer_GetPeriodEditContextCtx();
	(void)PresentationLayer_InitIntConfigurationPeriodMenu(&int_config_period_menu_deps);

	/* -- Access Denied modal screen presenter -- */
	AccessDeniedScreenPresenterDeps_t access_denied_deps = {0};
	access_denied_deps.view = EezAccessDeniedScreenView_GetInterface();
	access_denied_deps.router = router;
	access_denied_deps.buzzer = DI_GetBuzzerNotificationService(container);
	access_denied_deps.return_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION; /* default — overridden via setter */
	(void)PresentationLayer_InitAccessDenied(&access_denied_deps);

	/* ── Configuration Input (HH:MM:SS) screen presenter ──────────────── */
	ConfigurationInputScreenPresenterDeps_t config_input_deps = {0};
	config_input_deps.view = EezConfigurationInputScreenView_GetInterface();
	config_input_deps.router = router;
	config_input_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	config_input_deps.back_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION;
	(void)PresentationLayer_InitConfigurationInput(&config_input_deps);

	/* ── GPS Configuration screen presenter ───────────────────────────────────────────────────── */
	ListNavScreenPresenterDeps_t gps_config_deps = {0};
	gps_config_deps.view = EezGpsConfigurationScreenView_GetInterface();
	gps_config_deps.router = router;
	gps_config_deps.item_count = 4U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		gps_config_deps.item_screen_ids[i] = 0U;
	}
	gps_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_GPS_CONFIGURATION_ANTENNA; /* 1. Select antenna */
	gps_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;	   /* 2. Time offset   */
	gps_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;	   /* 3. UTC setting   */
	gps_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;	   /* 4. Auto-switch timeout */
	gps_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
	gps_config_deps.pre_navigate_fn = gps_config_pre_navigate_fn;
	gps_config_deps.guard_fn = gps_guard_fn;
	gps_config_deps.guard_ctx = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	gps_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
	gps_config_deps.on_guard_denied_fn = gps_config_on_guard_denied_fn;
	gps_config_deps.on_guard_denied_ctx = NULL;
	(void)PresentationLayer_InitGpsConfiguration(&gps_config_deps);

	/* ── GPS Configuration Antenna screen presenter ────────────────────── */
	GpsConfigurationAntennaScreenPresenterDeps_t gps_antenna_deps = {0};
	gps_antenna_deps.view = EezGpsConfigurationAntennaScreenView_GetInterface();
	gps_antenna_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	gps_antenna_deps.router = router;
	gps_antenna_deps.back_screen_id = (uint8_t)SCREEN_ID_GPS_CONFIGURATION;
	(void)PresentationLayer_InitGpsConfigurationAntenna(&gps_antenna_deps);

	/* ── Contact Configuration screen presenter ──────────────────────── */
	ListNavScreenPresenterDeps_t contact_config_deps = {0};
	contact_config_deps.view = EezContactConfigurationScreenView_GetInterface();
	contact_config_deps.router = router;
	contact_config_deps.item_count = 3U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		contact_config_deps.item_screen_ids[i] = 0U;
	}
	contact_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
	contact_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION_TYPE; /* 1. Contact type      */
	contact_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		/* 2. Comp. ON -> OFF   */
	contact_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		/* 3. Comp. OFF -> ON   */
	contact_config_deps.pre_navigate_fn = contact_config_pre_navigate_fn;
	contact_config_deps.guard_fn = contact_guard_fn;
	contact_config_deps.guard_ctx = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	contact_config_deps.guard_denied_screen_id = (uint8_t)SCREEN_ID_ACCESSDENIED;
	contact_config_deps.on_guard_denied_fn = contact_config_on_guard_denied_fn;
	contact_config_deps.on_guard_denied_ctx = NULL;
	(void)PresentationLayer_InitContactConfiguration(&contact_config_deps);

	/* ── Contact Configuration Type screen presenter ────────────────── */
	ContactConfigurationTypeScreenPresenterDeps_t contact_type_deps = {0};
	contact_type_deps.view = EezContactConfigurationTypeScreenView_GetInterface();
	contact_type_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	contact_type_deps.router = router;
	contact_type_deps.back_screen_id = (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION;
	(void)PresentationLayer_InitContactConfigurationType(&contact_type_deps);

	/* ── General Configuration screen presenter ──────────────────────── */
	ListNavScreenPresenterDeps_t general_config_deps = {0};
	general_config_deps.view = EezGeneralConfigurationScreenView_GetInterface();
	general_config_deps.router = router;
	general_config_deps.item_count = 6U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		general_config_deps.item_screen_ids[i] = 0U;
	}
	general_config_deps.back_screen_id = (uint8_t)SCREEN_ID_SETTINGS;
	general_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		 /* 1. Screen ON time     */
	general_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;		 /* 2. Buzzer sound time  */
	general_config_deps.item_screen_ids[2] = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION_ALARM; /* 3. Alarm configuration */
	general_config_deps.item_screen_ids[3] = (uint8_t)SCREEN_ID_SEGURITY;					 /* 4. Delete events      */
	general_config_deps.item_screen_ids[4] = (uint8_t)SCREEN_ID_SEGURITY;					 /* 5. Restore config     */
	general_config_deps.item_screen_ids[5] = (uint8_t)SCREEN_ID_SEGURITY;					 /* 6. Reset hourmeter    */
	general_config_deps.pre_navigate_fn = general_config_pre_navigate_fn;
	general_config_deps.pre_navigate_ctx = container;
	(void)PresentationLayer_InitGeneralConfiguration(&general_config_deps);

	/* ── General Configuration Alarm screen presenter ───────────────────── */
	GeneralConfigurationAlarmScreenPresenterDeps_t general_config_alarm_deps = {0};
	general_config_alarm_deps.view = EezGeneralConfigurationAlarmScreenView_GetInterface();
	general_config_alarm_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	general_config_alarm_deps.router = router;
	general_config_alarm_deps.back_screen_id = (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION;
	(void)PresentationLayer_InitGeneralConfigurationAlarm(&general_config_alarm_deps);

	/* ── Admin Configuration screen presenter ───────────────────────────── */
	ListNavScreenPresenterDeps_t admin_config_deps = {0};
	admin_config_deps.view = EezAdminConfigurationScreenView_GetInterface();
	admin_config_deps.router = router;
	admin_config_deps.item_count = 2U;
	for (uint8_t i = 0U; i < LIST_NAV_MAX_ITEMS; i++)
	{
		admin_config_deps.item_screen_ids[i] = 0U;
	}
	admin_config_deps.item_screen_ids[0] = (uint8_t)SCREEN_ID_ADMIN_OPERATION_MODE;
	admin_config_deps.item_screen_ids[1] = (uint8_t)SCREEN_ID_CONFIGURATION_INPUT;
	admin_config_deps.back_screen_id = (uint8_t)SCREEN_ID_MENU;
	admin_config_deps.pre_navigate_fn = admin_config_pre_navigate_fn;
	admin_config_deps.pre_navigate_ctx = NULL;
	(void)PresentationLayer_InitAdminConfiguration(&admin_config_deps);

	/* ── Admin Operation Mode screen presenter ───────────────────────────── */
	AdminOperationModeScreenPresenterDeps_t admin_op_mode_deps = {0};
	admin_op_mode_deps.view = EezAdminOperationModeScreenView_GetInterface();
	admin_op_mode_deps.config_storage = StorageCoordinatorConfigAdapter_v2_GetInterface(&s_cfg_adapter_v2);
	admin_op_mode_deps.router = router;
	admin_op_mode_deps.back_screen_id = (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION;
	(void)PresentationLayer_InitAdminOperationMode(&admin_op_mode_deps);

	/* ── 5. Register screens in the EEZ router after presenters are Init'd */
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_HOME,
							  PresentationLayer_GetHomeScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_SEGURITY,
							  PresentationLayer_GetSecurityScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_MENU,
							  PresentationLayer_GetMenuScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_SETTINGS,
							  PresentationLayer_GetSettingsScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_SYSTEM_INFORMATION,
							  PresentationLayer_GetSystemInformationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_WIFI_MODULE,
							  PresentationLayer_GetWifiModuleScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_WIFI_CONFIGURATION,
							  PresentationLayer_GetWifiConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INFORMATION_SHOW,
							  PresentationLayer_GetInformationShowScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_QR_INFO,
							  PresentationLayer_GetQrInfoScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION,
							  PresentationLayer_GetIntConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_STATE,
							  PresentationLayer_GetIntConfigurationStateScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_START,
							  PresentationLayer_GetIntConfigurationStartScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_PREDEFINED,
							  PresentationLayer_GetIntConfigurationPredefinedScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_DAYS,
							  PresentationLayer_GetIntConfigurationDaysScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD,
							  PresentationLayer_GetIntConfigurationPeriodScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU,
							  PresentationLayer_GetIntConfigurationPeriodMenuScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_GPS_CONFIGURATION,
							  PresentationLayer_GetGpsConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_GPS_CONFIGURATION_ANTENNA,
							  PresentationLayer_GetGpsConfigurationAntennaScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION_TYPE,
							  PresentationLayer_GetContactConfigurationTypeScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_CONTACT_CONFIGURATION,
							  PresentationLayer_GetContactConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION,
							  PresentationLayer_GetGeneralConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_GENERAL_CONFIGURATION_ALARM,
							  PresentationLayer_GetGeneralConfigurationAlarmScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_ADMIN_CONFIGURATION,
							  PresentationLayer_GetAdminConfigurationScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_ADMIN_OPERATION_MODE,
							  PresentationLayer_GetAdminOperationModeScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_ACCESSDENIED,
							  PresentationLayer_GetAccessDeniedScreen());
	EezScreenRouter_SetScreen(&container->eez_router,
							  (uint8_t)SCREEN_ID_CONFIGURATION_INPUT,
							  PresentationLayer_GetConfigurationInputScreen());

	/* Establish the initial active screen so GetActiveScreen() returns non-NULL
	 * from the very first PresentationLayer_Update() tick. No EEZ loadScreen()
	 * is called — EEZ already shows SCREEN_ID_HOME on boot. */
	/* STARTUP has no presenter — active_screen = NULL during boot.
	 * InputRouter routes no keys until HOME is reached via PresentationLayer_OnBootComplete().
	 * This prevents premature navigation to Security if the encoder is pressed
	 * during the startup / overview animation sequence. */
	EezScreenRouter_SetInitialScreen(&container->eez_router, (uint8_t)SCREEN_ID_STARTUP);

	container->ui_initialized = true;
	return ERR_OK;
}

/**
 * @copydoc DI_DeinitUISubsystem
 */
Result_t DI_DeinitUISubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}
	container->ui_initialized = false;
	return ERR_OK;
}

Result_t DI_InitBuzzerSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* ✅ Obtener Board Profile para configuración de hardware */
	const BoardProfile_t *profile = BSP_GetBoardProfile();

	/* ✅ Verificar si buzzer está habilitado en este variant */
	if (!profile->buzzer.enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "Buzzer disabled in board profile (variant: %s)", profile->variant_name);
		}
		container->buzzer_initialized = false;
		return ERR_OK; /* Success, but buzzer not initialized */
	}

	/* 1. Inicializar BuzzerAdapter (capa de hardware) */
	BuzzerAdapterConfig_t adapter_config = {
		.gpio_iface = container->bsp->gpio,
		.gpio_pin = profile->buzzer.pin,   /* ✅ Desde Board Profile */
		.gpio_port = profile->buzzer.port, /* ✅ Desde Board Profile */
		.active_high = true,
		.default_enabled = true};

	res = BuzzerAdapter_Init(&container->buzzer_adapter, &adapter_config);
	if (res != ERR_OK)
	{
		return res;
	}

	/* 2. Inicializar BuzzerAO (Active Object para gestión asíncrona) */
	BuzzerAO_Config_t ao_config = {
		.buzzer_control = BuzzerAdapter_GetInterface(&container->buzzer_adapter),
		.thread_priority = TASK_PRIO_BUZZER,
		.queue_size = DI_BUZZER_AO_QUEUE_SIZE,
		.stack_size = BUZZER_AO_THREAD_STACK_SIZE, /* BuzzerAO uses its own internal stack; this field is unused */
		.logger = DI_GetLogger(container),
	};

	res = BuzzerAO_Init(&container->buzzer_ao, &ao_config);
	if (res != ERR_OK)
	{
		BuzzerAdapter_Deinit(&container->buzzer_adapter);
		return res;
	}

	/* 3. Inicializar BuzzerNotificationService (lógica de dominio) */
	BuzzerNotificationConfig_t notif_config = {
		.beep_enabled = true,
		.button_beep_duration_ms = 50,
		.alarm_beep_duration_ms = 1000,
		.silent_mode = false,
		.boot_beep_enabled = true};

	/* ✅ CORRECTO: Inyectar BuzzerAO (IAudibleNotifier) en lugar de BuzzerAdapter */
	res = BuzzerNotificationService_Init(
		&container->buzzer_notification,
		BuzzerAO_GetAudibleNotifierInterface(&container->buzzer_ao),
		&notif_config);

	if (res != ERR_OK)
	{
		BuzzerAO_Deinit(&container->buzzer_ao);
		BuzzerAdapter_Deinit(&container->buzzer_adapter);
		return res;
	}

	container->buzzer_initialized = true;

	return ERR_OK;
}

/*============================================================================*
 * LED STATUS SUBSYSTEM (Polling Service — called from System_Update)
 *============================================================================*/

/**
 * @brief Initialize the LED Status Service — drives led_status[0].
 *
 * @details Reads led_status[0] from BoardProfile; skips init with ERR_OK if
 *          the pin is disabled in the current variant.  Injects relay, GPS, and
 *          license interfaces where available (all are nullable).
 *
 * @pre  DI_InitRelaySubsystem()  — relay interface available.
 * @pre  DI_InitGPSSubsystem()    — GPS source available.
 * @pre  DI_InitLicenseSubsystem()— license status available.
 */
Result_t DI_InitLedStatusSubsystem(DependencyContainer_t *container)
{
	if (container == NULL || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	const BoardProfile_t *profile = BSP_GetBoardProfile();

	if (!profile->led_status[0].enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "LED status[0] disabled in board profile (%s)", profile->variant_name);
		}
		container->led_status_initialized = false;
		return ERR_OK; /* Non-fatal: variant has no status LED */
	}

	LedStatusServiceConfig_t cfg = {
		.gpio_iface = container->bsp->gpio,
		.led_port = profile->led_status[0].port,
		.led_pin = profile->led_status[0].pin,
		.led_enabled = true,
		.relay = container->relay_initialized ? DI_GetRelayController(container) : NULL,
		.gps = container->gps_initialized ? DI_GetGPSSource(container) : NULL,
		.license = container->license_initialized ? DI_GetLicenseStatus(container) : NULL,
	};

	Result_t res = LedStatusService_Init(&container->led_status_service, &cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	container->led_status_initialized = true;

	return ERR_OK;
}

/*============================================================================*
 * HOURMETER SUBSYSTEM (AO + EEPROM Wear-Leveled Persistence)
 *============================================================================*/

/**
 * @brief Condition callback: GPS has 2D or 3D fix.
 * @param[in] ctx  IGPSSource* cast to void*.
 */
static bool di_hourmeter_gps_has_fix(void *ctx)
{
	IGPSSource *gps = (IGPSSource *)ctx;
	if (!gps)
	{
		return false;
	}
	GPSFixStatus_t status;
	if (GPS_Source_GetFixStatus(gps, &status) != ERR_OK)
	{
		return false;
	}
	return (status >= GPS_FIX_2D);
}

/**
 * @brief Condition callback: relay FSM is in ACTIVE_CYCLING state.
 * @note  Mapped via GetStatus: RELAY_FSM_ACTIVE_CYCLING → RELAY_STATE_ACTIVE.
 * @param[in] ctx  IRelayController* cast to void*.
 */
static bool di_hourmeter_relay_active(void *ctx)
{
	IRelayController *relay = (IRelayController *)ctx;
	if (!relay)
	{
		return false;
	}
	RelayStatus_t status;
	if (RelayController_GetStatus(relay, &status) != ERR_OK)
	{
		return false;
	}
	return (status.internal_state == RELAY_STATE_ACTIVE);
}

/**
 * @brief Condition callback: currently inside the configured time window.
 * @param[in] ctx  IRelayController* cast to void*.
 */
static bool di_hourmeter_in_window(void *ctx)
{
	IRelayController *relay = (IRelayController *)ctx;
	if (!relay)
	{
		return false;
	}
	RelayStatus_t status;
	if (RelayController_GetStatus(relay, &status) != ERR_OK)
	{
		return false;
	}
	return (status.is_in_window != 0);
}

Result_t DI_InitHourmeterSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	if (!container->storage_initialized || !container->gps_initialized ||
		!container->relay_initialized)
	{
		return ERR_INVALID_STATE;
	}

	IGPSSource *gps_src = DI_GetGPSSource(container);
	IRelayController *relay_ctrl = DI_GetRelayController(container);

	HourmeterAOConfig_t cfg = {
		.eeprom = container->eeprom_driver,
		.eeprom_handle = NULL,
		.gps_has_fix = di_hourmeter_gps_has_fix,
		.gps_context = gps_src,
		.relay_active = di_hourmeter_relay_active,
		.relay_context = relay_ctrl,
		.in_window = di_hourmeter_in_window,
		.window_context = relay_ctrl,
	};

	Result_t res = HourmeterAO_Init(&container->hourmeter_ao, &cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	container->hourmeter_initialized = true;
	return ERR_OK;
}

/*============================================================================*
 * LOGGING SUBSYSTEM (UART/SWO + EEPROM Events)
 *============================================================================*/

/**
 * @brief Inicializa subsistema de Logging (ILogger + IEventNotifier)
 *
 * @details
 * Inicializa dos componentes:
 * 1. ElogAdapter (lwprintf) → ILogger (logs volátiles a UART/SWO)
 * 2. StorageCoordinatorAdapter → IEventNotifier (eventos persistentes a EEPROM)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return ERR_OK si exitoso
 * @return ERR_INVALID_STATE si Storage no está inicializado
 * @return ERR_ERROR si falla inicialización de adaptadores
 *
 * @note Requiere que DI_InitStorageSubsystem() haya sido llamado primero
 * @note Thread-safe: lwprintf usa TX_MUTEX automático (LWPRINTF_CFG_OS=1)
 */
/**
 * @brief Inicializa solo el Logger (ILogger → UART/ITM).
 * @note  YA NO requiere storage_initialized.
 * @note  IEventNotifier se inicializa por separado en DI_InitEventNotifierSubsystem().
 */
Result_t DI_InitLoggingSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* ✅ Obtener Board Profile para configuración de hardware */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}
#if 0
	/* Obtener UART handle para debug logging */
	void *debug_uart = BSP_GetUARTHandle(profile->peripheral_map.debug_uart_index);
	if (!debug_uart)
	{
		return ERR_INVALID_PARAM; /* Índice UART inválido en profile */
	}
#endif
	/* Inicializar ElogAdapter (ILogger → lwprintf → UART/ITM) */
	ElogAdapterConfig_t logger_config = {
		.min_level = LOG_LEVEL_INFO,		/* DEBUG habilitado para desarrollo */
		.enable_colors = false,				/* ANSI colors deshabilitados para ITM */
		.show_file_line = false,			/* Mostrar file:line en logs */
		.get_tick_fn = os_ticks_get,		/* Timestamp usando OSAL */
		.output_fn = BSP_Logging_OutputITM, /* Callback ITM */
#if 0
			.output_fn = BSP_Logging_OutputUART, /* Callback UART */
			.output_arg = debug_uart            /* ✅ UART handle dinámico desde profile */
#endif

	};

	res = ElogAdapter_Init(&container->logger_adapter, &logger_config);
	if (res != ERR_OK)
	{
		return res;
	}

	container->logging_initialized = true;

	/* Log de inicialización exitosa */
	ILogger *logger = DI_GetLogger(container);
	if (Logger_IsValid(logger))
	{
		/*SHOW System info*/
		const char *display_driver = "None";
		if (profile->display.enabled)
		{
			switch (profile->display.driver)
			{
			case DISPLAY_DRIVER_ST7789:
				display_driver = "ST7789";
				break;
			case DISPLAY_DRIVER_SSD1306:
				display_driver = "SSD1306";
				break;
			case DISPLAY_DRIVER_ILI9341:
				display_driver = "ILI9341";
				break;
			default:
				display_driver = "Other";
				break;
			}
		}
		LOG_WARN(logger, "\r\nDI", "*********************************************************");
		LOG_WARN(logger, "DI", "* TCS-Current Interrupter                               *");
		LOG_WARN(logger, "DI", "* TARGET: %s", profile->variant_name);
		LOG_WARN(logger, "DI", "* HW Version: %s", HW_VERSION_STRING);
		LOG_WARN(logger, "DI", "* FW Version: %s", FW_VERSION_STRING);
		LOG_WARN(logger, "DI", "* Build: %s - %s", __DATE__, __TIME__);
		LOG_WARN(logger, "DI", "* Compiler: GCC %s", __VERSION__);
		LOG_WARN(logger, "DI", "* MCU: STM32U575VGTx @ 160 MHz");
		if (profile->display.enabled)
		{
			LOG_WARN(logger, "DI", "* Display: %dx%d %s",
					 profile->display.width, profile->display.height, display_driver);
		}
		LOG_WARN(logger, "DI", "* WiFi: %s", profile->esp32_wifi.enabled ? "ESP32-Hosted" : "Disabled");
		LOG_WARN(logger, "DI", "* RS485: %s", profile->rs485.enabled ? "Enabled" : "Disabled");
		LOG_WARN(logger, "DI", "* copyright TECNA PERU S.A.C.");
		LOG_WARN(logger, "DI", "* by Quino B. Jeffry");
		LOG_WARN(logger, "DI", "*********************************************************\r\n");

		LOG_INFO(logger, "DI", "Logger initialized (UART/ITM output)");
	}

	return ERR_OK;
}

/**
 * @brief Inicializa el Event Notifier (IEventNotifier → EEPROM).
 * @pre   DI_InitStorageSubsystem() debe haber sido llamado primero.
 */
Result_t DI_InitEventNotifierSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Verificar que Storage está inicializado (requerido para EventNotifier) */
	if (!container->storage_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Verificar que Logging está inicializado (para logs del EventNotifier) */
	if (!container->logging_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* Inicializar StorageCoordinatorAdapter_v2 (IEventNotifier → EEPROM direct) */
	/* ✅ Phase 4.5: Migrated to v2 - bypasses coordinator, writes direct to event_log */
	IEventLogStorage *event_log_iface = EventLogAO_GetInterface(&container->event_log_ao);
	res = StorageCoordinatorAdapter_v2_Init(
		&container->event_adapter_v2,
		event_log_iface);

	if (res != ERR_OK)
	{
		return res;
	}

	/* Log de inicialización exitosa */
	ILogger *logger = DI_GetLogger(container);
	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "Event Notifier initialized (EEPROM events)");
	}

	return ERR_OK;
}

/**
 * @brief De-inicializa solo el Logger (ILogger).
 */
Result_t DI_DeinitLoggingSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->logging_initialized)
	{
		return ERR_OK; /* Nada que de-inicializar */
	}

	ElogAdapter_Deinit(&container->logger_adapter);

	container->logging_initialized = false;

	return ERR_OK;
}

/**
 * @brief De-inicializa el Event Notifier (IEventNotifier).
 */
Result_t DI_DeinitEventNotifierSubsystem(DependencyContainer_t *container)
{
	if (!container)
	{
		return ERR_OK; /* Nada que de-inicializar */
	}

	StorageCoordinatorAdapter_v2_Deinit(&container->event_adapter_v2);

	return ERR_OK;
}

/*============================================================================*
 * WIFI SUBSYSTEM (ESP-Hosted SPI transport + WifiControlAdapter)
 *============================================================================*/

/**
 * @brief Parse dotted-decimal IPv4 string into NetworkIpv4Addr_t octets.
 * @note  Used only in DI wiring. Non-hot-path.
 */
static void di_parse_ipv4(NetworkIpv4Addr_t *out, const char *str)
{
	if (out == NULL)
	{
		return;
	}
	if (str == NULL || str[0] == '\0')
	{
		return;
	}
	uint8_t octet_idx = 0U;
	uint32_t val = 0U;
	for (const char *p = str; *p != '\0' && octet_idx < 4U; p++)
	{
		if (*p >= '0' && *p <= '9')
		{
			val = val * 10U + (uint32_t)(*p - '0');
		}
		else if (*p == '.')
		{
			out->octet[octet_idx++] = (uint8_t)val;
			val = 0U;
		}
	}
	if (octet_idx < 4U)
	{
		out->octet[octet_idx] = (uint8_t)val; /* last octet */
	}
}

bool sta_disconneted_event_save = false;
/**
 * @brief WiFi STA connected → notify NetworkAO immediately (event-driven).
 *
 * @note  Executed in WifiControlAdapter task context; must be ISR-safe
 *        (posts to queue with OS_NO_WAIT — never blocks).
 */
static void DI_OnStaConnected_NetworkAO(void *ctx)
{
	DependencyContainer_t *c = (DependencyContainer_t *)ctx;
	WifiStatusAdapter_SetConnected(true);
	(void)NetworkAO_NotifyLinkUp(&c->network_ao);
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (EventNotifier_IsValid(notifier))
	{
		DateTime_t now = {0};
		ITimeSource *ts = DI_GetTimeSource(c);
		if (ts != NULL)
		{
			(void)TimeSource_GetTime(ts, &now);
		}
		(void)EventNotifier_NotifyEvent(notifier, &now,
										EVENT_TYPE_WIFI_CONNECTED,
										EVENT_SEVERITY_INFO, 0U, 0U);
	}
	// reset flag
	sta_disconneted_event_save = false;
}

/**
 * @brief WiFi STA disconnected → notify NetworkAO immediately (event-driven).
 *
 * @note  Executed in WifiControlAdapter task context; must be ISR-safe
 *        (posts to queue with OS_NO_WAIT — never blocks).
 */
static void DI_OnStaDisconnected_NetworkAO(void *ctx)
{
	DependencyContainer_t *c = (DependencyContainer_t *)ctx;
	WifiStatusAdapter_SetConnected(false);
	(void)NetworkAO_NotifyLinkDown(&c->network_ao);
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (EventNotifier_IsValid(notifier) && (sta_disconneted_event_save == false))
	{
		DateTime_t now = {0};
		ITimeSource *ts = DI_GetTimeSource(c);
		if (ts != NULL)
		{
			(void)TimeSource_GetTime(ts, &now);
		}
		(void)EventNotifier_NotifyEvent(notifier, &now,
										EVENT_TYPE_WIFI_DISCONNECTED,
										EVENT_SEVERITY_WARNING, 0U, 0U);

		// set flag
		sta_disconneted_event_save = true;
	}
}

/**
 * @brief ESP-Hosted transport ready → notify NetworkAO (event-driven).
 *
 * Fired by WifiControlAdapter when CTRL_EVENT_ESP_INIT arrives from ESP32.
 * Sequence:
 *   hosted_Init() (in NetworkAO thread)
 *   → SPI events → CTRL_EVENT_ESP_INIT
 *   → WifiControlAdapter.internal_esp_init_cb
 *   → this callback (via IWifiControl_OnTransportReady)
 *   → NetworkAO_NotifyTransportReady() → NET_AO_MSG_TRANSPORT_READY
 *   → handle_msg_transport_ready: netInit() + port configure + WAITING_LINK
 *
 * @note ISR-safe: posts to queue with OS_NO_WAIT — never blocks.
 */
static void DI_OnTransportReady_NetworkAO(void *ctx)
{
	(void)NetworkAO_NotifyTransportReady((NetworkAO_t *)ctx);
}

Result_t DI_InitWifiSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Verificar que Logging está inicializado (requerido para ESP-Hosted logging) */
	if (!container->logging_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* ✅ Obtener Board Profile para configuración de pines WiFi */
	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	/* Si WiFi está deshabilitado en el board profile, skip initialization */
	if (!profile->esp32_wifi.enabled)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", " ESP-Hosted disabled in board profile (variant: %s)", profile->variant_name);
		}
		container->wifi_initialized = false;
		return ERR_OK; /* Success, pero sin inicializar */
	}

	Result_t res;
	ILogger *logger = DI_GetLogger(container);

	/* 2. Inicializar SPI Transport Adapter (usando pines del board profile) */
	ESP_SpiTransportConfig_t transport_config = {
		.spi_interface = container->bsp->spi,
		.gpio_interface = container->bsp->gpio,
		.exti_interface = container->bsp->exti,
		.spi_handle = BSP_GetSPIHandle(profile->peripheral_map.wifi_spi_index),
		.esp_reset_port = profile->esp32_wifi.reset_port,
		.esp_reset_pin = profile->esp32_wifi.reset_pin,
		.esp_handshake_port = profile->esp32_wifi.handshake_port,
		.esp_handshake_pin = profile->esp32_wifi.handshake_pin,
		.esp_data_ready_port = profile->esp32_wifi.data_ready_port, /* Same as handshake */
		.esp_data_ready_pin = profile->esp32_wifi.data_ready_pin,
		.esp_cs_port = profile->esp32_wifi.cs_port,
		.esp_cs_pin = profile->esp32_wifi.cs_pin,
		.cs_mode = (profile->esp32_wifi.cs_mode == ESP_CS_MODE_AUTO) ? ESP_SPI_CS_MODE_AUTO : ESP_SPI_CS_MODE_MANUAL};

	res = ESP_SpiTransportAdapter_Init(&container->esp_transport_adapter, &transport_config);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "ESP-Hosted: Transport adapter init failed (%d)", res);
		}
		return res;
	}

	/* 3. ✅ Obtener interfaz abstracta IWifiTransport (DIP compliance) */
	container->wifi_transport = ESP_SpiTransportAdapter_GetInterface(&container->esp_transport_adapter);
	if (!container->wifi_transport)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "ESP-Hosted: Failed to get IWifiTransport interface");
		}
		ESP_SpiTransportAdapter_Deinit(&container->esp_transport_adapter);
		return ERR_ERROR;
	}

	/* 4. ✅ Inicializar ESP-Hosted framework con dependencias inyectadas (DIP) */
	ESP_HostedExternalDeps_t esp_deps = {
		.logger = logger,
		.transport = container->wifi_transport};

	int esp_init_result = ESP_Hosted_Init(&esp_deps);
	if (esp_init_result != 0)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "ESP-Hosted framework init failed (%d)", esp_init_result);
		}
		container->wifi_transport = NULL;
		ESP_SpiTransportAdapter_Deinit(&container->esp_transport_adapter);
		return ERR_ERROR;
	}

	container->wifi_initialized = true;

	/* 5. ✅ Inicializar WifiControlAdapter (plano de control — llama init_hosted_control_lib) */
	res = WifiControlAdapter_Init(&container->wifi_control_adapter);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "WiFi Control Adapter init failed (%d)", res);
		}
		container->wifi_transport = NULL;
		ESP_Hosted_Deinit();
		ESP_SpiTransportAdapter_Deinit(&container->esp_transport_adapter);
		container->wifi_initialized = false;
		return res;
	}

	/* 5.1. ✅ Obtener interfaz abstracta IWifiControl_t (DIP compliance) */
	container->wifi_control = WifiControlAdapter_GetInterface(&container->wifi_control_adapter);
	if (!container->wifi_control)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "ESP-Hosted: Failed to get IWifiControl interface");
		}
		WifiControlAdapter_Deinit(&container->wifi_control_adapter);
		container->wifi_transport = NULL;
		ESP_Hosted_Deinit();
		ESP_SpiTransportAdapter_Deinit(&container->esp_transport_adapter);
		container->wifi_initialized = false;
		return ERR_ERROR;
	}

	/* 5.2. ✅ Inyectar IWifiControl_t en el NicDriver adapter (DIP) */
	EspWifiNicAdapter_SetControl(container->wifi_control);

	container->wifi_initialized = true;
	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "WiFi subsystem initialized (CS mode: %s)",
				 (profile->esp32_wifi.cs_mode == ESP_CS_MODE_AUTO) ? "AUTO" : "MANUAL");
	}
	return ERR_OK;
}

/*============================================================================*
 * WIFI ENABLE SERVICE (WiFi Module Power Control)
 *============================================================================*/
/**
 * @brief Callback fired by WifiEnableService when WiFi is disabled (manual or timeout).
 *
 * Runs in System_Update thread (100 ms tick) — NOT ISR context.
 * Forces WifiStatusAdapter to "disconnected" immediately so the home screen
 * icon updates on the next OnUpdate tick without waiting for an ESP32 event
 * that will never arrive (ESP32 is in reset).
 */
static void di_on_wifi_disabled(void *ctx)
{
	(void)ctx;
	WifiStatusAdapter_SetConnected(false);
}
/** @brief Static instance of WifiEnableService */
static WifiEnableService_t s_wifi_enable_service;

/**
 * @brief Inicializa el servicio de control de encendido/apagado del módulo WiFi.
 * @pre  DI_InitWifiSubsystem() (container->wifi_transport).
 * @pre  DI_InitStorageSubsystem() (container->config_storage).
 * @pre  DI_InitBuzzerSubsystem() (container->buzzer).
 */
Result_t DI_InitWifiEnableSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Verify dependencies are initialized */
	if (!container->wifi_initialized)
	{
		return ERR_INVALID_STATE; /* WiFi transport required */
	}
	if (!container->storage_v2_initialized)
	{
		return ERR_INVALID_STATE; /* Config storage required */
	}
	if (!container->buzzer_initialized)
	{
		return ERR_INVALID_STATE; /* Buzzer required */
	}

	Result_t res;
	ILogger *logger = DI_GetLogger(container);

	/* Get dependencies from container */
	IWifiTransport *transport = DI_GetWifiTransport(container);
	IConfigStorage *config_storage = DI_GetConfigStorage(container);
	BuzzerNotificationService_t *buzzer = &container->buzzer_notification;

	if (!transport || !config_storage || !buzzer)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "WiFi Enable Service: Missing dependencies");
		}
		return ERR_NULL_POINTER;
	}

	/* Configure WifiEnableService */
	WifiEnableServiceConfig_t cfg = {
		.transport = transport,
		.config_storage = config_storage,
		.buzzer = buzzer,
		.logger = logger,
		.on_wifi_disabled = di_on_wifi_disabled,
		.on_wifi_disabled_ctx = NULL,
	};

	/* Initialize service */
	res = WifiEnableService_Init(&s_wifi_enable_service, &cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "WiFi Enable Service init failed (%d)", res);
		}
		return res;
	}

	/* Get interface and store in container */
	container->wifi_module = WifiEnableService_GetInterface(&s_wifi_enable_service);
	if (!container->wifi_module)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "WiFi Enable Service: Failed to get interface");
		}
		return ERR_ERROR;
	}

	container->wifi_enable_service_initialized = true;

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "WiFi Enable Service initialized");
	}

	return ERR_OK;
}

Result_t DI_InitSideButtonSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile || !profile->buttons[BUTTON_SIDE].enabled)
	{
		container->side_button_initialized = false;
		return ERR_OK;
	}

	if (!container->digital_input_initialized ||
		!container->relay_initialized ||
		!container->gps_initialized ||
		!container->buzzer_initialized ||
		!container->storage_v2_initialized ||
		!container->wifi_enable_service_initialized)
	{
		return ERR_INVALID_STATE;
	}

	SideButtonServiceConfig_t cfg = {
		.relay = DI_GetRelayController(container),
		.wifi_module = DI_GetWifiModule(container),
		.gps = GPSAdapter_GetControlInterface(&container->gps_adapter),
		.buzzer = &container->buzzer_notification,
		.config = DI_GetConfigStorage(container),
		.logger = DI_GetLogger(container), /* Optional: can be NULL */
	};

	if (cfg.relay == NULL || cfg.wifi_module == NULL || cfg.gps == NULL ||
		cfg.buzzer == NULL || cfg.config == NULL)
	{
		return ERR_NULL_POINTER;
	}

	Result_t res = SideButtonService_Init(&container->side_button_service, &cfg);
	if (res != ERR_OK)
	{
		container->side_button_initialized = false;
		return res;
	}

	res = SideButtonService_RegisterEvents(&container->side_button_service,
										   DI_GetDigitalInputSource(container));
	if (res != ERR_OK)
	{
		container->side_button_initialized = false;
		return res;
	}

	container->side_button_initialized = true;
	return ERR_OK;
}

/*============================================================================*
 * NETWORK SUBSYSTEM (CycloneTCP stack + ports STA/AP/RNDIS + NetworkAO)
 *============================================================================*/

/* ── Helper: Create WiFi AP Port Config ──────────────────────────────────── */
/**
 * @brief Creates NetworkPortConfig_t for WiFi Access Point (netInterface[1]).
 *
 * @param[out] cfg   Output config structure (caller-provided).
 * @param[in]  wifi  WiFi configuration from storage cache.
 *
 * @note MUST be registered as port[0] in NetworkAO so it configures FIRST:
 *       AP calls SetMode(APSTA) + StartSoftAP(), then STA calls ConnectAP().
 */
static void di_create_ap_port_config(NetworkPortConfig_t *cfg, const WifiConfig_t *wifi)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->port_type = NETWORK_PORT_TYPE_WIFI_AP;
	strncpy(cfg->hostname, "tcs-cicx1-ap", sizeof(cfg->hostname) - 1U);

	/* IP configuration: static (AP is gateway for DHCP clients) */
	cfg->ip_config.mode = NETWORK_IP_MODE_STATIC;
	const char *ap_ip = (wifi->ap_ip[0] != '\0') ? wifi->ap_ip : WIFI_DEFAULT_AP_IP;
	const char *ap_msk = (wifi->ap_mask[0] != '\0') ? wifi->ap_mask : WIFI_DEFAULT_AP_MASK;
	di_parse_ipv4(&cfg->ip_config.ip_addr, ap_ip);
	di_parse_ipv4(&cfg->ip_config.subnet_mask, ap_msk);
	di_parse_ipv4(&cfg->ip_config.gateway, ap_ip); /* AP is its own gateway */

	cfg->dhcp_server_en = true; /* Always enable DHCP server on AP */

	/* WiFi settings: this port calls SetMode(APSTA) */
	cfg->wifi_mode = wifi->mode;
	strncpy(cfg->wifi_ssid, wifi->ap_ssid, sizeof(cfg->wifi_ssid) - 1U);
	strncpy(cfg->wifi_password, wifi->ap_pwd, sizeof(cfg->wifi_password) - 1U);
	cfg->wifi_channel = (wifi->ap_channel != 0U) ? wifi->ap_channel : (uint8_t)WIFI_DEFAULT_AP_CHANNEL;
	cfg->wifi_max_conn = (wifi->ap_max_conn != 0U) ? wifi->ap_max_conn : (uint8_t)WIFI_DEFAULT_AP_MAX_CONN;
}

/* ── Helper: Create WiFi STA Port Config ─────────────────────────────────── */
/**
 * @brief Creates NetworkPortConfig_t for WiFi Station (netInterface[0]).
 *
 * @param[out] cfg   Output config structure (caller-provided).
 * @param[in]  wifi  WiFi configuration from storage cache.
 *
 * @note MUST be registered as port[1] AFTER AP, so mode is already set.
 */
static void di_create_sta_port_config(NetworkPortConfig_t *cfg, const WifiConfig_t *wifi)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->port_type = NETWORK_PORT_TYPE_WIFI_STA;
	strncpy(cfg->hostname, "tcs-cicx1", sizeof(cfg->hostname) - 1U);

	/* IP configuration: DHCP or static per user settings */
	cfg->ip_config.mode = (wifi->sta_use_dhcp != 0U) ? NETWORK_IP_MODE_DHCP : NETWORK_IP_MODE_STATIC;
	if (wifi->sta_use_dhcp == 0U)
	{
		di_parse_ipv4(&cfg->ip_config.ip_addr, wifi->sta_ip);
		di_parse_ipv4(&cfg->ip_config.subnet_mask, wifi->sta_mask);
		di_parse_ipv4(&cfg->ip_config.gateway, wifi->sta_gateway);
	}

	/* WiFi settings: wifi_mode = 0 (mode already set by AP port) */
	cfg->wifi_mode = wifi->mode;
	strncpy(cfg->wifi_ssid, wifi->sta_ssid, sizeof(cfg->wifi_ssid) - 1U);
	strncpy(cfg->wifi_password, wifi->sta_pwd, sizeof(cfg->wifi_password) - 1U);
}

/* ── Helper: Create USB RNDIS Port Config ────────────────────────────────── */
/**
 * @brief Creates NetworkPortConfig_t for USB RNDIS gateway (netInterface[2]).
 *
 * @param[out] cfg  Output config structure (caller-provided).
 *
 * @note Only called if DI_InitUsbRndisSubsystem() succeeded (usb_rndis_initialized == true).
 *       Config: 192.168.9.1/24 with DHCP server (pool: .10-.99).
 */
static void di_create_rndis_port_config(NetworkPortConfig_t *cfg)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->port_type = NETWORK_PORT_TYPE_USB_RNDIS;
	cfg->mac_override = true;

	/* MAC Address per CycloneTCP example: 00-AB-CD-07-43-01 (Windows compatibility) */
	cfg->mac_addr[0] = 0x00U;
	cfg->mac_addr[1] = 0xABU;
	cfg->mac_addr[2] = 0xCDU;
	cfg->mac_addr[3] = 0x07U;
	cfg->mac_addr[4] = 0x43U;
	cfg->mac_addr[5] = 0x01U;

	strncpy(cfg->hostname, "tcs-cicx1-usb", sizeof(cfg->hostname) - 1U);

	/* IP configuration: static gateway with DHCP server */
	cfg->ip_config.mode = NETWORK_IP_MODE_STATIC;
	cfg->ip_config.ip_addr = (NetworkIpv4Addr_t){.octet = {192U, 168U, 9U, 1U}};
	cfg->ip_config.subnet_mask = (NetworkIpv4Addr_t){.octet = {255U, 255U, 255U, 0U}};
	cfg->ip_config.gateway = (NetworkIpv4Addr_t){.octet = {192U, 168U, 9U, 1U}};
	cfg->ip_config.dns_server = (NetworkIpv4Addr_t){.octet = {0U, 0U, 0U, 0U}};

	cfg->dhcp_server_en = true; /* DHCP pool: .10-.99 (calculated in start_dhcp_server()) */
}

/**
 * @brief Inicializa la pila de red, adaptadores de puerto y el NetworkAO.
 *
 * @details
 * 1. CycloneTCP stack adapter (netInit() deferred — ejecutado desde NetworkAO).
 * 2. CycloneNetworkPortAdapter para STA (netInterface[0]) y AP (netInterface[1]).
 * 3. HttpServerAdapter (escucha en todas las interfaces del stack).
 * 4. NetworkAO_Init() con puertos AP / STA / RNDIS (si disponible).
 * 5. Wiring: callbacks WiFi → NetworkAO (OnTransportReady, OnStaConnected, etc.).
 *
 * @note EspWifiNicAdapter_Init() y hosted_Init() son DEFERRED — se ejecutan desde
 *       el hilo del NetworkAO (handle_msg_start → INetworkStack_LaunchTransport):
 *         NetworkAO_Start() → EspWifiNicAdapter_Init() → hosted_Init() (reset ESP32)
 *         → CTRL_EVENT_ESP_INIT → DI_OnTransportReady_NetworkAO
 *         → NET_AO_MSG_TRANSPORT_READY → netInit() + NetworkPort_Configure()
 *
 * @pre  DI_InitWifiSubsystem() (container->wifi_initialized == true).
 * @pre  DI_InitUsbRndisSubsystem() si se desea puerto USB RNDIS (opcional).
 */
Result_t DI_InitNetworkSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}
	if (!container->logging_initialized)
	{
		return ERR_INVALID_STATE;
	}
	if (!container->wifi_initialized)
	{
		return ERR_INVALID_STATE; /* DI_InitWifiSubsystem() must be called first */
	}

	Result_t res;
	ILogger *logger = DI_GetLogger(container);

	/* 1. Crear CycloneTCP network stack adapter.
	 *    El adapter wrappea unicamente netInit(). La configuracion de interfaces
	 *    STA/AP pertenece a CycloneNetworkPortAdapter.
	 *    INetworkStack_Init() se llama desde NetworkAO.handle_msg_start. */
	res = CycloneTcpNetworkStackAdapter_Create(
		&container->network_stack_adapter);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "CycloneTcpNetworkStackAdapter create failed (%d)", res);
		}
		/* Non-fatal: network operates with degraded functionality */
	}

	/* 5.3a.1 ✅ Paso 5: Inicializar CycloneNetworkPortAdapter para STA y/o AP.
	 *
	 * CRITICAL ORDER: AP must be registered as port[0] when APSTA mode is used,
	 * so Configure() calls SetMode(APSTA) + StartSoftAP() BEFORE ConnectAP().
	 * Only initialize the adapters required by the configured WiFi mode:
	 *   WIFI_MODE_STA  (1) → STA only  (netInterface[0])
	 *   WIFI_MODE_AP   (2) → AP only   (netInterface[1])
	 *   WIFI_MODE_APSTA(3) → both AP (netInterface[1]) + STA (netInterface[0])
	 */

	/* Load mode BEFORE adapter init to decide which adapters to create */
	/* Load WiFi config from storage cache */
	WifiConfig_t wifi_cfg = {0};
	ConfigStorage_LoadWifiConfig(DI_GetConfigStorage(container), &wifi_cfg);

	/*verify wifi mode*/
	if (wifi_cfg.mode == 0 || wifi_cfg.mode > WIFI_MODE_APSTA)
	{
		LOG_INFO(logger, "DI", "WiFi mode set to AP default");

		wifi_cfg.mode = WIFI_MODE_AP;

		// save
		ConfigStorage_SaveWifiConfig(DI_GetConfigStorage(container), &wifi_cfg);
	}

	const uint8_t wifi_mode = wifi_cfg.mode;

	const bool need_sta = (wifi_mode == WIFI_MODE_STA) || (wifi_mode == WIFI_MODE_APSTA);
	const bool need_ap = (wifi_mode == WIFI_MODE_AP) || (wifi_mode == WIFI_MODE_APSTA);

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "WiFi mode: %u — STA=%d AP=%d",
				 (unsigned)wifi_mode, (int)need_sta, (int)need_ap);
	}

	if (need_sta)
	{
		CycloneNetworkPortAdapterConfig_t sta_port_cfg;
		memset(&sta_port_cfg, 0, sizeof(sta_port_cfg));
		sta_port_cfg.iface = &netInterface[0];
		sta_port_cfg.driver = &esp32WifiStaDriver;
		sta_port_cfg.dhcp_mode = CYCLONE_PORT_DHCP_CLIENT;
		sta_port_cfg.if_name = "eth0";
		sta_port_cfg.hostname = "tcs-cicx1";
		sta_port_cfg.mac_str = NULL;
		sta_port_cfg.wifi_ctrl = container->wifi_control;
		res = CycloneNetworkPortAdapter_Init(&container->sta_port_adapter, &sta_port_cfg);
		if (res != ERR_OK && Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "CycloneNetworkPortAdapter (STA) init failed (%d)", res);
		}
	}

	if (need_ap)
	{
		CycloneNetworkPortAdapterConfig_t ap_port_cfg;
		memset(&ap_port_cfg, 0, sizeof(ap_port_cfg));
		ap_port_cfg.iface = &netInterface[1];
		ap_port_cfg.driver = &esp32WifiApDriver;
		ap_port_cfg.dhcp_mode = CYCLONE_PORT_DHCP_SERVER;
		ap_port_cfg.if_name = "eth1";
		ap_port_cfg.hostname = "tcs-cicx1-ap";
		ap_port_cfg.mac_str = NULL;
		ap_port_cfg.wifi_ctrl = container->wifi_control;
		res = CycloneNetworkPortAdapter_Init(&container->ap_port_adapter, &ap_port_cfg);
		if (res != ERR_OK && Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "CycloneNetworkPortAdapter (AP) init failed (%d)", res);
		}
	}

	/* 5.3b. ✅ Inicializar HttpServerAdapter (INetworkService_t) — POST-REFACTOR v2
	 *         Routing: HttpRouteTable despacha peticiones a IHttpRequestHandler_t.
	 *         El servidor HTTP usa interface=NULL (escucha en todas las interfaces del stack).
	 *         P1-FIX: connections pool gestionado internamente por el adapter.
	 *         HttpServerAdapter.Start() se invoca desde NetworkAO tras netConfigInterface().
	 *
	 *         Para registrar nuevas rutas HTTP:
	 *           HttpRouteTable_AddRoute(&container->http_route_table,
	 *               &(HttpRoute_t){ .method = "GET", .prefix = "/api/relay",
	 *                               .handler = MyHandler_GetInterface(...) }); */
	HttpRouteTable_Init(&container->http_route_table);

	/* Inicializar handlers HTTP API con dependencias inyectadas */
	{
		const BoardProfile_t *bp = BSP_GetBoardProfile();
		SystemInfoApiHandlerDeps_t sysinfo_deps = {
			.time = DI_GetTimeSource(container),
			.gps = DI_GetGPSSource(container),
			.config = DI_GetConfigStorage(container),
			.relay = DI_GetRelayController(container),
			.battery = DI_GetBatteryMonitor(container),
			.license = DI_GetLicenseStatus(container),
			.hourmeter = DI_GetHourmeter(container),
			.temp_sensor = DI_GetTemperatureSensor(container),
			.has_battery = (bp != NULL) ? bp->battery.enabled : false,
			.logger = logger,
		};
		res = SystemInfoApiHandler_Init(&container->system_info_handler, &sysinfo_deps);
	}
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "SystemInfoApiHandler init failed (%d)", res);
	}

	res = WifiApiHandler_Init(
		&container->wifi_api_handler,
		DI_GetConfigStorage(container),
		logger,
		CycloneNetworkPortAdapter_GetPortInterface(&container->sta_port_adapter),
		CycloneNetworkPortAdapter_GetPortInterface(&container->ap_port_adapter));
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "WifiApiHandler init failed (%d)", res);
	}

	res = AdminApiHandler_Init(
		&container->admin_api_handler,
		DI_GetConfigStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "AdminApiHandler init failed (%d)", res);
	}

	res = AuthApiHandler_Init(
		&container->auth_api_handler,
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "AuthApiHandler init failed (%d)", res);
	}

	res = ConfigResetApiHandler_Init(
		&container->config_reset_api_handler,
		DI_GetConfigStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "ConfigResetApiHandler init failed (%d)", res);
	}

	res = ContactApiHandler_Init(
		&container->contact_api_handler,
		DI_GetConfigStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "ContactApiHandler init failed (%d)", res);
	}

	res = GeneralConfigApiHandler_Init(
		&container->general_config_handler,
		DI_GetConfigStorage(container),
		DI_GetEventLogStorage(container),
		DI_GetEventNotifier(container),
		DI_GetTimeSource(container),
		BSP_GetBoardProfile()->display.enabled,
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "GeneralConfigApiHandler init failed (%d)", res);
	}

	res = GpsApiHandler_Init(
		&container->gps_api_handler,
		DI_GetConfigStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "GpsApiHandler init failed (%d)", res);
	}

	res = HealthApiHandler_Init(
		&container->health_api_handler,
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "HealthApiHandler init failed (%d)", res);
	}

	res = HistoryApiHandler_Init(
		&container->history_api_handler,
		DI_GetEventLogStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "HistoryApiHandler init failed (%d)", res);
	}

	res = InterruptorConfigApiHandler_Init(
		&container->interruptor_config_handler,
		DI_GetConfigStorage(container),
		logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "InterruptorConfigApiHandler init failed (%d)", res);
	}

	res = UploadAuthApiHandler_Init(&container->upload_auth_api_handler, logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "UploadAuthApiHandler init failed (%d)", res);
	}

	/* TODO: replace logger-only Init once strategies are injected */
	IUpdateManager_t *update_mgr = DI_GetUpdateManager(container);
	res = UploadOtaApiHandler_Init(&container->upload_ota_api_handler, update_mgr, logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "UploadOtaApiHandler init failed (%d)", res);
	}

	res = UploadFsApiHandler_Init(&container->upload_fs_api_handler, update_mgr, logger);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "UploadFsApiHandler init failed (%d)", res);
	}

	/* EmbeddedWebHandler — sirve archivos web comprimidos desde firmware */
	res = EmbeddedWebHandler_Init(&container->embedded_web_handler);
	if (res != ERR_OK && Logger_IsValid(logger))
	{
		LOG_ERROR(logger, "DI", "EmbeddedWebHandler init failed (%d)", res);
	}

	/* Registrar rutas HTTP (after handlers are initialized) */
	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "GET",
								.prefix = "/api/interruptor/system-information",
								.handler = SystemInfoApiHandler_GetInterface(&container->system_info_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/wifi-manager",
								.handler = WifiApiHandler_GetInterface(&container->wifi_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/admin",
								.handler = AdminApiHandler_GetInterface(&container->admin_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "POST",
								.prefix = "/api/interruptor/auth",
								.handler = AuthApiHandler_GetInterface(&container->auth_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "POST",
								.prefix = "/api/configuration",
								.handler = ConfigResetApiHandler_GetInterface(&container->config_reset_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/contact-config",
								.handler = ContactApiHandler_GetInterface(&container->contact_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/general-configuration",
								.handler = GeneralConfigApiHandler_GetInterface(&container->general_config_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/gps-configuration",
								.handler = GpsApiHandler_GetInterface(&container->gps_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* ANY method */
								.prefix = "/api/health",
								.handler = HealthApiHandler_GetInterface(&container->health_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "GET",
								.prefix = "/api/interruptor/historical-events",
								.handler = HistoryApiHandler_GetInterface(&container->history_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = NULL, /* GET + POST */
								.prefix = "/api/interruptor/configuration",
								.handler = InterruptorConfigApiHandler_GetInterface(&container->interruptor_config_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "POST",
								.prefix = "/api/upload/auth",
								.handler = UploadAuthApiHandler_GetInterface(&container->upload_auth_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "POST",
								.prefix = "/api/upload/ota",
								.handler = UploadOtaApiHandler_GetInterface(&container->upload_ota_api_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "POST",
								.prefix = "/api/upload/fs",
								.handler = UploadFsApiHandler_GetInterface(&container->upload_fs_api_handler)});

	/* Embedded web resources (HTML, JS) — sirve archivos comprimidos gzip */
	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "GET",
								.prefix = "/update.html",
								.handler = EmbeddedWebHandler_GetInterface(&container->embedded_web_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "GET",
								.prefix = "/js/spark-md5.min.js",
								.handler = EmbeddedWebHandler_GetInterface(&container->embedded_web_handler)});

	HttpRouteTable_AddRoute(&container->http_route_table,
							&(HttpRoute_t){
								.method = "GET",
								.prefix = "/404.html",
								.handler = EmbeddedWebHandler_GetInterface(&container->embedded_web_handler)});

	HttpServerAdapterConfig_t http_cfg;
	memset(&http_cfg, 0, sizeof(http_cfg));
	http_cfg.port = 80U;
	http_cfg.route_table = &container->http_route_table;
	http_cfg.logger = logger;

	res = HttpServerAdapter_Init(&container->http_server_adapter, &http_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "HttpServerAdapter init failed (%d)", res);
		}
		/* Non-fatal: NetworkAO can run without HTTP if adapter init fails */
	}

	/* 5.4. ✅ Inicializar NetworkAO (Ports/Links FSM) — Fase N.8 */
	INetworkService_t *net_services[1];
	net_services[0] = HttpServerAdapter_GetInterface(&container->http_server_adapter);

	/* ========================================================================
	 * SECTION: WiFi Port Configurations (AP + STA)
	 * ======================================================================== */

	/* First-boot initialisation: if AP SSID is empty or the legacy placeholder,
	 * assign "TCS-<uid>" and persist it to EEPROM so it becomes the permanent
	 * stored value.  On subsequent boots the EEPROM value is used unchanged,
	 * allowing the user to customise it via settings. */
	{
		const bool is_uninitialised = (wifi_cfg.ap_ssid[0] == '\0' ||
									   strcmp(wifi_cfg.ap_ssid, "CICX1-AP") == 0);
		if (is_uninitialised)
		{
			IDeviceIdentity *dev_id = DI_GetDeviceIdentity(container);
			if (dev_id != NULL)
			{
				char uid_buf[DEVICE_IDENTITY_UID_STR_LEN];
				memset(uid_buf, 0, sizeof(uid_buf));
				if (DeviceIdentity_GetUidString(dev_id, uid_buf, sizeof(uid_buf)) == ERR_OK && uid_buf[0] != '\0')
				{
					/* ap_ssid[32]: "TCS-"(4) + max 27 UID chars + null = 32. */
					(void)lwprintf_snprintf(wifi_cfg.ap_ssid, sizeof(wifi_cfg.ap_ssid),
											"TCS-%.27s", uid_buf);
					/* Persist — from now on EEPROM is the single source of truth. */
					(void)ConfigStorage_SaveWifiConfig(DI_GetConfigStorage(container), &wifi_cfg);
				}
			}
		}
	}

	/* Create port configs using helper functions */
	NetworkPortConfig_t net_ap_cfg;
	NetworkPortConfig_t net_sta_cfg;

	/* ========================================================================
	 * SECTION: NetworkAO Configuration
	 * ======================================================================== */

	NetworkAO_Config_t net_ao_cfg;
	memset(&net_ao_cfg, 0, sizeof(net_ao_cfg));

	/* Stack adapter */
	net_ao_cfg.stack = CycloneTcpNetworkStackAdapter_GetInterface(&container->network_stack_adapter);

	/* Register WiFi ports matching the configured mode.
	 *
	 * CRITICAL ORDER for APSTA: AP must be port[0] so it fires SetMode(APSTA)
	 * + StartSoftAP() BEFORE STA calls ConnectAP(). This mirrors the legacy
	 * sequencing from old/network_ao_old.c handle_netstack_ready.
	 *
	 * Mode mapping:
	 *   WIFI_MODE_STA  (1) → port[0] = STA
	 *   WIFI_MODE_AP   (2) → port[0] = AP
	 *   WIFI_MODE_APSTA(3) → port[0] = AP, port[1] = STA
	 */
	switch ((WifiMode_t)wifi_cfg.mode)
	{
	case WIFI_MODE_APSTA:
		/* AP first (SetMode APSTA + StartSoftAP), then STA (ConnectAP) */
		di_create_ap_port_config(&net_ap_cfg, &wifi_cfg);
		di_create_sta_port_config(&net_sta_cfg, &wifi_cfg);
		net_ao_cfg.ports[0] = CycloneNetworkPortAdapter_GetPortInterface(&container->ap_port_adapter);
		net_ao_cfg.links[0] = CycloneNetworkPortAdapter_GetLinkInterface(&container->ap_port_adapter);
		net_ao_cfg.port_configs[0] = &net_ap_cfg;
		net_ao_cfg.ports[1] = CycloneNetworkPortAdapter_GetPortInterface(&container->sta_port_adapter);
		net_ao_cfg.links[1] = CycloneNetworkPortAdapter_GetLinkInterface(&container->sta_port_adapter);
		net_ao_cfg.port_configs[1] = &net_sta_cfg;
		net_ao_cfg.port_count = 2U;
		break;

	case WIFI_MODE_AP:
		/* AP only */
		di_create_ap_port_config(&net_ap_cfg, &wifi_cfg);
		net_ao_cfg.ports[0] = CycloneNetworkPortAdapter_GetPortInterface(&container->ap_port_adapter);
		net_ao_cfg.links[0] = CycloneNetworkPortAdapter_GetLinkInterface(&container->ap_port_adapter);
		net_ao_cfg.port_configs[0] = &net_ap_cfg;
		net_ao_cfg.port_count = 1U;
		break;

	case WIFI_MODE_STA:
		/* STA only */
		di_create_sta_port_config(&net_sta_cfg, &wifi_cfg);
		net_ao_cfg.ports[0] = CycloneNetworkPortAdapter_GetPortInterface(&container->sta_port_adapter);
		net_ao_cfg.links[0] = CycloneNetworkPortAdapter_GetLinkInterface(&container->sta_port_adapter);
		net_ao_cfg.port_configs[0] = &net_sta_cfg;
		net_ao_cfg.port_count = 1U;
		break;

	default: /* WIFI_MODE_DISABLED or unknown — no WiFi ports */
		if (Logger_IsValid(logger))
		{
			LOG_WARN(logger, "DI", "WiFi mode %u: no WiFi ports registered",
					 (unsigned)wifi_cfg.mode);
		}
		net_ao_cfg.port_count = 0U;
		break;
	}

	/* USB RNDIS: only if subsystem initialized (BoardProfile enabled).
	 * Appended after WiFi ports regardless of WiFi mode. */
	if (container->usb_rndis_initialized)
	{
		/* Port index after WiFi ports */
		const uint8_t rndis_idx = net_ao_cfg.port_count;
		/* Create RNDIS config only when needed */
		static NetworkPortConfig_t net_rndis_cfg;
		di_create_rndis_port_config(&net_rndis_cfg);

		net_ao_cfg.ports[rndis_idx] = CycloneUsbRndisPortAdapter_GetPortInterface(&container->rndis_port_adapter);
		net_ao_cfg.links[rndis_idx] = CycloneUsbRndisPortAdapter_GetLinkInterface(&container->rndis_port_adapter);
		net_ao_cfg.port_configs[rndis_idx] = &net_rndis_cfg;
		net_ao_cfg.port_count++;
	}

	/* Services (HTTP server) */
	net_ao_cfg.services[0] = net_services[0];
	net_ao_cfg.service_count = 1U;
	net_ao_cfg.logger = logger;
	net_ao_cfg.wifi_transport = DI_GetWifiTransport(container);
	net_ao_cfg.wifi_module = DI_GetWifiModule(container);

	/* WiFi ports require ESP-Hosted SPI transport; RNDIS does not */
	net_ao_cfg.needs_transport = container->wifi_initialized;

	/* Initialize NetworkAO with configured ports */
	res = NetworkAO_Init(&container->network_ao, &net_ao_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "NetworkAO init failed (%d)", res);
		}
		return res;
	}

	container->network_initialized = true;

	/* ========================================================================
	 * SECTION: Wiring (WiFi Events → NetworkAO Messages)
	 * ======================================================================== */

	/* All callbacks post ISR-safe messages to NetworkAO queue (DIP) */
	IWifiControl_OnTransportReady(container->wifi_control,
								  DI_OnTransportReady_NetworkAO,
								  &container->network_ao);
	IWifiControl_OnStaConnected(container->wifi_control,
								DI_OnStaConnected_NetworkAO,
								container);
	IWifiControl_OnStaDisconnected(container->wifi_control,
								   DI_OnStaDisconnected_NetworkAO,
								   container);

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "Network subsystem initialized (CycloneTCP + ports + NetworkAO)");
	}

	return ERR_OK;
}

Result_t DI_DeinitNetworkSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->wifi_initialized)
	{
		return ERR_OK;
	}

	ILogger *logger = DI_GetLogger(container);

	if (container->network_initialized)
	{
		NetworkAO_Stop(&container->network_ao);
		NetworkAO_WaitStopped(&container->network_ao, 5000U);
		container->network_initialized = false;
	}

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "Network subsystem deinitialized");
	}

	return ERR_OK;
}

/**
 * @brief Deinicializa el subsistema WiFi (WifiControlAdapter + ESP-Hosted + SPI transport).
 * @pre  Call DI_DeinitNetworkSubsystem() FIRST (NetworkAO depends on wifi_control).
 */
Result_t DI_DeinitWifiSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->wifi_initialized)
	{
		return ERR_OK;
	}

	ILogger *logger = DI_GetLogger(container);

	container->wifi_control = NULL;
	WifiControlAdapter_Deinit(&container->wifi_control_adapter);

	int esp_deinit_result = ESP_Hosted_Deinit();
	if (esp_deinit_result != 0 && Logger_IsValid(logger))
	{
		LOG_WARN(logger, "DI", "ESP-Hosted framework deinit failed (%d)", esp_deinit_result);
	}

	container->wifi_transport = NULL;
	ESP_SpiTransportAdapter_Deinit(&container->esp_transport_adapter);

	container->wifi_initialized = false;

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "WiFi subsystem deinitialized");
	}

	return ERR_OK;
}

/* =========================================================================
 * USB RNDIS Subsystem
 * IPv4 defaults (from rndis_example.c reference):
 *   Host IP  : 192.168.9.1 / 255.255.255.0
 *   DHCP pool: 192.168.9.10 – 192.168.9.99, lease 3600 s
 * ========================================================================= */

/**
 * @brief Inicializa el stack USB Device (RNDIS) y el CycloneTCP RNDIS port adapter.
 *
 * @details
 * 1. Si `profile->usb_rndis.enabled == false`, retorna ERR_OK sin hacer nada.
 * 2. Llama USBD_Init / USBD_RegisterClass / USBD_Start (USB Device Library).
 * 3. Inicializa CycloneUsbRndisPortAdapter en netInterface[2] con rndisDriver.
 *
 * @note La configuración IP se aplica separadamente via INetworkPort_Configure()
 *       desde un contexto de tarea RTOS (netConfigInterface() crea un thread).
 *       Usa DI_GetRndisPortInterface() para obtener el puntero a la interfaz.
 *
 * @pre DI_InitLoggingSubsystem() debe estar inicializado.
 */
Result_t DI_InitUsbRndisSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}
	if (!container->logging_initialized)
	{
		return ERR_INVALID_STATE;
	}

	const BoardProfile_t *profile = BSP_GetBoardProfile();
	if (!profile)
	{
		return ERR_INVALID_PARAM;
	}

	ILogger *logger = DI_GetLogger(container);

	/* Si USB RNDIS está deshabilitado en el board profile, skip */
	if (!profile->usb_rndis.enabled)
	{
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "DI", "USB RNDIS disabled in board profile (%s)", profile->variant_name);
		}
		container->usb_rndis_initialized = false;
		return ERR_OK;
	}

	/* 1. Inicializar USB Device Stack (STM32 USB Device Library) */
	USBD_StatusTypeDef usbd_res;

	usbd_res = USBD_Init(&USBD_Device, &usbdRndisDescriptors, 0);
	if (usbd_res != USBD_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "USBD_Init failed (%d)", (int)usbd_res);
		}
		return ERR_ERROR;
	}

	usbd_res = USBD_RegisterClass(&USBD_Device, USBD_RNDIS_CLASS);
	if (usbd_res != USBD_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "USBD_RegisterClass failed (%d)", (int)usbd_res);
		}
		return ERR_ERROR;
	}

	usbd_res = USBD_Start(&USBD_Device);
	if (usbd_res != USBD_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "USBD_Start failed (%d)", (int)usbd_res);
		}
		return ERR_ERROR;
	}

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI", "USB Device stack started (RNDIS class)");
	}

	/* 2. Inicializar RNDIS port adapter (registra rndisDriver en netInterface[2]) */
	static CycloneUsbRndisPortAdapterConfig_t rndis_cfg;
	memset(&rndis_cfg, 0, sizeof(rndis_cfg));
	rndis_cfg.if_index = 2U; /* netInterface[2] */
	rndis_cfg.port_type = NETWORK_PORT_TYPE_USB_RNDIS;
	rndis_cfg.net_iface = &netInterface[2];
	rndis_cfg.nic_driver = &rndisDriver;
	rndis_cfg.if_name = "usb0";
	rndis_cfg.logger = logger;

	Result_t res = CycloneUsbRndisPortAdapter_Init(&container->rndis_port_adapter, &rndis_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "CycloneUsbRndisPortAdapter init failed (%d)", res);
		}
		return res;
	}

	container->usb_rndis_initialized = true;

	if (Logger_IsValid(logger))
	{
		LOG_INFO(logger, "DI",
				 "USB RNDIS subsystem initialized (IP: 192.168.9.1, pool: .10-.99). "
				 "Call INetworkPort_Configure() from task context to bring up interface.");
	}

	return ERR_OK;
}

/**
 * @brief Retorna la interfaz INetworkPort_t del adapter USB RNDIS.
 */
INetworkPort_t *DI_GetRndisPortInterface(DependencyContainer_t *container)
{
	if (!container || !container->usb_rndis_initialized)
	{
		return NULL;
	}
	return CycloneUsbRndisPortAdapter_GetPortInterface(&container->rndis_port_adapter);
}

/*============================================================================*
 * WIRING (CALLBACKS Y OBSERVERS)
 *============================================================================*/

/* ===== Phase 4.9: Config Change Observer Callbacks ===== */

/**
 * @brief Callback invocado cuando RelayConfig_t cambia en EEPROM.
 * @note Ejecuta en thread StorageCoordinator, debe ser <1ms (no bloquear).
 */
static void DI_OnRelayConfigChanged(
	void *context,
	ConfigType_t config_type,
	const void *config_data,
	size_t config_size)
{
	if (config_type != CONFIG_TYPE_RELAY || config_data == NULL)
		return;

	DependencyContainer_t *container = (DependencyContainer_t *)context;
	if (!container || !container->relay_initialized)
		return;

	const RelayConfig_t *new_config = (const RelayConfig_t *)config_data;

	ILogger *logger = DI_GetLogger(container);

	LOG_INFO(logger, "DI", "Relay config changed");

	IRelayController *iface = RelayAdapter_GetInterface(&container->relay_adapter);

	/* Update RelayController config with new window_config */
	if (RelayController_UpdateConfig(iface, new_config) != ERR_OK)
	{
		LOG_ERROR(logger, "DI", "Failed to update RelayController config");
	}

	RelayAO_Event_t event = {
		.type = RELAY_EVENT_CONFIG,
	};

	RelayAO_PostEvent(&container->relay_ao, &event); /* OS_NO_WAIT for ISR safety */
}

/**
 * @brief Callback invocado cuando GPSConfig_t cambia en EEPROM.
 * @note Ejecuta en thread StorageCoordinator, debe ser <1ms (no bloquear).
 */
static void DI_OnGPSConfigChanged(
	void *context,
	ConfigType_t config_type,
	const void *config_data,
	size_t config_size)
{
	if (config_type != CONFIG_TYPE_GPS || config_data == NULL)
		return;

	DependencyContainer_t *container = (DependencyContainer_t *)context;
	if (!container || !container->gps_initialized)
		return;

	const GPSConfig_t *new_config = (const GPSConfig_t *)config_data;

	ILogger *logger = DI_GetLogger(container);

	/* Log de cambio de configuración GPS (nivel INFO) */
	LOG_INFO(logger, "DI", "GPS config changed: antenna_type=%d, utc_offset_index=%d, time_offset=%d",
			 new_config->antenna_type,
			 new_config->utc_offset_index,
			 new_config->time_offset);

	/* Update GPSAdapter fields */
	container->gps_adapter.antenna_type = new_config->antenna_type;
	container->gps_adapter.utc_offset_index = new_config->utc_offset_index;
	container->gps_adapter.seconds_offset = (int16_t)new_config->time_offset;

	/* Apply antenna selection via GPIO (if GPIO initialized) */
	if (container->gps_adapter.gpio)
	{
		IGPSControl *gps_ctrl = GPSAdapter_GetControlInterface(&container->gps_adapter);
		if (gps_ctrl)
		{
			LOG_INFO(logger, "DI", "Applying GPS antenna selection: %s",
					 (new_config->antenna_type == GPS_ANTENNA_INTERNAL) ? "INTERNAL" : "EXTERNAL");
			/*apply change */
			(void)GPS_Control_ApplyHardwareConfig(gps_ctrl);
		}
	}
}

/**
 * @brief Callback invocado cuando GeneralConfig_t cambia en EEPROM.
 * @note Ejecuta en thread StorageCoordinator, debe ser <1ms (no bloquear).
 * @note GeneralConfig_t no tiene campos que afecten directamente a adapters actuales.
 */
static void DI_OnGeneralConfigChanged(
	void *context,
	ConfigType_t config_type,
	const void *config_data,
	size_t config_size)
{
	(void)context;
	(void)config_type;
	(void)config_data;
	(void)config_size;
	/* No action needed - GeneralConfig doesn't affect current adapters */
}

/**
 * @brief Subscriber de cambios de configuración → emite EVENT_TYPE_CONFIG_* al log.
 *
 * @note  Ejecutado en el thread de StorageCoordinatorAO_v2. EventNotifier_NotifyEvent
 *        es non-blocking (post a queue del EventLogAO) → seguro en este contexto.
 */
static void DI_OnConfigChangedEventLog(
	void *context,
	ConfigType_t config_type,
	const void *config_data,
	size_t config_size)
{
	(void)config_data;
	(void)config_size;
	DependencyContainer_t *c = (DependencyContainer_t *)context;
	if (c == NULL)
	{
		return;
	}
	IEventNotifier *notifier = DI_GetEventNotifier(c);
	if (!EventNotifier_IsValid(notifier))
	{
		return;
	}

	EventType_t ev_type;
	switch (config_type)
	{
	case CONFIG_TYPE_RELAY:
		ev_type = EVENT_TYPE_CONFIG_RELAY;
		break;
	case CONFIG_TYPE_GPS:
		ev_type = EVENT_TYPE_CONFIG_GPS;
		break;
	case CONFIG_TYPE_GENERAL:
		ev_type = EVENT_TYPE_CONFIG_GENERAL;
		break;
	case CONFIG_TYPE_WIFI:
		ev_type = EVENT_TYPE_CONFIG_WIFI;
		break;
	default:
		return;
	}

	DateTime_t now = {0};
	ITimeSource *ts = DI_GetTimeSource(c);
	if (ts != NULL)
	{
		(void)TimeSource_GetTime(ts, &now);
	}
	(void)EventNotifier_NotifyEvent(notifier, &now, ev_type,
									EVENT_SEVERITY_INFO, (uint16_t)config_type, 0U);
}

Result_t DI_WireComponents(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Verify all subsystems are initialized */
	if (!container->storage_initialized ||
		!container->gps_initialized ||
		!container->time_initialized ||
		!container->relay_initialized ||
		!container->digital_input_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* ===== 1. Initialize PPS Dispatcher ===== */
	PPSDispatcher_Init(&container->pps_dispatcher);

	/* ===== 1.5. Late-wire EventNotifier to StorageCoordinatorAO (init order constraint) ===== */
	/* StorageCoordinatorAO is initialized before EventNotifier, so inject here after both are ready. */
	container->storage_coordinator_v2.event_notifier = DI_GetEventNotifier(container);

	/* ===== 2. Subscribe TimeAdapter to PPS (RTC sync) - Priority 1 ===== */
	ITimeSyncControl *sync_ctrl = TimeAdapter_GetSyncControlInterface(&container->time_adapter);
	res = PPSDispatcher_Subscribe(&container->pps_dispatcher, DI_PPS_To_TimeSync_Callback, sync_ctrl);
	if (res != ERR_OK)
		return res;

	/* ===== 3. Subscribe RelayAO to PPS (cycle control) - Priority 2 ===== */
	res = PPSDispatcher_Subscribe(&container->pps_dispatcher, DI_PPS_To_RelayAO_Callback, &container->relay_ao);
	if (res != ERR_OK)
		return res;

	/* ===== 4. Connect GPSAdapter to PPS Dispatcher ===== */
	IGPSSource *gps_iface = GPSAdapter_GetInterface(&container->gps_adapter);
	res = GPS_Source_RegisterPPSCallback(gps_iface,
										 (PPSCallback_t)PPSDispatcher_Dispatch,
										 &container->pps_dispatcher);
	if (res != ERR_OK)
		return res;

	/* ===== 5. Phase 4.9: Subscribe adapters to config changes (Observer Pattern) ===== */
	/* Verify storage v2 is initialized before subscribing */
	if (!container->storage_v2_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* 5.1. RelayAdapter callback: Update window_config + CycleScheduler on RELAY config change */
	res = StorageCoordinatorAO_v2_SubscribeConfigChange(
		&container->storage_coordinator_v2,
		DI_OnRelayConfigChanged,
		container);
	if (res != ERR_OK)
		return res;

	/* 5.2. GPSAdapter callback: Update antenna_type + UTC offset on GPS config change */
	res = StorageCoordinatorAO_v2_SubscribeConfigChange(
		&container->storage_coordinator_v2,
		DI_OnGPSConfigChanged,
		container);
	if (res != ERR_OK)
		return res;

	/* 5.3. General config callback: Currently no-op, reserved for future use */
	res = StorageCoordinatorAO_v2_SubscribeConfigChange(
		&container->storage_coordinator_v2,
		DI_OnGeneralConfigChanged,
		container);
	if (res != ERR_OK)
		return res;

	/* 5.4. Event log callback: Emite EVENT_TYPE_CONFIG_* al log de eventos */
	res = StorageCoordinatorAO_v2_SubscribeConfigChange(
		&container->storage_coordinator_v2,
		DI_OnConfigChangedEventLog,
		container);
	if (res != ERR_OK)
		return res;

	/* ===== 7. Register critical alarm callbacks (OVERTEMP → RelayAO + BuzzerService) ===== */
	IDigitalInputSource *di_source = DigitalInputAdapter_GetInterface(&container->digital_input);

	/* 7.1. RelayAO callback (PRESS/RELEASE): forced contact open/resume (CRITICAL PATH) */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_ALARM_OVERTEMP,
		DI_EVENT_PRESS | DI_EVENT_RELEASE,
		DI_OnOvertempAlarm_Callback, /* → RelayAO ONLY (safety-critical, no buzzer dependency) */
		&container->relay_ao);
	if (res != ERR_OK)
		return res;
	/* 7.2. BuzzerService callback (PRESS + KEEPALIVE): beep inicial + periódico cada 4s (ACTION-016) */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_ALARM_OVERTEMP,
		DI_EVENT_PRESS | DI_EVENT_KEEPALIVE,
		DI_OnOvertempAlarm_BuzzerCallback, /* → BuzzerService (beep inicial) */
		container);
	if (res != ERR_OK)
		return res;

	/* ===== 8. Register button callbacks (UI feedback via buzzer) ===== */

	/* 8.1. BUTTON_ONOFF: Feedback táctil en PRESS */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_BUTTON_ONOFF,
		DI_EVENT_PRESS,
		DI_OnButtonPress_BuzzerCallback,
		container);
	if (res != ERR_OK)
		return res;

	/* 8.2. BUTTON_ENTER: Feedback táctil en PRESS */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_BUTTON_ENTER,
		DI_EVENT_PRESS,
		DI_OnButtonPress_BuzzerCallback,
		container);
	if (res != ERR_OK)
		return res;

	/* 8.3. BUTTON_UP: Feedback táctil en PRESS */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_BUTTON_UP,
		DI_EVENT_PRESS,
		DI_OnButtonPress_BuzzerCallback,
		container);
	if (res != ERR_OK)
		return res;

	/* 8.4. BUTTON_DOWN: Feedback táctil en PRESS */
	res = DigitalInputSource_RegisterCallback(
		di_source,
		DI_ID_BUTTON_DOWN,
		DI_EVENT_PRESS,
		DI_OnButtonPress_BuzzerCallback,
		container);
	if (res != ERR_OK)
		return res;

	/* Side button is handled by SideButtonService callbacks (PRESS/RELEASE). */

	return ERR_OK;
}

/*============================================================================*
 * ACTIVE OBJECTS LIFECYCLE
 *============================================================================*/

Result_t DI_StartActiveObjects(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;

	/* Start in order of priority (highest first) — ver task_priorities.h */

	/* StorageCoordinatorAO v2 — must start before consumers (Relay, GPS, UI) */
	if (container->storage_v2_initialized)
	{
		res = StorageCoordinatorAO_v2_Start(&container->storage_coordinator_v2);
		if (res != ERR_OK)
			return res;
	}
	/* Start EventLogAO */
	res = EventLogAO_Start(&container->event_log_ao);
	if (res != ERR_OK)
	{
		return res;
	}
	/* RelayAO (TASK_PRIO_RELAY = 3) */
	if (container->relay_initialized)
	{
		res = RelayAO_Start(&container->relay_ao);
		if (res != ERR_OK)
			return res;
	}

	/* GPSAo (TASK_PRIO_GPS = 5) */
	if (container->gps_initialized)
	{
		res = GPSAo_Start(&container->gps_ao);
		if (res != ERR_OK)
			return res;
	}

	/* DigitalInputAO (TASK_PRIO_DIGITAL_INPUT = 8) */
	if (container->digital_input_initialized)
	{
		res = DigitalInputAO_Start(&container->digital_input_ao);
		if (res != ERR_OK)
			return res;
	}

	/* BuzzerAO (TASK_PRIO_BUZZER = 8) */
	if (container->buzzer_initialized)
	{
		res = BuzzerAO_Start(&container->buzzer_ao);
		if (res != ERR_OK)
			return res;
	}

	/* HourmeterAO — low priority, periodic accumulation + batch EEPROM saves */
	if (container->hourmeter_initialized)
	{
		res = HourmeterAO_Start(&container->hourmeter_ao);
		if (res != ERR_OK)
			return res;
	}

	/* NetworkAO (TASK_PRIO_NETWORK = 4) — starts WiFi FSM and HTTP server */
	if (container->network_initialized)
	{
		res = NetworkAO_Start(&container->network_ao);
		if (res != ERR_OK)
		{
			ILogger *logger = DI_GetLogger(container);
			if (Logger_IsValid(logger))
			{
				LOG_WARN(logger, "DI", "NetworkAO start failed (%d) — continuing", res);
			}
			/* Non-fatal: rest of system is operational without WiFi */
		}
	}

	/* ESP-Hosted (starts internal threads) */
	if (container->wifi_initialized)
	{
	}

	/* UiAO (TASK_PRIO_UI = 9 — GUI refresh LVGL). Arranca el hilo de renderizado. */
	if (container->ui_initialized)
	{
		res = UiAO_Start(&container->ui_ao);
		if (res != ERR_OK)
		{
			ILogger *logger = DI_GetLogger(container);
			if (Logger_IsValid(logger))
			{
				LOG_WARN(logger, "DI", "UiAO start failed (%d) — display unavailable", res);
			}
			/* Non-fatal: rest of system is operational without display */
		}
	}

	/* ===== Log System Boot Event ===== */
	if (container->logging_initialized)
	{
		ILogger *logger = DI_GetLogger(container);
		IEventNotifier *event_notifier = DI_GetEventNotifier(container);

		/* Log startup message to UART/SWO */
		if (Logger_IsValid(logger))
		{
			LOG_INFO(logger, "SYSTEM", "All Active Objects running");
		}

		/* Log system boot event to EEPROM */
		if (EventNotifier_IsValid(event_notifier))
		{
			DateTime_t boot_time;
			ITimeSource *time_src = DI_GetTimeSource(container);
			if (time_src)
			{
				TimeSource_GetTime(time_src, &boot_time);
			}
			else
			{
				memset(&boot_time, 0, sizeof(boot_time)); /* Fallback if time not ready */
			}

			EventNotifier_NotifyEvent(
				event_notifier,
				&boot_time,
				EVENT_TYPE_SYSTEM_BOOT,
				EVENT_SEVERITY_INFO,
				0, /* info = 0 */
				0  /* params = 0 */
			);
		}
	}

	return ERR_OK;
}

Result_t DI_StopActiveObjects(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Stop in reverse order */

	/* NetworkAO — stop before StorageCoordinatorAO (depends on cfg_cache) */
	if (container->network_initialized)
	{
		NetworkAO_Stop(&container->network_ao);
		NetworkAO_WaitStopped(&container->network_ao, 5000U);
	}

	/* BuzzerAO */
	if (container->buzzer_initialized)
	{
		BuzzerAO_Stop(&container->buzzer_ao);
	}

	/* HourmeterAO — Stop posts MSG_STOP; WaitStopped ensures final EEPROM persist is complete */
	if (container->hourmeter_initialized)
	{
		HourmeterAO_Stop(&container->hourmeter_ao);
		HourmeterAO_WaitStopped(&container->hourmeter_ao, 5000U);
	}

	/* ✅ Phase 4.7: StorageCoordinatorAO v3.0 removed (v2 stops via internal mechanism) */

	if (container->digital_input_initialized)
	{
		DigitalInputAO_Stop(&container->digital_input_ao);
	}

	if (container->gps_initialized)
	{
		GPSAo_Stop(&container->gps_ao);
	}

	if (container->relay_initialized)
	{
		RelayAO_Stop(&container->relay_ao);
	}

	return ERR_OK;
}

/*============================================================================*
 * DEINIT FUNCTIONS
 *============================================================================*/

Result_t DI_DeinitBuzzerSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->buzzer_initialized)
	{
		return ERR_INVALID_STATE;
	}

	BuzzerNotificationService_Deinit(&container->buzzer_notification);
	BuzzerAO_Deinit(&container->buzzer_ao);
	BuzzerAdapter_Deinit(&container->buzzer_adapter);

	container->buzzer_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitHourmeterSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->hourmeter_initialized)
	{
		return ERR_INVALID_STATE;
	}

	HourmeterAO_Stop(&container->hourmeter_ao);
	container->hourmeter_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitDigitalInputSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->digital_input_initialized)
	{
		return ERR_INVALID_STATE;
	}

	DigitalInputAO_Deinit(&container->digital_input_ao);
	DigitalInputAdapter_Deinit(&container->digital_input);

	container->digital_input_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitRelaySubsystem(DependencyContainer_t *container)
{
	if (!container || !container->relay_initialized)
	{
		return ERR_INVALID_STATE;
	}

	RelayAO_Deinit(&container->relay_ao);
	RelayAdapter_Deinit(&container->relay_adapter);
	CycleScheduler_Deinit(&container->cycle_scheduler);

	container->relay_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitTimeSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->time_initialized)
	{
		return ERR_INVALID_STATE;
	}

	TimeAdapter_Deinit(&container->time_adapter);

	container->time_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitGPSSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->gps_initialized)
	{
		return ERR_INVALID_STATE;
	}

	GPSAo_Deinit(&container->gps_ao);
	GPSAdapter_Deinit(&container->gps_adapter);

	container->gps_initialized = false;

	return ERR_OK;
}

Result_t DI_DeinitStorageSubsystem(DependencyContainer_t *container)
{
	if (!container || !container->storage_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* ✅ Phase 4.7: StorageCoordinatorAO v3.0 removed - only base adapters remain */
	/* EventLogAO lifecycle: Stop → WaitStopped → Deinit (Pattern B) */
	EventLogAO_Stop(&container->event_log_ao);
	EventLogAO_WaitStopped(&container->event_log_ao, 1000); /* 1 second timeout */
	EventLogAO_Deinit(&container->event_log_ao);

	/* Note: EEPROM driver is not deinitialized (no Destroy function in interface) */

	container->storage_initialized = false;

	return ERR_OK;
}

Result_t DI_Container_Deinit(DependencyContainer_t *container)
{
	if (!container || !container->is_initialized)
	{
		return ERR_INVALID_STATE;
	}

	/* Stop all Active Objects first */
	DI_StopActiveObjects(container);

	/* Deinitialize in reverse order */
	if (container->digital_input_initialized)
	{
		DI_DeinitDigitalInputSubsystem(container);
	}

	if (container->relay_initialized)
	{
		DI_DeinitRelaySubsystem(container);
	}

	if (container->time_initialized)
	{
		DI_DeinitTimeSubsystem(container);
	}

	if (container->gps_initialized)
	{
		DI_DeinitGPSSubsystem(container);
	}

	if (container->storage_initialized)
	{
		DI_DeinitStorageSubsystem(container);
	}

	if (container->buzzer_initialized)
	{
		DI_DeinitBuzzerSubsystem(container);
	}

	if (container->hourmeter_initialized)
	{
		DI_DeinitHourmeterSubsystem(container);
	}

	if (container->network_initialized)
	{
		DI_DeinitNetworkSubsystem(container);
	}

	if (container->wifi_initialized)
	{
		DI_DeinitWifiSubsystem(container);
	}

	container->is_initialized = false;

	return ERR_OK;
}

/*============================================================================*
 * GETTERS
 *============================================================================*/

ITimeSource *DI_GetTimeSource(DependencyContainer_t *container)
{
	if (!container || !container->time_initialized)
	{
		return NULL;
	}

	return TimeAdapter_GetTimeSourceInterface(&container->time_adapter);
}

IBatteryMonitor *DI_GetBatteryMonitor(DependencyContainer_t *container)
{
	if (!container || !container->battery_monitor_initialized)
	{
		return NULL;
	}

	return BQ27441Adapter_GetInterface(&container->battery_monitor);
}

IDeviceIdentity *DI_GetDeviceIdentity(DependencyContainer_t *container)
{
	if (!container || !container->device_identity_initialized)
	{
		return NULL;
	}

	return BspDeviceIdentity_GetInterface(&container->device_identity_bsp);
}

Result_t DI_InitLicenseSubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}
	if (!container->storage_v2_initialized || !container->time_initialized)
	{
		return ERR_INVALID_STATE;
	}

	LicenseServiceConfig_t cfg = {
		.config_storage = DI_GetConfigStorage(container),
		.time_source = DI_GetTimeSource(container),
	};

	Result_t res = LicenseService_Init(&container->license_service, &cfg);
	if (res != ERR_OK)
	{
		return res;
	}

	container->license_initialized = true;
	return ERR_OK;
}

ILicenseStatus *DI_GetLicenseStatus(DependencyContainer_t *container)
{
	if (!container || !container->license_initialized)
	{
		return NULL;
	}
	return LicenseService_GetInterface(&container->license_service);
}

/**
 * @brief Initializes the OTA Update subsystem (FirmwareUpdateStrategy, ExternalLoaderStrategy, UpdateManager).
 *
 * @pre container->bsp_initialized == true (I_EXT_FLASH available).
 * @pre container->storage_v2_initialized == true (ICycloneBootOps available).
 *
 * @param container DI container.
 * @return ERR_OK on success, ERR_NULL_POINTER, ERR_INVALID_STATE or init error.
 */
Result_t DI_InitUpdateSubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}
	if (!container->bsp_initialized)
	{
		return ERR_INVALID_STATE;
	}

	Result_t res;
	ILogger *logger = DI_GetLogger(container);

	/* ── Inyectar logger en adapters de OTA (se inicializaron sin logger en BSP) ── */
	CycloneCryptoVerifierAdapter_SetLogger(logger);
	CycloneBootExtFlashAdapter_BindLogger(logger);

	/* ── Create mutex for UpdateManager ──────────────────────────────────── */
	res = os_mutex_create(&container->update_manager_mutex, "mutex UpdateManager");
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "UpdateManager mutex create failed (%d)", res);
		}
		return res;
	}

	/* ── Initialize FirmwareUpdateStrategy (CycloneBOOT wrapper) ─────────── */
	FirmwareUpdateStrategyConfig_t fw_cfg = {
		.cboot_ops = container->bsp->i_cyclone_boot_ops,
		.flash_iface = container->bsp->ext_flash,
		.flash_handle = container->ext_flash_handle,
		.logger = logger,
	};

	res = FirmwareUpdateStrategy_Init(&container->firmware_update_strategy, &fw_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "FirmwareUpdateStrategy init failed (%d)", res);
		}
		os_mutex_delete(container->update_manager_mutex);
		return res;
	}

	/* ── Initialize ExternalLoaderStrategy (direct flash write + ECDSA) ──── */
	ExternalLoaderStrategyConfig_t ext_cfg = {
		.flash_iface = container->bsp->ext_flash,
		.flash_handle = container->ext_flash_handle,
		.crypto = container->bsp->i_crypto_verifier,
		.sector_size = 4096U, /* W25Q128 sector size = 4 KB */
		.logger = logger,
	};

	res = ExternalLoaderStrategy_Init(&container->external_loader_strategy, &ext_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "ExternalLoaderStrategy init failed (%d)", res);
		}
		os_mutex_delete(container->update_manager_mutex);
		return res;
	}

	/* ── Initialize UpdateManager facade ─────────────────────────────────── */
	UpdateManagerConfig_t mgr_cfg = {
		.fw_strategy = FirmwareUpdateStrategy_GetInterface(&container->firmware_update_strategy),
		.ext_strategy = ExternalLoaderStrategy_GetInterface(&container->external_loader_strategy),
		.mutex = container->update_manager_mutex,
	};

	res = UpdateManager_Init(&container->update_manager, &mgr_cfg);
	if (res != ERR_OK)
	{
		if (Logger_IsValid(logger))
		{
			LOG_ERROR(logger, "DI", "UpdateManager init failed (%d)", res);
		}
		os_mutex_delete(container->update_manager_mutex);
		return res;
	}

	container->update_initialized = true;
	return ERR_OK;
}

IUpdateManager_t *DI_GetUpdateManager(DependencyContainer_t *container)
{
	if (!container || !container->update_initialized)
	{
		return NULL;
	}
	return UpdateManager_GetInterface(&container->update_manager);
}

IRelayController *DI_GetRelayController(DependencyContainer_t *container)
{
	if (!container || !container->relay_initialized)
	{
		return NULL;
	}

	return RelayAdapter_GetInterface(&container->relay_adapter);
}

IGPSSource *DI_GetGPSSource(DependencyContainer_t *container)
{
	if (!container || !container->gps_initialized)
	{
		return NULL;
	}

	return GPSAdapter_GetInterface(&container->gps_adapter);
}

IDigitalInputSource *DI_GetDigitalInputSource(DependencyContainer_t *container)
{
	if (!container || !container->digital_input_initialized)
	{
		return NULL;
	}

	return DigitalInputAdapter_GetInterface(&container->digital_input);
}

/**
 * ❌ Phase 4.8: DI_GetConfigStorage() removed (v3.0 legacy getter)
 * @note All consumers migrated:
 *   - UI screens → StorageCoordinatorConfigAdapter_v2 (Phase 4.6)
 *   - GPS/Relay/Buzzer → StorageCoordinatorAO_v2 (Phase 4.3)
 *   - Logging → StorageCoordinatorAdapter_v2 (Phase 4.5)
 *
 * ✅ Phase 4.10: DI_GetConfigStorage() RESTORED
 * @note Violation DIP detected: DI Container was calling StorageCoordinatorAO_v2 directly
 * @note Correct arch: DI Container → IConfigStorage → Adapter → AO
 */
IConfigStorage *DI_GetConfigStorage(DependencyContainer_t *container)
{
	if (!container || !container->storage_v2_initialized)
	{
		return NULL;
	}

	return StorageCoordinatorConfigAdapter_v2_GetInterface(&container->config_storage_adapter_v2);
}

IModularConfigStorage *DI_GetModularConfigStorage(DependencyContainer_t *container)
{
	if (!container || !container->storage_v2_initialized)
	{
		return NULL;
	}

	return ModularConfigStorageAdapter_GetInterface(&container->modular_config_storage);
}

BuzzerNotificationService_t *DI_GetBuzzerNotificationService(DependencyContainer_t *container)
{
	if (!container || !container->buzzer_initialized)
	{
		return NULL;
	}

	return &container->buzzer_notification;
}

IBuzzerControl *DI_GetBuzzerControl(DependencyContainer_t *container)
{
	if (!container || !container->buzzer_initialized)
	{
		return NULL;
	}

	return BuzzerAdapter_GetInterface(&container->buzzer_adapter);
}

/**
 * @brief Obtiene interfaz ILogger (debug logs → UART/SWO)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return Puntero a ILogger, o NULL si logging no está inicializado
 *
 * @note ILogger es thread-safe (lwprintf usa TX_MUTEX automático)
 * @note Uso: ILogger *log = DI_GetLogger(container); LOG_INFO(log, "TAG", "Message");
 */
ILogger *DI_GetLogger(DependencyContainer_t *container)
{
	if (!container || !container->logging_initialized)
	{
		return NULL;
	}

	return ElogAdapter_GetInterface(&container->logger_adapter);
}

/**
 * @brief Obtiene interfaz IEventNotifier (eventos → EEPROM)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return Puntero a IEventNotifier, o NULL si logging no está inicializado
 *
 * @note IEventNotifier es ISR-safe (non-blocking, queue-based)
 * @note Uso: EventNotifier_NotifyEvent(notifier, timestamp, EVENT_TYPE_X, EVENT_SEVERITY_Y, info, params);
 */
IEventNotifier *DI_GetEventNotifier(DependencyContainer_t *container)
{
	if (!container || !container->logging_initialized)
	{
		return NULL;
	}

	/* ✅ Phase 4.5: Return v2 adapter (direct EEPROM writes, no coordinator queue) */
	return StorageCoordinatorAdapter_v2_GetInterface(&container->event_adapter_v2);
}

/**
 * @brief Obtiene interfaz IEventLogStorage (circular buffer de eventos en EEPROM)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return Puntero a IEventLogStorage, o NULL si storage no está inicializado
 *
 * @note IEventLogStorage provee acceso directo al log persistente (100 eventos max)
 * @note Soporta Observer Pattern: Subscribe callbacks para notificaciones de nuevos eventos
 * @note Operaciones son sincrónicas (bloquean durante I2C write/read)
 * @note Uso: EventLogStorage_Append(), EventLogStorage_ReadByIndex(), EventLogStorage_GetCount()
 *
 * @warning NO usar desde ISR context (operaciones I2C bloquean ~10ms)
 */
IEventLogStorage *DI_GetEventLogStorage(DependencyContainer_t *container)
{
	if (!container || !container->storage_initialized)
	{
		return NULL;
	}

	return EventLogAO_GetInterface(&container->event_log_ao);
}

/**
 * @brief Obtiene interfaz IWifiTransport (ESP-Hosted SPI transport)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return Puntero a IWifiTransport, o NULL si ESP-Hosted no está inicializado
 *
 * @note IWifiTransport provee abstracción sobre SPI/SDIO/UART hardware
 * @note Permite inyección de dependencias (DIP) para el framework ESP-Hosted
 * @note Thread-safety: La implementación subyacente gestiona concurrencia
 * @note Uso: IWifiTransport *transport = DI_GetWifiTransport(container);
 *           WifiTransport_Transmit(transport, data, len);
 */
IWifiTransport *DI_GetWifiTransport(DependencyContainer_t *container)
{
	if (!container || !container->wifi_initialized)
	{
		return NULL;
	}

	return container->wifi_transport;
}

/**
 * @brief Obtiene interfaz IWifiControl_t (plano de control WiFi)
 *
 * @param[in] container Contenedor de dependencias
 *
 * @return IWifiControl_t* si ESP-Hosted está inicializado, NULL en caso contrario
 *
 * @note Abstracción sobre ctrl_api.c: SetMode, ConnectAP, GetMacAddr, eventos
 * @note Thread-safety: Las operaciones de control son sincrónicas (bloquean hasta resp ESP32)
 */
IWifiControl_t *DI_GetWifiControl(DependencyContainer_t *container)
{
	if (!container || !container->wifi_initialized)
	{
		return NULL;
	}

	return container->wifi_control;
}

IWifiModule_t *DI_GetWifiModule(DependencyContainer_t *container)
{
	if (!container || !container->wifi_enable_service_initialized)
	{
		return NULL;
	}

	return container->wifi_module;
}

WifiEnableService_t *DI_GetWifiEnableService(DependencyContainer_t *container)
{
	if (!container || !container->wifi_enable_service_initialized)
	{
		return NULL;
	}

	return &s_wifi_enable_service;
}

HourmeterAO_t *DI_GetHourmeter(DependencyContainer_t *container)
{
	if (!container || !container->hourmeter_initialized)
	{
		return NULL;
	}

	return &container->hourmeter_ao;
}

I_Display *DI_GetDisplay(DependencyContainer_t *container)
{
	if (!container || !container->ui_initialized)
	{
		return NULL;
	}

	return container->ui_ao.display_if;
}

DisplayBacklightService_t *DI_GetBacklightService(DependencyContainer_t *container)
{
	if (!container || !container->ui_initialized)
	{
		return NULL;
	}

	return &container->backlight_service;
}
/*============================================================================*
 * TEMPERATURE SENSOR SUBSYSTEM
 *============================================================================*/

/**
 * @brief Inicializa el sensor de temperatura interno del MCU (ADC1).
 *
 * @details
 * El BSP ya llamó BspAdcTemp_Init(&hadc1) en BSP_Init(), por lo que aquí
 * solo se recupera la interfaz ya construida y se registra en el container.
 * Diseñado como non-fatal: si BspAdcTemp_GetInterface() retorna NULL
 * (ADC init falló), el flag queda false y el getter retorna NULL.
 */
Result_t DI_InitTemperatureSensorSubsystem(DependencyContainer_t *container)
{
	if (container == NULL)
	{
		return ERR_NULL_POINTER;
	}

	/* Interface is already initialised by BSP_Init() → BspAdcTemp_Init().
	 * We only need to confirm it is available. */
	ITemperatureSensor_t *iface = BspAdcTemp_GetInterface();
	if (iface == NULL)
	{
		ILogger *logger = DI_GetLogger(container);
		if (Logger_IsValid(logger))
		{
			LOG_WARN(logger, "DI", "Temperature sensor not available (ADC1 init failed)");
		}
		container->temperature_sensor_initialized = false;
		return ERR_OK; /* Non-fatal */
	}

	container->temperature_sensor_initialized = true;
	return ERR_OK;
}

ITemperatureSensor_t *DI_GetTemperatureSensor(DependencyContainer_t *container)
{
	if (!container || !container->temperature_sensor_initialized)
	{
		return NULL;
	}

	return BspAdcTemp_GetInterface();
}
