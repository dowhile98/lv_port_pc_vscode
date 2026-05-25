/**
 * @file digital_input_adapter.c
 * @brief Implementación del adaptador de entradas digitales.
 *
 * Integra lwbtn con I_GPIO usando Clean Architecture.
 * Observer Pattern para dispatch de callbacks.
 */

#include "infrastructure/adapters/digital_input_adapter.h"
#include <string.h>

/* ===== Forward Declarations ===== */
static Result_t di_adapter_read_input(void *self, DigitalInputID_t id, bool *out_state);
static Result_t di_adapter_get_type(void *self, DigitalInputID_t id, DigitalInputType_t *out_type);
static Result_t di_adapter_register_callback(void *self, DigitalInputID_t id,
                                             DigitalInputEvent_t event_mask,
                                             DigitalInputCallback_t callback, void *context);
static Result_t di_adapter_unregister_callback(void *self, DigitalInputID_t id,
                                               DigitalInputCallback_t callback,
                                               DigitalInputEvent_t event_mask);
static Result_t di_adapter_process(void *self);

/* lwbtn callbacks */
static uint8_t di_adapter_get_state(struct lwbtn *lw, struct lwbtn_btn *btn);
static void di_adapter_on_event(struct lwbtn *lw, struct lwbtn_btn *btn, lwbtn_evt_t evt);

/* Helper functions */

/* ===== Vtable Implementation ===== */
static const IDigitalInputSource_Vtable di_adapter_vtable = {
    .ReadInput = di_adapter_read_input,
    .GetInputType = di_adapter_get_type,
    .RegisterCallback = di_adapter_register_callback,
    .UnregisterCallback = di_adapter_unregister_callback,
    .Process = di_adapter_process};

/* ===== Public API ===== */

Result_t DigitalInputAdapter_Init(DigitalInputAdapter_t *self,
                                  const DigitalInputAdapterConfig_t *config)
{
    /* Validaciones */
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->gpio == NULL || config->inputs == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->input_count == 0 || config->input_count > DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    /* Inicializar estructura */
    memset(self, 0, sizeof(DigitalInputAdapter_t));

    /* Configurar interfaz */
    self->iface.vtable = &di_adapter_vtable;
    self->iface.impl = self;

    /* Guardar dependencias */
    self->gpio = config->gpio;
    self->inputs = config->inputs;
    self->input_count = config->input_count;
    self->backlight_svc = config->backlight_svc; /* puede ser NULL */

    /* Inicializar contextos y lwbtn por cada entrada ACTIVA */
    /* CRÍTICO: lwbtn procesa botones secuencialmente (índices 0, 1, 2...) */
    /* NO usar config->inputs[i].id como índice, usar i directamente */
    for (uint8_t i = 0; i < config->input_count; i++)
    {
        /* Setup context usando índice secuencial i (NO el ID del botón) */
        self->button_contexts[i].adapter = self;
        self->button_contexts[i].config = &self->inputs[i];

        /* Setup lwbtn button usando índice secuencial i */
        lwbtn_btn_t *btn = &self->buttons[i];
        btn->arg = (void *)&self->button_contexts[i];
    }

    /* Inicializar lwbtn global */
    /* lwbtn procesará buttons[0..input_count-1] secuencialmente */
    lwbtn_init(self->buttons, config->input_count,
               di_adapter_get_state, di_adapter_on_event);

    return ERR_OK;
}

IDigitalInputSource *DigitalInputAdapter_GetInterface(DigitalInputAdapter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}

/* ===== lwbtn Callbacks ===== */

/**
 * @brief Callback lwbtn para leer estado de pin GPIO.
 * @note Ejecuta en contexto de lwbtn_process() (polling thread).
 */
static uint8_t di_adapter_get_state(struct lwbtn *lw, struct lwbtn_btn *btn)
{
    (void)lw; /* No usado, tenemos adapter en context */
    static uint32_t init_ticks = 0;
    if ((os_ticks_get() - init_ticks < 500))
    {
        /* Ignorar eventos durante los primeros 100 ticks para evitar ruido de inicialización */
        return 0;
    }
    /* Recuperar context desde btn->arg */
    DI_ButtonContext_t *ctx = (DI_ButtonContext_t *)btn->arg;
    if (ctx == NULL || ctx->adapter == NULL || ctx->config == NULL)
    {
        return 0; /* Error → inactivo */
    }

    DigitalInputAdapter_t *self = ctx->adapter;
    const DigitalInputConfig_t *cfg = ctx->config;

    /* Leer GPIO */
    GPIO_State_t gpio_state;
    Result_t res = GPIO_ReadPin(self->gpio, cfg->port, cfg->pin, &gpio_state);

    if (res != ERR_OK)
    {
        return 0; /* Error → consideramos inactivo */
    }

    /* Lógica active_low */
    bool is_active = (gpio_state == I_GPIO_STATE_HIGH);
    if (cfg->active_low)
    {
        is_active = !is_active;
    }

    return is_active ? 1 : 0;
}

/**
 * @brief Callback lwbtn cuando detecta evento.
 * @note Despacha a todos los observers registrados (Observer Pattern).
 * @note ACTION-012: Construye metadata con click_count, keepalive_count, press_duration.
 */
static void di_adapter_on_event(struct lwbtn *lw, struct lwbtn_btn *btn, lwbtn_evt_t evt)
{
    (void)lw; /* No usado */

    /* Recuperar context */
    DI_ButtonContext_t *ctx = (DI_ButtonContext_t *)btn->arg;
    if (ctx == NULL || ctx->adapter == NULL || ctx->config == NULL)
    {
        return;
    }

    DigitalInputAdapter_t *self = ctx->adapter;
    const DigitalInputConfig_t *cfg = ctx->config;

    /* Mapear lwbtn_evt_t → DigitalInputEvent_t */
    DigitalInputEvent_t event;
    switch (evt)
    {
    case LWBTN_EVT_ONPRESS:
        event = DI_EVENT_PRESS;
        break;
    case LWBTN_EVT_ONRELEASE:
        event = DI_EVENT_RELEASE;
        break;
#if LWBTN_CFG_USE_CLICK
    case LWBTN_EVT_ONCLICK:
        event = DI_EVENT_CLICK;
        break;
#endif
#if LWBTN_CFG_USE_KEEPALIVE
    case LWBTN_EVT_KEEPALIVE:
        event = DI_EVENT_KEEPALIVE;
        break;
#endif
    default:
        return; /* Evento no soportado */
    }

    /* Construir metadata desde estado lwbtn (ACTION-012) */
    DigitalInputEventData_t data;
    data.timestamp_ms = os_ticks_get(); /* OSAL wrapper */

#if LWBTN_CFG_USE_CLICK
    data.click_count = lwbtn_click_get_count(btn); /* Macro lwbtn */
#else
    data.click_count = 0;
#endif

#if LWBTN_CFG_USE_KEEPALIVE
    data.keepalive_count = lwbtn_keepalive_get_count(btn); /* Macro lwbtn */
#else
    data.keepalive_count = 0;
#endif

    /* Duración de presión (solo válido para RELEASE/CLICK) */
    if (event == DI_EVENT_RELEASE || event == DI_EVENT_CLICK)
    {
        uint32_t current_time = data.timestamp_ms;
        uint32_t press_time = btn->time_change; /* lwbtn field: timestamp de último press válido */
        data.press_duration_ms = (current_time > press_time) ? (current_time - press_time) : 0;
    }
    else
    {
        data.press_duration_ms = 0;
    }

    /* ===== BACKLIGHT MANAGEMENT ===== */
    /* Gestionar backlight para eventos de usuario (excepto alarmas críticas) */
    /* EXCEPCIÓN: DI_ID_ALARM_OVERTEMP siempre ejecuta callbacks (alarma crítica) */
    if (self->backlight_svc != NULL )
    {
        if (!DisplayBacklightService_IsAwake(self->backlight_svc))
        {
            /* Pantalla dormida → despertar y CONSUMIR evento (no ejecutar callbacks) */
            DisplayBacklightService_WakeUp(self->backlight_svc);

            if(cfg->id != DI_ID_ALARM_OVERTEMP)
            {
            	return; /* Primer evento solo despierta, no ejecuta acción */
            }
        }
        else
        {
            /* Pantalla activa → resetear timer de inactividad */
            DisplayBacklightService_ResetTimer(self->backlight_svc);
        }
    }
    /* Si es alarma (OVERTEMP) o backlight_svc es NULL, continuar con dispatch normal */


    /* ===== DISPATCH con bitmask check ===== */
    /* Acceder a subscribers[id] (array 1D de todos los callbacks del input) */
    DigitalInputSubscriber_t *subs = self->subscribers[cfg->id];

    /* Iterar todos los slots y verificar si tienen el bit del evento activo */
    for (uint8_t i = 0; i < DI_MAX_CALLBACKS_PER_INPUT; i++)
    {
        /* Check: slot activo Y tiene el bit del evento en su máscara */
        if (subs[i].active && (subs[i].event_mask & event))
        {
            /* Callback suscrito a este evento → invocar */
            subs[i].callback(subs[i].context, cfg->id, event, &data);
        }
    }
}

/* ===== Helper Functions ===== */

/* map_lwbtn_event() removida en ACTION-012: mapeo inline en di_adapter_on_event() */

/* ===== Vtable Implementations ===== */

static Result_t di_adapter_read_input(void *impl, DigitalInputID_t id, bool *out_state)
{
    DigitalInputAdapter_t *self = (DigitalInputAdapter_t *)impl;

    if (self == NULL || out_state == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    /* Buscar config de la entrada */
    const DigitalInputConfig_t *cfg = NULL;
    for (uint8_t i = 0; i < self->input_count; i++)
    {
        if (self->inputs[i].id == id)
        {
            cfg = &self->inputs[i];
            break;
        }
    }

    if (cfg == NULL)
    {
        return ERR_INVALID_PARAM; /* ID no configurado */
    }

    /* Leer GPIO */
    GPIO_State_t gpio_state;
    Result_t res = GPIO_ReadPin(self->gpio, cfg->port, cfg->pin, &gpio_state);

    if (res != ERR_OK)
    {
        return res;
    }

    /* Aplicar lógica active_low */
    bool is_active = (gpio_state == I_GPIO_STATE_HIGH);
    if (cfg->active_low)
    {
        is_active = !is_active;
    }

    *out_state = is_active;
    return ERR_OK;
}

static Result_t di_adapter_get_type(void *impl, DigitalInputID_t id, DigitalInputType_t *out_type)
{
    DigitalInputAdapter_t *self = (DigitalInputAdapter_t *)impl;

    if (self == NULL || out_type == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    /* Buscar tipo en config */
    for (uint8_t i = 0; i < self->input_count; i++)
    {
        if (self->inputs[i].id == id)
        {
            *out_type = self->inputs[i].type;
            return ERR_OK;
        }
    }

    return ERR_INVALID_PARAM; /* ID no configurado */
}

static Result_t di_adapter_register_callback(void *impl, DigitalInputID_t id,
                                             DigitalInputEvent_t event_mask,
                                             DigitalInputCallback_t callback, void *context)
{
    DigitalInputAdapter_t *self = (DigitalInputAdapter_t *)impl;

    if (self == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (id >= DI_ID_COUNT || event_mask == DI_EVENT_NONE)
    {
        return ERR_INVALID_PARAM;
    }

    /* Acceder a subscribers[id] (array 1D de slots) */
    DigitalInputSubscriber_t *subs = self->subscribers[id];

    /* ===== Paso 1: Buscar si callback+context YA existe (para merge) ===== */
    for (uint8_t i = 0; i < DI_MAX_CALLBACKS_PER_INPUT; i++)
    {
        if (subs[i].active &&
            subs[i].callback == callback &&
            subs[i].context == context)
        {
            /* MERGE: Agregar nuevos eventos a máscara existente (OR bitwise) */
            subs[i].event_mask |= event_mask;
            return ERR_OK;
        }
    }

    /* ===== Paso 2: Callback no existe, buscar slot libre ===== */
    for (uint8_t i = 0; i < DI_MAX_CALLBACKS_PER_INPUT; i++)
    {
        if (!subs[i].active)
        {
            /* Nuevo registro */
            subs[i].callback = callback;
            subs[i].context = context;
            subs[i].event_mask = event_mask; /* Guardar máscara de eventos */
            subs[i].active = true;
            return ERR_OK;
        }
    }

    return ERR_BUSY; /* No hay slots disponibles */
}

static Result_t di_adapter_unregister_callback(void *impl, DigitalInputID_t id,
                                               DigitalInputCallback_t callback,
                                               DigitalInputEvent_t event_mask)
{
    DigitalInputAdapter_t *self = (DigitalInputAdapter_t *)impl;

    if (self == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (id >= DI_ID_COUNT)
    {
        return ERR_INVALID_PARAM;
    }

    /* Buscar callback en subscribers[id] (array 1D) */
    DigitalInputSubscriber_t *subs = self->subscribers[id];

    for (uint8_t i = 0; i < DI_MAX_CALLBACKS_PER_INPUT; i++)
    {
        if (subs[i].active && subs[i].callback == callback)
        {
            /* OPCIÓN B: Remover eventos especificados de la máscara (AND con NOT) */
            subs[i].event_mask &= ~event_mask;

            /* Si la máscara quedó vacía, liberar slot completamente */
            if (subs[i].event_mask == DI_EVENT_NONE)
            {
                subs[i].active = false;
                subs[i].callback = NULL;
                subs[i].context = NULL;
            }

            return ERR_OK; /* Primera coincidencia procesada */
        }
    }

    return ERR_ERROR; /* Callback no encontrado */
}

static Result_t di_adapter_process(void *impl)
{
    DigitalInputAdapter_t *self = (DigitalInputAdapter_t *)impl;

    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Obtener timestamp en ms para lwbtn */

    uint32_t current_time_ms = os_ticks_get();

    /* Procesar lwbtn (debouncing + detección de eventos) */
    lwbtn_process(current_time_ms);

    return ERR_OK;
}

Result_t DigitalInputAdapter_Deinit(DigitalInputAdapter_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Limpiar toda la estructura de forma segura (simétrico con Init) */
    /* Esto resetea: subscribers, button_contexts, buttons, y dependencias */
    memset(self, 0, sizeof(DigitalInputAdapter_t));

    return ERR_OK;
}
