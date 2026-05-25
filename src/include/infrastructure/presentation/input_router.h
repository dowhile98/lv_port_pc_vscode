/**
 * @file input_router.h
 * @brief Centralized input mediator — routes all physical button events to the active screen.
 *
 * Subscribes once (at system init) to the following button events via
 * IDigitalInputSource.RegisterCallback():
 *   - DI_ID_BUTTON_ONOFF / DI_EVENT_CLICK   → IScreen_OnBackPressed()
 *   - DI_ID_BUTTON_UP   / DI_EVENT_PRESS + KEEPALIVE → IScreen_OnKeyEvent(SCREEN_KEY_UP)
 *   - DI_ID_BUTTON_DOWN / DI_EVENT_PRESS + KEEPALIVE → IScreen_OnKeyEvent(SCREEN_KEY_DOWN)
 *   - DI_ID_BUTTON_ENTER/ DI_EVENT_PRESS                → IScreen_OnKeyEvent(SCREEN_KEY_ENTER)
 *
 * EezScreenRouter calls InputRouter_SetActiveScreen() on every NavigateTo(),
 * so the target is always up to date.
 *
 * This avoids polling IEncoder_t from Presenters: the IEncoder_t is reserved for
 * LVGL indev widget navigation only. Presenters receive key events via OnKeyEvent().
 *
 * @note Zero lvgl.h — fully testable on PC.
 * @note OnBackPressed() and OnKeyEvent() are NULL-safe.
 * @note Thread-Safety: callbacks run in DigitalInputAO polling context;
 *       InputRouter_SetActiveScreen() is called from EezScreenRouter (LVGL thread).
 *       Pointer writes are atomic on single-core ARM Cortex-M33.
 */
#ifndef INPUT_ROUTER_H
#define INPUT_ROUTER_H

#include "hal/hal_types.h"
#include "interfaces/i_digital_input_source.h"
#include "presentation/interfaces/i_screen.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief InputRouter instance (static allocation).
     */
    typedef struct
    {
        IDigitalInputSource *di_source; /**< Physical button source (injected) */
        IScreen_t *active_screen;       /**< Current screen — updated by EezScreenRouter */
    } InputRouter_t;

    /**
     * @brief Configuration passed to InputRouter_Init().
     */
    typedef struct
    {
        IDigitalInputSource *di_source; /**< Must not be NULL */
    } InputRouterConfig_t;

    /**
     * @brief Initialise and subscribe to DI_ID_BUTTON_ONOFF / DI_EVENT_CLICK.
     *
     * Registers on_back_click callback with the di_source. After this call the router
     * is active and will forward ONOFF clicks to the active screen's OnBackPressed().
     *
     * @param[in,out] self    Router instance (must not be NULL).
     * @param[in]     config  Must provide a valid di_source.
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, config, or config->di_source is NULL.
     * @return ERR_BUSY if IDigitalInputSource has no free callback slots.
     */
    Result_t InputRouter_Init(InputRouter_t *self, const InputRouterConfig_t *config);

    /**
     * @brief Update the screen that receives OnBackPressed().
     *
     * Called by EezScreenRouter_t on every NavigateTo(). May be called with NULL
     * (no active screen — back button becomes a no-op).
     *
     * @param[in] self    Router instance (NULL-safe).
     * @param[in] screen  New active screen target (may be NULL).
     */
    void InputRouter_SetActiveScreen(InputRouter_t *self, IScreen_t *screen);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_ROUTER_H */
