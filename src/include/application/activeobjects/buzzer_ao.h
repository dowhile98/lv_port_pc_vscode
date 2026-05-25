/**
 * @file buzzer_ao.h
 * @brief Buzzer Active Object - Gestión asíncrona de notificaciones sonoras
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Active Object que gestiona las notificaciones sonoras del sistema de forma
 * asíncrona mediante una cola de eventos. Implementa el patrón Active Object
 * con:
 * - Thread dedicado con prioridad configurable
 * - Cola de comandos con priorización
 * - Máquina de estados (IDLE/PLAYING)
 * - Polling automático del adapter (auto-stop sin timer RTOS)
 *
 * **Características:**
 * - Deferred Processing: comandos de beep no bloquean el llamador
 * - Priority Queue: comandos de alta prioridad (alarmas) saltan la cola
 * - Non-blocking: post de comandos es O(1) con timeout configurable
 * - Thread-safe: protegido por mutex interno de ThreadX
 * - Auto-stop automático: ejecuta BuzzerAdapter_Update() cada 20ms
 *
 * **Casos de Uso:**
 * - Feedback de botones UI (prioridad baja)
 * - Alarmas de temperatura (prioridad alta)
 * - Confirmaciones de operaciones (prioridad media)
 * - Secuencias de boot/shutdown
 *
 * **Flujo de Eventos:**
 * ```
 * [Productor] → BuzzerAO_PostCommand() → [Cola ThreadX]
 *                                              ↓
 *                                    [Thread BuzzerAO]
 *                                              ↓
 *                              BuzzerControl_Beep() → [Hardware]
 * ```
 *
 * **Diagrama de Estados:**
 * ```
 * ┌──────────┐   BeepCmd    ┌──────────┐
 * │   IDLE   │ ───────────→ │ PLAYING  │
 * └──────────┘              └──────────┘
 *      ↑                         │
 *      │      Timeout/Stop       │
 *      └─────────────────────────┘
 * ```
 *
 * @note Requiere ThreadX RTOS
 * @note Consume ~2KB de stack + cola
 * @note Prioridad recomendada: 10 (media-baja, no crítica)
 *
 * @see IBuzzerControl
 * @see BuzzerAdapter
 */

#ifndef BUZZER_AO_H
#define BUZZER_AO_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "common/task_priorities.h" /* Mapa centralizado de prioridades */
#include "interfaces/i_buzzer_control.h"
#include "domain/interfaces/i_audible_notifier.h"
#include "hal/hal_types.h"
#include "infrastructure/osal/osal.h" /* os_thread_t, os_queue_t */
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"
/*============================================================================*
 * CONSTANTS
 *============================================================================*/

/**
 * @brief Tamaño de cola de comandos de beep
 * @note Dimensionado para 10 eventos pendientes típicos
 */
#define BUZZER_AO_QUEUE_SIZE 10U

/**
 * @brief Tamaño de stack del thread del Active Object
 * @note 2KB suficiente para lógica del AO + llamadas a driver
 */
#define BUZZER_AO_THREAD_STACK_SIZE 2048U

/**
 * @brief Prioridad por defecto del thread BuzzerAO.
 *
 * Tier 4 (periféricos de usuario) — no crítico.  Ver @c task_priorities.h.
 */
#define BUZZER_AO_THREAD_PRIORITY TASK_PRIO_BUZZER

/**
 * @brief Timeout para post de comandos (ms)
 * @note Si la cola está llena, espera 50ms antes de descartar
 */
#define BUZZER_AO_POST_TIMEOUT_MS 50U

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Estados de la máquina de estados del BuzzerAO
     */
    typedef enum
    {
        BUZZER_AO_STATE_IDLE = 0, /**< Silencio, esperando comandos */
        BUZZER_AO_STATE_PLAYING,  /**< Reproduciendo beep activo */
    } BuzzerAO_State_t;

    /* Forward declaration */
    struct BuzzerAdapter;

    /**
     * @brief Configuración de inicialización del BuzzerAO
     */
    typedef struct
    {
        IBuzzerControl *buzzer_control; /**< Interfaz de control de buzzer (DI) */
        uint8_t thread_priority;        /**< Prioridad del thread (0-31) */
        uint16_t queue_size;            /**< Tamaño de cola de comandos */
        uint32_t stack_size;            /**< Tamaño de stack del thread (bytes) */
        /*log*/
        ILogger *logger; /**< Logger para debug (UART/SWO) */
    } BuzzerAO_Config_t;

    /**
     * @brief Estructura del BuzzerAO (definición completa)
     * @note Movida a header para permitir embedding en DependencyContainer
     */
    typedef struct BuzzerAO
    {
        /* Dependencias inyectadas */
        IBuzzerControl *buzzer_control;

        /* OSAL objects */
        os_thread_t thread;
        os_semaphore_t stopped_sem; /**< Señalizado al salir del inner loop (WaitStopped) y outer loop (Deinit) */
        os_semaphore_t run_sem;     /**< Gate del outer loop: dado por Start(), tomado por el hilo */
        os_queue_t queue;

        /* Buffers estáticos (NO malloc) */
        uint8_t stack[BUZZER_AO_THREAD_STACK_SIZE];
        uint8_t queue_storage[BUZZER_AO_QUEUE_SIZE * sizeof(BuzzerCommand_t)];

        /* Configuración */
        uint8_t thread_priority;
        uint16_t queue_size;
        /*log*/
        ILogger *logger; /**< Logger para debug (UART/SWO) */
        /* Estado */
        BuzzerAO_State_t state;
        volatile bool is_running; /**< Flag de ejecución del inner loop */
        volatile bool terminate;  /**< Flag de salida del outer loop (Deinit) */
        bool is_initialized;

        /* Comando actual en reproducción */
        BuzzerCommand_t current_command;
    } BuzzerAO_t;

    /*============================================================================*
     * PUBLIC API
     *============================================================================*/

    /**
     * @brief Inicializa el Buzzer Active Object
     *
     * @param[in] ao Puntero a estructura BuzzerAO (pre-alocada)
     * @param[in] config Configuración de inicialización
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si ao o config son NULL
     * @return ERR_INVALID_PARAM si buzzer_control es NULL
     * @return ERR_ERROR si falla creación de thread/cola
     *
     * @note Thread se crea suspendido, usar BuzzerAO_Start() para activar
     * @note Asigna memoria para stack y cola desde heap de ThreadX
     *
     * @pre config->buzzer_control != NULL
     * @post Thread creado y listo para start
     *
     * @code
     * BuzzerAO_t buzzer_ao;
     * BuzzerAO_Config_t config = {
     *     .buzzer_control = BuzzerAdapter_GetInterface(&buzzer_adapter),
     *     .thread_priority = BUZZER_AO_THREAD_PRIORITY,
     *     .queue_size = BUZZER_AO_QUEUE_SIZE,
     *     .stack_size = BUZZER_AO_THREAD_STACK_SIZE
     * };
     * Result_t res = BuzzerAO_Init(&buzzer_ao, &config);
     * @endcode
     */
    Result_t BuzzerAO_Init(BuzzerAO_t *ao, const BuzzerAO_Config_t *config);

    /**
     * @brief Inicia el thread del Active Object
     *
     * @param[in] ao Puntero a BuzzerAO inicializado
     * @return ERR_OK si exitoso, ERR_ERROR si falla start
     *
     * @note Debe llamarse después de BuzzerAO_Init()
     * @note Thread entra en loop esperando comandos
     */
    Result_t BuzzerAO_Start(BuzzerAO_t *ao);

    /**
     * @brief Detiene el thread del Active Object
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return ERR_OK si exitoso
     *
     * @note Termina el thread de forma segura
     * @note Comandos pendientes en cola son descartados
     * @note Buzzer se detiene inmediatamente
     */
    Result_t BuzzerAO_Stop(BuzzerAO_t *ao);

    /**
     * @brief Espera a que el thread confirme que el inner loop terminó.
     * @param[in] ao         Puntero al BuzzerAO.
     * @param[in] timeout_ms Timeout en ms (OS_WAIT_FOREVER para espera infinita).
     * @return ERR_OK si el thread se detuvo; ERR_TIMEOUT si expiró.
     */
    Result_t BuzzerAO_WaitStopped(BuzzerAO_t *ao, uint32_t timeout_ms);

    /**
     * @brief Posta un comando de beep al Active Object (no bloqueante)
     *
     * @param[in] ao Puntero a BuzzerAO
     * @param[in] command Comando de beep a ejecutar
     *
     * @return ERR_OK si comando encolado exitosamente
     * @return ERR_NULL_POINTER si ao o command son NULL
     * @return ERR_TIMEOUT si cola llena después de BUZZER_AO_POST_TIMEOUT_MS
     * @return ERR_INVALID_STATE si AO no está iniciado
     *
     * @note NO BLOQUEANTE: retorna inmediatamente (< 10μs)
     * @note Thread-safe: puede llamarse desde múltiples contextos
     * @note ISR-safe: puede llamarse desde ISR si timeout = 0
     *
     * @code
     * BuzzerCommand_t cmd = {
     *     .tone = BUZZER_TONE_STANDARD,
     *     .duration_ms = 500,
     *     .priority = 5
     * };
     * Result_t res = BuzzerAO_PostCommand(&buzzer_ao, &cmd);
     * @endcode
     */
    Result_t BuzzerAO_PostCommand(BuzzerAO_t *ao, const BuzzerCommand_t *command);

    /**
     * @brief Posta comando de parada inmediata (no bloqueante)
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return ERR_OK si comando encolado
     *
     * @note Detiene beep activo y limpia cola de comandos pendientes
     * @note Útil para silenciar alarmas manualmente
     * @note ISR-safe
     */
    Result_t BuzzerAO_PostStop(BuzzerAO_t *ao);

    /**
     * @brief Obtiene estado actual del Active Object
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return Estado actual (IDLE/PLAYING)
     *
     * @note Thread-safe: lectura atómica
     * @note ISR-safe
     */
    BuzzerAO_State_t BuzzerAO_GetState(const BuzzerAO_t *ao);

    /**
     * @brief Verifica si el Active Object está reproduciendo
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return true si en estado PLAYING, false si IDLE
     *
     * @note Shortcut para BuzzerAO_GetState() == PLAYING
     */
    bool BuzzerAO_IsPlaying(const BuzzerAO_t *ao);

    /**
     * @brief Obtiene cantidad de comandos pendientes en cola
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return Número de comandos en cola (0 si vacía)
     *
     * @note Útil para diagnosticar saturación de cola
     * @note Thread-safe
     */
    uint32_t BuzzerAO_GetQueueDepth(const BuzzerAO_t *ao);

    /**
     * @brief De-inicializa el Active Object y libera recursos
     *
     * @param[in] ao Puntero a BuzzerAO
     * @return ERR_OK si exitoso
     *
     * @note Detiene thread si estaba corriendo
     * @note Libera memoria de cola y stack
     * @note Debe llamarse en shutdown del sistema
     */
    Result_t BuzzerAO_Deinit(BuzzerAO_t *ao);

    /*============================================================================*
     * HELPER FUNCTIONS (Convenience API)
     *============================================================================*/

    /**
     * @brief Helper: Beep estándar con prioridad media
     */
    static inline Result_t BuzzerAO_Beep(BuzzerAO_t *ao, uint16_t duration_ms)
    {
        BuzzerCommand_t cmd = {
            .tone = BUZZER_TONE_STANDARD,
            .duration_ms = duration_ms,
            .priority = 5 // Media
        };
        return BuzzerAO_PostCommand(ao, &cmd);
    }

    /**
     * @brief Helper: Beep de error con alta prioridad
     */
    static inline Result_t BuzzerAO_BeepError(BuzzerAO_t *ao)
    {
        BuzzerCommand_t cmd = {
            .tone = BUZZER_TONE_ERROR,
            .duration_ms = 1000,
            .priority = 10 // Alta
        };
        return BuzzerAO_PostCommand(ao, &cmd);
    }

    /**
     * @brief Helper: Beep de advertencia
     */
    static inline Result_t BuzzerAO_BeepWarning(BuzzerAO_t *ao, uint16_t duration_ms)
    {
        BuzzerCommand_t cmd = {
            .tone = BUZZER_TONE_WARNING,
            .duration_ms = duration_ms,
            .priority = 8 // Alta-media
        };
        return BuzzerAO_PostCommand(ao, &cmd);
    }

    /**
     * @brief Helper: Beep de éxito
     */
    static inline Result_t BuzzerAO_BeepSuccess(BuzzerAO_t *ao)
    {
        BuzzerCommand_t cmd = {
            .tone = BUZZER_TONE_SUCCESS,
            .duration_ms = 500,
            .priority = 3 // Baja
        };
        return BuzzerAO_PostCommand(ao, &cmd);
    }

    /*============================================================================*
     * IAudibleNotifier IMPLEMENTATION
     *============================================================================*/

    /**
     * @brief Obtiene la interfaz IAudibleNotifier del BuzzerAO
     *
     * @param[in] ao Puntero a BuzzerAO inicializado
     * @return Puntero a interfaz IAudibleNotifier (domain layer)
     * @return NULL si ao es NULL
     *
     * @note Esta función conecta el Application layer (BuzzerAO) con el Domain layer
     * @note Sigue el patrón Adapter: BuzzerAO adapta IBuzzerControl → IAudibleNotifier
     * @note La interfaz retornada es válida mientras ao exista
     *
     * @code
     * BuzzerAO_t buzzer_ao;
     * BuzzerAO_Init(&buzzer_ao, &config);
     *
     * IAudibleNotifier *notifier = BuzzerAO_GetAudibleNotifierInterface(&buzzer_ao);
     * BuzzerNotificationService_Init(&service, notifier, &svc_config);
     * @endcode
     */
    IAudibleNotifier *BuzzerAO_GetAudibleNotifierInterface(BuzzerAO_t *ao);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_AO_H */
