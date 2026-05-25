/**
 * @file lv_threadx_osal.c
 * @brief Implementación del OSAL custom LVGL → Azure RTOS ThreadX.
 *
 * Implementa las 13 funciones requeridas por LVGL cuando LV_USE_OS == LV_OS_CUSTOM.
 * Solo este archivo y lv_threadx_osal.h conocen los tipos internos ThreadX.
 *
 * ## Funciones requeridas por LVGL (lv_os_private.h)
 *
 *  Mutex:       lv_mutex_init / lv_mutex_lock / lv_mutex_lock_isr
 *               lv_mutex_unlock / lv_mutex_delete
 *  Thread:      lv_thread_init / lv_thread_delete
 *  Sync:        lv_thread_sync_init / lv_thread_sync_wait
 *               lv_thread_sync_signal / lv_thread_sync_signal_isr
 *               lv_thread_sync_delete
 *  Sleep:       lv_sleep_ms
 *  Idle stats:  lv_os_get_idle_percent
 *
 * ## Nota sobre lv_mutex_lock_isr()
 *
 * TX_MUTEX no se puede usar desde ISR. Esta función retorna LV_RESULT_INVALID.
 * La condición de funcionamiento correcto es que lv_display_flush_ready() NUNCA
 * se llame desde un ISR → asegurado por el patrón flush_wait_cb en ui_port_disp.c.
 *
 * @author  Tecna Smart Lab
 * @date    24 de Febrero 2026
 */

/* lv_os_private.h provee: lv_thread_prio_t, declaraciones de funciones,
 * y hace #include LV_OS_CUSTOM_INCLUDE → lv_threadx_osal.h (tipos TX).
 * No hay inclusión circular: lv_threadx_osal.h NO incluye lv_os_private.h. */
#include "src/osal/lv_os_private.h" /* path relativo a Third_Party/lvgl/ */

#include "infrastructure/osal/osal.h"
#include "tx_api.h"
#include <string.h>

/* ============================================================================
 * HELPERS INTERNOS
 * ========================================================================== */

/** Prioridad ThreadX para hilos creados por LVGL (features opcionales). */
#define LV_THREADX_DEFAULT_PRIORITY (10U)

/**
 * @brief Wrapper de entrada para hilos LVGL.
 *
 * ThreadX pasa un ULONG como argumento; LVGL usa void*.
 */
static void thread_entry_wrapper(ULONG arg)
{
    lv_thread_t *thread = (lv_thread_t *)(uintptr_t)arg;
    thread->entry(thread->arg);
}

/* ============================================================================
 * MUTEX
 * ========================================================================== */

/**
 * @brief Inicializa un mutex LVGL (TX_MUTEX).
 */
lv_result_t lv_mutex_init(lv_mutex_t *mutex)
{
    if (mutex == NULL)
        return LV_RESULT_INVALID;

    UINT status = tx_mutex_create(&mutex->handle, (CHAR *)"lv_mutex", TX_NO_INHERIT);
    if (status != TX_SUCCESS)
        return LV_RESULT_INVALID;

    mutex->initialized = true;
    return LV_RESULT_OK;
}

/**
 * @brief Adquiere el mutex desde contexto de hilo (bloqueante).
 */
lv_result_t lv_mutex_lock(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_mutex_get(&mutex->handle, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/**
 * @brief Intento de adquisición desde ISR.
 *
 * TX_MUTEX no es ISR-seguro. Se retorna LV_RESULT_INVALID para señalizar
 * a LVGL que no puede adquirir el lock desde ISR. Esto es seguro siempre
 * que lv_display_flush_ready() no se llame desde ISR (ver ui_port_disp.c).
 *
 * @return  LV_RESULT_INVALID siempre (ThreadX no admite mutex en ISR).
 */
lv_result_t lv_mutex_lock_isr(lv_mutex_t *mutex)
{
    (void)mutex;
    /* TX_MUTEX no admite uso desde ISR.
     * Si LVGL llega aquí desde un ISR real, es un error de diseño:
     * verificar que lv_display_flush_ready() se llama desde hilo (flush_wait_cb). */
    return LV_RESULT_INVALID;
}

/**
 * @brief Libera el mutex.
 */
lv_result_t lv_mutex_unlock(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_mutex_put(&mutex->handle);
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/**
 * @brief Destruye el mutex.
 */
lv_result_t lv_mutex_delete(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_mutex_delete(&mutex->handle);
    mutex->initialized = false;
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/* ============================================================================
 * THREAD SYNC — semáforo binario (condition variable simplificado)
 * ========================================================================== */

/**
 * @brief Inicializa el objeto de sincronización (semáforo cuenta 0).
 */
lv_result_t lv_thread_sync_init(lv_thread_sync_t *sync)
{
    if (sync == NULL)
        return LV_RESULT_INVALID;

    /* Semáforo inicial en 0: el wait bloqueará hasta que llegue una señal. */
    UINT status = tx_semaphore_create(&sync->handle, (CHAR *)"lv_sync", 0U);
    if (status != TX_SUCCESS)
        return LV_RESULT_INVALID;

    sync->initialized = true;
    return LV_RESULT_OK;
}

/**
 * @brief Bloquea el hilo hasta recibir una señal.
 *
 * Si ya había una señal pendiente (semáforo > 0), retorna inmediatamente.
 * Así no se pierden señales que lleguen antes del wait.
 */
lv_result_t lv_thread_sync_wait(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_semaphore_get(&sync->handle, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/**
 * @brief Envía señal desde contexto de hilo.
 */
lv_result_t lv_thread_sync_signal(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_semaphore_put(&sync->handle);
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/**
 * @brief Envía señal desde ISR.
 *
 * tx_semaphore_put() es ISR-seguro en Azure RTOS ThreadX (no bloquea,
 * no accede a estructuras de kernel que requieran scheduler suspendido).
 */
lv_result_t lv_thread_sync_signal_isr(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    /* ISR-safe en ThreadX: tx_semaphore_put no bloquea. */
    UINT status = tx_semaphore_put(&sync->handle);
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/**
 * @brief Destruye el objeto de sincronización.
 */
lv_result_t lv_thread_sync_delete(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    UINT status = tx_semaphore_delete(&sync->handle);
    sync->initialized = false;
    return (status == TX_SUCCESS) ? LV_RESULT_OK : LV_RESULT_INVALID;
}

/* ============================================================================
 * THREADS
 * ========================================================================== */

/**
 * @brief Crea un hilo LVGL usando el byte pool ThreadX.
 *
 * En la arquitectura estándar LVGL no llama esta función (el hilo GUI es
 * UiAO_t). Está implementada para features opcionales (renderizado paralelo,
 * etc.) por si se habilitan en el futuro.
 *
 * @note  El stack se asigna desde el byte pool de ThreadX (determinista).
 *        La prioridad es fija (LV_THREADX_DEFAULT_PRIORITY).
 */
lv_result_t lv_thread_init(lv_thread_t *thread,
                           const char *const name,
                           lv_thread_prio_t prio,
                           void (*callback)(void *),
                           size_t stack_size,
                           void *user_data)
{
    if (thread == NULL || callback == NULL || stack_size == 0U)
        return LV_RESULT_INVALID;

    /* Mapear prioridad LVGL a ThreadX (valor menor = mayor prioridad en TX). */
    static const UINT prio_map[] = {
        [LV_THREAD_PRIO_LOWEST] = 20U,
        [LV_THREAD_PRIO_LOW] = 16U,
        [LV_THREAD_PRIO_MID] = LV_THREADX_DEFAULT_PRIORITY,
        [LV_THREAD_PRIO_HIGH] = 6U,
        [LV_THREAD_PRIO_HIGHEST] = 2U,
    };
    UINT tx_prio = (prio < LV_THREAD_PRIO_HIGHEST) ? prio_map[prio] : prio_map[LV_THREAD_PRIO_HIGHEST];

    /* Asignar stack desde byte pool (equivale a memoria estática determinista). */
    Result_t res = os_alloc((uint32_t)stack_size, &thread->stack_ptr);
    if (res != ERR_OK)
        return LV_RESULT_INVALID;

    thread->entry = callback;
    thread->arg = user_data;

    UINT status = tx_thread_create(
        &thread->handle,
        (CHAR *)name,
        thread_entry_wrapper,
        (ULONG)(uintptr_t)thread,
        thread->stack_ptr,
        (ULONG)stack_size,
        tx_prio,
        tx_prio,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        (void)os_free(thread->stack_ptr);
        thread->stack_ptr = NULL;
        return LV_RESULT_INVALID;
    }

    return LV_RESULT_OK;
}

/**
 * @brief Termina y destruye un hilo LVGL.
 */
lv_result_t lv_thread_delete(lv_thread_t *thread)
{
    if (thread == NULL)
        return LV_RESULT_INVALID;

    (void)tx_thread_terminate(&thread->handle);
    (void)tx_thread_delete(&thread->handle);

    if (thread->stack_ptr != NULL)
    {
        (void)os_free(thread->stack_ptr);
        thread->stack_ptr = NULL;
    }

    return LV_RESULT_OK;
}

/* ============================================================================
 * SLEEP y ESTADÍSTICAS
 * ========================================================================== */

/**
 * @brief Duerme el hilo actual N milisegundos.
 */
void lv_sleep_ms(uint32_t ms)
{
    (void)os_thread_sleep(ms);
}

/**
 * @brief Retorna el porcentaje de idle (no disponible sin hook ThreadX).
 *
 * ThreadX no expone idle% sin instrumentar el idle thread.
 * Se retorna 0 para no bloquear dashboards de sysmon LVGL.
 *
 * @return  0 (estadística no disponible).
 */
uint32_t lv_os_get_idle_percent(void)
{
    return 0U;
}
