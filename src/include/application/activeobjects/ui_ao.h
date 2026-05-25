/**
 * @file ui_ao.h
 * @brief Active Object del subsistema UI — hilo dedicado al refresh LVGL.
 *
 * Sigue el mismo patrón que @c NetworkAO_t y @c BuzzerAO_t:
 *  - Struct autocontenido con stack y thread estáticos (sin malloc).
 *  - @c UiAO_Init() configura el AO sin arrancar el hilo.
 *  - @c UiAO_Start() arranca el hilo.
 *
 * ## Responsabilidades del hilo (SRP)
 *
 *  1. Inicializar hardware UI via BSP (display ST7789 + encoder).
 *  2. Inicializar LVGL.
 *  3. Ejecutar @c ui_init() de EEZ Studio.
 *  4. Loop: @c ui_tick() + @c lv_task_handler() + @c os_thread_sleep().
 *
 * ## Swapabilidad de motor UI
 *
 * Los Presenters (ACTION-024) solo dependen de @c IMainScreenView_t.
 * Si se reemplaza LVGL por u8g2 u otro motor, solo cambian:
 *  - @c ui_port_disp.c  (callback de flush/render)
 *  - @c ui_port_indev.c (callback de lectura indev)
 *  - @c ui_ao.c         (swap de @c lv_task_handler por equivalente)
 *  - @c src/presentation/ui/ (archivos generados por el nuevo tool)
 *
 * Los Presenters NO cambian — cumplen MVP.
 *
 * ## Capas y dependencias
 *
 *  - Este archivo vive en @c src/presentation/ui_port/ (presentation infrastructure).
 *  - **PROHIBIDO** incluir @c stm32u5xx_hal.h o cualquier header de dominio aquí.
 *  - @c lvgl.h solo se incluye en el @c .c (implementation detail).
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef UI_AO_H
#define UI_AO_H

#include "common/task_priorities.h" /* Mapa centralizado de prioridades */
#include "hal/hal_types.h"
#include "infrastructure/osal/osal.h"
#include "interfaces/i_digital_input_source.h"
#include "interfaces/i_display.h"
#include "interfaces/i_encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================================================================
 * PARÁMETROS DE SIZING (ajustables por board profile)
 * ======================================================================== */

/** Stack del hilo GUI en bytes. LVGL + doble buffer requieren al menos 4 KB. */
#ifndef UI_AO_STACK_SIZE
#define UI_AO_STACK_SIZE (1024 * 12U)
#endif

/**
 * @brief Prioridad del hilo GUI.
 *
 * Tier 5 (presentación) — la más baja del sistema.  El display es cosmético
 * y no debe desalojar la pila de red, el control loop ni el safety relay.
 * Ver @c task_priorities.h para la tabla completa.
 */
#ifndef UI_AO_THREAD_PRIORITY
#define UI_AO_THREAD_PRIORITY (TASK_PRIO_UI)
#endif

    /* ========================================================================
     * CONFIGURACIÓN (inyectada en UiAO_Init)
     * ======================================================================== */

    /**
     * @brief Dependencias inyectadas al AO de UI.
     *
     * @note Todos los punteros deben ser válidos antes de llamar @c UiAO_Init().
     */
    typedef struct
    {
        IDigitalInputSource *di_source; /**< Fuente de eventos de botones (del DI container). */
        uint16_t display_width;         /**< Resolución horizontal inyectada desde BoardProfile. */
        uint16_t display_height;        /**< Resolución vertical inyectada desde BoardProfile. */
    } UiAO_Config_t;

    /* ========================================================================
     * ESTADO DEL ACTIVE OBJECT
     * ======================================================================== */

    /**
     * @brief Estados del Active Object de UI.
     */
    typedef enum
    {
        UI_AO_STATE_UNINITIALIZED = 0, /**< Antes de UiAO_Init(). */
        UI_AO_STATE_IDLE,              /**< Init OK, hilo suspendido. */
        UI_AO_STATE_RUNNING,           /**< Hilo activo y renderizando. */
        UI_AO_STATE_ERROR,             /**< Fallo en init de hardware UI. */
    } UiAO_State_t;

    /* ========================================================================
     * STRUCT DEL ACTIVE OBJECT
     * ======================================================================== */

    /**
     * @brief Active Object del subsistema UI.
     *
     * @note Instanciar una sola vez, en memoria estática (nunca en stack).
     * @note No acceder directamente a los campos internos; usar la API pública.
     */
    typedef struct
    {
        /* ---- Dependencias inyectadas ---- */
        IDigitalInputSource *di_source;
        uint16_t display_width;  /**< Resolución horizontal (inyectada en Init). */
        uint16_t display_height; /**< Resolución vertical (inyectada en Init). */

        /* ---- Hardware interfaces (populated by UiAO_Init via BSP) ---- */
        I_Display *display_if;  /**< ST7789 I_Display — used by LVGL thread. */
        IEncoder_t *encoder_if; /**< LvglEncoderAdapter IEncoder_t — injected into Presenters. */

        /* ---- Estado ---- */
        UiAO_State_t state;

        /* ---- OSAL ---- */
        os_thread_t thread;

        /* ---- Buffers estáticos (sin malloc) ---- */
        uint8_t _stack[UI_AO_STACK_SIZE];

    } UiAO_t;

    /* ========================================================================
     * API PÚBLICA
     * ======================================================================== */

    /**
     * @brief Inicializa el AO de UI (sin hilo, sin mutex dedicado).
     *
     * El hilo NO arranca aquí. Llamar @c UiAO_Start() después.
     *
     * @param[in,out] self  Instancia (no NULL, memoria estática del caller).
     * @param[in]     cfg   Configuración con dependencias inyectadas (no NULL).
     *
     * @return ERR_OK            Inicialización exitosa.
     * @return ERR_NULL_POINTER  Si @p self, @p cfg o @c cfg->di_source son NULL.
     * @return ERR_ERROR         Si falla la inicialización interna.
     */
    Result_t UiAO_Init(UiAO_t *self, const UiAO_Config_t *cfg);

    /**
     * @brief Arranca el hilo del AO de UI.
     *
     * Crea y activa el hilo OSAL. El hilo inicializa el hardware UI,
     * LVGL y EEZ Studio de forma interna.
     *
     * @param[in,out] self  Instancia inicializada (no NULL).
     *
     * @return ERR_OK           Hilo creado y arrancado.
     * @return ERR_NULL_POINTER Si @p self es NULL.
     * @return ERR_BUSY         Si el AO ya está en estado @c UI_AO_STATE_RUNNING.
     * @return ERR_ERROR        Si falla la creación del hilo OSAL.
     */
    Result_t UiAO_Start(UiAO_t *self);

    /**
     * @brief Obtiene el estado actual del AO.
     *
     * @param[in] self  Instancia (no NULL).
     * @return Estado actual (@c UiAO_State_t).
     */
    UiAO_State_t UiAO_GetState(const UiAO_t *self);

    /**
     * @brief Returns the IEncoder_t interface initialised during UiAO_Init().
     *
     * Available after a successful UiAO_Init() — before UiAO_Start().
     * The DI container uses this to inject the encoder into Presenters.
     *
     * @param[in] self  Initialised UiAO instance (must not be NULL).
     * @return Pointer to IEncoder_t, or NULL if not yet initialised.
     */
    IEncoder_t *UiAO_GetEncoderInterface(UiAO_t *self);

#ifdef __cplusplus
}
#endif

#endif /* UI_AO_H */
