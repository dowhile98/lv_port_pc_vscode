/**
 * @file ui_port_disp_pc.c
 * @brief PC implementation of UI_Port_Disp_Init — replaces ui_port_disp.c in PC builds.
 *
 * Why this file exists
 * --------------------
 * The firmware's ui_port_disp.c creates an lv_display_t via lv_display_create()
 * and wires a DMA flush_cb → I_Display::WriteArea (ST7789 SPI+DMA path).
 *
 * For SDL, lv_sdl_window_create() creates its own lv_display_t with SDL's
 * internal flush handler.  Calling both would create two LVGL displays and
 * break rendering.
 *
 * This file provides the same public symbols as ui_port_disp.c but uses
 * lv_sdl_window_create() instead:
 *
 *   UI_Port_Disp_Init(display, config)
 *     → Display_Init(display)           ← no-op on SdlDisplayAdapter
 *     → lv_sdl_window_create(w, h)      ← ONE display, SDL flush built-in
 *     → mouse indev + cursor setup
 *     → lv_display_set_default()
 *
 *   UI_Port_Disp_SignalFlushReady()     ← no-op (SDL is self-contained)
 *
 * CMakeLists.txt integration
 * --------------------------
 * Add to APP_SOURCES:
 *   src/hal/ui_port_disp_pc.c
 * Remove from sources (or guard with NOT PC_BUILD):
 *   src/src/presentation/ui_port/ui_port_disp.c
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "presentation/ui_port/ui_port_disp.h"
#include "interfaces/i_display.h"
#include "lvgl/lvgl.h"

/* mouse cursor image declared in mouse_cursor_icon.c */
LV_IMAGE_DECLARE(mouse_cursor_icon);

/* ============================================================================
 * PUBLIC SYMBOLS
 * ========================================================================== */

/**
 * @brief PC version of UI_Port_Disp_Init — creates an SDL window as LVGL display.
 *
 * Called from ui_ao_thread_entry() after lv_init().
 * The I_Display* argument is accepted to satisfy the existing call site but
 * Display_Init() on the SdlDisplayAdapter is a no-op; actual SDL window
 * creation happens here.
 */
Result_t UI_Port_Disp_Init(I_Display *display, const UiPortDispConfig_t *config)
{
    if (display == NULL || display->vtable == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Call Display_Init on the adapter (no-op for SdlDisplayAdapter).
     * Keeps the same call sequence as ui_port_disp.c for consistency. */
    const Result_t init_res = display->vtable->Init(display->impl);
    if (init_res != ERR_OK)
    {
        return init_res;
    }

    /* Create the SDL window — this registers SDL's own flush callback on the
     * returned lv_display_t.  No separate lv_display_create() needed. */
    lv_display_t *sdl_disp = lv_sdl_window_create((int32_t)config->hor_res,
                                                  (int32_t)config->ver_res);
    if (sdl_disp == NULL)
    {
        return ERR_ERROR;
    }

    lv_display_set_default(sdl_disp);

    /* Mouse indev + cursor (replaces sdl_hal_init() from the old freertos_main) */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_set_display(mouse, sdl_disp);
    lv_indev_set_group(mouse, lv_group_get_default());

    lv_obj_t *cursor_obj = lv_image_create(lv_screen_active());
    lv_image_set_src(cursor_obj, &mouse_cursor_icon);
    lv_indev_set_cursor(mouse, cursor_obj);

    return ERR_OK;
}

/**
 * @brief No-op for SDL — SDL signals flush completion internally.
 *
 * On firmware this is called from the DMA transfer-complete ISR.
 * On PC there is no DMA and no semaphore to signal.
 */
void UI_Port_Disp_SignalFlushReady(void)
{
    /* no-op */
}
