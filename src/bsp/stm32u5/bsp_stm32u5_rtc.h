/**
 * @file bsp_stm32u5_rtc.h
 * @brief Implementación de la interfaz I_RTC para el hardware STM32U5.
 * @version 1.0.0
 */

#ifndef BSP_STM32U5_RTC_H
#define BSP_STM32U5_RTC_H

#include "interfaces/i_rtc.h"

/**
 * @brief Obtiene la interfaz I_RTC para el hardware STM32U5.
 * @return Puntero a la interfaz RTC (singleton).
 */
I_RTC *Bsp_Stm32U5_Rtc_GetInterface(void);

#endif /* BSP_STM32U5_RTC_H */
