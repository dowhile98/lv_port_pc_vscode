/**
 * @file bsp_logging_output.c
 * @brief Implementación de output callbacks para logging system.
 */

#include "bsp_logging_output.h"
#include "stm32u5xx_hal.h"
#include "usart.h" /* huart2 para UART logging */

/**
 * @brief Output callback para UART (USART2 por defecto).
 * @note Usa transmisión bloqueante simple para evitar overhead.
 * @warning DEBE retornar ch (no 0) incluso si UART falla,
 *          para evitar que lwprintf detenga la escritura mid-stream.
 */
int BSP_Logging_OutputUART(int ch, void *arg)
{
	/* arg puede ser UART_HandleTypeDef* o NULL (usa default) */
	UART_HandleTypeDef *huart = (arg != NULL) ? (UART_HandleTypeDef *)arg : &huart2;

	uint8_t byte = (uint8_t)ch;

	if(ch == '\0')
	{
		return ch; /* No enviar null char, pero retornar para lwprintf */
	}
	/* Transmisión bloqueante con timeout (1ms) */
	HAL_StatusTypeDef status = HAL_UART_Transmit(huart, &byte, 1, 1);

	/* CRÍTICO: Siempre retornar ch para que lwprintf continue */
	/* Si UART falla (busy/error), el carácter se descarta silenciosamente */
	(void)status; /* Ignorar status - no break lwprintf chain */
	return ch;
}

/**
 * @brief Output callback para ITM (SWO debug).
 * @note ITM_SendChar es inline en core_cm33.h (CMSIS).
 * @warning DEBE retornar ch (no 0) incluso si ITM no disponible,
 *          para evitar que lwprintf detenga la escritura mid-stream.
 */
int BSP_Logging_OutputITM(int ch, void *arg)
{
	(void)arg; /* No usado */

	if(ch == '\0')
	{
		return ch; /* No enviar null char, pero retornar para lwprintf */
	}

	ITM_SendChar(ch);

	return ch;
}
