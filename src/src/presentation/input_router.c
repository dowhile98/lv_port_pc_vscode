/**
 * @file input_router.c
 * @brief Back-button mediator implementation.
 *
 * Subscribes once to DI_ID_BUTTON_ONOFF / DI_EVENT_CLICK and forwards
 * the event to the currently active IScreen_t via OnBackPressed().
 *
 * @note NO #include "lvgl.h" — intentional. Must remain LVGL-free.
 * @note Thread-Safety: on_back_click runs in the context of whoever
 *       calls IDigitalInputSource.Process() (DigitalInputAO thread).
 *       InputRouter_SetActiveScreen() is called from EezScreenRouter
 *       (LVGL thread). If these differ, a short critical section or
 *       atomic pointer store may be needed on multi-core targets.
 *       For single-core STM32U575 Cortex-M33, pointer writes are atomic.
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/input_router.h"
#include "presentation/interfaces/i_screen.h"
#include <string.h>

/*============================================================================*
 * PRIVATE — DI callbacks
 *============================================================================*/

/**
 * @brief Callback fired by IDigitalInputSource on DI_ID_BUTTON_ONOFF + DI_EVENT_CLICK.
 *
 * Runs in the DigitalInputAO thread (NOT an ISR — Process() is polled).
 * Must be fast: just dispatches to the active screen.
 *
 * @note Backlight wake/sleep is handled by DigitalInputAdapter BEFORE this callback.
 */
static void on_back_click(void *ctx,
                          DigitalInputID_t id,
                          DigitalInputEvent_t evt,
                          const DigitalInputEventData_t *data)
{
    (void)id;
    (void)evt;
    (void)data;

    InputRouter_t *self = (InputRouter_t *)ctx;
    IScreen_OnBackPressed(self->active_screen); /* NULL-safe inline */
}

/**
 * @brief Callback for UP button PRESS + KEEPALIVE.
 */
static void on_up_key(void *ctx,
                      DigitalInputID_t id,
                      DigitalInputEvent_t evt,
                      const DigitalInputEventData_t *data)
{
    (void)id;
    (void)data;

    InputRouter_t *self = (InputRouter_t *)ctx;
    IScreen_OnKeyEvent(self->active_screen, SCREEN_KEY_UP, (ScreenKeyEvent_t)(uint8_t)evt);
}

/**
 * @brief Callback for DOWN button PRESS + KEEPALIVE.
 */
static void on_down_key(void *ctx,
                        DigitalInputID_t id,
                        DigitalInputEvent_t evt,
                        const DigitalInputEventData_t *data)
{
    (void)id;
    (void)data;

    InputRouter_t *self = (InputRouter_t *)ctx;
    IScreen_OnKeyEvent(self->active_screen, SCREEN_KEY_DOWN, (ScreenKeyEvent_t)(uint8_t)evt);
}

/**
 * @brief Callback for ENTER button PRESS (rising edge by definition).
 */
static void on_enter_key(void *ctx,
                         DigitalInputID_t id,
                         DigitalInputEvent_t evt,
                         const DigitalInputEventData_t *data)
{
    (void)id;
    (void)data;

    InputRouter_t *self = (InputRouter_t *)ctx;
    IScreen_OnKeyEvent(self->active_screen, SCREEN_KEY_ENTER, (ScreenKeyEvent_t)(uint8_t)evt);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t InputRouter_Init(InputRouter_t *self, const InputRouterConfig_t *config)
{
    if (self == NULL || config == NULL || config->di_source == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(InputRouter_t));
    self->di_source = config->di_source;
    self->active_screen = NULL;

    Result_t res;
    /* Back button */
    res = DigitalInputSource_RegisterCallback(
        self->di_source, DI_ID_BUTTON_ONOFF, DI_EVENT_PRESS, on_back_click, self);
    if (res != ERR_OK)
    {
        return res;
    }
    /* UP: PRESS for immediate response + KEEPALIVE for auto-repeat */
    res = DigitalInputSource_RegisterCallback(
        self->di_source, DI_ID_BUTTON_UP, DI_EVENT_PRESS | DI_EVENT_KEEPALIVE, on_up_key, self);
    if (res != ERR_OK)
    {
        return res;
    }

    /* DOWN: PRESS + KEEPALIVE */
    res = DigitalInputSource_RegisterCallback(
        self->di_source, DI_ID_BUTTON_DOWN, DI_EVENT_PRESS | DI_EVENT_KEEPALIVE, on_down_key, self);
    if (res != ERR_OK)
    {
        return res;
    }

    /* ENTER: PRESS only (one event per press — no auto-repeat for passwords) */
    res = DigitalInputSource_RegisterCallback(
        self->di_source, DI_ID_BUTTON_ENTER, DI_EVENT_PRESS, on_enter_key, self);
    if (res != ERR_OK)
    {
        return res;
    }

    return ERR_OK;
}

void InputRouter_SetActiveScreen(InputRouter_t *self, IScreen_t *screen)
{
    if (self != NULL)
    {
        self->active_screen = screen;
    }
}
