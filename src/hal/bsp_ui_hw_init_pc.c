/**
 * @file bsp_ui_hw_init_pc.c
 * @brief PC implementation of BSP_UI_HW_Init — SDL display + LvglEncoderAdapter.
 *
 * Provides the SAME public function as bsp_stm32u5_display.c (STM32U5 target)
 * but adapted for the PC simulator:
 *
 *   BSP_UI_HW_Init(di_source, &display_out, &encoder_out)
 *     │
 *     ├─ encoder_out ← LvglEncoderAdapter_Init(di_source)
 *     │                 (identical to firmware — no changes needed)
 *     │
 *     └─ display_out ← &s_sdl_display  (SdlDisplayAdapter)
 *                       Init() deferred to UI_Port_Disp_Init() call in
 *                       ui_ao_thread_entry → calls lv_sdl_window_create()
 *                       AFTER lv_init(), as required by LVGL.
 *
 * IMPORTANT — CMakeLists.txt integration:
 *   For PC builds, compile this file INSTEAD OF bsp_stm32u5_display.c.
 *   Also compile ui_port_disp_pc.c INSTEAD OF ui_port_disp.c to avoid
 *   the double-display conflict (ui_port_disp.c calls lv_display_create()
 *   internally; ui_port_disp_pc.c uses lv_sdl_window_create() directly).
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "bsp/stm32u5/bsp_stm32u5_display.h"
#include "presentation/ui_port/lvgl_encoder_adapter.h"
#include "interfaces/i_display.h"
#include <string.h>

/* ============================================================================
 * SDL DISPLAY ADAPTER — minimal I_Display stub for PC
 *
 * UI_Port_Disp_PC_Init (ui_port_disp_pc.c) receives this adapter and calls
 * Display_Init() on it.  The Init vtable function creates the actual SDL
 * window at the right time (after lv_init()).
 *
 * WriteArea and the other vtable entries are no-ops: SDL manages rendering
 * via its own flush callback registered inside lv_sdl_window_create().
 * ========================================================================== */

typedef struct
{
    I_Display base; /**< MUST be first — allows cast to I_Display* */
    uint32_t hor_res;
    uint32_t ver_res;
} SdlDisplayAdapter_t;

/* Forward declarations for vtable */
static Result_t sdl_disp_init(void *self_ptr);
static Result_t sdl_disp_write_area(void *self_ptr,
                                    uint16_t x1, uint16_t y1,
                                    uint16_t x2, uint16_t y2,
                                    uint8_t *data,
                                    Display_TransferCompleteCallback_t cb);
static Result_t sdl_disp_set_brightness(void *self_ptr, uint8_t percent);
static Result_t sdl_disp_set_orientation(void *self_ptr, Display_Orientation_t ori);
static Result_t sdl_disp_on(void *self_ptr);
static Result_t sdl_disp_off(void *self_ptr);
static Result_t sdl_disp_get_caps(void *self_ptr, Display_Capabilities_t *caps);

static const I_Display_Vtable s_sdl_vtable = {
    .Init = sdl_disp_init,
    .WriteArea = sdl_disp_write_area,
    .SetBrightness = sdl_disp_set_brightness,
    .SetOrientation = sdl_disp_set_orientation,
    .On = sdl_disp_on,
    .Off = sdl_disp_off,
    .GetCapabilities = sdl_disp_get_caps,
};

/* Single static instance — one display per simulator process */
static SdlDisplayAdapter_t s_sdl_display;
static LvglEncoderAdapter_t s_encoder_adapter;

/* ============================================================================
 * PUBLIC BSP FUNCTION
 * ========================================================================== */

/**
 * @brief PC implementation of BSP_UI_HW_Init.
 *
 * Creates:
 *   - SdlDisplayAdapter (I_Display*) — SDL window created lazily by
 *     ui_port_disp_pc.c::UI_Port_Disp_Init → Display_Init().
 *   - LvglEncoderAdapter (IEncoder_t*) — wired to di_source callbacks;
 *     identical to the STM32 implementation.
 */
Result_t BSP_UI_HW_Init(IDigitalInputSource *di_source,
                        I_Display **out_display,
                        IEncoder_t **out_encoder)
{
    if (di_source == NULL || out_display == NULL || out_encoder == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* --- Encoder (LvglEncoderAdapter — portable, unchanged from firmware) --- */
    memset(&s_encoder_adapter, 0, sizeof(s_encoder_adapter));
    const Result_t enc_res = LvglEncoderAdapter_Init(&s_encoder_adapter, di_source);
    if (enc_res != ERR_OK)
    {
        return enc_res;
    }

    /* --- Display (SDL adapter — dimensions set by UiAO_Config_t) --- */
    memset(&s_sdl_display, 0, sizeof(s_sdl_display));
    s_sdl_display.base.vtable = &s_sdl_vtable;
    s_sdl_display.base.impl = &s_sdl_display;
    /* hor_res / ver_res are filled by UI_Port_Disp_Init via Display_Init()  */

    *out_display = &s_sdl_display.base;
    *out_encoder = LvglEncoderAdapter_GetInterface(&s_encoder_adapter);

    return ERR_OK;
}

/* ============================================================================
 * SDLDISPLAYADAPTER VTABLE IMPLEMENTATIONS
 *
 * Init() is the only meaningful one: it is called by UI_Port_Disp_Init()
 * (PC version in ui_port_disp_pc.c) with the config dimensions BEFORE
 * calling lv_sdl_window_create(), so we can forward them here.
 *
 * WriteArea() and the rest are no-ops because lv_sdl_window_create()
 * registers SDL's own flush callback on the lv_display_t it creates.
 * ========================================================================== */

static Result_t sdl_disp_init(void *self_ptr)
{
    /* Actual SDL window creation is done inside ui_port_disp_pc.c
     * using the UiPortDispConfig_t dimensions passed to UI_Port_Disp_Init.
     * This function is a no-op — just signal success. */
    (void)self_ptr;
    return ERR_OK;
}

static Result_t sdl_disp_write_area(void *self_ptr,
                                    uint16_t x1, uint16_t y1,
                                    uint16_t x2, uint16_t y2,
                                    uint8_t *data,
                                    Display_TransferCompleteCallback_t cb)
{
    /* SDL handles its own rendering flush internally.
     * Signal completion immediately so LVGL does not block. */
    (void)self_ptr;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)data;
    if (cb != NULL)
    {
        cb();
    }
    return ERR_OK;
}

static Result_t sdl_disp_set_brightness(void *self_ptr, uint8_t percent)
{
    (void)self_ptr;
    (void)percent;
    return ERR_OK;
}

static Result_t sdl_disp_set_orientation(void *self_ptr, Display_Orientation_t ori)
{
    (void)self_ptr;
    (void)ori;
    return ERR_OK;
}

static Result_t sdl_disp_on(void *self_ptr)
{
    (void)self_ptr;
    return ERR_OK;
}

static Result_t sdl_disp_off(void *self_ptr)
{
    (void)self_ptr;
    return ERR_OK;
}

static Result_t sdl_disp_get_caps(void *self_ptr, Display_Capabilities_t *caps)
{
    (void)self_ptr;
    if (caps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    caps->width = (uint16_t)s_sdl_display.hor_res;
    caps->height = (uint16_t)s_sdl_display.ver_res;
    caps->color_fmt = I_DISPLAY_COLOR_FORMAT_RGB565;
    caps->supports_dma = false;
    caps->supports_backlight = false;
    return ERR_OK;
}
