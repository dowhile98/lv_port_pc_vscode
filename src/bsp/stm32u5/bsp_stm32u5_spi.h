/**
 * @file bsp_stm32u5_spi.h
 * @brief BSP STM32U5 para interfaz portátil I_SPI (DMA/IT + polling).
 */

#ifndef BSP_STM32U5_SPI_H
#define BSP_STM32U5_SPI_H

#include "i_spi.h"

/**
 * @brief Obtiene la instancia singleton de la interfaz SPI para STM32U5.
 * @return I_SPI* Puntero no-NULL a la interfaz.
 */
I_SPI *Bsp_Stm32U5_Spi_GetInterface(void);

#endif /* BSP_STM32U5_SPI_H */
