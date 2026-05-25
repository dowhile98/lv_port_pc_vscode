/**
 * @file ui_port_disp.h
 * @brief Puente entre I_Display (Clean Architecture) y el driver de display LVGL.
 *
 * Únicas responsabilidades (SRP):
 *   1. Crear y registrar un @c lv_display_t con @c flush_cb apuntando a @c I_Display.
 *   2. Exponer @c UI_Port_Disp_SignalFlushReady() para llamar desde el ISR/DMA.
 *
 * @note Solo este archivo y @c ui_port_indev.h incluyen @c lvgl.h en la capa de
 *       presentación. Ningún presentador, modelo ni servicio los incluirá directamente
 *       (Dependency Inversion Principle).
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef SRC_PRESENTATION_UI_PORT_UI_PORT_DISP_H_
#define SRC_PRESENTATION_UI_PORT_UI_PORT_DISP_H_

#include "interfaces/i_display.h"
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================
     * TIPOS PÚBLICOS
     * ======================================================================== */

    /**
     * @brief Configuración del port de display.
     */
    typedef struct
    {
        uint32_t hor_res;        /**< Resolución horizontal en píxeles (ej. 320). */
        uint32_t ver_res;        /**< Resolución vertical en píxeles (ej. 240). */
        uint32_t buf_size_lines; /**< Líneas de render por buffer (ej. 10). Ajusta RAM. */
    } UiPortDispConfig_t;

    /* ========================================================================
     * API PÚBLICA
     * ======================================================================== */

    /**
     * @brief Inicializa el port de display.
     *
     * Crea y registra en LVGL el @c lv_display_t con buffers de render doble
     * y el @c flush_cb que delega a @c I_Display->WriteArea con DMA.
     *
     * @pre @c lv_init() debe haberse llamado antes de esta función.
     * @pre Esta función debe llamarse antes de @c ui_init().
     *
     * @param[in] display  Implementación de I_Display (ST7789, mock, etc.).
     *                     Debe permanecer válida durante toda la vida del sistema.
     * @param[in] config   Resolución y parámetros de buffer. No puede ser NULL.
     *
     * @return ERR_OK           Inicialización correcta.
     * @return ERR_NULL_POINTER @p display o @p config es NULL, o vtable es NULL.
     * @return ERR_ERROR        @c lv_display_create() devolvió NULL.
     * @return Otro             Error devuelto por @c I_Display->Init().
     */
    Result_t UI_Port_Disp_Init(I_Display *display, const UiPortDispConfig_t *config);

    /**
     * @brief Señala a LVGL que la transferencia DMA ha completado.
     *
     * Llama a @c lv_display_flush_ready(), liberando a LVGL para continuar
     * el renderizado.
     *
     * @note ISR-safe: @c lv_display_flush_ready() es ISR-safe en LVGL 9.x cuando
     *       @c LV_USE_OS = @c LV_OS_THREADX.
     *
     * @note Debe ser llamado exactamente una vez por cada invocación del @c flush_cb
     *       que devolvió @c ERR_OK. Llamarlo más veces provoca comportamiento indefinido.
     */
    void UI_Port_Disp_SignalFlushReady(void);

/* ========================================================================
 * TEST UTILITIES (solo disponibles en UNIT_TEST)
 * ======================================================================== */
#ifdef UNIT_TEST
    /**
     * @brief Resetea el estado interno estático del módulo.
     *
     * @note Solo disponible cuando se compila con -DUNIT_TEST=1.
     *       No debe llamarse en producción.
     */
    void UI_Port_Disp_Reset_ForTest(void);
#endif /* UNIT_TEST */

#ifdef __cplusplus
}
#endif

#endif /* SRC_PRESENTATION_UI_PORT_UI_PORT_DISP_H_ */
