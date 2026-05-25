/**
 * @file ui_port_disp.c
 * @brief Implementación del puente I_Display → LVGL display driver.
 *
 * ÚNICO archivo de la capa de presentación que incluye lvgl.h para la parte
 * gráfica de display. Ningún presentador ni modelo incluirá este archivo.
 *
 * ## Patrón flush DMA + LV_OS_CUSTOM
 *
 * Con LV_OS_CUSTOM activo, lv_display_flush_ready() NO puede llamarse desde
 * ISR porque LVGL internamente usa lv_lock_isr() → TX_MUTEX desde ISR → illegal.
 *
 * Solución: patrón flush_wait_cb:
 *  1. flush_cb        → inicia DMA, no llama flush_ready.
 *  2. DMA ISR         → UI_Port_Disp_SignalFlushReady() → pone semáforo (ISR-safe).
 *  3. flush_wait_cb   → toma semáforo (bloquea hilo LVGL) → llama flush_ready (hilo).
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#include "ui_port_disp.h"
#include "lvgl.h"
#include "infrastructure/osal/osal.h"

/* ========================================================================
 * ESTADO PRIVADO ESTÁTICO (sin malloc)
 * ======================================================================== */

/**
 * Tamaño máximo del buffer en píxeles.
 * ST7789 320 × 20 líneas × 2 bytes/px (RGB565) = 12 800 bytes por buffer.
 *
 * NOTA LVGL 9.x: lv_color_t es SIEMPRE struct {blue, green, red} (3 bytes),
 * independientemente de LV_COLOR_DEPTH. Con LV_COLOR_DEPTH=16 el tipo nativo
 * del buffer de render es lv_color16_t (2 bytes, RGB565). Usar lv_color_t
 * provocaría buffers sobredimensionados y datos corruptos al enviarlos al SPI.
 */
#ifndef UI_PORT_DISP_MAX_WIDTH
#define UI_PORT_DISP_MAX_WIDTH 320U /**< Ancho máximo soportado (px). */
#endif
#define UI_PORT_DISP_MAX_BUF_LINES 20U /**< Líneas máximas por buffer. */
#define UI_PORT_DISP_MAX_BUF_SIZE (UI_PORT_DISP_MAX_WIDTH * UI_PORT_DISP_MAX_BUF_LINES)

static I_Display *s_display = NULL;
static lv_display_t *s_lv_disp = NULL;

/**
 * Semáforo binario: ISR pone (tx_semaphore_put) cuando DMA termina;
 * flush_wait_cb toma (bloqueante) antes de notificar a LVGL.
 * Asignado desde el byte pool ThreadX en UI_Port_Disp_Init().
 */
static os_semaphore_t s_flush_sem = NULL;

/* Doble buffer estático RGB565 (lv_color16_t = 2 bytes/px) */
static lv_color16_t s_buf1[UI_PORT_DISP_MAX_BUF_SIZE]; /* buffer primario  */
static lv_color16_t s_buf2[UI_PORT_DISP_MAX_BUF_SIZE]; /* buffer secundario */

/* ========================================================================
 * FLUSH WAIT CALLBACK — espera fin de DMA en contexto de hilo
 * ======================================================================== */

/**
 * @brief Bloquea el hilo LVGL hasta que el ISR DMA confirme fin de transferencia.
 *
 * LVGL 9.x llama este callback (registrado vía lv_display_set_flush_wait_cb)
 * justo después de flush_cb para esperar a que el hardware termine. Garantiza
 * que lv_display_flush_ready() siempre se llama desde contexto de hilo,
 * nunca desde ISR — requerido por TX_MUTEX (lv_general_mutex) de LVGL.
 *
 * @param disp  Display LVGL activo.
 */
static void flush_wait_cb(lv_display_t *disp)
{
    if (s_flush_sem != NULL)
    {
        /* Bloquea hasta que UI_Port_Disp_SignalFlushReady() señalice el semáforo. */
        (void)os_semaphore_get(s_flush_sem, OS_WAIT_FOREVER);
    }
    /* Notificar a LVGL que el buffer puede reutilizarse.
     * Siempre en contexto de hilo → lv_lock() es re-entrante con TX_MUTEX. */
    lv_display_flush_ready(disp);
}

/* ========================================================================
 * FLUSH CALLBACK — invocado por LVGL para transferir un tile al display
 * ======================================================================== */

/**
 * @brief Callback que LVGL llama para transferir un tile renderizado al display.
 *
 * Inicia la transferencia DMA y retorna inmediatamente. El hilo LVGL queda
 * bloqueado en flush_wait_cb() hasta que el ISR DMA señalice s_flush_sem.
 *
 * @param disp    Display LVGL registrado.
 * @param area    Área rectangular del tile a transferir.
 * @param px_map  Puntero al buffer de píxeles (formato nativo: RGB565).
 */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (s_display == NULL || s_display->vtable == NULL)
    {
        /* Sin display inicializado: liberar LVGL para no bloquearlo */
        lv_display_flush_ready(disp);
        return;
    }

    const uint16_t x1 = (uint16_t)area->x1;
    const uint16_t y1 = (uint16_t)area->y1;
    const uint16_t x2 = (uint16_t)area->x2;
    const uint16_t y2 = (uint16_t)area->y2;

    /*
     * Inicia la transferencia DMA.
     * UI_Port_Disp_SignalFlushReady() es el callback que llamará lv_display_flush_ready()
     * cuando el ISR de DMA SPI notifique el fin de transferencia.
     * NO llamar lv_display_flush_ready() aquí: LVGL debe quedar bloqueado hasta que
     * el hardware confirme que el buffer puede reutilizarse.
     */
    const Result_t res = Display_WriteArea(
        s_display,
        x1, y1, x2, y2,
        (uint8_t *)px_map,
        UI_Port_Disp_SignalFlushReady /* DMA-complete ISR → pone s_flush_sem */
    );

    if (res != ERR_OK)
    {
        /* DMA no pudo iniciarse: señalizar el semáforo para desbloquear
         * flush_wait_cb y notificar a LVGL de inmediato. */
        if (s_flush_sem != NULL)
        {
            (void)os_semaphore_put(s_flush_sem);
        }
        else
        {
            lv_display_flush_ready(disp);
        }
    }
    /* Si DMA inició OK: flush_wait_cb bloqueará hasta UI_Port_Disp_SignalFlushReady() */
}

/* ========================================================================
 * API PÚBLICA
 * ======================================================================== */

Result_t UI_Port_Disp_Init(I_Display *display, const UiPortDispConfig_t *config)
{
    if (display == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (display->vtable == NULL)
    {
        return ERR_NULL_POINTER;
    }

    s_display = display;

    /* Crear semáforo binario para sincronización DMA ↔ flush_wait_cb.
     * Asignado desde byte pool ThreadX (determinista, no usa malloc). */
    const Result_t sem_res = os_semaphore_create(&s_flush_sem, "lvgl_flush", 0U);
    if (sem_res != ERR_OK)
    {
        return sem_res;
    }

    /* Inicializar hardware del display (reset, wake, pixel format, etc.) */
    const Result_t res = Display_Init(s_display);

    if (res != ERR_OK)
    {
        return res;
    }

    /* Crear lv_display_t en LVGL */
    /* NOTA: NO llamar Display_SetOrientation aquí — el display se inicializa en
     * PORTRAIT (MADCTL=0x00) dentro de st7789_configure(), que es la orientación
     * correcta para este panel. Cambiarla aquí desincroniza LVGL con el hardware. */
    s_lv_disp = lv_display_create((int32_t)config->hor_res, (int32_t)config->ver_res);
    if (s_lv_disp == NULL)
    {
        return ERR_ERROR;
    }

    /* Calcular tamaño del buffer respetando el máximo estático */
    uint32_t buf_size = config->hor_res * config->buf_size_lines;
    if (buf_size > UI_PORT_DISP_MAX_BUF_SIZE)
    {
        buf_size = UI_PORT_DISP_MAX_BUF_SIZE;
    }

    /* Registrar doble buffer DMA en LVGL (modo partial render) */
    lv_display_set_buffers(s_lv_disp,
                           s_buf1, s_buf2,
                           buf_size * sizeof(lv_color16_t), /* 2 bytes/px RGB565 */
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Registrar flush callback */
    lv_display_set_flush_cb(s_lv_disp, flush_cb);

    /* Registrar flush_wait_cb: LVGL llama este callback en el hilo de render
     * para esperar fin de DMA. Garantiza que lv_display_flush_ready() siempre
     * se llame desde hilo (nunca ISR) — requerido con LV_OS_CUSTOM/TX_MUTEX. */
    lv_display_set_flush_wait_cb(s_lv_disp, flush_wait_cb);

    return ERR_OK;
}

void UI_Port_Disp_SignalFlushReady(void)
{
    /* Llamado desde ISR DMA. tx_semaphore_put() es ISR-seguro en ThreadX.
     * NO llama lv_display_flush_ready() directamente — flush_wait_cb lo hace
     * en contexto de hilo para evitar TX_MUTEX desde ISR. */
    if (s_flush_sem != NULL)
    {
        (void)os_semaphore_put(s_flush_sem);
    }
}

/* ========================================================================
 * TEST UTILITIES
 * ======================================================================== */
#ifdef UNIT_TEST
void UI_Port_Disp_Reset_ForTest(void)
{
    s_display = NULL;
    s_lv_disp = NULL;
    s_flush_sem = NULL;
}
#endif /* UNIT_TEST */
