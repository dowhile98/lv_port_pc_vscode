/**
 * @file ui_port_indev.c
 * @brief Implementación del port de entrada encoder para LVGL.
 *
 * ÚNICO archivo de la capa de presentación que incluye lvgl.h para
 * la parte de entrada de usuario. Ningún presentador ni modelo lo incluirá.
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#include "ui_port_indev.h"
#include "lvgl.h"

/* ========================================================================
 * ESTADO PRIVADO ESTÁTICO (sin malloc)
 * ======================================================================== */

static IEncoder_t *s_encoder = NULL;
static lv_indev_t *s_indev = NULL;
static lv_group_t *s_group = NULL;

/* ========================================================================
 * READ CALLBACK — invocado periódicamente por lv_task_handler()
 * ======================================================================== */

/**
 * @brief Callback de lectura del indev encoder.
 *
 * LVGL llama a esta función en cada ciclo de @c lv_task_handler() para
 * obtener el estado actual del encoder.
 * Contexto: thread de LVGL refresh (no ISR).
 *
 * @param indev  Indev LVGL (no utilizado directamente).
 * @param data   Estructura de datos de LVGL a rellenar.
 */
static void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (s_encoder == NULL)
    {
        return;
    }

    int32_t delta = 0;
    bool pressed = false;
    Encoder_Poll(s_encoder, &delta, &pressed);

    data->enc_diff = (int16_t)delta;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* ========================================================================
 * API PÚBLICA
 * ======================================================================== */

Result_t UI_Port_Indev_Init(IEncoder_t *encoder)
{
    if (!encoder_is_valid(encoder))
    {
        return ERR_NULL_POINTER;
    }

    s_encoder = encoder;

    /* 1. Crear y configurar indev tipo encoder */
    s_indev = lv_indev_create();
    if (s_indev == NULL)
    {
        return ERR_ERROR;
    }

    lv_indev_set_type(s_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(s_indev, encoder_read_cb);

    /* 2. Crear grupo LVGL y establecer como default.
     *    CRÍTICO: hacerlo ANTES de ui_init() para que los widgets generados
     *    por EEZ Studio se registren automáticamente en este grupo. */
    s_group = lv_group_create();
    if (s_group == NULL)
    {
        return ERR_ERROR;
    }

    lv_group_set_default(s_group);

    /* 3. Vincular indev al grupo */
    lv_indev_set_group(s_indev, s_group);

    return ERR_OK;
}

/* ========================================================================
 * TEST UTILITIES
 * ======================================================================== */
#ifdef UNIT_TEST
void UI_Port_Indev_Reset_ForTest(void)
{
    s_encoder = NULL;
    s_indev = NULL;
    s_group = NULL;
}
#endif /* UNIT_TEST */
