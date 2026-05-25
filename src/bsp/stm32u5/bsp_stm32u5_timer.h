/**
 * @file bsp_stm32u5_timer.h
 * @brief BSP STM32U5 para interfaz portátil I_TIMER (base tiempo + Output Compare).
 */

#ifndef BSP_STM32U5_TIMER_H
#define BSP_STM32U5_TIMER_H

#include "i_timer.h"

/**
 * @brief Obtiene la instancia singleton de I_TIMER para STM32U5.
 * @return I_TIMER* Puntero no-NULL a la interfaz portátil.
 *
 * @note Idempotente; siempre retorna la misma instancia.
 */
I_TIMER *Bsp_Stm32U5_Timer_GetInterface(void);

#endif /* BSP_STM32U5_TIMER_H */
