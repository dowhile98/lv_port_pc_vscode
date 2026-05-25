/**
 * @file dependency_container.h
 * @brief Dependency Injection (DI) Container - Raíz de Composición del Sistema.
 * @version 2.1.0
 * @date 2026-03-17
 *
 * @details
 * Este módulo actúa como el Composition Root (Raíz de Composición) del sistema TCS CICX1.
 * Su responsabilidad es centralizar la creación, configuración y cableado (wiring)
 * de todos los componentes, asegurando que las dependencias se inyecten correctamente
 * siguiendo el principio de Inversión de Dependencia (DIP).
 *
 * Estructura de Capas gestionada:
 * 1. Infrastructure (Adapters/Drivers): Implementaciones concretas de hardware.
 * 2. Application (Active Objects): Hilos de ejecución y lógica de flujo.
 * 3. Domain (Services): Lógica pura de negocio inyectada en los AOs.
 */

#ifndef DEPENDENCY_CONTAINER_H
#define DEPENDENCY_CONTAINER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "stm32u5/bsp_init.h"

/* --- Interfaces (Contratos) --- */
#include "interfaces/i_buzzer_control.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_display.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_modular_config_storage.h"
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"
#include "interfaces/i_wifi_transport.h"
#include "interfaces/i_wifi_module.h"
#include "interfaces/i_memory_allocator.h"
#include "interfaces/i_event_log_storage.h"

/* --- Infrastructure Adapters (Implementaciones) --- */
#include "infrastructure/adapters/buzzer_adapter.h"
#include "infrastructure/adapters/gps_adapter.h"
#include "infrastructure/adapters/relay_adapter.h"
#include "infrastructure/adapters/time_adapter.h"
#include "infrastructure/adapters/bq27441_adapter.h"
#include "stm32u5/bsp_stm32u5_device_identity.h"
#include "infrastructure/adapters/storage/modular_config_storage_adapter.h"
#include "infrastructure/adapters/storage/storage_coordinator_adapter_v2.h"
#include "infrastructure/adapters/storage/storage_coordinator_config_adapter_v2.h"
#include "infrastructure/adapters/digital_input_adapter.h"
#include "infrastructure/adapters/elog_adapter.h"
#include "infrastructure/adapters/esp_hosted/esp_spi_transport_adapter.h"
#include "infrastructure/adapters/wifi/wifi_control_adapter.h"
#include "infrastructure/adapters/network/http_server_adapter.h"
#include "infrastructure/adapters/network/http_route_table.h"
#include "infrastructure/adapters/network/api/system_info_api_handler.h"
#include "infrastructure/adapters/network/api/wifi_api_handler.h"
#include "infrastructure/adapters/network/api/admin_api_handler.h"
#include "infrastructure/adapters/network/api/auth_api_handler.h"
#include "infrastructure/adapters/network/api/config_reset_api_handler.h"
#include "infrastructure/adapters/network/api/contact_api_handler.h"
#include "infrastructure/adapters/network/api/general_config_api_handler.h"
#include "infrastructure/adapters/network/api/gps_api_handler.h"
#include "infrastructure/adapters/network/api/health_api_handler.h"
#include "infrastructure/adapters/network/api/history_api_handler.h"
#include "infrastructure/adapters/network/api/interruptor_config_api_handler.h"
#include "infrastructure/adapters/network/api/upload_auth_api_handler.h"
#include "infrastructure/adapters/network/api/upload_ota_api_handler.h"
#include "infrastructure/adapters/network/api/upload_fs_api_handler.h"
#include "infrastructure/adapters/network/api/embedded_web_handler.h"
#include "infrastructure/adapters/network/cyclone_network_stack_adapter.h"
#include "infrastructure/adapters/network/cyclone_network_port_adapter.h"
#include "infrastructure/adapters/network/cyclone_usb_rndis_port_adapter.h"

/* --- Application Layer (Active Objects) --- */
#include "application/activeobjects/eventlog_ao.h"
#include "application/activeobjects/buzzer_ao.h"
#include "application/activeobjects/gps_ao.h"
#include "application/activeobjects/relay_ao.h"
#include "application/activeobjects/storage_coordinator_ao_v2.h"
#include "application/activeobjects/digital_input_ao.h"
#include "application/activeobjects/network_ao.h"
#include "application/activeobjects/ui_ao.h"
#include "application/activeobjects/hourmeter_ao.h"

/* --- Presentation Layer --- */
#include "infrastructure/presentation/screens/eez_screen_router.h"
#include "infrastructure/presentation/input_router.h"

/* --- Domain Layer (Services) --- */
#include "domain/services/buzzer_notification_service.h"
#include "domain/services/cycle_scheduler.h"
#include "application/services/led_status_service.h"
#include "application/services/wifi_enable_service.h"
#include "application/services/side_button_service.h"
#include "application/services/gps_antenna_auto_switch_service.h"
#include "domain/services/pps_dispatcher.h"
#include "domain/services/license_service.h"
#include "application/services/display_backlight_service.h"

/* --- Domain Layer (Update/OTA) --- */
#include "domain/update/firmware_update_strategy.h"
#include "domain/update/external_loader_strategy.h"
#include "domain/update/update_manager.h"

/* --- Drivers --- */
#include "interfaces/i_ext_eeprom.h"

    /*============================================================================*
     * CONFIGURATION CONSTANTS (Recursos Estáticos)
     *============================================================================*/

#define DI_GPS_AO_STACK_SIZE 1024 * 4
#define DI_GPS_UART_DMA_BUFFER_SIZE 1024
#define DI_RELAY_AO_STACK_SIZE 1024 * 4
#define DI_RELAY_AO_QUEUE_SIZE 16
/* NOTE: DI_STORAGE_AO_V2_STACK_SIZE and DI_STORAGE_AO_V2_QUEUE_BUFFER_SIZE removed:
 *       StorageCoordinatorAO_v2_t owns its own internal thread_stack[4096] — no
 *       external stack/queue buffers needed in the DI container. */
#define DI_DIGITALINPUT_AO_STACK_SIZE (1024 * 8)
/* NOTE: DI_BUZZER_AO_STACK_SIZE removed: BuzzerAO_t owns its own internal
 *       stack[BUZZER_AO_THREAD_STACK_SIZE=2048]; no external buffer needed. */
#define DI_BUZZER_AO_QUEUE_SIZE 8

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Estructura del Contenedor de Dependencias.
     *
     * Contiene las instancias de todos los componentes del sistema.
     * La memoria es pre-asignada estáticamente para evitar fragmentación.
     */
    typedef struct
    {
        /* --- Interfaces del BSP (Inyectadas) --- */
        const BSP_Interfaces_t *bsp;

        /* --- Drivers de Bajo Nivel --- */
        I_EXT_EEPROM *eeprom_driver; /**< Instancia del driver M24M01E */

        /* --- Subsistema Flash Externo (OctoSPI Memory-Mapped) --- */
        void *ext_flash_handle; /**< OctoSPI handle activo tras Init + EnableMemoryMapped */

        /* --- Subsistema de Almacenamiento (v4.0 Modular) --- */
        EventLogAO_t event_log_ao;
        ModularConfigStorageAdapter_t modular_config_storage;
        StorageCoordinatorAO_v2_t storage_coordinator_v2;               /**< Coordinador asíncrono con cache write-through */
        StorageCoordinatorConfigAdapter_v2_t config_storage_adapter_v2; /**< Implementación de IConfigStorage */

        /* --- Subsistema GPS --- */
        GPSAdapter gps_adapter;
        GPSAo_t gps_ao;
        GpsAntennaAutoSwitchService_t gps_antenna_auto_switch_service; /**< Auto-switch antenna service */

        /* --- Subsistema de Tiempo --- */
        TimeAdapter_t time_adapter;

        /* --- Subsistema de Energía (Battery Monitor) --- */
        BQ27441Adapter_t battery_monitor;

        /* --- Subsistema de Identidad de Dispositivo (96-bit UID) --- */
        BspDeviceIdentity_t device_identity_bsp;

        /* --- Subsistema de Licencia --- */
        LicenseService_t license_service;

        /* --- Subsistema de Actualizaciones OTA --- */
        FirmwareUpdateStrategy_t firmware_update_strategy; /**< OTA firmware via CycloneBOOT */
        ExternalLoaderStrategy_t external_loader_strategy; /**< OTA resources via direct flash write */
        UpdateManager_t update_manager;                    /**< Facade: selecciona estrategia + mutex */
        os_mutex_t update_manager_mutex;                   /**< Mutex para serializar updates */

        /* --- Subsistema de Relé (Control de Interrupción) --- */
        RelayAdapter_t relay_adapter;
        CycleSchedulerService_t cycle_scheduler;
        RelayAO_t relay_ao;

        /* --- Subsistema de Entradas Digitales --- */
        DigitalInputAdapter_t digital_input;
        DigitalInputAO_t digital_input_ao;

        /* --- Subsistema de Audio (Buzzer) --- */
        BuzzerAdapter_t buzzer_adapter;
        BuzzerAO_t buzzer_ao;
        BuzzerNotificationService_t buzzer_notification;

        /* --- Subsistema de Horómetro (Accumulation + EEPROM Persistence) --- */
        HourmeterAO_t hourmeter_ao;

        /* --- Subsistema de Diagnóstico (Logging) --- */
        ElogAdapter_t logger_adapter;                    /**< Logs volátiles (UART/SWO) */
        StorageCoordinatorAdapter_v2_t event_adapter_v2; /**< Notificador de eventos (EEPROM) */

        /* --- Subsistema WiFi (ESP-Hosted) --- */
        ESP_SpiTransportAdapter_t esp_transport_adapter; /**< Adapter concreto de transporte SPI */
        IWifiTransport *wifi_transport;                  /**< Interfaz de transporte (DIP) */
        WifiControlAdapter_t wifi_control_adapter;       /**< Adapter de control WiFi */
        IWifiControl_t *wifi_control;                    /**< Interfaz de control (DIP) */
        PPSDispatcher_t pps_dispatcher;                  /**< Distribuidor de señal PPS GPS */
        IWifiModule_t *wifi_module;                      /**< WiFi module power control service */
        SideButtonService_t side_button_service;         /**< Side button multi-function service */

        /* --- Capa de Red (CycloneTCP) --- */
        HttpRouteTable_t http_route_table;
        SystemInfoApiHandler_t system_info_handler;               /**< GET /api/interruptor/system-information */
        WifiApiHandler_t wifi_api_handler;                        /**< GET/POST /api/interruptor/wifi-manager */
        AdminApiHandler_t admin_api_handler;                      /**< GET/POST /api/interruptor/admin */
        AuthApiHandler_t auth_api_handler;                        /**< POST /api/interruptor/auth */
        ConfigResetApiHandler_t config_reset_api_handler;         /**< POST /api/configuration */
        ContactApiHandler_t contact_api_handler;                  /**< GET/POST /api/interruptor/contact-config */
        GeneralConfigApiHandler_t general_config_handler;         /**< GET/POST /api/interruptor/general-configuration */
        GpsApiHandler_t gps_api_handler;                          /**< GET/POST /api/interruptor/gps-configuration */
        HealthApiHandler_t health_api_handler;                    /**< ANY /api/health */
        HistoryApiHandler_t history_api_handler;                  /**< GET /api/interruptor/historical-events */
        InterruptorConfigApiHandler_t interruptor_config_handler; /**< GET/POST /api/interruptor/configuration */
        UploadAuthApiHandler_t upload_auth_api_handler;           /**< POST /api/upload/auth — Valida contraseña de actualización */
        UploadOtaApiHandler_t upload_ota_api_handler;             /**< POST /api/upload/ota (firmware OTA) */
        UploadFsApiHandler_t upload_fs_api_handler;               /**< POST /api/upload/fs  (resources OTA) */
        EmbeddedWebHandler_t embedded_web_handler;                /**< GET /update.html, GET /js/spark-md5.min.js */
        HttpServerAdapter_t http_server_adapter;
        CycloneTcpNetworkStackAdapter_t network_stack_adapter;
        CycloneNetworkPortAdapter_t sta_port_adapter;
        CycloneNetworkPortAdapter_t ap_port_adapter;
        CycloneUsbRndisPortAdapter_t rndis_port_adapter; /**< USB RNDIS gateway (netInterface[2]) */
        NetworkAO_t network_ao;

        /* --- Subsistema de Indicación LED --- */
        LedStatusService_t led_status_service; /**< Blink-pattern status LED (led_status[0]) */

        /* --- Subsistema de Interfaz de Usuario (LVGL) --- */
        UiAO_t ui_ao;                                /**< Hilo de renderizado LVGL */
        EezScreenRouter_t eez_router;                /**< Enrutador de pantallas (EEZ Studio) */
        InputRouter_t input_router;                  /**< Mediador de eventos de entrada */
        DisplayBacklightService_t backlight_service; /**< Fase 4: backlight timeout */

        /* --- Recursos Estáticos (Stacks y Buffers) --- */
        uint8_t gps_ao_stack[DI_GPS_AO_STACK_SIZE];
        uint8_t gps_uart_dma_buffer[DI_GPS_UART_DMA_BUFFER_SIZE];
        uint8_t relay_ao_stack[DI_RELAY_AO_STACK_SIZE];
        uint8_t relay_ao_queue_buf[DI_RELAY_AO_QUEUE_SIZE * sizeof(RelayAO_Event_t)];
        /* storage_ao_v2_stack / storage_ao_v2_queue_buf removed: StorageCoordinatorAO_v2_t
         * carries its own internal thread_stack[4096]; these were dead padding. */
        uint8_t digitalinput_ao_stack[DI_DIGITALINPUT_AO_STACK_SIZE];
        /* buzzer_ao_stack removed: BuzzerAO_t carries its own internal
         * stack[2048]; this was dead padding (config.stack_size is never
         * used as stack_ptr in BuzzerAO_Init). */

        /* --- Flags de Estado de Inicialización --- */
        bool is_initialized;
        bool bsp_initialized;
        bool storage_initialized;
        bool storage_v2_initialized;
        bool update_initialized;
        bool gps_initialized;
        bool logging_initialized;
        bool time_initialized;
        bool battery_monitor_initialized;
        bool device_identity_initialized;
        bool license_initialized;
        bool relay_initialized;
        bool digital_input_initialized;
        bool buzzer_initialized;
        bool hourmeter_initialized;
        bool temperature_sensor_initialized;  /**< true if ADC1 temperature sensor is ready */
        bool wifi_initialized;                /**< true if ESP-Hosted transport + WifiControlAdapter ready */
        bool wifi_enable_service_initialized; /**< true if WifiEnableService is ready */
        bool side_button_initialized;         /**< true if SideButtonService is ready */
        bool network_initialized;             /**< true if CycloneTCP stack + ports + NetworkAO ready */
        bool usb_rndis_initialized;           /**< true if USB RNDIS stack + adapter are ready */
        bool ui_initialized;
        bool led_status_initialized;         /**< true if LedStatusService is ready */
        bool blackligth_service_initialized; // BacklightService
        bool ext_flash_initialized;          /**< true if OctoSPI flash is in memory-mapped mode */
    } DependencyContainer_t;

    /*============================================================================*
     * PUBLIC API - CICLO DE VIDA DEL CONTENEDOR
     *============================================================================*/

    /**
     * @brief Inicializa el contenedor y vincula las interfaces de hardware (BSP).
     * @param[in,out] container Puntero al contenedor (alocado estáticamente).
     * @param[in] bsp Estructura de funciones del BSP obtenida de BSP_GetInterfaces().
     * @return Result_t ERR_OK si la estructura se limpió y vinculó correctamente.
     */
    Result_t DI_Container_Init(DependencyContainer_t *container, const BSP_Interfaces_t *bsp);

    /**
     * @brief Libera recursos y marca el contenedor como no inicializado.
     * @param[in] container Puntero al contenedor.
     * @return Result_t ERR_OK si exitoso.
     */
    Result_t DI_Container_Deinit(DependencyContainer_t *container);

    /*============================================================================*
     * PUBLIC API - INICIALIZACIÓN DE SUBSISTEMAS
     *============================================================================*/

    /**
     * @brief Inicializa el Almacenamiento (EEPROM + Configuración Modular).
     * Requiere que el BSP esté vinculado.
     */
    Result_t DI_InitStorageSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa GPS (Adapter + AO). Requiere Storage inicializado.
     */
    Result_t DI_InitGPSSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa Tiempo (RTC + Sincronización). Requiere GPS inicializado.
     */
    Result_t DI_InitTimeSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa Monitor de Batería (BQ27441). Requiere Storage inicializado.
     */
    Result_t DI_InitBatteryMonitorSubsystem(DependencyContainer_t *container);

    /**
     * @brief Initialise the device identity subsystem (BSP 96-bit UID reader).
     *
     * @pre  No prerequisites — reads factory ROM, may be called early.
     *
     * @param[in,out] container Container instance.
     * @return ERR_OK on success.
     */
    Result_t DI_InitDeviceIdentitySubsystem(DependencyContainer_t *container);

    /**
     * @brief Initialises the LicenseService.
     * @pre  DI_InitStorageSubsystem() (config_storage) and DI_InitTimeSubsystem() required.
     */
    Result_t DI_InitLicenseSubsystem(DependencyContainer_t *container);

    /**
     * @brief Returns the ILicenseStatus interface of the LicenseService.
     *        Returns NULL if licence subsystem is not initialised.
     */
    ILicenseStatus *DI_GetLicenseStatus(DependencyContainer_t *container);

    /**
     * @brief Initializes the OTA Update subsystem (FirmwareUpdateStrategy, ExternalLoaderStrategy, UpdateManager).
     *
     * Wires both update strategies and the mutex-protected UpdateManager facade.
     * Strategies use I_EXT_FLASH (for flash access), ICycloneBootOps (firmware OTA),
     * and ICryptoVerifier (ECDSA signature verification for resources).
     *
     * @pre  DI_InitBspSubsystem() (I_EXT_FLASH available).
     * @pre  DI_InitStorageSubsystem() (ICycloneBootOps available).
     *
     * @param[in,out] container Container instance.
     * @return ERR_OK on success, ERR_NULL_POINTER, ERR_INVALID_STATE or init error.
     */
    Result_t DI_InitUpdateSubsystem(DependencyContainer_t *container);

    /**
     * @brief Returns the IUpdateManager_t interface of the UpdateManager facade.
     *        Returns NULL if update subsystem is not initialized.
     *
     * @param[in] container Container instance.
     * @return IUpdateManager_t* or NULL.
     */
    IUpdateManager_t *DI_GetUpdateManager(DependencyContainer_t *container);

    /**
     * @brief Inicializa el Relé y Scheduler de Ciclos. Requiere Time y Storage.
     */
    Result_t DI_InitRelaySubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa Botones y Alarmas. Requiere Relay (para callbacks de seguridad).
     */
    Result_t DI_InitDigitalInputSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el Buzzer y el Servicio de Notificaciones.
     */
    Result_t DI_InitBuzzerSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el Horómetro (AO + EEPROM storage + condition callbacks).
     * @pre  DI_InitStorageSubsystem(), DI_InitGPSSubsystem(), DI_InitRelaySubsystem() deben estar listos.
     */
    Result_t DI_InitHourmeterSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el sensor de temperatura interno del MCU (ADC1).
     * @pre  BSP_Init() — ADC1 ya inicializado por MX_ADC1_Init() y BspAdcTemp_Init().
     * @note Non-fatal: la plataforma opera sin temperatura si el ADC falla (retorna ERR_OK).
     */
    Result_t DI_InitTemperatureSensorSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el Logger (ILogger → UART/ITM) sin Storage.
     * @note  NO inicializa IEventNotifier (eventos a EEPROM).
     * @note  Ya no requiere storage_initialized.
     */
    Result_t DI_InitLoggingSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa la Flash externa (OctoSPI) y la pone en modo Memory-Mapped.
     *
     * Obtiene el handle desde el BSP board profile, llama EXT_FLASH_Init() +
     * EXT_FLASH_EnableMemoryMapped() y guarda el handle en container->ext_flash_handle.
     * Debe llamarse ANTES del init de cualquier subsistema que lea desde la flash externa
     * (imágenes LVGL, LUT de zonas horarias, etc.).
     *
     * @pre  BSP_Init() completado.
     * @param[in,out] container  Container instance.
     * @return ERR_OK on success, ERR_ERROR if flash or memory-map init fails.
     */
    Result_t DI_InitExtFlashSubsystem(DependencyContainer_t *container);

    /**
     * @brief Returns the OctoSPI handle stored after DI_InitExtFlashSubsystem().
     * @return Non-NULL void* on success, NULL if subsystem was not initialised.
     */
    void *DI_GetExtFlashHandle(const DependencyContainer_t *container);

    /**
     * @brief Inicializa el Event Notifier (IEventNotifier → EEPROM).
     * @pre   DI_InitStorageSubsystem() debe haber sido llamado primero.
     */
    Result_t DI_InitEventNotifierSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el co-procesador WiFi (ESP-Hosted SPI transport + WifiControlAdapter).
     * @pre  DI_InitLoggingSubsystem() debe haber sido llamado primero.
     */
    Result_t DI_InitWifiSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el servicio de control de encendido/apagado del módulo WiFi.
     * @pre  DI_InitWifiSubsystem(), DI_InitStorageSubsystem(), DI_InitBuzzerSubsystem().
     */
    Result_t DI_InitWifiEnableSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa SideButtonService para boards con boton lateral.
     * @pre  DI_InitDigitalInputSubsystem(), DI_InitRelaySubsystem(), DI_InitGPSSubsystem(),
     *       DI_InitBuzzerSubsystem(), DI_InitStorageSubsystem(), DI_InitWifiEnableSubsystem().
     */
    Result_t DI_InitSideButtonSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa la pila de red (CycloneTCP + puertos STA/AP/RNDIS + NetworkAO + HTTP).
     * @pre  DI_InitWifiSubsystem() debe haber sido llamado (container->wifi_initialized).
     *       DI_InitUsbRndisSubsystem() si se desea el puerto USB RNDIS (opcional).
     */
    Result_t DI_InitNetworkSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa el puerto USB RNDIS (USB Device stack + CycloneTCP adapter).
     *
     * @details Llama a USBD_Init / USBD_RegisterClass / USBD_Start y registra
     *          `rndisDriver` en `netInterface[2]`. Saltado si
     *          `profile->usb_rndis.enabled == false`.
     *
     * @pre  DI_InitLoggingSubsystem() debe haber sido llamado primero.
     * @note La configuraci\u00f3n IP (INetworkPort_Configure) se realiza desde el
     *       contexto de tarea RTOS \u2014 no desde esta funci\u00f3n.
     *
     * @param[in,out] container Container instance.
     * @return ERR_OK on success (or skipped). ERR_ERROR on USB stack failure.
     */
    Result_t DI_InitUsbRndisSubsystem(DependencyContainer_t *container);

    /**
     * @brief Retorna la interfaz INetworkPort_t del adapter USB RNDIS.
     * @param[in] container Initialized container.
     * @return INetworkPort_t* or NULL if not initialized.
     */
    INetworkPort_t *DI_GetRndisPortInterface(DependencyContainer_t *container);

    /**
     * @brief Inicializa el servicio de indicación LED (led_status[0]).
     * @pre  DI_InitRelaySubsystem(), DI_InitGPSSubsystem(), DI_InitLicenseSubsystem() deben
     *       haber sido llamados para que las interfaces opcionales estén disponibles.
     * @note Non-fatal: el sistema opera sin indicación LED si el perfil deshabilita el pin.
     */
    Result_t DI_InitLedStatusSubsystem(DependencyContainer_t *container);

    /**
     * @brief Inicializa la Interfaz de Usuario (LVGL + AO). Requiere Digital Input.
     */
    Result_t DI_InitUISubsystem(DependencyContainer_t *container);

    /*============================================================================*
     * PUBLIC API - CABLEADO Y EJECUCIÓN
     *============================================================================*/

    /**
     * @brief Conecta los componentes mediante el registro de Observadores y Callbacks.
     * @note Debe llamarse una vez que TODOS los subsistemas necesarios estén configurados.
     */
    Result_t DI_WireComponents(DependencyContainer_t *container);

    /**
     * @brief Lanza la ejecución de todos los Active Objects (Threads).
     * El orden de inicio respeta las prioridades críticas (Relay > GPS > Net).
     */
    Result_t DI_StartActiveObjects(DependencyContainer_t *container);

    /**
     * @brief Detiene de forma controlada todos los Active Objects.
     */
    Result_t DI_StopActiveObjects(DependencyContainer_t *container);

    /*============================================================================*
     * PUBLIC API - DES-INICIALIZACIÓN (MANTENIMIENTO/SHUTDOWN)
     *============================================================================*/

    Result_t DI_DeinitNetworkSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitWifiSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitUISubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitLoggingSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitEventNotifierSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitBuzzerSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitHourmeterSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitDigitalInputSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitRelaySubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitTimeSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitGPSSubsystem(DependencyContainer_t *container);
    Result_t DI_DeinitStorageSubsystem(DependencyContainer_t *container);

    /*============================================================================*
     * GETTERS - INTERFACES PÚBLICAS (DIP Compliance)
     *============================================================================*/

    /** @brief Interfaz para consulta de fecha/hora sincronizada */
    ITimeSource *DI_GetTimeSource(DependencyContainer_t *container);

    /** @brief Interfaz para estado de batería */
    IBatteryMonitor *DI_GetBatteryMonitor(DependencyContainer_t *container);

    /** @brief Interfaz para identidad única del dispositivo (96-bit UID) */
    IDeviceIdentity *DI_GetDeviceIdentity(DependencyContainer_t *container);

    /** @brief Interfaz para control directo del relé */
    IRelayController *DI_GetRelayController(DependencyContainer_t *container);

    /** @brief Interfaz para datos GPS NMEA procesados */
    IGPSSource *DI_GetGPSSource(DependencyContainer_t *container);

    /** @brief Interfaz para eventos de botones y alarmas */
    IDigitalInputSource *DI_GetDigitalInputSource(DependencyContainer_t *container);

    /** @brief Interfaz de acceso modular a la EEPROM */
    IModularConfigStorage *DI_GetModularConfigStorage(DependencyContainer_t *container);

    /** @brief Interfaz de configuración general del sistema */
    IConfigStorage *DI_GetConfigStorage(DependencyContainer_t *container);

    /** @brief Servicio de notificaciones acústicas (Buzzer) */
    BuzzerNotificationService_t *DI_GetBuzzerNotificationService(DependencyContainer_t *container);

    /** @brief Interfaz de control de bajo nivel del buzzer */
    IBuzzerControl *DI_GetBuzzerControl(DependencyContainer_t *container);

    /** @brief Interfaz de depuración (UART/SWO) */
    ILogger *DI_GetLogger(DependencyContainer_t *container);

    /** @brief Interfaz para registro de eventos persistentes */
    IEventNotifier *DI_GetEventNotifier(DependencyContainer_t *container);

    /** @brief Interfaz de almacenamiento de event log (circular buffer EEPROM) */
    IEventLogStorage *DI_GetEventLogStorage(DependencyContainer_t *container);

    /** @brief Interfaz de transporte SPI para el módulo WiFi */
    IWifiTransport *DI_GetWifiTransport(DependencyContainer_t *container);

    /** @brief Interfaz de gestión WiFi (Asociación, Escaneo, AP) */
    IWifiControl_t *DI_GetWifiControl(DependencyContainer_t *container);

    /** @brief Interfaz de control de encendido/apagado del módulo WiFi */
    IWifiModule_t *DI_GetWifiModule(DependencyContainer_t *container);

    /** @brief Instancia del servicio WiFi Enable (para sincronización de estado) */
    WifiEnableService_t *DI_GetWifiEnableService(DependencyContainer_t *container);

    /** @brief Instancia del horómetro AO (thread-safe queries) */
    HourmeterAO_t *DI_GetHourmeter(DependencyContainer_t *container);

    /**
     * @brief Retorna la interfaz ITemperatureSensor del sensor interno del MCU.
     * @return NULL si el subsistema no fue inicializado o si el ADC falló.
     */
    ITemperatureSensor_t *DI_GetTemperatureSensor(DependencyContainer_t *container);

    /**
     * @brief Returns the I_Display interface (available after DI_InitUISubsystem).
     * @return NULL if display is disabled in board profile or UI not initialized.
     */
    I_Display *DI_GetDisplay(DependencyContainer_t *container);

    /**
     * @brief Returns the DisplayBacklightService (available after DI_InitUISubsystem).
     * @return NULL if display is disabled or UI not initialized.
     */
    DisplayBacklightService_t *DI_GetBacklightService(DependencyContainer_t *container);

#ifdef __cplusplus
}
#endif

#endif /* DEPENDENCY_CONTAINER_H */
