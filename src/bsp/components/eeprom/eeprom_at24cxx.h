/**
 * @file eeprom_at24cxx.h
 * @brief Driver para EEPROM AT24CXX (genérico para serie AT24).
 */

#ifndef EEPROM_AT24CXX_H
#define EEPROM_AT24CXX_H

#include "i_ext_eeprom.h"
#include "i_i2c.h"

/**
 * @brief Variantes de AT24CXX soportadas.
 */
typedef enum
{
    AT24C02 = 256,    /**< 256 bytes (2 Kbit) */
    AT24C04 = 512,    /**< 512 bytes (4 Kbit) */
    AT24C08 = 1024,   /**< 1 KB (8 Kbit) */
    AT24C16 = 2048,   /**< 2 KB (16 Kbit) */
    AT24C32 = 4096,   /**< 4 KB (32 Kbit) */
    AT24C64 = 8192,   /**< 8 KB (64 Kbit) */
    AT24C128 = 16384, /**< 16 KB (128 Kbit) */
    AT24C256 = 32768, /**< 32 KB (256 Kbit) */
    AT24C512 = 65536  /**< 64 KB (512 Kbit) */
} AT24CXX_Variant_t;

/**
 * @brief Contexto privado del driver AT24CXX.
 */
typedef struct
{
    I_I2C *i2c_iface;             /**< Interfaz I2C subyacente. */
    I_I2C_Handle_t i2c_handle;    /**< Handle del bus I2C. */
    I_EXT_EEPROM_Config_t config; /**< Configuración de la EEPROM. */
    AT24CXX_Variant_t variant;    /**< Variante específica. */
} AT24CXX_Context_t;

/**
 * @brief Crea instancia del driver AT24CXX.
 *
 * @param i2c_iface Interfaz I2C a usar.
 * @param i2c_handle Handle del bus I2C.
 * @param variant Variante específica de AT24CXX.
 * @return I_EXT_EEPROM* Puntero a la interfaz genérica.
 */
I_EXT_EEPROM *AT24CXX_Create(I_I2C *i2c_iface, I_I2C_Handle_t i2c_handle, AT24CXX_Variant_t variant);

#endif /* EEPROM_AT24CXX_H */
