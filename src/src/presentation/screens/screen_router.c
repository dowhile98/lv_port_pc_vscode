/**
 * @file screen_router.c
 * @brief ScreenRouter_t — pure lifecycle router (ACTION-029).
 *
 * Manages the active screen pointer and dispatches OnExit/OnEnter on
 * navigation. Zero LVGL dependency — fully testable on PC.
 *
 * The EEZ-specific wrapper (eez_screen_router.c) extends this by also
 * calling _loadScreen()_ after NavigateTo succeeds.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/screen_router.h"
#include <string.h>

/*============================================================================*
 * PRIVATE — vtable implementations
 *============================================================================*/

/**
 * @brief Transition from the current screen to the screen at @p screen_id.
 *
 * Sequence:
 *   1. Validate id.
 *   2. Call OnExit on the current screen (NULL-safe — IScreen_OnExit handles NULL).
 *   3. Update current_id and current pointer.
 *   4. Call OnEnter on the new screen (NULL-safe).
 */
static Result_t navigate_to(IScreenRouter_t *self, uint8_t screen_id)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    ScreenRouter_t *r = (ScreenRouter_t *)self;

    if (screen_id >= r->screen_count)
    {
        return ERR_INVALID_PARAM;
    }

    /* Exit current screen */
    IScreen_OnExit(r->screens[r->current_id]);

    /* Switch */
    r->current_id = screen_id;

    /* Enter new screen */
    IScreen_OnEnter(r->screens[r->current_id]);

    return ERR_OK;
}

/**
 * @brief Return the currently active IScreen_t pointer.
 */
static IScreen_t *get_active_screen(IScreenRouter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    ScreenRouter_t *r = (ScreenRouter_t *)self;
    return r->screens[r->current_id];
}

static const IScreenRouter_t s_vtable = {
    navigate_to,
    get_active_screen,
};

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t ScreenRouter_Init(ScreenRouter_t *self, const ScreenRouterConfig_t *cfg)
{
    if (self == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (cfg->screens == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (cfg->screen_count == 0U)
    {
        return ERR_INVALID_PARAM;
    }
    if (cfg->initial_id >= cfg->screen_count)
    {
        return ERR_INVALID_PARAM;
    }

    memset(self, 0, sizeof(ScreenRouter_t));

    /* Store pointer — caller owns the array; no copy needed. */
    self->screens = cfg->screens;
    self->screen_count = cfg->screen_count;
    self->current_id = cfg->initial_id;

    /* Wire vtable — copy from const so every instance has its own pointer fields */
    self->iface = s_vtable;

    return ERR_OK;
}

IScreenRouter_t *ScreenRouter_GetInterface(ScreenRouter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}
