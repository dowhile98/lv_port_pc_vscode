/**
 * @file bsp_init.h
 * @brief Inicializador central del BSP STM32U5 - Factory de Interfaces HAL
 */

#ifndef BSP_INIT_H
#define BSP_INIT_H

#include "bsp_config.h"
#include "hal_types.h"
#include "interfaces/i_gpio.h"
#include "interfaces/i_uart.h"
#include "interfaces/i_timer.h"
#include "interfaces/i_spi.h"
#include "interfaces/i_i2c.h"
#include "interfaces/i_exti.h"
#include "interfaces/i_ext_flash.h"
#include "interfaces/i_rtc.h"
#include "interfaces/i_temperature_sensor.h"
#include "interfaces/i_crypto_verifier.h"
#include "domain/update/firmware_update_strategy.h" /* ICycloneBootOps_t */

/**
 * @note Peripheral handles son opacos (void*) para mantener hardware-agnosticism.
 * @note BSP adapters internamente realizan cast a tipos HAL específicos.
 * @note Consistente con I_I2C_Handle_t, I_Timer_Handle_t definidos en interfaces.
 */

/**
 * @brief Contenedor de todas las interfaces HAL del BSP
 */
typedef struct
{
    I_GPIO *gpio;
    I_UART *uart;
    I_TIMER *timer;
    I_SPI *spi;
    I_I2C *i2c;
    I_EXTI *exti;
    I_EXT_FLASH *ext_flash;
    I_RTC *rtc;
    ITemperatureSensor_t *temperature_sensor; /**< Internal MCU temperature sensor (ADC1, may be NULL if init failed) */
    ICryptoVerifier_t *i_crypto_verifier;     /**< ECDSA-SHA256 verifier for resources .img (ExternalLoaderStrategy) */
    ICycloneBootOps_t *i_cyclone_boot_ops;    /**< CycloneBOOT operations wrapper (FirmwareUpdateStrategy) */
    bool is_initialized;
} BSP_Interfaces_t;

/**
 * @brief Inicializa todo el BSP y los periféricos de hardware.
 * @note  Debe llamarse una sola vez al inicio del sistema.
 * @return ERR_OK si todo se inicializó correctamente.
 */
Result_t BSP_Init(void);

/**
 * @brief Obtiene el contenedor de interfaces HAL del BSP.
 * @warning Solo válido después de llamar BSP_Init().
 * @return Puntero al contenedor (nunca NULL si BSP_Init() tuvo éxito).
 */
const BSP_Interfaces_t *BSP_GetInterfaces(void);

/**
 * @brief Verifica si el BSP fue inicializado correctamente.
 */
bool BSP_IsInitialized(void);

/*============================================================================*
 * PERIPHERAL HANDLE LOOKUP (Board Profile Pattern Support)
 *============================================================================*/

/**
 * @brief Obtiene handle UART por índice (para Board Profile Pattern).
 * @param index Índice del UART (0=huart1, 1=huart2, 2=huart3, etc.)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.gps_uart_index
 * @note Cast a UART_HandleTypeDef* ocurre dentro de BSP adapters
 */
void *BSP_GetUARTHandle(uint8_t index);

/**
 * @brief Obtiene handle I2C por índice (para Board Profile Pattern).
 * @param index Índice del I2C (0=hi2c1, 1=hi2c2, 2=hi2c3, etc.)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.eeprom_i2c_index
 */
void *BSP_GetI2CHandle(uint8_t index);

/**
 * @brief Obtiene handle SPI por índice (para Board Profile Pattern).
 * @param index Índice del SPI (0=hspi1, 1=hspi2, 2=hspi3, etc.)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.display_spi_index
 */
void *BSP_GetSPIHandle(uint8_t index);

/**
 * @brief Obtiene handle Timer por índice (para Board Profile Pattern).
 * @param index Índice del Timer (0=htim2, 1=htim6, 2=htim7, etc.)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.relay_timer_index
 */
void *BSP_GetTimerHandle(uint8_t index);

/**
 * @brief Obtiene handle RTC (para Board Profile Pattern).
 * @param index Índice del RTC (usualmente 0, solo hay un RTC)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.rtc_index
 */
void *BSP_GetRTCHandle(uint8_t index);

/**
 * @brief Obtiene handle OctoSPI por índice (para Board Profile Pattern).
 * @param index Índice del OctoSPI (0=hospi1, 1=hospi2, etc.)
 * @return Handle opaco (void*) o NULL si índice inválido
 * @note Usado por dependency_container.c con profile->peripheral_map.octospi_index
 * @note OctoSPI se usa para external flash (MX25LM51245G en CICX1-V5)
 */
void *BSP_GetOctoSPIHandle(uint8_t index);

#endif /* BSP_INIT_H */
