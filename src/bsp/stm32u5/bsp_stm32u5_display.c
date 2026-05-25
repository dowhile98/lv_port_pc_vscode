/**
 * @file bsp_stm32u5_display.c
 * @brief Inicialización de hardware del subsistema UI para STM32U5.
 *
 * Único archivo del BSP que cablea:
 *  - ST7789_Driver_t → I_Display (SPI del board profile)
 *  - LvglEncoderAdapter_t → IEncoder_t (sobre IDigitalInputSource inyectado)
 *
 * Los pines GPIO (RST, DC, CS, BL) y la resolución se leen en tiempo de
 * ejecución desde @c BSP_GetBoardProfile() → DisplayConfig_t, eliminando
 * cualquier referencia a defines hardcodeados de CubeMX en este archivo.
 *  - Pines CICX1-V5 definidos en: @c bsp_board_profiles.c → BOARD_PROFILE_CICX1_V5.display
 *  - SPI: display_spi_index → BSP_GetSPIHandle()
 *
 * @note Solo se compila para el firmware de hardware; no está en CMakeLists.txt
 *       de PC tests porque no hay tests de PC para esta capa.
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#include "bsp_stm32u5_display.h"
#include "bsp_init.h"
#include "bsp_board_profile.h"
#include "bsp_stm32u5_gpio.h"
#include "bsp_stm32u5_spi.h"

/* ST7789 driver y encoder bridge — ambos en BSP/presentation layer */
#include "bsp/components/display/st7789_driver.h"
#include "src/presentation/ui_port/lvgl_encoder_adapter.h"
#include "src/presentation/ui_port/ui_port_disp.h" /* UI_Port_Disp_SignalFlushReady */

/* HAL — solo permitido en BSP */
#include "stm32u5xx_hal.h"
/* main.h ya NO se incluye: los pines vienen del BoardProfile (bsp_board_profile.h) */

/* ========================================================================
 * BOUND GPIO — I_GPIO por pin con port+pin almacenados en impl
 *
 * ST7789 llama GPIO_WritePin(gpio, NULL, 0, state) — los parámetros
 * port/pin son NULL/0. La vtable bound_gpio_vtable extrae port+pin del
 * campo `impl` del I_GPIO, donde se almacena un puntero a esta struct.
 * ======================================================================== */

typedef struct
{
    GPIO_TypeDef *hal_port; /**< Puerto HAL (GPIOA, GPIOB, …) */
    uint16_t hal_pin;       /**< Pin HAL (GPIO_PIN_x) */
} BoundGpio_Ctx_t;

static Result_t bound_write_pin(void *impl,
                                GPIO_Port_t port,
                                GPIO_Pin_t pin,
                                GPIO_State_t state)
{
    (void)port;
    (void)pin;

    const BoundGpio_Ctx_t *ctx = (const BoundGpio_Ctx_t *)impl;
    const GPIO_PinState hst = (state == I_GPIO_STATE_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(ctx->hal_port, ctx->hal_pin, hst);
    return ERR_OK;
}

static Result_t bound_read_pin(void *impl,
                               GPIO_Port_t port,
                               GPIO_Pin_t pin,
                               GPIO_State_t *out)
{
    (void)port;
    (void)pin;

    if (out == NULL)
        return ERR_NULL_POINTER;
    const BoundGpio_Ctx_t *ctx = (const BoundGpio_Ctx_t *)impl;
    *out = (HAL_GPIO_ReadPin(ctx->hal_port, ctx->hal_pin) == GPIO_PIN_SET)
               ? I_GPIO_STATE_HIGH
               : I_GPIO_STATE_LOW;
    return ERR_OK;
}

/* Vtable mínima para operaciones de display (solo WritePin/ReadPin) */
static const I_GPIO_Vtable s_bound_gpio_vtable = {
    .Init = NULL,
    .DeInit = NULL,
    .WritePin = bound_write_pin,
    .ReadPin = bound_read_pin,
    .TogglePin = NULL,
    .WritePort = NULL,
    .ReadPort = NULL,
    .SetBits = NULL,
    .ResetBits = NULL,
    .LockPin = NULL,
};

/* ========================================================================
 * INSTANCIAS ESTÁTICAS — pin contexts + I_GPIO bound
 * ======================================================================== */

/* Inicializados en BSP_UI_HW_Init() desde el board profile (no hardcodeados). */
static BoundGpio_Ctx_t s_ctx_rst;
static BoundGpio_Ctx_t s_ctx_dc;
static BoundGpio_Ctx_t s_ctx_cs;
static BoundGpio_Ctx_t s_ctx_bl;

static I_GPIO s_gpio_rst = {.vtable = &s_bound_gpio_vtable, .impl = &s_ctx_rst};
static I_GPIO s_gpio_dc = {.vtable = &s_bound_gpio_vtable, .impl = &s_ctx_dc};
static I_GPIO s_gpio_cs = {.vtable = &s_bound_gpio_vtable, .impl = &s_ctx_cs};
static I_GPIO s_gpio_bl = {.vtable = &s_bound_gpio_vtable, .impl = &s_ctx_bl};

/* ========================================================================
 * INSTANCIAS DE DRIVER — sin malloc
 * ======================================================================== */

static ST7789_Driver_t s_st7789;
static LvglEncoderAdapter_t s_lvgl_encoder;

/* ========================================================================
 * DMA TX COMPLETE CALLBACK — contexto ISR
 *
 * Se ejecuta en el contexto de la IRQ del DMA cuando la transferencia SPI
 * termina. Debe ser extremadamente rápida.
 * ======================================================================== */

/**
 * @brief Firma compatible con I_SPI_Callback_t: void (*)(void*, uint16_t).
 */
static void spi_dma_tx_complete_cb(void *ctx, uint16_t size)
{
    (void)size;
    /* ST7789_SignalTransferComplete → llama internamente al dma_callback
     * registrado en el driver (= UI_Port_Disp_SignalFlushReady).
     * Toda la cadena se resuelve aquí en <50 µs (solo pasa flags). */
    ST7789_SignalTransferComplete((ST7789_Driver_t *)ctx);
}

/* ========================================================================
 * BSP_UI_HW_Init
 * ======================================================================== */

/**
 * @brief Inicializa el hardware del subsistema UI.
 *
 * Secuencia:
 *  1. Valida parámetros.
 *  2. Lee el perfil de placa activo para determinar SPI y resolución.
 *  3. Inicializa ST7789 con pines CubeMX + SPI BSP.
 *  4. Registra el callback DMA TX complete.
 *  5. Construye LvglEncoderAdapter sobre el DI source inyectado.
 *
 * @param[in]  di_source     Fuente de eventos de botones (del DI container).
 * @param[out] out_display   I_Display lista para UI_Port_Disp_Init().
 * @param[out] out_encoder   IEncoder_t listo para UI_Port_Indev_Init().
 */
Result_t BSP_UI_HW_Init(IDigitalInputSource *di_source,
                        I_Display **out_display,
                        IEncoder_t **out_encoder)
{
    if (di_source == NULL || out_display == NULL || out_encoder == NULL)
    {
        return ERR_NULL_POINTER;
    }

    const BoardProfile_t *profile = BSP_GetBoardProfile();
    if (!profile->display.enabled)
    {
        /* Variante sin display (ej. SSR3-V1): devolver stubs nulos
         * para que el hilo GUI omita la inicialización del display. */
        *out_display = NULL;
        *out_encoder = NULL;
        return ERR_OK;
    }

    Result_t res;

    /* ------------------------------------------------------------------ */
    /* 1. Vincular pines GPIO desde el board profile (Single Source of Truth) */
    /* ------------------------------------------------------------------ */

    s_ctx_rst.hal_port = profile->display.rst_port;
    s_ctx_rst.hal_pin = profile->display.rst_pin;
    s_ctx_dc.hal_port = profile->display.dc_port;
    s_ctx_dc.hal_pin = profile->display.dc_pin;
    s_ctx_cs.hal_port = profile->display.cs_port;
    s_ctx_cs.hal_pin = profile->display.cs_pin;
    s_ctx_bl.hal_port = profile->display.bl_port;
    s_ctx_bl.hal_pin = profile->display.bl_pin;

    /* ------------------------------------------------------------------ */
    /* 2. Configurar ST7789                                                 */
    /* ------------------------------------------------------------------ */

    I_SPI *const spi = Bsp_Stm32U5_Spi_GetInterface();
    void *const handle = BSP_GetSPIHandle(profile->peripheral_map.display_spi_index);

    const ST7789_Config_t st7789_cfg = {
        .width = profile->display.width,   /* 320 — dimensión lógica X en landscape */
        .height = profile->display.height, /* 240 — dimensión lógica Y en landscape */
        .color_format = ST7789_COLOR_FORMAT_RGB565,
        /* Equivale a Init(3) del driver de referencia:
         * OrientationTab[3] = MY | MV = 0xA0 (LV_DISPLAY_ROTATION_270).
         * Rota el panel físico 240×320 → pantalla lógica 320×240. */
        .orientation = I_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED,
        .invert_colors = false,
        .initial_brightness = 80U,
    };

    res = ST7789_Init(&s_st7789,
                      spi,
                      handle,
                      &s_gpio_rst,
                      &s_gpio_dc,
                      &s_gpio_cs,
                      &s_gpio_bl,
                      &st7789_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    /* ------------------------------------------------------------------ */
    /* 2. Registrar callback DMA → ST7789 → UI_Port_Disp_SignalFlushReady  */
    /* ------------------------------------------------------------------ */

    res = SPI_RegisterTxCallback(spi, handle, spi_dma_tx_complete_cb, &s_st7789);
    if (res != ERR_OK)
    {
        return res;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Inicializar bridge encoder digital input → IEncoder_t             */
    /* ------------------------------------------------------------------ */

    res = LvglEncoderAdapter_Init(&s_lvgl_encoder, di_source);
    if (res != ERR_OK)
    {
        return res;
    }

    *out_display = &s_st7789.base;
    *out_encoder = LvglEncoderAdapter_GetInterface(&s_lvgl_encoder);

    return ERR_OK;
}
