/**
 * @file digital_input_ao.c
 * @brief Active Object para polling de entradas digitales.
 */

#include "application/activeobjects/digital_input_ao.h"
#include <stddef.h>

/* ===== Thread Entry Point ===== */

/**
 * @brief Worker thread del Active Object.
 * @note Ejecuta polling periódico indefinidamente.
 */
static void digital_input_ao_thread_entry(void *arg)
{
    DigitalInputAO_t *self = (DigitalInputAO_t *)arg;

    while (!self->terminate)
    {
        os_semaphore_get(self->run_sem, OS_WAIT_FOREVER);
        if (self->terminate)
        {
            break;
        }

        if (Logger_IsValid(self->logger))
        {
            LOG_INFO(self->logger, "INPUT_AO", "Thread started");
        }

        while (self->running)
        {
            /* Polling periódico del adapter */
            DigitalInputSource_Process(self->input_source);
            os_thread_sleep(self->poll_interval_ms);
        }

        if (Logger_IsValid(self->logger))
        {
            LOG_INFO(self->logger, "INPUT_AO", "Thread stopped");
        }
        (void)os_semaphore_put(self->stopped_sem); /* ← WaitStopped() */
    }
    (void)os_semaphore_put(self->stopped_sem); /* ← Deinit() */
}

/* ===== Public API ===== */

Result_t DigitalInputAO_Init(DigitalInputAO_t *self, const DigitalInputAO_Config_t *config)
{
    /* Validaciones */
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->input_source == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->stack_ptr == NULL || config->stack_size == 0)
    {
        return ERR_INVALID_PARAM;
    }

    if (config->poll_interval_ms == 0)
    {
        return ERR_INVALID_PARAM; /* Polling interval debe ser > 0 */
    }

    /* Configurar instancia */
    self->input_source = config->input_source;
    self->poll_interval_ms = config->poll_interval_ms;
    self->logger = config->logger;
    self->event_notifier = config->event_notifier;
    self->running = false;
    self->terminate = false;
    self->is_initialized = false;

    /* stopped_sem: binary, init=0 */
    if (os_semaphore_create(&self->stopped_sem, "DI_STOP_SEM", 0U) != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* run_sem: binary, init=0 — gate para el outer loop */
    if (os_semaphore_create(&self->run_sem, "DI_RUN_SEM", 0U) != ERR_OK)
    {
        os_semaphore_delete(self->stopped_sem);
        return ERR_ERROR;
    }

    /* Crear thread — queda bloqueado en run_sem hasta Start() */
    os_thread_config_t t_cfg = {
        .name = "DigitalInputAO",
        .entry = digital_input_ao_thread_entry,
        .arg = self,
        .stack_ptr = config->stack_ptr,
        .stack_size = config->stack_size,
        .priority = config->priority,
        .auto_start = true};

    Result_t res = os_thread_create(&self->thread, &t_cfg);
    if (res != ERR_OK)
    {
        os_semaphore_delete(self->run_sem);
        os_semaphore_delete(self->stopped_sem);
        return res;
    }

    self->is_initialized = true;

    if (Logger_IsValid(self->logger))
    {
        LOG_INFO(self->logger, "INPUT_AO", "Initialized (poll=%dms, priority=%d)",
                 config->poll_interval_ms, config->priority);
    }

    return ERR_OK;
}

Result_t DigitalInputAO_Start(DigitalInputAO_t *self)
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
        LOG_INFO(self->logger, "INPUT_AO", "Started");
    }
    return ERR_OK;
}

Result_t DigitalInputAO_Stop(DigitalInputAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    self->running = false;
    return ERR_OK;
}

Result_t DigitalInputAO_WaitStopped(DigitalInputAO_t *self, uint32_t timeout_ms)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t DigitalInputAO_Deinit(DigitalInputAO_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (self->running)
        return ERR_INVALID_STATE; /* Llamar Stop() + WaitStopped() antes */

    /* Despertar el outer loop con la señal de terminación */
    self->terminate = true;
    (void)os_semaphore_put(self->run_sem);

    /* Esperar a que el hilo salga del outer loop */
    (void)os_semaphore_get(self->stopped_sem, OS_WAIT_FOREVER);

    os_semaphore_delete(self->run_sem);
    os_semaphore_delete(self->stopped_sem);

    self->is_initialized = false;
    return ERR_OK;
}
