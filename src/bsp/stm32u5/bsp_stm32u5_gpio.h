/**
 * @file bsp_stm32u5_gpio.h
 * @brief Implementación del BSP GPIO para la plataforma STM32U5.
 * @version 1.0.0
 */

#ifndef BSP_STM32U5_GPIO_H
#define BSP_STM32U5_GPIO_H

#include "interfaces/i_gpio.h"

/**
 * @brief Obtiene la instancia de la interfaz GPIO para STM32U5.
 * 
 * Esta función retorna un puntero a una estructura constante que contiene
 * la V-Table mapeada a las funciones del HAL de STM32U5.
 * 
 * @return I_GPIO* Puntero a la interfaz abstracta.
 */
I_GPIO* Bsp_Stm32U5_Gpio_GetInterface(void);

#endif /* BSP_STM32U5_GPIO_H */
