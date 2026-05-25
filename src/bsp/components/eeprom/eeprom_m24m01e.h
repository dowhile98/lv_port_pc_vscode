/**
 * @file eeprom_m24m01e.h
 * @brief Driver para EEPROM M24M01E (STMicroelectronics, 1Mbit I2C).
 *
 * @details Este driver implementa la interfaz I_EXT_EEPROM para la EEPROM
 *          M24M01E de 1Mbit (128KB) con las siguientes características:
 *          - Capacidad: 131072 bytes (128 KB)
 *          - Página de escritura: 256 bytes
 *          - Dirección I2C: 0xA0 (defecto), soporta A16 para direccionamiento extendido
 *          - Manejo automático de fronteras de bloque de 64KB
 *          - Soporta escritura por página y lectura secuencial
 *
 * @note El bit A16 se maneja automáticamente en la dirección I2C efectiva.
 *       Block 0: 0x00000-0x0FFFF usa 0xA0
 *       Block 1: 0x10000-0x1FFFF usa 0xA2
 */

#ifndef EEPROM_M24M01E_H
#define EEPROM_M24M01E_H

#include "i_ext_eeprom.h"
#include "i_i2c.h"
#include "bsp/stm32u5/bsp_board_profile.h" /* Para EepromConfig_t */

/**
 * @brief Contexto privado del driver M24M01E.
 * @note Este contexto es manejado internamente por el driver.
 */
typedef struct
{
    I_I2C *i2c_iface;             /**< Interfaz I2C subyacente. */
    I_I2C_Handle_t i2c_handle;    /**< Handle del bus I2C. */
    I_EXT_EEPROM_Config_t config; /**< Configuración de la EEPROM. */
} M24M01E_Context_t;

/**
 * @brief Crea instancia del driver M24M01E.
 * @note  Thread-safe en la creación. El singleton interno previene múltiples instancias.
 *
 * @param[in]  i2c_iface     Interfaz I2C a usar (no debe ser NULL).
 * @param[in]  i2c_handle    Handle del bus I2C (no debe ser NULL).
 * @param[in]  board_config  Configuración del EEPROM desde BoardProfile (no debe ser NULL).
 *
 * @return I_EXT_EEPROM* Puntero a la interfaz genérica EEPROM.
 * @retval NULL si algún parámetro es inválido o la config está deshabilitada.
 * @retval !NULL puntero válido a la interfaz I_EXT_EEPROM.
 */
I_EXT_EEPROM *M24M01E_Create(I_I2C *i2c_iface, I_I2C_Handle_t i2c_handle,
                             const EepromConfig_t *board_config);

#endif /* EEPROM_M24M01E_H */
