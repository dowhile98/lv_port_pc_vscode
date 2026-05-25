/**
 * @file bsp_stm32u5_octospi.h
 * @brief BSP STM32U5 para flash externo vía OCTOSPI (W25Q16/W25Q128).
 */

#ifndef BSP_STM32U5_OCTOSPI_H
#define BSP_STM32U5_OCTOSPI_H

#include "i_ext_flash.h"

/**
 * @brief Obtiene la instancia singleton de la interfaz OCTOSPI para STM32U5.
 * @return I_EXT_FLASH* Puntero no-NULL a la interfaz.
 */
I_EXT_FLASH *Bsp_Stm32U5_Octospi_GetInterface(void);

#endif /* BSP_STM32U5_OCTOSPI_H */
