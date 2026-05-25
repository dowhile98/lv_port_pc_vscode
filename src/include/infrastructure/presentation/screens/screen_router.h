/**
 * @file screen_router.h
 * @brief Pure-logic Screen Router — manages active screen + lifecycle dispatch.
 *
 * `ScreenRouter_t` knows the full set of registered screens indexed by ID.
 * On navigation it calls `IScreen_OnExit(current)` then `IScreen_OnEnter(new)`.
 * It does NOT load EEZ/LVGL screens — that responsibility belongs to the
 * EEZ-specific wrapper (`eez_screen_router.c`, firmware-only).
 *
 * ## Architecture placement
 * ```
 * Control AO thread → PresentationLayer_Update()
 *   → IScreenRouter_GetActiveScreen(router) → IScreen_OnUpdate(active)
 *
 * ui_actions.c (LVGL thread) → IScreenRouter_NavigateTo(router, id)
 *   → IScreen_OnExit(current), IScreen_OnEnter(new)
 * ```
 *
 * ## Threading
 * - `NavigateTo` is called from the LVGL thread (action callbacks).
 * - `GetActiveScreen` is called from the Control AO thread.
 * - `ScreenRouter_t` does NOT guard against concurrent access.
 *   The caller (EezScreenRouter / PresentationLayer) is responsible for
 *   any necessary synchronisation if screen transitions and OnUpdate overlap.
 *
 * ## Zero LVGL — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef SCREEN_ROUTER_H
#define SCREEN_ROUTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_screen.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialisation configuration for ScreenRouter_t.
     */
    typedef struct ScreenRouterConfig_t
    {
        IScreen_t **screens;  /**< Array of IScreen_t* pointers, indexed by screen_id. */
        uint8_t screen_count; /**< Number of entries in @p screens (1 … SCREEN_ROUTER_MAX_SCREENS). */
        uint8_t initial_id;   /**< ID of the screen active at startup (< screen_count). */
    } ScreenRouterConfig_t;

    /**
     * @brief Concrete Screen Router.
     *
     * @note `iface` MUST be the first field — C99 first-field cast:
     *       (IScreenRouter_t *)&router  ↔  &router
     */
    typedef struct ScreenRouter_t
    {
        IScreenRouter_t iface; /**< MUST be first — vtable exposed to callers. */
        /* private */
        IScreen_t **screens;  /**< Pointer to caller-supplied screen array (by ID). */
        uint8_t screen_count; /**< Number of entries in @p screens. */
        uint8_t current_id;   /**< ID of the currently active screen. */
    } ScreenRouter_t;

    /**
     * @brief Initialise the router with a registered screen set.
     *
     * Copies the `screens` array entries into the router's internal table.
     * Does NOT call OnEnter on the initial screen — the EEZ `action_xxx_loaded`
     * callback already fires OnEnter when the first screen loads.
     *
     * @param[in] self  Router instance (must not be NULL).
     * @param[in] cfg   Configuration bundle (must not be NULL;
     *                  cfg->screens must not be NULL;
     *                  cfg->screen_count must be >= 1;
     *                  cfg->initial_id must be < cfg->screen_count).
     *
     * @note  The caller retains ownership of the @p cfg->screens array.
     *        The router stores only a pointer to it — the array must remain
     *        valid for the lifetime of the router.
     *
     * @return ERR_OK             on success.
     * @return ERR_NULL_POINTER   if self, cfg, or cfg->screens is NULL.
     * @return ERR_INVALID_PARAM  if screen_count is 0, exceeds the maximum,
     *                            or initial_id is out of range.
     */
    Result_t ScreenRouter_Init(ScreenRouter_t *self, const ScreenRouterConfig_t *cfg);

    /**
     * @brief Return the `IScreenRouter_t` interface pointer.
     *
     * @param[in] self  Router instance.
     * @return Pointer to the embedded `IScreenRouter_t`, or NULL if self is NULL.
     */
    IScreenRouter_t *ScreenRouter_GetInterface(ScreenRouter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_ROUTER_H */
