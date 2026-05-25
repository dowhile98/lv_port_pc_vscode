/**
 * @file eez_screen_router.c
 * @brief EEZ-backed Screen Router — concrete IScreenRouter_t implementation.
 *
 * The ONLY production file that calls loadScreen() (EEZ generated function).
 * On every NavigateTo():
 *   1. IScreen_OnExit(current screen)
 *   2. lv_group_remove_all_objs()    — clear encoder group before fade
 *   3. loadScreen(id)                — LVGL fade-in transition
 *   4. InputRouter_SetActiveScreen(new screen)
 *   5. IScreen_OnEnter(new screen)
 *
 * @note LVGL includes are intentional and expected in this file only.
 * @note NOT added to PC test targets (depends on EEZ generated code + lvgl).
 */
#include "presentation/screens/eez_screen_router.h"
#include "presentation/ui_helpers/lvgl_screen_helper.h"
#include "ui/ui.h" /* loadScreen(), ScreensEnum — EEZ generated, DO NOT EDIT */
#include "lvgl.h"
#include <string.h>

/* Compile-time guard: EEZ_SCREEN_COUNT in eez_screen_router.h must stay
 * in sync with _SCREEN_ID_LAST in the EEZ-generated screens.h.
 * If this fires, update EEZ_SCREEN_COUNT to match _SCREEN_ID_LAST. */
_Static_assert((uint8_t)_SCREEN_ID_LAST == EEZ_SCREEN_COUNT,
               "EEZ_SCREEN_COUNT must equal _SCREEN_ID_LAST — update eez_screen_router.h");

/*============================================================================*
 * PRIVATE — IScreenRouter_t vtable entry
 *============================================================================*/

static Result_t navigate_to(IScreenRouter_t *iface, uint8_t screen_id)
{
    EezScreenRouter_t *self = (EezScreenRouter_t *)iface;

    if (screen_id < (uint8_t)SCREEN_ID_STARTUP ||
        screen_id > (uint8_t)_SCREEN_ID_LAST)
    {
        return ERR_INVALID_PARAM;
    }

    /* -- Guard: reject re-entrant navigation while a transition animation
     *    is still running.  LVGL 9 sets disp->rendering_in_progress during
     *    the DMA flush phase; calling lv_obj_invalidate() (triggered by the
     *    next loadScreen) at that moment asserts at lv_inv_area:282 with
     *    "Invalidate area is not allowed during rendering."
     *
     *    lv_display_get_screen_loading() returns non-NULL for exactly the
     *    duration of an active lv_screen_load_anim(), so it covers the full
     *    200 ms fade-in window — not just the DMA flush instant.
     *    Returning ERR_BUSY silently drops the second press; the user can
     *    press back again once the transition completes. ---------------- */
    if (lv_display_get_screen_loading(NULL) != NULL)
    {
        return ERR_BUSY;
    }

    uint8_t idx = (uint8_t)(screen_id - 1U);

    /* -- Lifecycle: exit current screen ----------------------------------- */
    IScreen_OnExit(self->active_screen);

    /* -- Clear default input group before the LVGL transition begins.
     *    This prevents encoder/keyboard events during the fade-in from
     *    reaching widgets that belong to the screen being left.
     *    Each action_xxx_loaded() re-populates the group for screens that
     *    need encoder navigation (menu, settings, …). ------------------- */
    lv_lock();
    lv_group_t *default_group = lv_group_get_default();
    if (default_group != NULL)
    {
        lv_group_remove_all_objs(default_group);
    }
    lv_unlock();

    /* -- EEZ navigation (Fnone, 0 ms, 0 delay) ----------------------- */
    LvglScreenHelper_Change((enum ScreensEnum)screen_id);

    /* -- Update active tracking ------------------------------------------ */
    self->active_screen = self->screen_map[idx];

    /* -- Route physical back button to the new active screen -------------- */
    InputRouter_SetActiveScreen(self->input_router, self->active_screen);

    /* -- Lifecycle: enter new screen ------------------------------------- */

    return ERR_OK;
}

/**
 * @brief Returns the currently active IScreen_t — used by PresentationLayer_Update().
 */
static IScreen_t *get_active_screen(IScreenRouter_t *iface)
{
    EezScreenRouter_t *self = (EezScreenRouter_t *)iface;
    return self->active_screen;
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t EezScreenRouter_Init(EezScreenRouter_t *self, InputRouter_t *input_router)
{
    if (self == NULL || input_router == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(EezScreenRouter_t));
    self->base.NavigateTo = navigate_to;
    self->base.GetActiveScreen = get_active_screen;
    self->active_screen = NULL;
    self->input_router = input_router;

    return ERR_OK;
}

void EezScreenRouter_SetScreen(EezScreenRouter_t *self,
                               uint8_t screen_id,
                               IScreen_t *screen)
{
    if (self != NULL &&
        screen_id >= (uint8_t)SCREEN_ID_STARTUP &&
        screen_id <= (uint8_t)_SCREEN_ID_LAST)
    {
        self->screen_map[screen_id - 1U] = screen;
    }
}

void EezScreenRouter_SetInitialScreen(EezScreenRouter_t *self, uint8_t screen_id)
{
    if (self == NULL ||
        screen_id < (uint8_t)SCREEN_ID_STARTUP ||
        screen_id > (uint8_t)_SCREEN_ID_LAST)
    {
        return;
    }

    uint8_t idx = (uint8_t)(screen_id - 1U);
    self->active_screen = self->screen_map[idx];

    /* Wire InputRouter so back/key buttons work from the first frame. */
    InputRouter_SetActiveScreen(self->input_router, self->active_screen);
    /* NOTE: loadScreen() and IScreen_OnEnter() are NOT called here.
     *   - EEZ already displays the initial screen on boot.
     *   - OnEnter is called by ui_actions.c via PresentationLayer_OnXxxEnter(). */
}

IScreenRouter_t *EezScreenRouter_GetInterface(EezScreenRouter_t *self)
{
    return (self != NULL) ? &self->base : NULL;
}
