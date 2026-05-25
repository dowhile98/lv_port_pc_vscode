/**
 * @file bsp_config.h
 * @brief Configuración de hardware del BSP STM32U5 - Mapeo de pines y periféricos
 * @version 1.1.0
 *
 * Este archivo centraliza la configuración de hardware específica de la placa.
 * Facilita el portado a diferentes variantes de hardware sin modificar el código BSP.
 *
 * @note Versión 1.1.0: Añadido soporte Board Profile Pattern para variantes múltiples.
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include "stm32u5xx_hal.h"
#include "main.h"

/*============================================================================*
 * BOARD VARIANT SELECTION (Configure HERE or in build flags)
 *============================================================================*/

/**
 * @brief Selección de variante de hardware.
 *
 * Descomenta UNA de las siguientes líneas O define en build flags:
 * - BOARD_VARIANT_V1: Full-featured (display + 4 botones + SSR_CTRL)
 * - BOARD_VARIANT_V2: Minimal (solo SIDE_BUTTON + SY_TXD + sin display)
 * - BOARD_VARIANT_MOCK: Para tests unitarios en PC
 *
 * @note ÚNICO lugar donde se define la variante manualmente.
 *       Alternativamente, usar build flags: -DBOARD_VARIANT_V1
 */

/* ===== Opción 1: Definir aquí (un solo uncomment) ===== */
#ifndef BOARD_VARIANT_CICX1_V5
#ifndef BOARD_VARIANT_SSR3_V1
#ifndef BOARD_VARIANT_MOCK
/* Default: V1 para mantener compatibilidad con código existente */
#define BOARD_VARIANT_CICX1_V5
#endif
#endif
#endif

/* ===== Opción 2: Build flags (recomendado para CI/CD) ===== */
/* cmake -DBOARD_VARIANT_V1 .. */
/* make CFLAGS="-DBOARD_VARIANT_V2" */

/*============================================================================*
 * BOARD PROFILE INTERFACE (Configuración data-driven)
 *============================================================================*/

#include "bsp_board_profile.h" /**< ✅ Nuevo: Board Profile Pattern */

/* ===== Referencias a Handles de Periféricos (definidos en Core/) ===== */

/* Declaraciones externas de handles generados por CubeMX */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern OSPI_HandleTypeDef hospi1;
extern RTC_HandleTypeDef hrtc;

/* ===== Mapeo de Periféricos a Funciones de la Aplicación ===== */

/**
 * @defgroup BSP_UART_Mapping Mapeo de periféricos UART
 * @{
 */
#define BSP_UART_GPS &huart2   /**< UART conectado al módulo GPS. */
#define BSP_UART_DEBUG &huart1 /**< UART para debug/logs. */
/** @} */

/**
 * @defgroup BSP_TIMER_Mapping Mapeo de timers
 * @{
 */
#define BSP_TIMER_CYCLES &htim6  /**< Timer para ciclos de interrupción. */
#define BSP_TIMER_GENERAL &htim7 /**< Timer de propósito general. */
/** @} */

/**
 * @defgroup BSP_BUS_Mapping Mapeo de buses de comunicación
 * @{
 */
#define BSP_SPI_WIFI &hspi1    /**< SPI para módulo WiFi (futura expansión). */
#define BSP_I2C_EEPROM &hi2c1  /**< I2C para EEPROM externa. */
#define BSP_OSPI_FLASH &hospi1 /**< OctoSPI para flash externa. */
/** @} */

/**
 * @defgroup BSP_RTC_Mapping Mapeo de RTC
 * @{
 */
#define BSP_RTC_MAIN &hrtc /**< RTC principal del sistema. */
/** @} */

/* ===== Mapeo de Pines GPIO Críticos ===== */
/**
 * @defgroup BSP_GPIO_Buttons Pines de botones de usuario
 * @{
 */
#define BSP_GPIO_SIDE_BUTON_PORT SIDE_BUTTON_GPIO_Port
#define BSP_GPIO_SIDE_BUTON_PIN SIDE_BUTTON_Pin

#define BSP_GPIO_DB0_PORT DB0_GPIO_Port
#define BSP_GPIO_DB0_PIN DB0_Pin
#define BSP_GPIO_DB1_PORT DB1_GPIO_Port
#define BSP_GPIO_DB1_PIN DB1_Pin
#define BSP_GPIO_DB2_PORT DB2_GPIO_Port
#define BSP_GPIO_DB2_PIN DB2_Pin

#define BSP_GPIO_PBOUT_PORT PBOUT_GPIO_Port
#define BSP_GPIO_PBOUT_PIN PBOUT_Pin
// LED0
#define BSP_GPIO_LED0_PORT SIDE_LED_GPIO_Port
#define BSP_GPIO_LED0_PIN SIDE_LED_Pin

// DB4
#define BSP_GPIO_DB4_PORT LED0_GPIO_Port
#define BSP_GPIO_DB4_PIN LED0_Pin

// DB5
#define BSP_GPIO_DB5_PORT DB5_GPIO_Port
#define BSP_GPIO_DB5_PIN DB5_Pin

/* ✅ MOVED TO BOARD PROFILE:
 * - BUZZER (buzzer.port/pin)
 * - GPS_RST (gps_reset.port/pin)
 * - GPS_PPS (gps_pps.port/pin)
 * Usar BSP_GetBoardProfile() en vez de estas macros hardcodeadas.
 */

// SY_TXD (SSR CONTROL)
#define BSP_GPIO_SYS_TXD_PORT SY_TXD_GPIO_Port
#define BSP_GPIO_SYS_TXD_PIN SY_TXD_Pin

// SY_RXD (OVERTEMP ALARM)
#define BSP_GPIO_SYS_RXD_PORT OTP_IN_GPIO_Port
#define BSP_GPIO_SYS_RXD_PIN OTP_IN_Pin

/** @} */
/**
 * @defgroup BSP_GPIO_Relays Pines de control de relés
 * @{
 */
#define BSP_GPIO_RELAY_DB4_PORT GPIOB
#define BSP_GPIO_RELAY_DB4_PIN GPIO_PIN_4
/** @} */

/**
 * @defgroup BSP_GPIO_GPS Pines de control de GPS
 * ✅ MOVED TO BOARD PROFILE:
 * - GPS_RST: profile->gps_reset.port/pin
 * - GPS_PPS: profile->gps_pps.port/pin
 * Usar BSP_GetBoardProfile() en vez de estas macros hardcodeadas.
 * @{
 */
/* Removed hardcoded definitions - use BoardProfile instead */
/** @} */

/**
 * @defgroup BSP_GPIO_Power Pines de gestión de energía
 * @{
 */
#define BSP_GPIO_BATTERY_SENSE_PORT GPIOC
#define BSP_GPIO_BATTERY_SENSE_PIN GPIO_PIN_0
/** @} */

/* ===== Configuración de Prioridades de Interrupciones ===== */

/**
 * @defgroup BSP_IRQ_Priorities Prioridades de interrupciones
 * @note ThreadX requiere que las prioridades sean >= 5 para permitir context switch
 * @{
 */
#define BSP_IRQ_PRIORITY_GPS_PPS 5      /**< Alta prioridad para PPS crítico. */
#define BSP_IRQ_PRIORITY_TIMER_CYCLES 6 /**< Alta prioridad para timer de ciclos. */
#define BSP_IRQ_PRIORITY_UART_GPS 7     /**< Media-alta para recepción GPS. */
#define BSP_IRQ_PRIORITY_RTC_ALARM 8    /**< Media para alarmas RTC. */
#define BSP_IRQ_PRIORITY_I2C 10         /**< Media-baja para I2C. */
#define BSP_IRQ_PRIORITY_SPI 10         /**< Media-baja para SPI. */
/** @} */

/* ===== Configuración de EEPROM ===== */

/**
 * @defgroup BSP_EEPROM_Config Configuración de EEPROM externa
 * @{
 */
#define BSP_EEPROM_I2C_ADDRESS 0xA0  /**< Dirección I2C de la EEPROM (7-bit). */
#define BSP_EEPROM_SIZE_BYTES 131072 /**< Tamaño: 128KB (M24M01E). */
#define BSP_EEPROM_PAGE_SIZE 256     /**< Tamaño de página para escritura. */
#define BSP_EEPROM_WRITE_DELAY_MS 5  /**< Delay después de escritura. */
/** @} */

/* ===== Configuración de Display (LVGL) ===== */

/**
 * @defgroup BSP_Display_Config Configuración de display
 * @{
 */
#define BSP_DISPLAY_WIDTH 800
#define BSP_DISPLAY_HEIGHT 480
#define BSP_DISPLAY_BPP 16 /**< Bits por pixel (RGB565). */
/** @} */

/* ===== Validación de Configuración ===== */

#if !defined(STM32U575xx)
#error "Este BSP está diseñado para STM32U575VGTx. Verifica la configuración."
#endif

#endif /* BSP_CONFIG_H */
