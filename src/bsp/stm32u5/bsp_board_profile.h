/**
 * @file bsp_board_profile.h
 * @brief Board-specific configuration profiles for hardware variants
 * @version 1.0.0
 * @date 2026-02-11
 *
 * Este archivo define el contrato de configuración de hardware.
 * Cada variante de placa implementa este contrato con datos concretos.
 *
 * @note Clean Architecture: Domain/Application NO dependen de este archivo.
 *       Solo BSP y DI Container lo incluyen.
 *
 * @author Tecna Smart Lab
 */

#ifndef BSP_BOARD_PROFILE_H
#define BSP_BOARD_PROFILE_H

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
#include "interfaces/i_gpio.h"
#include "interfaces/i_display.h"
    /*============================================================================*
     * BUTTON CONFIGURATION
     *============================================================================*/

    /**
     * @brief Identificadores lógicos de botones (agnósticos a hardware).
     *
     * Define IDs semánticos para botones. El mapeo físico (puerto/pin)
     * se define en el BoardProfile_t de cada variante.
     */
    typedef enum
    {
        BUTTON_ONOFF = 0, /**< Botón ON/OFF principal (PBOUT en V1) */
        BUTTON_ENTER,     /**< Botón Enter/Confirm (DB1 en V1) */
        BUTTON_UP,        /**< Botón Up/Increment (DB0 en V1) */
        BUTTON_DOWN,      /**< Botón Down/Decrement (DB2 en V1) */
        BUTTON_SIDE,      /**< Botón lateral alternativo (SIDE_BUTTON en V2) */
        BUTTON_MAX_COUNT  /**< Total de botones posibles en el sistema */
    } ButtonID_t;

    /**
     * @brief Configuración de un botón físico.
     *
     * Describe el mapeo hardware y comportamiento de un botón.
     *
     * @note Si `enabled = false`, el botón NO existe en esta variante y
     *       será omitido durante la inicialización.
     */
    typedef struct
    {
        GPIO_Port_t port;       /**< Puerto GPIO (ej: GPIOE) */
        GPIO_Pin_t pin;         /**< Pin GPIO (ej: GPIO_PIN_2) */
        bool active_low;        /**< true = botón presionado = LOW */
        uint32_t long_press_ms; /**< Tiempo para long press (0 = sin long press) */
        bool enabled;           /**< false = botón no existe en esta variante */
    } ButtonPinConfig_t;

    /*============================================================================*
     * OUTPUT PIN CONFIGURATION (Relay, LEDs)
     *============================================================================*/

    /**
     * @brief Configuración de un pin de salida (relay, LED, etc.).
     *
     * Describe el mapeo hardware de un pin de salida digital.
     */
    typedef struct
    {
        GPIO_Port_t port; /**< Puerto GPIO */
        GPIO_Pin_t pin;   /**< Pin GPIO */
        bool enabled;     /**< false = pin no disponible en esta variante */
    } OutputPinConfig_t;


    typedef struct
    {
    	GPIO_Port_t rs485_rx_en_port;
    	GPIO_Port_t rs485_tx_en_port;
    	GPIO_Pin_t rs485_rx_en_pin;
    	GPIO_Pin_t rs485_tx_en_pin;
    	bool enabled;
    }RS485EnablePinConfig_t;
    /*============================================================================*
     * DISPLAY CONFIGURATION
     *============================================================================*/

    /**
     * @brief Tipo de driver de display soportado.
     *
     * Define los controladores de display que el sistema puede usar.
     * Permite al DI Container seleccionar la implementación correcta del driver.
     */
    typedef enum
    {
        DISPLAY_DRIVER_NONE = 0, /**< Sin display en esta variante */
        DISPLAY_DRIVER_ST7789,   /**< ST7789 TFT 240x320 SPI (V1) */
        DISPLAY_DRIVER_SSD1306,  /**< SSD1306 OLED 128x64 I2C/SPI */
        DISPLAY_DRIVER_ILI9341,  /**< ILI9341 TFT 240x320 SPI (alternativa) */
        DISPLAY_DRIVER_MAX_COUNT /**< Cantidad de drivers soportados */
    } DisplayDriverType_t;

    /**
     * @brief Configuración de display LCD/OLED.
     *
     * Define si la variante tiene display, sus dimensiones y driver específico.
     */
    typedef struct
    {
        bool enabled;               /**< false = sin pantalla en esta variante */
        uint16_t width;             /**< Ancho en pixels (0 si disabled) */
        uint16_t height;            /**< Alto en pixels (0 si disabled) */
        DisplayDriverType_t driver; /**< Tipo de driver a usar (ST7789, SSD1306, etc.) */

        /* ---- Pines GPIO de control (para drivers SPI: ST7789, ILI9341) ---- */
        GPIO_Port_t rst_port; /**< Puerto del pin RESET del display */
        GPIO_Pin_t rst_pin;   /**< Pin RESET del display */
        GPIO_Port_t dc_port;  /**< Puerto del pin D/C (Data / Command) */
        GPIO_Pin_t dc_pin;    /**< Pin D/C */
        GPIO_Port_t cs_port;  /**< Puerto del pin CS (Chip Select) */
        GPIO_Pin_t cs_pin;    /**< Pin CS */
        GPIO_Port_t bl_port;  /**< Puerto del pin BL (Backlight) */
        GPIO_Pin_t bl_pin;    /**< Pin BL */
    } DisplayConfig_t;

    /*============================================================================*
     * EEPROM CONFIGURATION
     *============================================================================*/

    /**
     * @brief Tipo de driver de EEPROM soportado.
     *
     * Define los chips de EEPROM que el sistema puede usar.
     * Cada chip tiene diferentes tamaños de página y capacidad total.
     */
    typedef enum
    {
        EEPROM_DRIVER_NONE = 0, /**< Sin EEPROM externa en esta variante */
        EEPROM_DRIVER_M24M01E,  /**< M24M01E: I2C, 1Mbit (128KB), 256-byte page (V1) */
        EEPROM_DRIVER_AT24C256, /**< AT24C256: I2C, 256Kbit (32KB), 64-byte page */
        EEPROM_DRIVER_AT24C512, /**< AT24C512: I2C, 512Kbit (64KB), 128-byte page */
        EEPROM_DRIVER_MAX_COUNT /**< Cantidad de drivers soportados */
    } EepromDriverType_t;

    /**
     * @brief Configuración de EEPROM externa.
     *
     * Define el tipo de chip EEPROM y sus parámetros.
     * El DI Container usa esto para instanciar el driver correcto.
     */
    typedef struct
    {
        bool enabled;              /**< false = sin EEPROM externa */
        EepromDriverType_t driver; /**< Tipo de driver a usar */
        uint32_t size_bytes;       /**< Capacidad total en bytes */
        uint16_t page_size;        /**< Tamaño de página para escritura */
        uint8_t i2c_address;       /**< Dirección I2C (7-bit, ej: 0xA0) */
    } EepromConfig_t;

    /*============================================================================*
     * BATTERY/ANALOG CONFIGURATION (Future)
     *============================================================================*/

    /**
     * @brief Configuración de monitoreo de batería.
     *
     * Define si hay sensor de batería y cómo leerlo.
     *
     * @note Futuro: Agregar ADC channel, voltage divider ratio, etc.
     */
    typedef struct
    {
        bool enabled;        /**< false = sin monitoreo de batería */
        void *handle;
    } BatteryConfig_t;

    /*============================================================================*
     * ESP32 WiFi CONFIGURATION (ESP-Hosted)
     *============================================================================*/

    /**
     * @brief Modo de control de Chip Select para ESP32.
     *
     * ESP32 original usa manejo automático de CS por hardware.
     * ESP32-S2/S3/C3/C6 requieren control manual de CS por software.
     */
    typedef enum
    {
        ESP_CS_MODE_AUTO = 0,   /**< ESP32 original: CS automático por hardware */
        ESP_CS_MODE_MANUAL = 1, /**< ESP32-S2/S3/C3/C6: CS manual por software */
    } ESP_ChipSelectMode_t;

    /**
     * @brief Configuración de módulo ESP32 WiFi co-processor.
     *
     * Define si la variante tiene módulo ESP32, sus pines de control y modo CS.
     * Usado por ESP_SpiTransportAdapter para configurar comunicación SPI.
     */
    typedef struct
    {
        bool enabled; /**< false = sin módulo WiFi en esta variante */

        /* Pines de control ESP32 */
        GPIO_Port_t reset_port; /**< Puerto GPIO reset (ej: GPIOA) */
        GPIO_Pin_t reset_pin;   /**< Pin GPIO reset (ej: GPIO_PIN_0) */

        GPIO_Port_t handshake_port; /**< Puerto GPIO data ready (ej: GPIOB) */
        GPIO_Pin_t handshake_pin;   /**< Pin GPIO data ready (ej: GPIO_PIN_1) */

        GPIO_Port_t data_ready_port; /**< Puerto GPIO handshake (ej: GPIOC) */
        GPIO_Pin_t data_ready_pin;   /**< Pin GPIO handshake (ej: GPIO_PIN_2) */

        GPIO_Port_t cs_port; /**< Puerto GPIO CS (si ESP_CS_MODE_MANUAL, 0 si AUTO) */
        GPIO_Pin_t cs_pin;   /**< Pin GPIO CS (si ESP_CS_MODE_MANUAL, 0 si AUTO) */

        /* Configuración de hardware */
        ESP_ChipSelectMode_t cs_mode; /**< Modo CS (AUTO para ESP32, MANUAL para ESP32-S2+) */
        uint8_t handshake_exti_line;  /**< Línea EXTI para handshake/data ready (ej: EXTI15_10) */
        uint8_t data_ready_exti_line; /**< Línea EXTI para data ready/handshake (ej: EXTI15_10) */

    } ESP32WifiConfig_t;

    /**
     * @brief Configuración del puerto USB RNDIS.
     *
     * Cuando `enabled = true` el DI container inicializa el USB Device Stack
     * (USBD_Init / USBD_RegisterClass / USBD_Start) y registra el NIC driver
     * `rndisDriver` en CycloneTCP como tercera interfaz de red (netInterface[2]).
     *
     * Los parámetros IPv4 se mantienen fijos: 192.168.9.1 / 255.255.255.0,
     * pool DHCP .10-.99. Cambiarlos requiere modificar DI_InitUsbRndisSubsystem.
     */
    typedef struct
    {
        bool enabled; /**< false = sin USB RNDIS en esta variante de hardware */
    } UsbRndisConfig_t;

    /*============================================================================*
     * BOARD PROFILE (Agregación de todas las configuraciones)
     *============================================================================*/

    /**
     * @brief Perfil de hardware de una variante de placa.
     *
     * Este struct encapsula TODA la configuración específica de hardware.
     * Código de alto nivel lee este struct en vez de hardcodear pines.
     *
     * @note Inmutable después de BSP_GetBoardProfile().
     * @note Pattern: Strategy Pattern + Dependency Injection
     */
    typedef struct
    {
        const char *variant_name; /**< Nombre legible (ej: "CICX1-V1-FULL") */

        /* ===== Input Configuration ===== */
        ButtonPinConfig_t buttons[BUTTON_MAX_COUNT]; /**< Configuración de botones UI */
        ButtonPinConfig_t alarm_input;               /**< Entrada de alarma (overtemp) */

        /* ===== Output Configuration ===== */
        OutputPinConfig_t relay_control; /**< Control del relé (SSR_CTRL o SY_TXD) */
        OutputPinConfig_t led_status[3]; /**< LEDs de estado (LED0, DB4, DB5) */
        OutputPinConfig_t side_led;      /**< LED lateral/indicador general (SIDE_LED) */

        /* ===== Display Configuration ===== */
        DisplayConfig_t display; /**< Configuración de pantalla LCD/OLED */

        /* ===== EEPROM Configuration ===== */
        EepromConfig_t eeprom; /**< Configuración de EEPROM externa */

        /* ===== Battery/Power Monitoring (Future) ===== */
        BatteryConfig_t battery; /**< Configuración de monitoreo de batería */

        /* ===== ESP32 WiFi Configuration ===== */
        ESP32WifiConfig_t esp32_wifi; /**< Configuración de módulo ESP32 WiFi co-processor */

        /* ===== USB RNDIS Configuration ===== */
        UsbRndisConfig_t usb_rndis; /**< Configuración de puerto USB RNDIS (CDC/RNDIS gateway) */

        /* =======RS485 ===================*/
        RS485EnablePinConfig_t rs485;
        /* ===== Additional GPIO Pins ===== */
        OutputPinConfig_t buzzer;    /**< Pin del buzzer/piezo speaker */
        OutputPinConfig_t gps_reset; /**< Pin de reset del módulo GPS */
        ButtonPinConfig_t gps_pps;   /**< Pin PPS (Pulse Per Second) del GPS (input) */
        OutputPinConfig_t rf_ctrl1;  /**< Pin control RF antena GPS (selector antena interna/externa) */
        OutputPinConfig_t rf_ctrl2;  /**< Pin control RF antena GPS (power/enable) */

        /* ===== Peripheral Handle Mapping ===== */
        /**
         * @brief Mapeo de periféricos STM32 a funciones del sistema.
         *
         * Indica qué instancia de periférico usar para cada función.
         * Los handles reales se obtienen mediante funciones lookup:
         *   - BSP_GetUARTHandle(gps_uart_index) → UART_HandleTypeDef* (huart1, huart2, etc.)
         *   - BSP_GetI2CHandle(eeprom_i2c_index) → I2C_HandleTypeDef* (hi2c1, hi2c2, etc.)
         *   - BSP_GetSPIHandle(display_spi_index) → SPI_HandleTypeDef* (hspi1, hspi2, etc.)
         *
         * @example
         *   // En dependency_container.c
         *   const BoardProfile_t *profile = BSP_GetBoardProfile();
         *   UART_HandleTypeDef *gps_uart = BSP_GetUARTHandle(profile->peripheral_map.gps_uart_index);
         *   UART_RegisterRxEventCallback(bsp->uart, gps_uart, callback, context);
         *
         * @example
         *   // Uso con OctoSPI (external flash driver - futuro)
         *   const BoardProfile_t *profile = BSP_GetBoardProfile();
         *   OSPI_HandleTypeDef *ext_flash = BSP_GetOctoSPIHandle(profile->peripheral_map.octospi_index);
         *   ExtFlash_Init(bsp->ext_flash, ext_flash, &flash_config);
         *
         * @note Mapping actual:
         *   - UART:     0=huart2 (ver bsp_init.c uart_map)
         *   - I2C:      0=hi2c1 (ver bsp_init.c i2c_map)
         *   - SPI:      0=hspi1, 1=hspi3 (ver bsp_init.c spi_map)
         *   - Timer:    0=htim2 (ver bsp_init.c timer_map)
         *   - RTC:      0=hrtc (ver bsp_init.c rtc_map)
         *   - OctoSPI:  0=hospi1 (ver bsp_init.c octospi_map)
         *
         * @warning Si se agrega un nuevo periférico en CubeMX, actualizar:
         *          1. bsp_init.c (uart_map/i2c_map/spi_map arrays)
         *          2. Profiles en bsp_board_profiles.c con nuevo índice
         */
        struct
        {
            uint8_t gps_uart_index;    /**< UART para GPS (usar BSP_GetUARTHandle) */
            uint8_t debug_uart_index;  /**< UART para debug/logs (usar BSP_GetUARTHandle) */
            uint8_t wifi_spi_index;    /**< SPI para WiFi (usar BSP_GetSPIHandle) */
            uint8_t eeprom_i2c_index;  /**< I2C para EEPROM (usar BSP_GetI2CHandle) */
            uint8_t battery_i2c_index; /**< I2C para BQXXX (usar BSP_GetI2CHandle) */
            uint8_t display_spi_index; /**< SPI para display (usar BSP_GetSPIHandle) */
            uint8_t relay_timer_index; /**< Timer para relay cycle timing (usar BSP_GetTimerHandle) */
            uint8_t rtc_index;         /**< RTC para timekeeping (usar BSP_GetRTCHandle, usualmente 0) */
            uint8_t octospi_index;     /**< OctoSPI para external flash (usar BSP_GetOctoSPIHandle) */
        } peripheral_map;

    } BoardProfile_t;

    /*============================================================================*
     * PUBLIC API
     *============================================================================*/

    /**
     * @brief Obtiene el perfil de hardware activo.
     *
     * Esta función retorna el perfil seleccionado en tiempo de compilación
     * mediante el define BOARD_VARIANT_V1 / BOARD_VARIANT_V2.
     *
     * @return Puntero al perfil activo (nunca NULL, estático).
     *
     * @note Es el ÚNICO lugar en todo el sistema que usa #ifdef para variantes.
     * @note Thread-safe (retorna puntero a const static).
     *
     * @example
     * ```c
     * const BoardProfile_t *profile = BSP_GetBoardProfile();
     * if (profile->display.enabled) {
     *     init_lvgl(profile->display.width, profile->display.height);
     * }
     * ```
     */
    const BoardProfile_t *BSP_GetBoardProfile(void);

    /*============================================================================*
     * PROFILE DECLARATIONS (implementados en bsp_board_profiles.c)
     *============================================================================*/

    /**
     * @brief Perfil CICX1-V5: Full-featured variant.
     *
     * Características:
     * - Display: ST7789 240x320 TFT SPI
     * - EEPROM: M24M01E 128KB I2C (1Mbit, 256-byte page)
     * - Botones: DB0 (UP), DB1 (ENTER), DB2 (DOWN), PBOUT (ON/OFF)
     * - Control Relé: SY_TXD (GPIOD Pin6)
     * - Alarma: SY_RXD (GPIOD Pin7)
     * - LEDs: LED0, DB4, DB5, SIDE_LED
     * - Batería: Monitoreo ADC (futuro)
     */
    extern const BoardProfile_t BOARD_PROFILE_CICX1_V5;

    /**
     * @brief Perfil SSR3-V1: Minimal variant (sin display).
     *
     * Características:
     * - Display: Ninguno
     * - EEPROM: AT24C256 32KB I2C (256Kbit, 64-byte page, económica)
     * - Botones: Solo SIDE_BUTTON
     * - Control Relé: SSR_CTRL (GPIOB Pin2) en vez de SY_TXD
     * - Alarma: OTP_IN (GPIOB Pin5) en vez de SY_RXD
     * - LEDs: LED0, DB4 (mapeado a LED0), SIDE_LED
     * - Batería: Sin monitoreo
     */
    extern const BoardProfile_t BOARD_PROFILE_SSR3_V1;

    /**
     * @brief Perfil MOCK: Para testing en PC sin hardware real.
     *
     * Usa direcciones ficticias válidas para punteros GPIO.
     * Permite tests unitarios en PC sin STM32 HAL.
     */
    extern const BoardProfile_t BOARD_PROFILE_MOCK;

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_PROFILE_H */
