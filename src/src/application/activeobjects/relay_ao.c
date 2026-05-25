#include "application/activeobjects/relay_ao.h"
#include <stddef.h>
#include <string.h>

/** Obtiene la hora actual via ITimeSource. Si no está disponible retorna {0}. */
static DateTime_t get_now(const RelayAO_t *self)
{
    DateTime_t ts = {0};
    if (self->time_source != NULL)
    {
        (void)TimeSource_GetTime(self->time_source, &ts);
    }
    return ts;
}

/**
 * @brief Detecta transiciones de estado del relay controller y emite eventos de log.
 * @note  Llamar tras cada RelayController_Update(). Non-blocking.
 */
static void check_relay_transitions(RelayAO_t *self)
{
    if (!EventNotifier_IsValid(self->event_notifier))
    {
        return;
    }

    RelayStatus_t cur = {0};
    if (RelayController_GetStatus(self->relay_controller, &cur) != ERR_OK)
    {
        return;
    }

    DateTime_t now = get_now(self);
    RelayInternalState_t prev_state = self->prev_status.internal_state;
    RelayInternalState_t cur_state = cur.internal_state;

    if (prev_state != RELAY_STATE_ACTIVE && cur_state == RELAY_STATE_ACTIVE)
    {
        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                        EVENT_TYPE_RELAY_CYCLE_START,
                                        EVENT_SEVERITY_INFO, 0U, 0U);
    }
    else if (prev_state == RELAY_STATE_ACTIVE && cur_state != RELAY_STATE_ACTIVE && cur_state != RELAY_STATE_ERROR)
    {
        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                        EVENT_TYPE_RELAY_CYCLE_END,
                                        EVENT_SEVERITY_INFO, 0U, 0U);
    }

    if (prev_state != RELAY_STATE_ERROR && cur_state == RELAY_STATE_ERROR)
    {
        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                        EVENT_TYPE_RELAY_ERROR,
                                        EVENT_SEVERITY_ERROR, 0U, 0U);
    }

    self->prev_status = cur;
}

/**
 * @brief Hilo de ejecución (Worker Thread) del Active Object.
 * @note REFACTORED (ACTION-005): Uses IRelayController interface methods.
 */
static void relay_ao_thread_entry(void *arg)
{
    RelayAO_t *self = (RelayAO_t *)arg;

    while (!self->terminate)
    {
        os_semaphore_get(self->run_sem, OS_WAIT_FOREVER);
        if (self->terminate)
        {
            break;
        }

        RelayAO_Event_t event;
        RelayController_SetState(self->relay_controller, RELAY_CONTACT_CLOSED);
        if (Logger_IsValid(self->logger))
        {
            LOG_INFO(self->logger, "RELAY_AO", "Thread started");
        }

        /* Inner loop: procesa eventos de la cola */
        while (self->running)
        {
            if (os_queue_receive(self->queue, &event, OS_WAIT_FOREVER) == ERR_OK)
            {
                switch (event.type)
                {
                case RELAY_EVENT_PPS:
                    RelayController_OnPPS(self->relay_controller, (const DateTime_t *)event.payload);
                    RelayController_Update(self->relay_controller);
                    check_relay_transitions(self);
                    break;

                case RELAY_EVENT_TIMER:
                case RELAY_EVENT_CONFIG:
                    RelayController_Update(self->relay_controller);
                    check_relay_transitions(self);
                    break;

                case RELAY_EVENT_TIMEOUT:
                    if (EventNotifier_IsValid(self->event_notifier))
                    {
                        DateTime_t now = get_now(self);
                        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                                        EVENT_TYPE_RELAY_SYNC_FAIL,
                                                        EVENT_SEVERITY_WARNING, 0U, 0U);
                    }
                    RelayController_Update(self->relay_controller);
                    check_relay_transitions(self);
                    break;

                case RELAY_EVENT_ALARM_OVERTEMP:
                    if (Logger_IsValid(self->logger))
                    {
                        LOG_WARN(self->logger, "RELAY_AO", "Overtemp alarm event received");
                    }
                    if (EventNotifier_IsValid(self->event_notifier))
                    {
                        DateTime_t now = get_now(self);
                        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                                        EVENT_TYPE_ALARM_HIGH_TEMP_ACTIVE,
                                                        EVENT_SEVERITY_CRITICAL, 0U, 0U);
                        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                                        EVENT_TYPE_RELAY_FORCED_OPEN,
                                                        EVENT_SEVERITY_CRITICAL, 0U, 0U);
                    }
                    RelayController_SetAlarmState(self->relay_controller, true);
                    break;

                case RELAY_EVENT_ALARM_CLEARED:
                    if (Logger_IsValid(self->logger))
                    {
                        LOG_INFO(self->logger, "RELAY_AO", "Overtemp alarm cleared event received");
                    }
                    if (EventNotifier_IsValid(self->event_notifier))
                    {
                        DateTime_t now = get_now(self);
                        (void)EventNotifier_NotifyEvent(self->event_notifier, &now,
                                                        EVENT_TYPE_ALARM_HIGH_TEMP_CLEARED,
                                                        EVENT_SEVERITY_WARNING, 0U, 0U);
                    }
                    RelayController_SetAlarmState(self->relay_controller, false);
                    RelayController_Update(self->relay_controller);
                    break;

                case RELAY_EVENT_SHUTDOWN:
                    if (Logger_IsValid(self->logger))
                    {
                        LOG_INFO(self->logger, "RELAY_AO", "Shutdown requested");
                    }
                    RelayController_SetState(self->relay_controller, RELAY_CONTACT_CLOSED);
                    self->running = false; /* salir del inner loop */
                    break;

                default:
                    break;
                }
            }
        }

        if (Logger_IsValid(self->logger))
        {
            LOG_INFO(self->logger, "RELAY_AO", "Thread stopped");
        }
        (void)os_semaphore_put(self->stopped_sem); /* ← WaitStopped() */
    }
    (void)os_semaphore_put(self->stopped_sem); /* ← Deinit() */
}

Result_t RelayAO_Init(RelayAO_t *self, const RelayAO_Config_t *config)
{
    // ✅ Validate interface instead of concrete adapter (ACTION-005)
    if (self == NULL || config == NULL || config->relay_controller == NULL)
    {
        return ERR_NULL_POINTER;
    }

    self->relay_controller = config->relay_controller;
    self->logger = config->logger;
    self->event_notifier = config->event_notifier;
    self->time_source = config->time_source; /* optional, may be NULL */
    self->running = false;
    self->terminate = false;
    self->is_initialized = false;
    memset(&self->prev_status, 0, sizeof(self->prev_status));

    /* 1. Crear Cola de Mensajes */
    os_queue_config_t q_cfg = {
        .name = "RelayQueue",
        .buffer = config->queue_buf,
        .buffer_size = config->queue_buf_size,
        .item_size = sizeof(RelayAO_Event_t)};
    Result_t res = os_queue_create(&self->queue, &q_cfg);
    if (res != ERR_OK)
        return res;

    /* 2. stopped_sem: binary, init=0 */
    if (os_semaphore_create(&self->stopped_sem, "RELAY_STOP_SEM", 0U) != ERR_OK)
    {
        os_queue_delete(self->queue);
        self->queue = NULL;
        return ERR_ERROR;
    }

    /* 3. run_sem: binary, init=0 — gate para el outer loop */
    if (os_semaphore_create(&self->run_sem, "RELAY_RUN_SEM", 0U) != ERR_OK)
    {
        os_semaphore_delete(self->stopped_sem);
        os_queue_delete(self->queue);
        self->queue = NULL;
        return ERR_ERROR;
    }

    /* 4. Crear Hilo — queda bloqueado en run_sem hasta Start() */
    os_thread_config_t t_cfg = {
        .name = "RelayAO",
        .entry = relay_ao_thread_entry,
        .arg = self,
        .stack_ptr = config->stack_ptr,
        .stack_size = config->stack_size,
        .priority = config->priority,
        .auto_start = true};

    res = os_thread_create(&self->thread, &t_cfg);
    if (res != ERR_OK)
    {
        os_semaphore_delete(self->run_sem);
        os_semaphore_delete(self->stopped_sem);
        os_queue_delete(self->queue);
        self->queue = NULL;
        return res;
    }

    self->is_initialized = true;

    if (Logger_IsValid(self->logger))
    {
        LOG_INFO(self->logger, "RELAY_AO", "Initialized (priority=%d)", config->priority);
    }

    return ERR_OK;
}

Result_t RelayAO_PostEvent(RelayAO_t *self, const RelayAO_Event_t *event)
{
    if (self == NULL || event == NULL)
        return ERR_NULL_POINTER;
    if (self->queue == NULL)
        return ERR_INVALID_STATE;

    // Envío asíncrono (OS_NO_WAIT) para garantizar que es seguro desde ISR (EXTI/Timer)
    return os_queue_send(self->queue, event, OS_NO_WAIT);
}

Result_t RelayAO_Start(RelayAO_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_initialized)
        return ERR_INVALID_STATE;
    if (self->running)
        return ERR_INVALID_STATE;

    self->running = true;
    (void)os_semaphore_put(self->run_sem);

    if (Logger_IsValid(self->logger))
    {
        LOG_INFO(self->logger, "RELAY_AO", "Started");
    }
    return ERR_OK;
}

Result_t RelayAO_Stop(RelayAO_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->running)
        return ERR_INVALID_STATE;

    self->running = false;
    /* Postear SHUTDOWN para desbloquear el os_queue_receive del inner loop */
    RelayAO_Event_t shutdown_event = {.type = RELAY_EVENT_SHUTDOWN};
    (void)RelayAO_PostEvent(self, &shutdown_event);

    return ERR_OK;
}

Result_t RelayAO_WaitStopped(RelayAO_t *self, uint32_t timeout_ms)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t RelayAO_Deinit(RelayAO_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (self->running)
        return ERR_BUSY; /* Llamar Stop() + WaitStopped() antes */

    /* Despertar el outer loop con la señal de terminación */
    self->terminate = true;
    (void)os_semaphore_put(self->run_sem);

    /* Esperar a que el hilo salga del outer loop */
    (void)os_semaphore_get(self->stopped_sem, OS_WAIT_FOREVER);

    os_semaphore_delete(self->run_sem);
    os_semaphore_delete(self->stopped_sem);
    if (self->queue != NULL)
    {
        os_queue_delete(self->queue);
        self->queue = NULL;
    }
    self->is_initialized = false;
    return ERR_OK;
}
