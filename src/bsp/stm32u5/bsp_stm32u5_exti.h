/**
 * @file bsp_stm32u5_exti.h
 * @brief Implementación del BSP EXTI para STM32U5.
 * @version 1.0.0
 * @author Tecna Smart Lab
 *
 * Proporciona la interfaz abstracta I_EXTI implementada para el hardware STM32U5,
 * permitiendo la configuración y manejo de interrupciones externas (EXTI) de forma
 * independiente de la plataforma.
 */

#ifndef BSP_STM32U5_EXTI_H
#define BSP_STM32U5_EXTI_H

#include "i_exti.h"

/**
 * @brief Obtiene la instancia singleton de la interfaz EXTI para STM32U5.
 *
 * Retorna un puntero a una estructura constante que contiene la V-Table
 * mapeada a las funciones del HAL de STM32U5. Esta instancia debe ser
 * utilizada por adaptadores de alto nivel para registrar callbacks y
 * configurar líneas EXTI.
 *
 * @return I_EXTI* Puntero no-NULL a la interfaz EXTI portátil.
 *
 * @note Esta función es idempotente y siempre retorna la misma instancia.
 * @note La instancia es singleton y reside en memoria ROM.
 *
 * @see I_EXTI
 * @see EXTI_Init
 * @see EXTI_RegisterCallback
 */
I_EXTI *Bsp_Stm32U5_Exti_GetInterface(void);

#endif /* BSP_STM32U5_EXTI_H */
