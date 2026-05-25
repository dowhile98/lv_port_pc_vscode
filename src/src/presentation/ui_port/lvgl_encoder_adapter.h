/**
 * @file lvgl_encoder_adapter.h
 * @brief Bridge entre IDigitalInputSource y IEncoder_t para LVGL.
 *
 * Suscribe los eventos UP/DOWN/ENTER del sistema de entradas digitales
 * existente (DigitalInputAO + IDigitalInputSource) y los expone como
 * IEncoder_t para UI_Port_Indev.
 *
 * Equivalente limpio de @c tcs_cicx1_lv_button.c del legacy v4.0.0,
 * con inyección de dependencia en lugar de acceso a globales.
 *
 * @par Convención UP/DOWN (heredada del legacy)
 * - Botón UP   → delta = -1  (navega al elemento anterior en lista)
 * - Botón DOWN → delta = +1  (navega al elemento siguiente)
 *
 * @note Completamente testeable en PC via fake de IDigitalInputSource.
 * @note Sin lectura directa de GPIO — el debounce es responsabilidad del
 *       DigitalInputAdapter (lwbtn, 20 ms).
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef SRC_PRESENTATION_UI_PORT_LVGL_ENCODER_ADAPTER_H_
#define SRC_PRESENTATION_UI_PORT_LVGL_ENCODER_ADAPTER_H_

#include "interfaces/i_encoder.h"
#include "interfaces/i_digital_input_source.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================
     * TIPO
     * ======================================================================== */

    /**
     * @brief Instancia del LvglEncoderAdapter.
     *
     * @note El campo @c base DEBE ser el primero del struct para que el puntero
     *       a la instancia pueda usarse como @c IEncoder_t* directamente.
     */
    typedef struct
    {
        IEncoder_t base;                /**< V-Table IEncoder_t (primer campo, no mover) */
        IDigitalInputSource *di_source; /**< Fuente de eventos de botones (inyectada) */

        volatile int16_t counter; /**< Acumulador de posición (UP--, DOWN++) */
        int16_t last_counter;     /**< Valor en el último Poll() para delta */
        volatile bool pressed;    /**< Estado del botón ENTER (press=true) */
    } LvglEncoderAdapter_t;

    /* ========================================================================
     * API PÚBLICA
     * ======================================================================== */

    /**
     * @brief Inicializa el adapter y suscribe los callbacks al IDigitalInputSource.
     *
     * Registra exactamente 6 callbacks:
     *   - @c DI_ID_BUTTON_UP   + @c DI_EVENT_PRESS     → counter--
     *   - @c DI_ID_BUTTON_UP   + @c DI_EVENT_KEEPALIVE → counter-- (scroll continuo)
     *   - @c DI_ID_BUTTON_DOWN + @c DI_EVENT_PRESS     → counter++
     *   - @c DI_ID_BUTTON_DOWN + @c DI_EVENT_KEEPALIVE → counter++ (scroll continuo)
     *   - @c DI_ID_BUTTON_ENTER + @c DI_EVENT_PRESS    → pressed = true
     *   - @c DI_ID_BUTTON_ENTER + @c DI_EVENT_RELEASE  → pressed = false
     *
     * @param[out] self       Instancia a inicializar (memoria provista por el caller).
     * @param[in]  di_source  IDigitalInputSource ya inicializado (DigitalInputAdapter).
     *
     * @return ERR_OK             Inicialización correcta.
     * @return ERR_NULL_POINTER   @p self o @p di_source es NULL.
     * @return ERR_BUSY           No hay slots de callback disponibles en di_source.
     */
    Result_t LvglEncoderAdapter_Init(LvglEncoderAdapter_t *self,
                                     IDigitalInputSource *di_source);

    /**
     * @brief Devuelve el puntero @c IEncoder_t para inyectar en @c UI_Port_Indev_Init.
     *
     * @param[in] self  Instancia inicializada.
     * @return Puntero válido a @c IEncoder_t, o NULL si @p self es NULL.
     */
    IEncoder_t *LvglEncoderAdapter_GetInterface(LvglEncoderAdapter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* SRC_PRESENTATION_UI_PORT_LVGL_ENCODER_ADAPTER_H_ */
