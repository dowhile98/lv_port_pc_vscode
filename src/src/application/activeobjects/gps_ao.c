#include "application/activeobjects/gps_ao.h"
#include <string.h>

#ifndef GPS_AO_QUEUE_SIZE
#define GPS_AO_QUEUE_SIZE 16
#endif

static void gps_ao_entry(void *input);

Result_t GPSAo_Init(GPSAo_t *self, IGPSIngestor *ingestor, const GPSAoConfig_t *config)
{
	if (self == NULL || ingestor == NULL || config == NULL)
	{
		return ERR_NULL_POINTER;
	}

	self->ingestor = ingestor;
	self->logger = config->logger;
	self->event_notifier = config->event_notifier;
	self->running = false;
	self->terminate = false;
	self->is_initialized = false;

	/* 1. Crear Queue de Eventos usando OSAL */
	os_queue_config_t queue_config = {
		.name = "GPS_AO_Q",
		.buffer = self->_queue_storage,
		.buffer_size = sizeof(self->_queue_storage),
		.item_size = sizeof(GPSAoEvent_t)};

	self->queue = NULL;
	Result_t result = os_queue_create(&self->queue, &queue_config);
	if (result != ERR_OK)
		return result;

	/* 2. stopped_sem: binary, init=0 */
	if (os_semaphore_create(&self->stopped_sem, "GPS_STOP_SEM", 0U) != ERR_OK)
	{
		os_queue_delete(self->queue);
		return ERR_ERROR;
	}

	/* 3. run_sem: binary, init=0 — gate para el outer loop */
	if (os_semaphore_create(&self->run_sem, "GPS_RUN_SEM", 0U) != ERR_OK)
	{
		os_semaphore_delete(self->stopped_sem);
		os_queue_delete(self->queue);
		return ERR_ERROR;
	}

	/* 4. Crear Thread — queda bloqueado en run_sem hasta Start() */
	os_thread_config_t thread_config = {
		.name = "GPS_AO_Thread",
		.entry = gps_ao_entry,
		.arg = self,
		.stack_ptr = config->stack_ptr,
		.stack_size = config->stack_size,
		.priority = config->priority,
		.auto_start = true};

	self->thread = NULL;
	result = os_thread_create(&self->thread, &thread_config);
	if (result != ERR_OK)
	{
		os_semaphore_delete(self->run_sem);
		os_semaphore_delete(self->stopped_sem);
		os_queue_delete(self->queue);
		return result;
	}

	self->is_initialized = true;

	if (Logger_IsValid(self->logger))
	{
		LOG_INFO(self->logger, "GPS_AO", "Initialized (queue_size=%d, priority=%d)",
				 GPS_AO_QUEUE_SIZE, config->priority);
	}

	return ERR_OK;
}

Result_t GPSAo_Start(GPSAo_t *self)
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
		LOG_INFO(self->logger, "GPS_AO", "Started");
	}
	return ERR_OK;
}

Result_t GPSAo_OnRxData(GPSAo_t *self, const uint8_t *data, size_t len)
{
	if (self == NULL || data == NULL)
		return ERR_NULL_POINTER;

	/* 1. Delegar buffering al Adapter (Thread-safe single writer) */
	/* Nota: Esto se llama desde ISR UART */
	Result_t ingest_result = GPS_Ingest_ProcessRxBuffer(self->ingestor, data, (uint16_t)len);

	/* 2. Notificar al Thread */
	GPSAoEvent_t evt;
	evt.type = GPS_EVT_DATA_READY;

	/* Usamos OS_NO_WAIT porque estamos en ISR */
	Result_t queue_result = os_queue_send(self->queue, &evt, OS_NO_WAIT);

	return (queue_result == ERR_OK) ? ingest_result : queue_result;
}

Result_t GPSAo_OnPPS(GPSAo_t *self)
{
	if (self == NULL)
		return ERR_NULL_POINTER;

	GPSAoEvent_t evt;
	evt.type = GPS_EVT_PPS;

	/* Usamos OS_NO_WAIT porque esto se llamará desde ISR EXTI */
	return os_queue_send(self->queue, &evt, OS_NO_WAIT);
}

void GPSAo_ProcessPacket(GPSAo_t *self)
{
	if (self == NULL)
		return;

	/* Delegar el procesamiento y parseo al Adapter */
	(void)GPS_Ingest_Process(self->ingestor);
}

static void gps_ao_entry(void *input)
{
	GPSAo_t *self = (GPSAo_t *)input;
	GPSAoEvent_t evt;

	while (!self->terminate)
	{
		os_semaphore_get(self->run_sem, OS_WAIT_FOREVER);
		if (self->terminate)
		{
			break;
		}

		if (Logger_IsValid(self->logger))
		{
			LOG_INFO(self->logger, "GPS_AO", "Thread started");
		}

		/* Inner loop: procesa eventos de la cola */
		while (self->running)
		{
			if (os_queue_receive(self->queue, &evt, OS_WAIT_FOREVER) == ERR_OK)
			{
				switch (evt.type)
				{
				case GPS_EVT_DATA_READY:
					GPSAo_ProcessPacket(self);
					break;

				case GPS_EVT_PPS:
					/* Handled by PPSDispatcher (see system_init.c). */
					break;

				case GPS_EVT_SHUTDOWN:
					if (Logger_IsValid(self->logger))
					{
						LOG_INFO(self->logger, "GPS_AO", "Shutdown event received");
					}
					self->running = false; /* salir del inner loop */
					break;

				default:
					break;
				}
			}
		}

		if (Logger_IsValid(self->logger))
		{
			LOG_INFO(self->logger, "GPS_AO", "Thread stopped");
		}
		(void)os_semaphore_put(self->stopped_sem); /* ← WaitStopped() */
	}
	(void)os_semaphore_put(self->stopped_sem); /* ← Deinit() */
}

Result_t GPSAo_Stop(GPSAo_t *self)
{
	if (self == NULL)
		return ERR_NULL_POINTER;
	if (!self->running)
		return ERR_INVALID_STATE;

	self->running = false;
	/* Postear SHUTDOWN para desbloquear el os_queue_receive del inner loop */
	GPSAoEvent_t shutdown_evt = {.type = GPS_EVT_SHUTDOWN};
	(void)os_queue_send(self->queue, &shutdown_evt, OS_NO_WAIT);

	return ERR_OK;
}

Result_t GPSAo_WaitStopped(GPSAo_t *self, uint32_t timeout_ms)
{
	if (self == NULL)
		return ERR_NULL_POINTER;
	return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t GPSAo_Deinit(GPSAo_t *self)
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
