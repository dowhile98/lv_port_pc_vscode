/**
 * @file i_screen_router.h
 * @brief Navigation contract — Presenter → Router (zero LVGL).
 *
 * Allows Presenters to trigger navigation without including LVGL or knowing
 * about loadScreen() / lv_scr_load_anim_t. The concrete implementation
 * (EezScreenRouter_t) is the only file allowed to call loadScreen().
 *
 * Architecture reference: docs/architecture/lvgl/LVGL_MVP_GAP_ANALYSIS.md §5 / P-1.2
 */
#ifndef I_SCREEN_ROUTER_H
#define I_SCREEN_ROUTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IScreenRouter_t IScreenRouter_t;

    struct IScreenRouter_t
    {
        /**
         * @brief Navigate to the screen identified by screen_id.
         *
         * Triggers the full lifecycle:
         *   IScreen_OnExit(current) → loadScreen(id) → InputRouter_SetActiveScreen()
         *   → IScreen_OnEnter(new)
         *
         * @param[in] self       Router instance.
         * @param[in] screen_id  Target screen (maps to ScreensEnum in EEZ screens.h).
         * @return ERR_OK on success.
         * @return ERR_INVALID_PARAM if screen_id is out of range.
         * @return ERR_NULL_POINTER if self is NULL.
         */
        Result_t (*NavigateTo)(IScreenRouter_t *self, uint8_t screen_id);

        /**
         * @brief Return the currently active IScreen_t.
         *
         * Called from the Control AO thread to dispatch OnUpdate() to whichever
         * screen is currently active, without knowing its concrete type.
         *
         * @param[in] self  Router instance.
         * @return Pointer to the active IScreen_t, or NULL if none is active.
         */
        IScreen_t *(*GetActiveScreen)(IScreenRouter_t *self);
    };

    /**
     * @brief NULL-safe NavigateTo dispatch.
     */
    static inline Result_t IScreenRouter_NavigateTo(IScreenRouter_t *r, uint8_t id)
    {
        if (r && r->NavigateTo)
        {
            return r->NavigateTo(r, id);
        }
        return ERR_NULL_POINTER;
    }

    /**
     * @brief NULL-safe GetActiveScreen dispatch.
     * @return Active IScreen_t pointer, or NULL when r is NULL or vtable entry missing.
     */
    static inline IScreen_t *IScreenRouter_GetActiveScreen(IScreenRouter_t *r)
    {
        if (r && r->GetActiveScreen)
        {
            return r->GetActiveScreen(r);
        }
        return NULL;
    }

#ifdef __cplusplus
}
#endif

#endif /* I_SCREEN_ROUTER_H */
