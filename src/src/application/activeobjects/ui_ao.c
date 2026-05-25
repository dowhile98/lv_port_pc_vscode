/**
 * @file ui_ao.c
 * @brief Active Object del subsistema UI — implementación del hilo LVGL.
 *
 * ## Secuencia de arranque del hilo (orden crítico)
 *
 *  1. @c BSP_UI_HW_Init()       — inicia ST7789 (I_Display) + encoder (IEncoder_t)
 *  2. @c lv_init()              — inicializa motor LVGL (debe ir antes que cualquier lv_*)
 *  3. @c UI_Port_Disp_Init()    — registra lv_display_t con flush_cb → I_Display
 *  4. @c UI_Port_Indev_Init()   — registra lv_indev_t encoder + lv_group por defecto
 *  5. @c ui_init()              — carga screens generadas por EEZ Studio
 *  6. loop: lv_task_handler() (lv_lock/unlock interno con LV_OS_CUSTOM) → sleep
 *
 * ## Acceso LVGL desde Presenters (ACTION-024 en adelante)
 *
 * Los Presenters NO pueden llamar @c lv_*() directamente porque LVGL no es
 * thread-safe. Con LV_OS_CUSTOM activo, usar las funciones nativas de LVGL:
 * ```c
 * lv_lock();   // adquiere lv_general_mutex (TX_MUTEX)
 * // ... actualizar widgets via IMainScreenView_t::UpdateXxx() ...
 * lv_unlock();
 * ```
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */

#include "application/activeobjects/ui_ao.h"
#include "presentation/ui_actions.h" /* ui_actions_init() — ACTION-025 */
#include "src/presentation/ui_port/ui_port_disp.h"
#include "src/presentation/ui_port/ui_port_indev.h"
#include "bsp/stm32u5/bsp_stm32u5_display.h"
#include "infrastructure/osal/osal.h"
#include "interfaces/i_digital_input_source.h"

/* EEZ / LVGL headers (solo aquí y en ui_port_*.c — nunca en Presenters) */
#include "lvgl.h"
#include "ui/ui.h"

#include <string.h>

/*============================================================================*
 * CONFIGURATION
 *============================================================================*/

/** Líneas por buffer LVGL (compromiso RAM/performance). */
#define UI_AO_RENDER_LINES (20U)

/** Periodo de iteración del loop LVGL en ms. */
#define UI_AO_LOOP_PERIOD_MS (10U)

/*============================================================================*
 * PRIVATE — LVGL SYSTEM CALLBACKS
 *============================================================================*/

/**
 * @brief Wrapper de delay para LVGL.
 *
 * lv_delay_cb_t requiere void(*)(uint32_t); os_thread_sleep retorna Result_t
 * así que se necesita este adaptador.
 *
 * @param[in] ms  Milisegundos a dormir.
 */
static void lvgl_delay_cb(uint32_t ms)
{
    (void)os_thread_sleep(ms);
}

/*============================================================================*
 * PRIVATE — THREAD ENTRY
 *============================================================================*/

/**
 * @brief Punto de entrada del hilo GUI.
 *
 * @note Se ejecuta en contexto de hilo OSAL; NUNCA llamar directamente.
 * @param[in] arg  Puntero a @c UiAO_t (inyectado por @c UiAO_Start()).
 */
static void ui_ao_thread_entry(void *arg)
{
    UiAO_t *self = (UiAO_t *)arg;

    /* 1. Hardware already initialised in UiAO_Init() — use stored interfaces.
     *    (BSP_UI_HW_Init was called synchronously in UiAO_Init so that the
     *     DI container can extract the encoder interface before the thread starts.) */
    I_Display *display = self->display_if;
    IEncoder_t *encoder = self->encoder_if;
    Result_t res;

    /* 2. Inicializar motor LVGL. */
    lv_init();

    /* Fuente de tiempo — OBLIGATORIO para que LVGL funcione.
     * os_ticks_get() retorna ms desde boot (implementado sobre tx_time_get). */
    lv_tick_set_cb((lv_tick_get_cb_t)os_ticks_get);
    lv_delay_set_cb(lvgl_delay_cb);

    /* 3. Registrar display en LVGL. */
    UiPortDispConfig_t disp_cfg = {
        .hor_res = self->display_width,  /* Leído del BoardProfile vía UiAO_Config_t */
        .ver_res = self->display_height, /* Leído del BoardProfile vía UiAO_Config_t */
        .buf_size_lines = UI_AO_RENDER_LINES,
    };
    res = UI_Port_Disp_Init(display, &disp_cfg);
    if (res != ERR_OK)
    {
        self->state = UI_AO_STATE_ERROR;
        return;
    }

    /* 4. Registrar encoder como indev + crear grupo por defecto.
     *    CRÍTICO: antes de ui_init() para que widgets se auto-registren. */
    res = UI_Port_Indev_Init(encoder);
    if (res != ERR_OK)
    {
        self->state = UI_AO_STATE_ERROR;
        return;
    }

    /* 5. Inicializar UI generada por EEZ Studio. */
    ui_init();
    ui_actions_init(); /* Register overview action not wired by EEZ — ACTION-025 */

    /* 6. Loop de render. */
    self->state = UI_AO_STATE_RUNNING;

    for (;;)
    {
        /* Drive button debounce — mirrors DigitalInputAO task on firmware.
         * Must be called at ~20ms intervals for lwbtn timing to be accurate. */
        (void)DigitalInputSource_Process(self->di_source);

        /* Con LV_OS_CUSTOM activo, lv_task_handler() adquiere y libera
         * lv_general_mutex (TX_MUTEX) internamente — no necesitamos el mutex
         * manual. Para acceso externo desde Presenters (ACTION-024), usar:
         *   lv_lock();  ... actualizar widgets ...  lv_unlock(); */
        ui_tick();                                  /* Animaciones y lógica EEZ. */
        uint32_t sleep_time_ms = lv_task_handler(); /* Procesa eventos + redibuja; gestiona lv_lock internamente. */

        if (sleep_time_ms == LV_NO_TIMER_READY)
        {
            sleep_time_ms = LV_DEF_REFR_PERIOD;
        }

        /* Cap sleep to UI_AO_LOOP_PERIOD_MS so Process() is called regularly. */
        if (sleep_time_ms > UI_AO_LOOP_PERIOD_MS)
        {
            sleep_time_ms = UI_AO_LOOP_PERIOD_MS;
        }

        os_thread_sleep(sleep_time_ms);
    }
}

/*============================================================================*
 * PUBLIC API
 *============================================================================*/

/**
 * @copydoc UiAO_Init
 */
Result_t UiAO_Init(UiAO_t *self, const UiAO_Config_t *cfg)
{
    if (self == NULL || cfg == NULL || cfg->di_source == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Limpieza determinista (sin malloc). */
    memset(self, 0, sizeof(UiAO_t));

    self->di_source = cfg->di_source;
    self->display_width = cfg->display_width;
    self->display_height = cfg->display_height;
    self->state = UI_AO_STATE_IDLE;

    /* Initialise BSP hardware (ST7789 display + LvglEncoderAdapter) synchronously.
     * This MUST happen before UiAO_Start() so the DI container can retrieve
     * the encoder interface and inject it into Presenters via UiAO_GetEncoderInterface(). */
    Result_t res = BSP_UI_HW_Init(self->di_source, &self->display_if, &self->encoder_if);
    if (res != ERR_OK)
    {
        self->state = UI_AO_STATE_ERROR;
        return res;
    }

    /* Sincronización LVGL gestionada internamente por lv_lock()/lv_unlock()
     * (LV_OS_CUSTOM → TX_MUTEX en lv_general_mutex). Los Presenters usan
     * lv_lock()/lv_unlock() directamente; no se necesita mutex adicional. */
    return ERR_OK;
}

IEncoder_t *UiAO_GetEncoderInterface(UiAO_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->encoder_if;
}

/**
 * @copydoc UiAO_Start
 */
Result_t UiAO_Start(UiAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (self->state == UI_AO_STATE_RUNNING)
    {
        return ERR_BUSY;
    }

    if (self->state != UI_AO_STATE_IDLE)
    {
        return ERR_ERROR;
    }

    os_thread_config_t thread_cfg = {
        .name = "ui_ao",
        .priority = UI_AO_THREAD_PRIORITY,
        .stack_ptr = self->_stack,
        .stack_size = UI_AO_STACK_SIZE,
        .entry = ui_ao_thread_entry,
        .arg = self,
        .auto_start = true,
    };

    Result_t res = os_thread_create(&self->thread, &thread_cfg);
    if (res != ERR_OK)
    {
        self->state = UI_AO_STATE_ERROR;
        return ERR_ERROR;
    }

    /* Estado -> RUNNING se asigna dentro del hilo tras init exitoso. */
    return ERR_OK;
}

/**
 * @copydoc UiAO_GetState
 */
UiAO_State_t UiAO_GetState(const UiAO_t *self)
{
    if (self == NULL)
    {
        return UI_AO_STATE_UNINITIALIZED;
    }
    return self->state;
}
