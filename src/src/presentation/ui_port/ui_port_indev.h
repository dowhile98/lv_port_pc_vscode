/**
 * @file ui_port_indev.h
 * @brief Registra un IEncoder_t en LVGL como dispositivo de entrada encoder.
 *
 * Responsabilidades (SRP):
 *   1. Crear y configurar @c lv_indev_t tipo @c LV_INDEV_TYPE_ENCODER.
 *   2. Crear @c lv_group_t y establecerlo como grupo por defecto.
 *   3. Vincular el indev al grupo.
 *
 * @note Solo este archivo y @c ui_port_disp.h incluyen @c lvgl.h en la capa
 *       de presentación.
 *
 * @note Debe llamarse ANTES de @c ui_init() para que los widgets generados
 *       por EEZ Studio se registren automáticamente en el grupo.
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef SRC_PRESENTATION_UI_PORT_UI_PORT_INDEV_H_
#define SRC_PRESENTATION_UI_PORT_UI_PORT_INDEV_H_

#include "interfaces/i_encoder.h"
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Inicializa el indev de encoder en LVGL.
     *
     * Orden interno (crítico):
     * -# @c lv_indev_create() + @c lv_indev_set_type(ENCODER) + @c lv_indev_set_read_cb()
     * -# @c lv_group_create() + @c lv_group_set_default()
     * -# @c lv_indev_set_group()
     *
     * @param[in] encoder  Encoder ya inicializado (LvglEncoderAdapter o real).
     *                     Debe permanecer válido durante toda la vida del sistema.
     *
     * @return ERR_OK           Inicialización correcta.
     * @return ERR_NULL_POINTER @p encoder es NULL, vtable NULL o Poll NULL.
     * @return ERR_ERROR        @c lv_indev_create() o @c lv_group_create() devolvió NULL.
     */
    Result_t UI_Port_Indev_Init(IEncoder_t *encoder);

/* ========================================================================
 * TEST UTILITIES (solo con -DUNIT_TEST=1)
 * ======================================================================== */
#ifdef UNIT_TEST
    /**
     * @brief Resetea el estado interno estático del módulo.
     * @note No usar en producción.
     */
    void UI_Port_Indev_Reset_ForTest(void);
#endif /* UNIT_TEST */

#ifdef __cplusplus
}
#endif

#endif /* SRC_PRESENTATION_UI_PORT_UI_PORT_INDEV_H_ */
