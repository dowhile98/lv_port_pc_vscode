/**
 * @file bsp_stm32u5_uart.h
 * @brief BSP STM32U5 para interfaz portátil I_UART (DMA + Idle Line detection).
 * @version 1.0.0
 * @author Tecna Smart Lab
 */

#ifndef BSP_STM32U5_UART_H
#define BSP_STM32U5_UART_H

#include "i_uart.h"

/**
 * @brief Obtiene la instancia singleton de I_UART para STM32U5.
 * @return I_UART* Puntero no-NULL a la interfaz portátil.
 *
 * @note Idempotente; siempre retorna la misma instancia.
 * @note Soporta operaciones bloqueantes (polling) y asíncronas (DMA).
 * @note ReceiveUntilIdle_Async utiliza interrupción IDLE Line para detección de fin de trama.
 */
I_UART *Bsp_Stm32U5_Uart_GetInterface(void);

#endif /* BSP_STM32U5_UART_H */
