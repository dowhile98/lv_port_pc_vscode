/**
 * @file lv_threadx_osal.h
 * @brief Tipos para el bridge LVGL OS custom → Azure RTOS ThreadX.
 *
 * Este header es el único punto donde LVGL ve thread/mutex/sync types.
 * Sólo debe incluirse desde LVGL internals (vía LV_OS_CUSTOM_INCLUDE) y
 * desde lv_threadx_osal.c. NUNCA desde capas Application/Domain.
 *
 * ## Mapeo de primitivas
 *
 * | LVGL              | ThreadX         | Nota                               |
 * |-------------------|-----------------|------------------------------------|
 * | lv_mutex_t        | TX_MUTEX        | Re-entrante. NO usar desde ISR.    |
 * | lv_thread_sync_t  | TX_SEMAPHORE    | Contador binario. ISR-seguro (put).|
 * | lv_thread_t       | TX_THREAD       | Solo para features opcionales LVGL.|
 *
 * @note  lv_mutex_lock_isr() retorna LV_RESULT_INVALID porque TX_MUTEX no
 *        admite uso desde ISR. La solución arquitectural es garantizar que
 *        lv_display_flush_ready() siempre se llame desde contexto de hilo
 *        (patrón flush_wait_cb en ui_port_disp.c).
 *
 * @author  Tecna Smart Lab
 * @date    24 de Febrero 2026
 */

#ifndef LV_THREADX_OSAL_H
#define LV_THREADX_OSAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* tx_api.h permitido aquí: este archivo vive en la capa Infrastructure.
 * La capa Application/Domain NUNCA incluye este header directamente. */
#include "tx_api.h"
#include <stdbool.h>
#include <stdint.h>

    /* ============================================================================
     * lv_mutex_t — TX_MUTEX embebido (sin malloc)
     * ========================================================================== */

    /**
     * @brief Mutex LVGL respaldado por TX_MUTEX.
     *
     * TX_MUTEX soporta adquisición re-entrante (TX_INHERIT opcional), adecuado
     * para el general_mutex de LVGL que se toma/suelta en el mismo hilo.
     */
    typedef struct
    {
        TX_MUTEX handle;  /**< Objeto TX_MUTEX embebido (sin malloc). */
        bool initialized; /**< true después de tx_mutex_create exitoso.  */
    } lv_mutex_t;

    /* ============================================================================
     * lv_thread_sync_t — TX_SEMAPHORE binario (ISR-seguro para put)
     * ========================================================================== */

    /**
     * @brief Objeto de sincronización LVGL (condition variable simplificado).
     *
     * Implementado como semáforo binario:
     *  - lv_thread_sync_wait()       → tx_semaphore_get(TX_WAIT_FOREVER)
     *  - lv_thread_sync_signal()     → tx_semaphore_put()       [hilo]
     *  - lv_thread_sync_signal_isr() → tx_semaphore_put()       [ISR — seguro]
     *
     * Si la señal llega antes del wait, el semáforo queda en 1 y el siguiente
     * wait retorna inmediatamente (sin señal perdida).
     */
    typedef struct
    {
        TX_SEMAPHORE handle; /**< Objeto TX_SEMAPHORE embebido.         */
        bool initialized;    /**< true después de tx_semaphore_create.  */
    } lv_thread_sync_t;

    /* ============================================================================
     * lv_thread_t — TX_THREAD embebido
     * ========================================================================== */

    /**
     * @brief Hilo LVGL respaldado por TX_THREAD.
     *
     * En la arquitectura estándar de este proyecto, LVGL NO crea sus propios
     * hilos (ui_ao_thread_entry es el único hilo GUI). Esta struct existe para
     * compatibilidad con features opcionales de LVGL (ej. renderizado paralelo).
     * El stack se asigna desde el byte pool de ThreadX (memoria estática/determinista).
     */
    typedef struct
    {
        TX_THREAD handle;      /**< Objeto TX_THREAD embebido.                  */
        void (*entry)(void *); /**< Función de entrada del hilo.                */
        void *arg;             /**< Argumento pasado al hilo.                   */
        void *stack_ptr;       /**< Puntero al stack asignado desde byte pool.  */
    } lv_thread_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_THREADX_OSAL_H */
