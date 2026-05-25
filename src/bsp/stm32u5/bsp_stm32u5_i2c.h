/**
 * @file bsp_stm32u5_i2c.h
 * @brief Interfaz del BSP I2C para STM32U5.
 */

#ifndef BSP_STM32U5_I2C_H
#define BSP_STM32U5_I2C_H

#include "i_i2c.h"

/**
 * @brief Obtiene la instancia de la interfaz I2C para STM32U5.
 * @return I_I2C* Puntero a la interfaz.
 */
I_I2C* Bsp_Stm32U5_I2c_GetInterface(void);

#endif /* BSP_STM32U5_I2C_H */
