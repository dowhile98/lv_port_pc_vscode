/**
 * @file bsp_stm32u5_display.h
 * @brief Inicialización del subsistema UI de hardware para STM32U5.
 *
 * Proporciona @c BSP_UI_HW_Init(), única función pública que:
 *  - Inicializa el driver ST7789 (I_Display) vía SPI + GPIO del board profile.
 *  - Construye el bridge LvglEncoderAdapter (IEncoder_t) sobre el
 *    IDigitalInputSource inyectado.
 *
 * Reglas de arquitectura:
 *  - Este archivo vive en BSP → puede incluir stm32u5xx_hal.h.
 *  - Sus salidas (`I_Display*`, `IEncoder_t*`) son PURAS interfaces portables.
 *  - `ui_ao.c` (presentation layer) recibe las interfaces vía @c UiAO_Config_t
 *    a través de la inyección de dependencias del DI Container (ACTION-023).
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef BSP_STM32U5_DISPLAY_H
#define BSP_STM32U5_DISPLAY_H

#include "hal_types.h"
#include "interfaces/i_display.h"
#include "interfaces/i_encoder.h"
#include "interfaces/i_digital_input_source.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Inicializa el hardware del subsistema UI.
     *
     * Internamente inicializa:
     *  - @c ST7789_Driver_t (I_Display) con SPI + pines del board profile activo.
     *  - @c LvglEncoderAdapter_t (IEncoder_t) suscrito a @p di_source.
     *
     * @note Llamar desde el hilo GUI antes de @c lv_init().
     * @note Thread-safe: must be called once.
     *
     * @param[in]  di_source     Fuente de eventos de entrada digital (inyectada —
     *                           obtenida del DI container). No debe ser NULL.
     * @param[out] out_display   Recibe puntero a @c I_Display (implementado por ST7789).
     * @param[out] out_encoder   Recibe puntero a @c IEncoder_t (implementado por el adapter).
     *
     * @return ERR_OK en éxito.
     * @return ERR_NULL_POINTER si any param es NULL.
     * @return ERR_ERROR si el hardware no responde.
     */
    Result_t BSP_UI_HW_Init(IDigitalInputSource *di_source,
                            I_Display **out_display,
                            IEncoder_t **out_encoder);

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32U5_DISPLAY_H */
