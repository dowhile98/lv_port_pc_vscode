/**
 * @file mock_digital_input_source.c
 * @brief PC mock IDigitalInputSource — event queue with callback dispatch.
 */
#include "mock_digital_input_source.h"
#include <string.h>

/* ---------------------------------------------------------------------------
 * Constants
 * ------------------------------------------------------------------------- */
#define MOCK_DI_MAX_CALLBACKS  8
#define MOCK_DI_EVENT_QUEUE    16

/* ---------------------------------------------------------------------------
 * Types
 * ------------------------------------------------------------------------- */
typedef struct {
    void             *context;
    DigitalInputID_t  id;
    DigitalInputEvent_t event_mask;
    DigitalInputCallback_t callback;
} MockCallbackSlot;

typedef struct {
    DigitalInputID_t    id;
    DigitalInputEvent_t event;
    uint32_t            timestamp_ms;
} MockQueuedEvent;

typedef struct {
    bool             input_states[DI_ID_COUNT];
    MockCallbackSlot callbacks[MOCK_DI_MAX_CALLBACKS];
    int              num_callbacks;
    MockQueuedEvent  queue[MOCK_DI_EVENT_QUEUE];
    int              queue_head;
    int              queue_tail;
} MockDIState_t;

static MockDIState_t s_state;

/* ---------------------------------------------------------------------------
 * VTable methods
 * ------------------------------------------------------------------------- */

static Result_t mock_read(void *self, DigitalInputID_t id, bool *out_state)
{
    (void)self;
    if (out_state == NULL) return ERR_NULL_POINTER;
    if (id >= DI_ID_COUNT) return ERR_INVALID_PARAM;
    *out_state = s_state.input_states[id];
    return ERR_OK;
}

static Result_t mock_get_type(void *self, DigitalInputID_t id, DigitalInputType_t *out_type)
{
    (void)self;
    if (out_type == NULL) return ERR_NULL_POINTER;
    if (id >= DI_ID_COUNT) return ERR_INVALID_PARAM;
    /* All buttons are DI_TYPE_BUTTON in the mock */
    *out_type = DI_TYPE_BUTTON;
    return ERR_OK;
}

static Result_t mock_register_callback(void *self, DigitalInputID_t id,
                                       DigitalInputEvent_t event_mask,
                                       DigitalInputCallback_t callback,
                                       void *context)
{
    (void)self;
    if (callback == NULL) return ERR_NULL_POINTER;
    if (event_mask == DI_EVENT_NONE) return ERR_INVALID_PARAM;
    if (id >= DI_ID_COUNT) return ERR_INVALID_PARAM;

    /* Look for existing slot with same callback+context */
    for (int i = 0; i < s_state.num_callbacks; i++) {
        MockCallbackSlot *s = &s_state.callbacks[i];
        if (s->callback == callback && s->context == context && s->id == id) {
            s->event_mask |= event_mask;
            return ERR_OK;
        }
    }

    /* New slot */
    if (s_state.num_callbacks >= MOCK_DI_MAX_CALLBACKS) {
        return ERR_BUSY;
    }
    MockCallbackSlot *slot = &s_state.callbacks[s_state.num_callbacks++];
    slot->context    = context;
    slot->id         = id;
    slot->event_mask = event_mask;
    slot->callback   = callback;
    return ERR_OK;
}

static Result_t mock_unregister_callback(void *self, DigitalInputID_t id,
                                         DigitalInputCallback_t callback,
                                         DigitalInputEvent_t event_mask)
{
    (void)self;
    if (callback == NULL) return ERR_NULL_POINTER;

    for (int i = 0; i < s_state.num_callbacks; i++) {
        MockCallbackSlot *s = &s_state.callbacks[i];
        if (s->callback == callback && s->id == id) {
            s->event_mask &= ~event_mask;
            if (s->event_mask == DI_EVENT_NONE) {
                /* Remove slot */
                int remaining = s_state.num_callbacks - i - 1;
                if (remaining > 0) {
                    memmove(&s_state.callbacks[i], &s_state.callbacks[i + 1],
                            remaining * sizeof(MockCallbackSlot));
                }
                s_state.num_callbacks--;
            }
            return ERR_OK;
        }
    }
    return ERR_ERROR;
}

static Result_t mock_process(void *self)
{
    (void)self;
    /* Dispatch all queued events */
    while (s_state.queue_head != s_state.queue_tail) {
        MockQueuedEvent ev = s_state.queue[s_state.queue_head];
        s_state.queue_head = (s_state.queue_head + 1) % MOCK_DI_EVENT_QUEUE;

        /* Update internal state */
        if (ev.event == DI_EVENT_PRESS) {
            s_state.input_states[ev.id] = true;
        } else if (ev.event == DI_EVENT_RELEASE) {
            s_state.input_states[ev.id] = false;
        }

        /* Notify matching callbacks */
        DigitalInputEventData_t data;
        memset(&data, 0, sizeof(data));
        data.timestamp_ms = ev.timestamp_ms;

        for (int i = 0; i < s_state.num_callbacks; i++) {
            MockCallbackSlot *s = &s_state.callbacks[i];
            if (s->id == ev.id && (s->event_mask & ev.event)) {
                s->callback(s->context, ev.id, ev.event, &data);
            }
        }
    }
    return ERR_OK;
}

static const IDigitalInputSource_Vtable s_vtable = {
    .ReadInput          = mock_read,
    .GetInputType       = mock_get_type,
    .RegisterCallback   = mock_register_callback,
    .UnregisterCallback = mock_unregister_callback,
    .Process            = mock_process,
};

static IDigitalInputSource s_instance = {
    .vtable = &s_vtable,
    .impl   = &s_state,
};

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

IDigitalInputSource *MockDigitalInputSource_GetInstance(void)
{
    return &s_instance;
}

Result_t MockDigitalInputSource_InjectEvent(DigitalInputID_t id, DigitalInputEvent_t event)
{
    int next = (s_state.queue_tail + 1) % MOCK_DI_EVENT_QUEUE;
    if (next == s_state.queue_head) {
        return ERR_BUSY;
    }
    s_state.queue[s_state.queue_tail].id           = id;
    s_state.queue[s_state.queue_tail].event        = event;
    s_state.queue[s_state.queue_tail].timestamp_ms = 0; /* not used on PC */
    s_state.queue_tail = next;
    return ERR_OK;
}

Result_t MockDigitalInputSource_Process(void)
{
    return mock_process(&s_state);
}
