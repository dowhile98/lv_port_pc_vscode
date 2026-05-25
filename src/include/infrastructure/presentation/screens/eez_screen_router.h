/**
 * @file eez_screen_router.h
 * @brief EEZ-backed Screen Router — concrete IScreenRouter_t implementation.
 *
 * The ONLY file that may call loadScreen() (EEZ generated function).
 * Holds a screen_map[] of IScreen_t* and manages InputRouter so the physical
 * back button always targets the currently active Presenter.
 *
 * @note May include lvgl.h (it calls loadScreen which needs LVGL types).
 * @note NOT testable on PC — eez_screen_router.c is excluded from cmake PC targets.
 *       Use manual integration test / hardware validation instead.
 */
#ifndef EEZ_SCREEN_ROUTER_H
#define EEZ_SCREEN_ROUTER_H

#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/input_router.h"
#include "hal/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Number of EEZ screens — must match _SCREEN_ID_LAST */
#define EEZ_SCREEN_COUNT 29U

    /**
     * @brief Concrete Screen Router backed by EEZ loadScreen().
     *
     * First field is IScreenRouter_t — cast to IScreenRouter_t* is valid (C99 §6.7.2.1).
     */
    typedef struct
    {
        IScreenRouter_t base;                    /**< MUST be first field */
        IScreen_t *screen_map[EEZ_SCREEN_COUNT]; /**< index = screen_id - 1 */
        IScreen_t *active_screen;
        InputRouter_t *input_router;
    } EezScreenRouter_t;

    /**
     * @brief Initialise the router.
     *
     * Does NOT call any LVGL function — safe to call before ui_init().
     *
     * @param[in,out] self          Router instance (must not be NULL).
     * @param[in]     input_router  Back-button mediator (must not be NULL).
     * @return ERR_OK, ERR_NULL_POINTER.
     */
    Result_t EezScreenRouter_Init(EezScreenRouter_t *self, InputRouter_t *input_router);

    /**
     * @brief Register an IScreen_t implementation for a given screen_id.
     *
     * Must be called for every screen BEFORE any NavigateTo().
     *
     * @param[in] self       Router instance.
     * @param[in] screen_id  Screen identifier (SCREEN_ID_STARTUP … SCREEN_ID_HOME).
     * @param[in] screen     Presenter implementing IScreen_t (first-field rule).
     */
    void EezScreenRouter_SetScreen(EezScreenRouter_t *self,
                                   uint8_t screen_id,
                                   IScreen_t *screen);

    /**
     * @brief Set the initial active screen WITHOUT triggering EEZ loadScreen().
     *
     * Call once after all SetScreen() registrations, before the Control AO thread
     * starts calling PresentationLayer_Update(). This makes GetActiveScreen() return
     * a valid pointer from the very first update tick.
     *
     * Wires InputRouter so physical buttons dispatch to the initial screen immediately.
     * Does NOT call IScreen_OnEnter() — ui_actions.c handles that via the EEZ
     * on-screen-loaded callback (e.g. PresentationLayer_OnHomeEnter).
     *
     * @param[in] self       Router instance.
     * @param[in] screen_id  Initial screen (typically SCREEN_ID_HOME).
     */
    void EezScreenRouter_SetInitialScreen(EezScreenRouter_t *self, uint8_t screen_id);

    /**
     * @brief Return the IScreenRouter_t interface pointer.
     *
     * Presenters receive this to call NavigateTo().
     *
     * @param[in] self  Router instance.
     * @return Pointer to IScreenRouter_t base.
     */
    IScreenRouter_t *EezScreenRouter_GetInterface(EezScreenRouter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* EEZ_SCREEN_ROUTER_H */
