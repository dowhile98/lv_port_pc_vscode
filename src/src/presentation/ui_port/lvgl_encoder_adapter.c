/**
 * @file lvgl_encoder_adapter.c
 * @brief Implementación del bridge IDigitalInputSource → IEncoder_t.
 *
 * Equivalente limpio de tcs_cicx1_lv_button.c del legacy v4.0.0.
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#include "lvgl_encoder_adapter.h"
#include "osal/osal.h"
#include <string.h>

/* ========================================================================
 * CALLBACKS — Ejecutan en contexto del thread de DigitalInputAO
 * ======================================================================== */

/**
 * @brief UP PRESS o KEEPALIVE → navegar al elemento anterior (counter--).
 */
static void on_up_event(void *ctx,
                        DigitalInputID_t id,
                        DigitalInputEvent_t evt,
                        const DigitalInputEventData_t *data)
{
    (void)id;
    (void)data;
    static uint32_t ticks = 0;

    LvglEncoderAdapter_t *self = (LvglEncoderAdapter_t *)ctx;

    if (evt == DI_EVENT_PRESS)
    {
        ticks = os_ticks_get();
        self->counter--;
    }

    if ((os_ticks_get() - ticks) >= 500)
    {
        self->counter--;
    }

    if ((os_ticks_get() - ticks) >= 500)
    {
        self->counter--;
    }
}

/**
 * @brief DOWN PRESS o KEEPALIVE → navegar al elemento siguiente (counter++).
 */
static void on_down_event(void *ctx,
                          DigitalInputID_t id,
                          DigitalInputEvent_t evt,
                          const DigitalInputEventData_t *data)
{
    (void)id;
    (void)data;
    static uint32_t ticks = 0;

    LvglEncoderAdapter_t *self = (LvglEncoderAdapter_t *)ctx;

    if (evt == DI_EVENT_PRESS)
    {
        ticks = os_ticks_get();
        self->counter++;
    }

    if ((os_ticks_get() - ticks) >= 500)
    {
        self->counter++;
    }
}

/**
 * @brief ENTER PRESS → marcar botón pulsado.
 */
static void on_enter_press(void *ctx,
                           DigitalInputID_t id,
                           DigitalInputEvent_t evt,
                           const DigitalInputEventData_t *data)
{
    (void)id;
    (void)evt;
    (void)data;

    LvglEncoderAdapter_t *self = (LvglEncoderAdapter_t *)ctx;
    self->pressed = true;
}

/**
 * @brief ENTER RELEASE → marcar botón liberado.
 */
static void on_enter_release(void *ctx,
                             DigitalInputID_t id,
                             DigitalInputEvent_t evt,
                             const DigitalInputEventData_t *data)
{
    (void)id;
    (void)evt;
    (void)data;

    LvglEncoderAdapter_t *self = (LvglEncoderAdapter_t *)ctx;
    self->pressed = false;
}

/* ========================================================================
 * IMPLEMENTACIÓN DE IEncoder_t::Poll
 * Ejecuta en contexto del thread LVGL refresh
 * ======================================================================== */

static Result_t poll(IEncoder_t *iface, int32_t *delta, bool *pressed)
{
    if (iface == NULL || delta == NULL || pressed == NULL)
    {
        return ERR_NULL_POINTER;
    }

    LvglEncoderAdapter_t *self = (LvglEncoderAdapter_t *)iface->impl;

    /*
     * Leer counter y calcular delta desde el último poll.
     * En Cortex-M33 single-core (STM32U575) la lectura de int16_t volátil
     * es atómica — no se requiere mutex.
     */
    const int16_t current = self->counter;
    *delta = (int32_t)(current - self->last_counter);
    *pressed = self->pressed;

    self->last_counter = current;

    return ERR_OK;
}

static const IEncoder_Vtable_t s_vtable = {
    .Poll = poll,
};

/* ========================================================================
 * API PÚBLICA
 * ======================================================================== */

Result_t LvglEncoderAdapter_Init(LvglEncoderAdapter_t *self,
                                 IDigitalInputSource *di_source)
{
    if (self == NULL || di_source == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(LvglEncoderAdapter_t));
    self->base.vtable = &s_vtable;
    self->base.impl = self;
    self->di_source = di_source;

    Result_t res;

    /* UP: PRESS + KEEPALIVE → counter-- */
    res = DigitalInputSource_RegisterCallback(di_source,
                                              DI_ID_BUTTON_UP, DI_EVENT_PRESS | DI_EVENT_KEEPALIVE, on_up_event, self);
    if (res != ERR_OK)
    {
        return res;
    }
    /* DOWN: PRESS + KEEPALIVE → counter++ */
    res = DigitalInputSource_RegisterCallback(di_source,
                                              DI_ID_BUTTON_DOWN, DI_EVENT_PRESS | DI_EVENT_KEEPALIVE, on_down_event, self);
    if (res != ERR_OK)
    {
        return res;
    }
    /* ENTER: PRESS y RELEASE para estado continuo (pass-through a LVGL) */
    res = DigitalInputSource_RegisterCallback(di_source,
                                              DI_ID_BUTTON_ENTER, DI_EVENT_PRESS, on_enter_press, self);
    if (res != ERR_OK)
    {
        return res;
    }

    res = DigitalInputSource_RegisterCallback(di_source,
                                              DI_ID_BUTTON_ENTER, DI_EVENT_RELEASE, on_enter_release, self);
    return res;
}

IEncoder_t *LvglEncoderAdapter_GetInterface(LvglEncoderAdapter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->base;
}
