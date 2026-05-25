/**
 * @file osal.h
 * @brief Interfaz genérica de abstracción del Sistema Operativo (OSAL).
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Esta interfaz desacopla la aplicación del RTOS específico (ThreadX, FreeRTOS, etc).
 * No contiene dependencias de headers de proveedores ni de logs.
 *
 * **Nota v2.0:** Los tipos de concurrencia (os_mutex_t, etc.) ahora están definidos
 * en hal/hal_types.h para permitir uso en Domain layer sin violar Clean Architecture.
 *
 * @see hal/hal_types.h para definiciones de tipos portables
 */

#ifndef OS_PORT_H
#define OS_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h" /* ✅ Tipos portables (Result_t, os_mutex_t, etc.) */
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /* ===== Definiciones de Tiempo ===== */

#ifndef OS_WAIT_FOREVER
#define OS_WAIT_FOREVER 0xFFFFFFFFUL
#endif

#ifndef OS_NO_WAIT
#define OS_NO_WAIT 0x00000000UL
#endif

    /**
     * @brief Convierte milisegundos a ticks del sistema.
     */
    uint32_t os_ms_to_ticks(uint32_t ms);

    /**
     * @brief Obtiene el tick actual del sistema (contador de scheduler).
     */
    uint32_t os_ticks_get(void);

    /* ===== Inicialización y Memoria ===== */

    /**
     * @brief Inicializa el OSAL con un pool de memoria del RTOS.
     * @param[in] memory_ptr Puntero genérico al pool (ThreadX: TX_BYTE_POOL*).
     * @return ERR_OK en éxito, ERR_NULL_POINTER si memory_ptr es NULL.
     */
    Result_t os_init(void *memory_ptr);

    /**
     * @brief Asigna memoria desde el pool del OSAL.
     * @param[in]  size    Tamaño en bytes.
     * @param[out] out_ptr Puntero a memoria asignada.
     * @return ERR_OK en éxito, código de error en fallo.
     */
    Result_t os_alloc(uint32_t size, void **out_ptr);

    void *os_calloc(size_t num, size_t size);

    void *os_realloc(void *ptr, size_t new_size);
    /**
     * @brief Libera memoria al pool del OSAL.
     * @param[in] ptr Puntero a memoria previamente asignada.
     * @return ERR_OK en éxito, código de error en fallo.
     */
    Result_t os_free(void *ptr);

    /* ===== Tipos de Objetos del SO ===== */

    /**
     * @note Los tipos os_*_t ahora están definidos en hal/hal_types.h (v2.0)
     * @note Esto permite que Domain layer use os_mutex_t sin incluir osal.h
     * @see hal/hal_types.h
     */

    /* ===== Gestión de Hilos (Threads) ===== */

    typedef void (*os_thread_entry_t)(void *arg);

    typedef struct
    {
        const char *name;        /**< Nombre del hilo (opcional). */
        os_thread_entry_t entry; /**< Función de entrada del hilo. */
        void *arg;               /**< Argumento de usuario. */
        void *stack_ptr;         /**< Memoria de stack provista por el caller (no se usa malloc). */
        uint32_t stack_size;     /**< Tamaño del stack en bytes. */
        uint32_t priority;       /**< Prioridad relativa del hilo. */
        bool auto_start;         /**< true: inicia al crear, false: queda en suspenso. */
    } os_thread_config_t;

    /**
     * @brief Crea un thread RTOS.
     * @param[in,out] thread Handle a llenar. Caller debe inicializar en NULL; esta función lo asignará.
     * @param[in]     config Configuración del thread.
     * @return ERR_OK en éxito, código de error en fallo.
     * @note El ownership del handle interno lo mantiene el RTOS; el caller no debe liberar memoria.
     */
    Result_t os_thread_create(os_thread_t *thread, const os_thread_config_t *config);
    Result_t os_thread_terminate(os_thread_t thread);
    Result_t os_thread_sleep(uint32_t ms);
    Result_t os_thread_suspend(os_thread_t thread);
    Result_t os_thread_resume(os_thread_t thread);

    /* ===== Mutex ===== */

    Result_t os_mutex_create(os_mutex_t *mutex, const char *name);
    Result_t os_mutex_delete(os_mutex_t mutex);
    Result_t os_mutex_acquire(os_mutex_t mutex, uint32_t timeout_ms);
    Result_t os_mutex_release(os_mutex_t mutex);

    /* ===== Semáforos ===== */

    Result_t os_semaphore_create(os_semaphore_t *sem, const char *name, uint32_t initial_count);
    Result_t os_semaphore_delete(os_semaphore_t sem);
    Result_t os_semaphore_put(os_semaphore_t sem);
    Result_t os_semaphore_get(os_semaphore_t sem, uint32_t timeout_ms);

/* ===== Event Flags ===== */

/**
 * @brief Portable flag option constants for os_event_flags_set / os_event_flags_get.
 *
 * Maps directly to ThreadX TX_OR / TX_OR_CLEAR / TX_AND / TX_AND_CLEAR.
 * Use these instead of TX_* constants to keep callers RTOS-independent.
 */
#define OS_FLAGS_OR 0x02U        /**< Set/get with OR  (do not clear bits after get). */
#define OS_FLAGS_OR_CLEAR 0x03U  /**< Get with OR_CLEAR (clear matched bits after get). */
#define OS_FLAGS_AND 0x01U       /**< Get with AND (all requested bits must be set). */
#define OS_FLAGS_AND_CLEAR 0x00U /**< Get with AND_CLEAR (clear all if all requested are set). */

    Result_t os_event_flags_create(os_event_flags_t *flags, const char *name);
    Result_t os_event_flags_delete(os_event_flags_t flags);
    Result_t os_event_flags_set(os_event_flags_t flags, uint32_t mask, uint32_t option);
    Result_t os_event_flags_get(os_event_flags_t flags, uint32_t mask, uint32_t option, uint32_t *actual_flags, uint32_t timeout_ms);

    /* ===== Queues (mensajes) ===== */

    typedef struct
    {
        const char *name;     /**< Nombre opcional. */
        void *buffer;         /**< Buffer provisto por el caller. Debe ser múltiplo de sizeof(uint32_t). */
        uint32_t buffer_size; /**< Tamaño del buffer en bytes. */
        uint32_t item_size;   /**< Tamaño de cada mensaje en bytes (múltiplo de 4). */
    } os_queue_config_t;

    /**
     * @brief Crea una queue RTOS.
     * @param[in,out] queue  Handle a llenar. Caller debe inicializar en NULL; esta función lo asignará.
     * @param[in]     config Configuración de la queue.
     * @return ERR_OK en éxito, código de error en fallo.
     * @note El ownership del handle interno lo mantiene el RTOS; el caller no debe liberar memoria.
     */
    Result_t os_queue_create(os_queue_t *queue, const os_queue_config_t *config);
    Result_t os_queue_delete(os_queue_t queue);
    Result_t os_queue_send(os_queue_t queue, const void *item, uint32_t timeout_ms);
    Result_t os_queue_receive(os_queue_t queue, void *item, uint32_t timeout_ms);

    /* ===== Timers ===== */

    typedef void (*os_timer_callback_t)(void *arg);

    typedef struct
    {
        const char *name;             /**< Nombre del timer (opcional). */
        os_timer_callback_t callback; /**< Función callback al expirar. */
        void *arg;                    /**< Argumento de usuario para el callback. */
        uint32_t initial_ticks;       /**< Ticks iniciales (0 = no auto-start). */
        uint32_t reschedule_ticks;    /**< Ticks de recarga (0 = one-shot). */
    } os_timer_config_t;

    /**
     * @brief Crea un timer de software.
     * @param[in,out] timer  Handle a llenar. Caller debe inicializar en NULL.
     * @param[in]     config Configuración del timer.
     * @return ERR_OK en éxito, código de error en fallo.
     */
    Result_t os_timer_create(os_timer_t *timer, const os_timer_config_t *config);
    Result_t os_timer_delete(os_timer_t timer);
    Result_t os_timer_activate(os_timer_t timer);
    Result_t os_timer_deactivate(os_timer_t timer);
    Result_t os_timer_change(os_timer_t timer, uint32_t initial_ticks, uint32_t reschedule_ticks);

#ifdef __cplusplus
}
#endif

#endif /* OS_PORT_H */
