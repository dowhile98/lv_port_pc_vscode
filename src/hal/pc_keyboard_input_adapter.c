/**
 * @file pc_keyboard_input_adapter.c
 * @brief Implementation of PCKeyboardInputAdapter — IDigitalInputSource via SDL keyboard.
 *
 * Architecture:
 *   SDL_GetKeyboardState()  ←  pc_kbd_get_state() (lwbtn read callback)
 *         ↓
 *   lwbtn_process_ex()      ←  IDigitalInputSource_Process() called every ~20ms
 *         ↓
 *   pc_kbd_on_event()       — dispatches DI_EVENT_* to registered callbacks
 *         ↓
 *   LvglEncoderAdapter / InputRouter (same path as DigitalInputAdapter on firmware)
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "pc_keyboard_input_adapter.h"
#include <SDL2/SDL.h>
#include "infrastructure/osal/osal.h"
#include <string.h>

/* ============================================================================
 * PRIVATE FORWARD DECLARATIONS
 * ========================================================================== */

static Result_t pc_kbd_read_input(void *impl, DigitalInputID_t id, bool *out_state);
static Result_t pc_kbd_get_type(void *impl, DigitalInputID_t id, DigitalInputType_t *out_type);
static Result_t pc_kbd_register_cb(void *impl, DigitalInputID_t id,
                                   DigitalInputEvent_t event_mask,
                                   DigitalInputCallback_t cb, void *ctx);
static Result_t pc_kbd_unregister_cb(void *impl, DigitalInputID_t id,
                                     DigitalInputCallback_t cb,
                                     DigitalInputEvent_t event_mask);
static Result_t pc_kbd_process(void *impl);

/* lwbtn callbacks */
static uint8_t pc_kbd_get_state(struct lwbtn *lw, struct lwbtn_btn *btn);
static void pc_kbd_on_event(struct lwbtn *lw, struct lwbtn_btn *btn, lwbtn_evt_t evt);

/* ============================================================================
 * VTABLE
 * ========================================================================== */

static const IDigitalInputSource_Vtable s_vtable = {
    .ReadInput = pc_kbd_read_input,
    .GetInputType = pc_kbd_get_type,
    .RegisterCallback = pc_kbd_register_cb,
    .UnregisterCallback = pc_kbd_unregister_cb,
    .Process = pc_kbd_process,
};

/* ============================================================================
 * BUTTON INDEX → DI_ID MAPPING
 * Index 0 = ONOFF (BACK), 1 = ENTER, 2 = UP, 3 = DOWN
 * ========================================================================== */

static const DigitalInputID_t s_id_map[PC_KBD_BTN_COUNT] = {
    DI_ID_BUTTON_ONOFF,
    DI_ID_BUTTON_ENTER,
    DI_ID_BUTTON_UP,
    DI_ID_BUTTON_DOWN,
};

/* ============================================================================
 * HELPERS
 * ========================================================================== */

/**
 * @brief Map DI_ID to the configured SDL scancode.
 */
static int get_scancode(const PCKeyboardInputAdapter_t *self, DigitalInputID_t id)
{
    switch (id)
    {
    case DI_ID_BUTTON_ONOFF:
        return self->config.scancode_back;
    case DI_ID_BUTTON_ENTER:
        return self->config.scancode_enter;
    case DI_ID_BUTTON_UP:
        return self->config.scancode_up;
    case DI_ID_BUTTON_DOWN:
        return self->config.scancode_down;
    default:
        return -1;
    }
}

/**
 * @brief Map lwbtn event to DI_EVENT bitmask value.
 */
static DigitalInputEvent_t lwbtn_to_di_event(lwbtn_evt_t evt)
{
    switch (evt)
    {
    case LWBTN_EVT_ONPRESS:
        return DI_EVENT_PRESS;
    case LWBTN_EVT_ONRELEASE:
        return DI_EVENT_RELEASE;
    case LWBTN_EVT_ONCLICK:
        return DI_EVENT_CLICK;
    case LWBTN_EVT_KEEPALIVE:
        return DI_EVENT_KEEPALIVE;
    default:
        return DI_EVENT_NONE;
    }
}

/* ============================================================================
 * PUBLIC API
 * ========================================================================== */

Result_t PCKeyboardInputAdapter_Init(PCKeyboardInputAdapter_t *self,
                                     const PCKeyboardInputConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));

    self->iface.vtable = &s_vtable;
    self->iface.impl = self;
    self->config = *config;

    /* Wire per-button contexts and lwbtn array */
    const int scancodes[PC_KBD_BTN_COUNT] = {
        config->scancode_back,
        config->scancode_enter,
        config->scancode_up,
        config->scancode_down,
    };

    for (uint8_t i = 0; i < PC_KBD_BTN_COUNT; i++)
    {
        self->contexts[i].adapter = self;
        self->contexts[i].id = s_id_map[i];
        self->contexts[i].scancode = scancodes[i];
        self->buttons[i].arg = &self->contexts[i];
    }

    /* Initialise lwbtn with the per-instance group so multiple adapters can
     * coexist (though only one keyboard adapter is expected in practice). */
    lwbtn_init_ex(&self->lwbtn_instance,
                  self->buttons,
                  PC_KBD_BTN_COUNT,
                  pc_kbd_get_state,
                  pc_kbd_on_event);

    return ERR_OK;
}

IDigitalInputSource *PCKeyboardInputAdapter_GetInterface(PCKeyboardInputAdapter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}

/* ============================================================================
 * LWBTN CALLBACKS
 * ========================================================================== */

/**
 * @brief Read current SDL keyboard state for a button.
 *
 * Called by lwbtn every time lwbtn_process_ex() is invoked.
 * SDL_GetKeyboardState() returns the array pumped by SDL_PumpEvents()
 * (which lv_task_handler() triggers via the SDL event timer).
 */
static uint8_t pc_kbd_get_state(struct lwbtn *lw, struct lwbtn_btn *btn)
{
    (void)lw;
    const PCKbdBtnContext_t *ctx = (const PCKbdBtnContext_t *)btn->arg;
    if (ctx == NULL || ctx->scancode < 0)
    {
        return 0U;
    }

    const uint8_t *kb = SDL_GetKeyboardState(NULL);
    if (kb == NULL)
    {
        return 0U;
    }

    return kb[ctx->scancode] ? 1U : 0U;
}

/**
 * @brief Dispatch a lwbtn event to all registered IDigitalInputSource callbacks.
 *
 * Executes in the context of pc_kbd_process() → IDigitalInputSource_Process()
 * → the DigitalInputAO polling task (~20ms period). Same threading model as
 * the firmware's DigitalInputAdapter.
 */
static void pc_kbd_on_event(struct lwbtn *lw, struct lwbtn_btn *btn, lwbtn_evt_t evt)
{
    (void)lw;

    const PCKbdBtnContext_t *ctx = (const PCKbdBtnContext_t *)btn->arg;
    if (ctx == NULL)
    {
        return;
    }

    PCKeyboardInputAdapter_t *self = ctx->adapter;
    if (self == NULL)
    {
        return;
    }

    const DigitalInputEvent_t di_evt = lwbtn_to_di_event(evt);
    if (di_evt == DI_EVENT_NONE)
    {
        return;
    }

    const DigitalInputID_t id = ctx->id;
    if ((uint8_t)id >= DI_ID_COUNT)
    {
        return;
    }

    const DigitalInputEventData_t data = {
        .timestamp_ms = os_ticks_get(),
        .click_count = btn->click.cnt,
        .keepalive_count = btn->keepalive.cnt,
        .press_duration_ms = 0U,
    };

    /* Dispatch to all active subscribers that include this event in their mask */
    for (uint8_t s = 0U; s < PC_KBD_MAX_CALLBACKS_PER_INPUT; s++)
    {
        const PCKbdSubscriber_t *sub = &self->subscribers[id][s];
        if (sub->active && ((sub->event_mask & (uint32_t)di_evt) != 0U))
        {
            sub->callback(sub->context, id, di_evt, &data);
        }
    }
}

/* ============================================================================
 * IDIGITALINPUTSOURCE VTABLE IMPLEMENTATIONS
 * ========================================================================== */

static Result_t pc_kbd_read_input(void *impl, DigitalInputID_t id, bool *out_state)
{
    const PCKeyboardInputAdapter_t *self = (const PCKeyboardInputAdapter_t *)impl;
    if (self == NULL || out_state == NULL)
    {
        return ERR_NULL_POINTER;
    }

    const int sc = get_scancode(self, id);
    if (sc < 0)
    {
        *out_state = false;
        return ERR_INVALID_PARAM;
    }

    const uint8_t *kb = SDL_GetKeyboardState(NULL);
    *out_state = (kb != NULL) && (kb[sc] != 0U);
    return ERR_OK;
}

static Result_t pc_kbd_get_type(void *impl,
                                DigitalInputID_t id,
                                DigitalInputType_t *out_type)
{
    (void)impl;
    if (out_type == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if ((uint8_t)id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }
    *out_type = DI_TYPE_BUTTON;
    return ERR_OK;
}

static Result_t pc_kbd_register_cb(void *impl,
                                   DigitalInputID_t id,
                                   DigitalInputEvent_t event_mask,
                                   DigitalInputCallback_t cb,
                                   void *ctx)
{
    PCKeyboardInputAdapter_t *self = (PCKeyboardInputAdapter_t *)impl;
    if (self == NULL || cb == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (event_mask == DI_EVENT_NONE)
    {
        return ERR_INVALID_PARAM;
    }
    if ((uint8_t)id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    /* Merge mask if callback+context already registered */
    for (uint8_t s = 0U; s < PC_KBD_MAX_CALLBACKS_PER_INPUT; s++)
    {
        PCKbdSubscriber_t *sub = &self->subscribers[id][s];
        if (sub->active && sub->callback == cb && sub->context == ctx)
        {
            sub->event_mask |= (uint32_t)event_mask;
            return ERR_OK;
        }
    }

    /* Find a free slot */
    for (uint8_t s = 0U; s < PC_KBD_MAX_CALLBACKS_PER_INPUT; s++)
    {
        PCKbdSubscriber_t *sub = &self->subscribers[id][s];
        if (!sub->active)
        {
            sub->callback = cb;
            sub->context = ctx;
            sub->event_mask = (uint32_t)event_mask;
            sub->active = true;
            return ERR_OK;
        }
    }

    return ERR_BUSY;
}

static Result_t pc_kbd_unregister_cb(void *impl,
                                     DigitalInputID_t id,
                                     DigitalInputCallback_t cb,
                                     DigitalInputEvent_t event_mask)
{
    PCKeyboardInputAdapter_t *self = (PCKeyboardInputAdapter_t *)impl;
    if (self == NULL || cb == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if ((uint8_t)id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    for (uint8_t s = 0U; s < PC_KBD_MAX_CALLBACKS_PER_INPUT; s++)
    {
        PCKbdSubscriber_t *sub = &self->subscribers[id][s];
        if (sub->active && sub->callback == cb)
        {
            sub->event_mask &= ~(uint32_t)event_mask;
            if (sub->event_mask == 0U)
            {
                memset(sub, 0, sizeof(*sub));
            }
            return ERR_OK;
        }
    }

    return ERR_ERROR;
}

/**
 * @brief Process lwbtn debounce state machine for all keyboard buttons.
 *
 * Call from a FreeRTOS task every ~20ms (same period as DigitalInputAO on firmware).
 * SDL events are pumped by lv_task_handler() via the SDL event timer (5ms interval),
 * so SDL_GetKeyboardState() is always fresh when this runs.
 */
static Result_t pc_kbd_process(void *impl)
{
    PCKeyboardInputAdapter_t *self = (PCKeyboardInputAdapter_t *)impl;
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    lwbtn_process_ex(&self->lwbtn_instance, (lwbtn_time_t)os_ticks_get());
    return ERR_OK;
}
