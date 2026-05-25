/**
 * @file digital_input_ao.h
 * @brief Active Object para polling periódico de entradas digitales.
 *
 * Ejecuta DigitalInputSource_Process() en thread dedicado cada poll_interval_ms.
 * Los callbacks registrados en el adapter ejecutan en contexto de este thread.
 *
 * @note Patrón Active Object:
 *       - Encapsula concurrencia (thread + polling loop)
 *       - Garantiza que Process() ejecuta en un solo thread (no race conditions)
 *       - Callbacks son síncronos (no queue interna, dispatch directo)
 *
 * @note Comparación con RelayAO:
 *       - RelayAO: Usa queue para eventos async (PPS desde ISR)
 *       - DigitalInputAO: Polling síncrono (no necesita queue, callbacks directos)
 */

#ifndef DIGITAL_INPUT_AO_H
#define DIGITAL_INPUT_AO_H

#include "infrastructure/osal/osal.h"
#include "interfaces/i_digital_input_source.h"
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"

/* ===== Configuración del Active Object ===== */

/**
 * @brief Configuración de inicialización del DigitalInputAO.
 */
typedef struct
{
    IDigitalInputSource *input_source; /**< Adapter (inyectado, no NULL) */
    uint32_t poll_interval_ms;         /**< Intervalo de polling en ms (típico: 20ms) */
    uint8_t priority;                  /**< Prioridad del thread (0-255, típico: 10) */
    void *stack_ptr;                   /**< Buffer de stack (no NULL) */
    uint32_t stack_size;               /**< Tamaño de stack en bytes (típico: 1024) */
    ILogger *logger;                   /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier;    /**< Event notifier para eventos (EEPROM) */
} DigitalInputAO_Config_t;

/* ===== Estructura del Active Object ===== */

/**
 * @brief Instancia del DigitalInputAO.
 */
typedef struct
{
    os_thread_t thread;                /**< Handle del thread (OSAL) */
    os_semaphore_t stopped_sem;        /**< Señalizado al salir del inner loop (WaitStopped) y outer loop (Deinit) */
    os_semaphore_t run_sem;            /**< Gate del outer loop: dado por Start(), tomado por el hilo */
    IDigitalInputSource *input_source; /**< Interfaz del adapter */
    uint32_t poll_interval_ms;         /**< Intervalo de polling */
    ILogger *logger;                   /**< Logger para debug (UART/SWO) */
    IEventNotifier *event_notifier;    /**< Event notifier para eventos (EEPROM) */
    volatile bool running;             /**< Flag de ejecución del inner loop */
    volatile bool terminate;           /**< Flag de salida del outer loop (Deinit) */
    bool is_initialized;               /**< Flag de inicialización */
} DigitalInputAO_t;

/* ===== API Pública ===== */

/**
 * @brief Inicializa y arranca el Active Object.
 *
 * @pre config != NULL, config->input_source != NULL
 * @pre Adapter ya inicializado con DigitalInputAdapter_Init()
 *
 * @param[in,out] self   Instancia a inicializar (no NULL).
 * @param[in]     config Configuración (no NULL).
 *
 * @return ERR_OK si thread creado y corriendo.
 * @return ERR_NULL_POINTER si algún parámetro es NULL.
 * @return ERR_ERROR si falla creación de thread.
 *
 * @note Thread arranca automáticamente (auto_start = true en os_thread_create).
 * @note Polling loop infinito:
 *       while (is_running) {
 *           DigitalInputSource_Process(input_source);
 *           os_thread_sleep_ms(poll_interval_ms);
 *       }
 */
Result_t DigitalInputAO_Init(DigitalInputAO_t *self, const DigitalInputAO_Config_t *config);

/**
 * @brief Inicia el thread del Active Object (lifecycle pattern)
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 */
Result_t DigitalInputAO_Start(DigitalInputAO_t *self);

/**
 * @brief Detiene el Active Object.
 * @note Establece is_running = false. Thread termina en siguiente iteración.
 * @warning No espera a que thread termine. Usar con precaución.
 */
Result_t DigitalInputAO_Stop(DigitalInputAO_t *self);

/**
 * @brief Espera a que el thread confirme que el inner loop terminó.
 * @param[in] self       Puntero al Active Object.
 * @param[in] timeout_ms Timeout en ms (OS_WAIT_FOREVER para espera infinita).
 * @return ERR_OK si el thread se detuvo; ERR_TIMEOUT si expiró.
 */
Result_t DigitalInputAO_WaitStopped(DigitalInputAO_t *self, uint32_t timeout_ms);

/**
 * @brief De-inicializa Active Object y libera recursos.
 * @note Llama Stop() + WaitStopped() internamente si el AO está corriendo.
 * @param[in] self Puntero al Active Object
 * @return ERR_OK si exitoso
 */
Result_t DigitalInputAO_Deinit(DigitalInputAO_t *self);

#endif /* DIGITAL_INPUT_AO_H */
