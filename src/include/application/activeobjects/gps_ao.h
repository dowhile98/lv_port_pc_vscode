/*
 * gsp_ao.h
 *
 *  Created on: Jan 27, 2026
 *      Author: tecna-smart-lab
 */

#ifndef GPS_AO_H
#define GPS_AO_H

#include <stddef.h>
#include "infrastructure/osal/osal.h"
#include "interfaces/i_gps_ingestor.h"
#include "interfaces/i_time_sync_control.h"
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"
#include "hal/hal_types.h"

// Tipos de Eventos
typedef enum
{
    GPS_EVT_DATA_READY, // Nuevos datos en el Ring Buffer
    GPS_EVT_PPS,        // Evento PPS detectado
    GPS_EVT_ERROR,
    GPS_EVT_SHUTDOWN // Solicitud de shutdown graceful
} GPSAoEventType_t;

typedef struct
{
    GPSAoEventType_t type;
} GPSAoEvent_t;

// Configuración
typedef struct
{
    void *stack_ptr;
    uint32_t stack_size;
    uint8_t priority;
    /* REMOVED: ITimeSyncControl *time_sync; */
    /* PPS is now handled by PPSDispatcher, not GPSAo */
    ILogger *logger;                /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier; /**< Event notifier para eventos persistentes (EEPROM) */
} GPSAoConfig_t;

// Estructura del Active Object
typedef struct
{
    os_thread_t thread;         /**< Handle del thread (OSAL) */
    os_semaphore_t stopped_sem; /**< Señalizado al salir del inner loop (WaitStopped) y outer loop (Deinit) */
    os_semaphore_t run_sem;     /**< Gate del outer loop: dado por Start(), tomado por el hilo */
    os_queue_t queue;           /**< Cola de eventos (OSAL) */
    IGPSIngestor *ingestor;

    ILogger *logger;                /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier; /**< Event notifier para eventos (EEPROM) */

    /* Buffer estático de la queue (NO malloc) */
    uint8_t _queue_storage[16 * sizeof(GPSAoEvent_t)];

    volatile bool running;   /**< Flag de ejecución del inner loop */
    volatile bool terminate; /**< Flag de salida del outer loop (Deinit) */
    bool is_initialized;
} GPSAo_t;

/**
 * @brief Inicializa el Active Object del GPS.
 */
Result_t GPSAo_Init(GPSAo_t *self, IGPSIngestor *ingestor, const GPSAoConfig_t *config);

/**
 * @brief Arranca el thread.
 */
Result_t GPSAo_Start(GPSAo_t *self);

/**
 * @brief Detiene thread NMEA parsing de GPSAo
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si self es NULL
 * @return ERR_INVALID_STATE si no está corriendo
 * @note Graceful shutdown con timeout de 1 segundo
 */
Result_t GPSAo_Stop(GPSAo_t *self);

/**
 * @brief Espera a que el thread confirme que el inner loop terminó.
 * @param[in] self       Puntero al Active Object.
 * @param[in] timeout_ms Timeout en ms (OS_WAIT_FOREVER para espera infinita).
 * @return ERR_OK si el thread se detuvo; ERR_TIMEOUT si expiró.
 */
Result_t GPSAo_WaitStopped(GPSAo_t *self, uint32_t timeout_ms);

/**
 * @brief De-inicializa GPSAo y libera recursos OSAL.
 * @note Debe llamarse Stop() + WaitStopped() antes. Si el AO sigue corriendo retorna ERR_BUSY.
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 */
Result_t GPSAo_Deinit(GPSAo_t *self);

/**
 * @brief Callback invocado cuando se reciben datos UART (desde ISR).
 *
 * Escribe datos en ring buffer del adapter y notifica al thread vía queue.
 *
 * @warning Esta función ejecuta en contexto ISR.
 *          NO usar LOG_*() macros (lwprintf usa TX_MUTEX, no ISR-safe).
 *          Solo EventNotifier es ISR-safe si se necesita logging.
 *
 * @param[in] self  Puntero al Active Object (no NULL)
 * @param[in] data  Buffer de datos recibidos (no NULL)
 * @param[in] len   Longitud de datos en bytes
 *
 * @return ERR_OK si datos ingestados y evento encolado
 * @return ERR_BUSY si queue está llena (datos se pierden)
 * @return ERR_NULL_POINTER si self o data es NULL
 *
 * @note Thread-safety: ISR-safe (usa OS_NO_WAIT)
 * @note Llamada desde: DI_GPS_UART_RxCallback() (ISR UART)
 */
Result_t GPSAo_OnRxData(GPSAo_t *self, const uint8_t *data, size_t len);

/**
 * @brief Callback invocado cuando ocurre flanco PPS (desde ISR EXTI).
 *
 * Envía evento GPS_EVT_PPS al thread. PPS handling delegado a PPSDispatcher.
 *
 * @warning Esta función ejecuta en contexto ISR.
 *          NO usar LOG_*() macros.
 *
 * @param[in] self  Puntero al Active Object (no NULL)
 *
 * @return ERR_OK si evento encolado
 * @return ERR_BUSY si queue está llena
 * @return ERR_NULL_POINTER si self es NULL
 *
 * @note Thread-safety: ISR-safe (usa OS_NO_WAIT)
 * @note Llamada desde: HAL_GPIO_EXTI_Callback() (ISR EXTI)
 */
Result_t GPSAo_OnPPS(GPSAo_t *self);

/**
 * @brief (Solo para tests/interno) Procesa datos pendientes en el RB.
 */
void GPSAo_ProcessPacket(GPSAo_t *self);

#endif // GPS_AO_H
