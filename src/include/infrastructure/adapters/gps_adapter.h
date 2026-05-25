#ifndef GPS_ADAPTER_H
#define GPS_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"
#include "interfaces/i_uart.h"
#include "interfaces/i_exti.h"
#include "interfaces/i_gpio.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_gps_ingestor.h"
#include "interfaces/i_gps_control.h"
#include "interfaces/i_config_storage.h" /* SystemConfig_t */
#include "lwgps/lwgps.h"
#include "lwrb/lwrb.h"
#include "osal/osal.h"
#ifndef GPS_ADAPTER_RX_BUFFER_SIZE
#define GPS_ADAPTER_RX_BUFFER_SIZE (1024 + 256U) /**< Tamaño del buffer circular UART. */
#endif
/**
 * @brief Configuración del GPSAdapter.
 */
typedef struct GPSAdapterConfig
{
    uint32_t pps_minimum_valid; /**< PPS consecutivos requeridos para considerar la hora confiable. */

    /* ACTION-007 Phase 4: UTC offset configuration */
    int8_t utc_offset_index; /**< Índice en array UTC_OFFSETS [0..37], default 14 (UTC+0). */
    int16_t seconds_offset;  /**< Offset adicional en segundos (±10s). */

    /* Hardware control (antenna selection + reset) */
    uint8_t antenna_type;      /**< 0=interna, 1=externa. */
    I_GPIO *gpio;              /**< GPIO interface para control hardware. */
    GPIO_Port_t rf_ctrl1_port; /**< Puerto RF_CTRL1 (antenna switch). */
    GPIO_Pin_t rf_ctrl1_pin;   /**< Pin RF_CTRL1. */
    GPIO_Port_t rf_ctrl2_port; /**< Puerto RF_CTRL2 (antenna switch). */
    GPIO_Pin_t rf_ctrl2_pin;   /**< Pin RF_CTRL2. */
    GPIO_Port_t gps_rst_port;  /**< Puerto GPS_RST (reset). */
    GPIO_Pin_t gps_rst_pin;    /**< Pin GPS_RST. */
} GPSAdapterConfig_t;

/**
 * @brief Implementación concreta de IGPSSource sobre UART + PPS + lwGPS.
 */
typedef struct GPSAdapter
{
    IGPSSource iface;    /**< Interfaz pública IGPSSource. */
    IGPSIngestor ingest; /**< Interfaz para ingestión y procesamiento. */
    IGPSControl control; /**< Interfaz para control de hardware. */
    I_UART *uart;        /**< Dependencia UART. */
    I_EXTI *exti;        /**< Dependencia EXTI para PPS. */
    GPIO_Pin_t pps_pin;  /**< Pin asociado al PPS. */

    PPSCallback_t pps_callback; /**< Callback notificado cuando PPS es válido. */
    void *pps_context;          /**< Contexto del callback PPS. */

    GPSPosition_t last_position; /**< Última posición recibida. */
    DateTime_t last_time;        /**< Última hora UTC recibida. */
    GPSFixStatus_t fix_status;   /**< Estado de fix actual. */
    bool has_time;               /**< Indica si la hora es válida. */
    bool has_position;           /**< Indica si la posición es válida. */
    uint32_t pps_valid_count;    /**< Contador de PPS válidos consecutivos. */
    uint32_t pps_minimum_valid;  /**< Umbral de PPS válidos requerido. */

    /* ACTION-007 Phase 4: UTC offset configuration */
    int8_t utc_offset_index; /**< Índice UTC offset aplicado al tiempo GPS. */
    int16_t seconds_offset;  /**< Offset en segundos aplicado al tiempo GPS. */

    /* Hardware control (antenna selection + reset) */
    uint8_t antenna_type; /**< 0=interna, 1=externa. */
    I_GPIO *gpio;         /**< GPIO interface para control hardware. */
    GPIO_Port_t rf_ctrl1_port;
    GPIO_Pin_t rf_ctrl1_pin;
    GPIO_Port_t rf_ctrl2_port;
    GPIO_Pin_t rf_ctrl2_pin;
    GPIO_Port_t gps_rst_port;
    GPIO_Pin_t gps_rst_pin;

    /* BLOCKER 1: Integración lwGPS + lwRB */
    lwgps_t gps_parser;                                 /**< Parser NMEA lwGPS. */
    lwrb_t rx_ringbuffer;                               /**< Buffer circular para datos UART. */
    uint8_t rx_buffer_data[GPS_ADAPTER_RX_BUFFER_SIZE]; /**< Almacenamiento del buffer circular. */

    /* Estado de inicialización */
    bool is_initialized; /**< Flag de inicialización del adapter. */
} GPSAdapter;

Result_t GPSAdapter_Init(GPSAdapter *self,
                         I_UART *uart,
                         I_EXTI *exti,
                         GPIO_Pin_t pps_pin,
                         const GPSAdapterConfig_t *config);

/**
 * @brief De-inicializa GPSAdapter y libera recursos
 * @param[in] self Puntero al adapter
 * @return ERR_OK si exitoso
 */
Result_t GPSAdapter_Deinit(GPSAdapter *self);

/**
 * @brief Consume datos del buffer circular y ejecuta el parser NMEA.
 * @note  Esta función debe llamarse desde el thread de procesamiento (GpsAO).
 *
 * @param[in] self  Adapter GPS (no NULL).
 * @return ERR_OK si se procesaron datos, ERR_IDLE si no había datos, o código de error.
 */
Result_t GPSAdapter_Process(GPSAdapter *self);

/**
 * @brief Procesa un byte NMEA desde UART (interno o para tests).
 * @note  Esta función procesa un único byte. Usar GPSAdapter_Process para consumir el buffer.
 *
 * @param[in] self  Adapter GPS (no NULL).
 * @param[in] byte  Byte recibido del UART.
 *
 * @return ERR_OK si procesado, ERR_NULL_POINTER si self es NULL.
 */
Result_t GPSAdapter_ProcessNMEAByte(GPSAdapter *self, uint8_t byte);

/**
 * @brief Escribe datos recibidos (UART ISR) en el buffer circular del adapter.
 *
 * @param[in] self  Adapter GPS (no NULL).
 * @param[in] data  Buffer de datos UART.
 * @param[in] len   Longitud del buffer.
 *
 * @return ERR_OK si copiado al buffer, ERR_NULL_POINTER si self o data es NULL.
 */
Result_t GPSAdapter_ProcessRxBuffer(GPSAdapter *self, const uint8_t *data, uint16_t len);

/**
 * @brief Maneja un flanco PPS (llamado por EXTI callback o manualmente en tests).
 */
Result_t GPSAdapter_OnPPS(GPSAdapter *self);

/**
 * @brief Aplica configuración de hardware (selección antena + reset).
 * @note Thread-safe. Debe ejecutarse después de Init o en cambio de config.
 *
 * @param[in] self  Adapter GPS (no NULL).
 * @return ERR_OK si configuración aplicada correctamente.
 */
Result_t GPSAdapter_ApplyHardwareConfig(GPSAdapter *self);

/**
 * @brief Ejecuta secuencia de reset hardware del GPS (LOW → delay → HIGH).
 * @note Thread-safe. Bloquea brevemente (~100ms) durante reset pulse.
 * @warning NO llamar desde ISR.
 *
 * @param[in] self  Adapter GPS (no NULL).
 * @return ERR_OK si reset ejecutado correctamente, ERR_NULL_POINTER si self es NULL.
 *
 * @note Casos de uso:
 *       - Recovery de GPS hang (no responde NMEA)
 *       - Después de cambio de antena física
 *       - Debugging/troubleshooting manual
 */
Result_t GPSAdapter_Reset(GPSAdapter *self);

/**
 * @brief Obtiene la interfaz polimórfica IGPSSource asociada.
 */
IGPSSource *GPSAdapter_GetInterface(GPSAdapter *self);

/**
 * @brief Obtiene la interfaz de ingestión asociada (buffer + procesamiento).
 */
IGPSIngestor *GPSAdapter_GetIngestInterface(GPSAdapter *self);

/**
 * @brief Obtiene la interfaz de control de hardware (reset, config).
 */
IGPSControl *GPSAdapter_GetControlInterface(GPSAdapter *self);

#endif /* GPS_ADAPTER_H */
