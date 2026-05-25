/**
 * @file relay_ao.h
 * @brief Active Object para el subsistema de relés.
 *
 * Encapsula la concurrencia utilizando OSAL para procesar eventos
 * del RelayAdapter de forma asíncrona.
 *
 * @note REFACTORED (ACTION-005): Now depends on IRelayController interface (DIP compliant).
 *       This enables unit testing with mock controllers and follows Clean Architecture.
 */

#ifndef RELAY_AO_H
#define RELAY_AO_H

#include "infrastructure/osal/osal.h"
#include "interfaces/i_relay_controller.h" /* ✅ Interface, not concrete adapter */
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"
#include "interfaces/i_time_source.h"

/**
 * @brief Tipos de eventos que procesa el Relay Active Object.
 * @note EXTENDED (ACTION-009): Added alarm events for critical temperature monitoring.
 */
typedef enum
{
    RELAY_EVENT_PPS,            /**< Evento de sincronización GPS. */
    RELAY_EVENT_TIMER,          /**< Evento de transición del Timer. */
    RELAY_EVENT_CONFIG,         /**< Cambio de configuración. */
    RELAY_EVENT_TIMEOUT,        /**< Pérdida de sincronización. */
    RELAY_EVENT_ALARM_OVERTEMP, /**< ✨ Alarma de sobretemperatura activa (safety-critical). */
    RELAY_EVENT_ALARM_CLEARED,  /**< ✨ Alarma de sobretemperatura liberada. */
    RELAY_EVENT_SHUTDOWN        /**< ✨ Solicitud de apagado graceful (lifecycle). */
} RelayAO_EventType_t;

/**
 * @brief Estructura de mensaje para la cola.
 */
typedef struct
{
    RelayAO_EventType_t type;
    void *payload;
} RelayAO_Event_t;

/**
 * @brief Configuración para el Relay Active Object.
 * @note CHANGED (ACTION-005): relay_controller is now IRelayController*, not RelayAdapter_t*.
 */
typedef struct
{
    IRelayController *relay_controller; /* ✅ Interface (was: RelayAdapter_t *adapter) */
    uint8_t priority;                   /**< Prioridad del thread (0-255). */
    void *stack_ptr;
    uint32_t stack_size;
    void *queue_buf;
    uint32_t queue_buf_size;
    ILogger *logger;                /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier; /**< Event notifier para eventos (EEPROM) */
    ITimeSource *time_source;       /**< Time source para obtener timestamp real (opcional). */
} RelayAO_Config_t;

/**
 * @brief Estructura del Active Object.
 * @note CHANGED (ACTION-005): Stores IRelayController*, not RelayAdapter_t*.
 */
typedef struct
{
    os_thread_t thread;
    os_semaphore_t stopped_sem; /**< Señalizado al salir del inner loop (WaitStopped) y outer loop (Deinit) */
    os_semaphore_t run_sem;     /**< Gate del outer loop: dado por Start(), tomado por el hilo */
    os_queue_t queue;
    IRelayController *relay_controller; /* ✅ Interface (was: RelayAdapter_t *adapter) */
    ILogger *logger;                    /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier;     /**< Event notifier para eventos (EEPROM) */
    ITimeSource *time_source;           /**< Time source para timestamps en eventos (opcional). */
    RelayStatus_t prev_status;          /**< Estado previo para detección de transiciones de ciclo */
    volatile bool running;              /**< Flag de ejecución del inner loop */
    volatile bool terminate;            /**< Flag de salida del outer loop (Deinit) */
    bool is_initialized;
} RelayAO_t;

/**
 * @brief Inicializa el Active Object.
 *
 * @param self Instancia.
 * @param config Configuración.
 * @return ERR_OK si éxito.
 */
Result_t RelayAO_Init(RelayAO_t *self, const RelayAO_Config_t *config);

/**
 * @brief Inicia el thread del Active Object RelayAO
 * @param[in] self Puntero al Active Object (debe estar inicializado)
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si self es NULL
 * @return ERR_INVALID_STATE si no está inicializado o ya está corriendo
 * @note Thread-safe: puede llamarse desde cualquier contexto
 * @note Idempotente: llamar Start() en un AO ya corriendo retorna ERR_INVALID_STATE
 */
Result_t RelayAO_Start(RelayAO_t *self);

/**
 * @brief Detiene el thread del Active Object RelayAO
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si self es NULL
 * @return ERR_INVALID_STATE si no está corriendo
 * @note Graceful shutdown: procesa eventos pendientes antes de detener
 * @note Timeout: 1 segundo máximo para shutdown
 */
Result_t RelayAO_Stop(RelayAO_t *self);

/**
 * @brief Espera a que el thread confirme que el inner loop terminó.
 * @param[in] self       Puntero al Active Object.
 * @param[in] timeout_ms Timeout en ms (OS_WAIT_FOREVER para espera infinita).
 * @return ERR_OK si el thread se detuvo; ERR_TIMEOUT si expiró.
 */
Result_t RelayAO_WaitStopped(RelayAO_t *self, uint32_t timeout_ms);

/**
 * @brief De-inicializa el Active Object y libera recursos OSAL.
 * @note Debe llamarse Stop() + WaitStopped() antes. Si el AO sigue corriendo retorna ERR_BUSY.
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 */
Result_t RelayAO_Deinit(RelayAO_t *self);

/**
 * @brief Envía un evento al Active Object (Seguro para ISR).
 *
 * @param self Instancia.
 * @param event Evento a postear.
 * @return ERR_OK si el mensaje se encoló.
 */
Result_t RelayAO_PostEvent(RelayAO_t *self, const RelayAO_Event_t *event);

#endif /* RELAY_AO_H */
