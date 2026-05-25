/**
 * @file lv_freertos_osal.c
 * @brief Implementación del OSAL custom LVGL → FreeRTOS.
 *
 * Implementa las funciones requeridas por LVGL cuando LV_USE_OS == LV_OS_CUSTOM.
 * Solo este archivo y lv_freertos_osal.h conocen los tipos internos FreeRTOS.
 *
 * ## Funciones implementadas
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
 * xSemaphoreTakeFromISR no funciona con mutexes recursivos en FreeRTOS.
 * Esta función retorna LV_RESULT_INVALID. Esto es seguro siempre que
 * lv_display_flush_ready() se llame desde contexto de hilo (patrón
 * flush_wait_cb en ui_port_disp.c).
 *
 * ## Nota sobre lv_thread_sync
 *
 * Se implementa con Task Notifications de FreeRTOS (más ligeras que semáforos
 * y completamente ISR-seguras). El handle del task que llama wait() se
 * almacena en xTaskToNotify para que signal() pueda notificarlo. Las
 * notificaciones se acumulan en el TCB del task, por lo que las señales
 * emitidas antes del wait() no se pierden.
 *
 * ## Prioridades de thread
 *
 * lv_thread_init mapea la prioridad abstracta LVGL a prioridades FreeRTOS
 * (valor mayor = mayor prioridad en FreeRTOS, al revés que ThreadX).
 *
 * @author  Tecna Smart Lab
 * @date    25 de Mayo 2026
 */

/* lv_os_private.h provee: lv_thread_prio_t, declaraciones de funciones,
 * y hace #include LV_OS_CUSTOM_INCLUDE → lv_freertos_osal.h (tipos FreeRTOS). */
#include "src/osal/lv_os_private.h" /* path relativo a lvgl/ */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string.h>

/* ============================================================================
 * HELPERS INTERNOS
 * ========================================================================== */

/** Prioridad FreeRTOS para hilos creados por LVGL (features opcionales). */
#define LV_FREERTOS_DEFAULT_PRIORITY (tskIDLE_PRIORITY + 3U)

/** Stack size mínimo (words) para tareas LVGL opcionales. */
#define LV_FREERTOS_MIN_STACK_WORDS (configMINIMAL_STACK_SIZE)

/**
 * @brief Wrapper de entrada para hilos LVGL.
 *
 * FreeRTOS task entry recibe void*; LVGL usa un puntero a lv_thread_t.
 */
static void thread_entry_wrapper(void *arg)
{
    lv_thread_t *thread = (lv_thread_t *)arg;
    thread->entry(thread->arg);
    /* Tarea FreeRTOS no debe retornar; se auto-elimina. */
    vTaskDelete(NULL);
}

/* ============================================================================
 * MUTEX — recursive semaphore (re-entrante, NO desde ISR)
 * ========================================================================== */

/**
 * @brief Inicializa un mutex LVGL (mutex recursivo FreeRTOS).
 */
lv_result_t lv_mutex_init(lv_mutex_t *mutex)
{
    if (mutex == NULL)
        return LV_RESULT_INVALID;

    mutex->xHandle = xSemaphoreCreateRecursiveMutex();
    if (mutex->xHandle == NULL)
        return LV_RESULT_INVALID;

    mutex->initialized = true;
    return LV_RESULT_OK;
}

/**
 * @brief Adquiere el mutex desde contexto de hilo (bloqueante).
 *
 * Usa toma recursiva: el mismo hilo puede adquirir el mutex más de una vez.
 */
lv_result_t lv_mutex_lock(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    if (xSemaphoreTakeRecursive(mutex->xHandle, portMAX_DELAY) == pdTRUE)
        return LV_RESULT_OK;

    return LV_RESULT_INVALID;
}

/**
 * @brief Intento de adquisición desde ISR.
 *
 * xSemaphoreTakeFromISR no funciona con mutexes recursivos en FreeRTOS.
 * Se retorna LV_RESULT_INVALID para señalizar a LVGL que no puede adquirir
 * el lock desde ISR. Esto es seguro siempre que lv_display_flush_ready()
 * no se llame desde ISR (ver ui_port_disp.c, patrón flush_wait_cb).
 *
 * @return  LV_RESULT_INVALID siempre (FreeRTOS no admite mutex recursivo en ISR).
 */
lv_result_t lv_mutex_lock_isr(lv_mutex_t *mutex)
{
    (void)mutex;
    /* Mutex recursivo no admite uso desde ISR en FreeRTOS.
     * Si LVGL llega aquí desde un ISR real, es un error de diseño:
     * verificar que lv_display_flush_ready() se llama desde hilo. */
    return LV_RESULT_INVALID;
}

/**
 * @brief Libera el mutex.
 */
lv_result_t lv_mutex_unlock(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    if (xSemaphoreGiveRecursive(mutex->xHandle) == pdTRUE)
        return LV_RESULT_OK;

    return LV_RESULT_INVALID;
}

/**
 * @brief Destruye el mutex.
 */
lv_result_t lv_mutex_delete(lv_mutex_t *mutex)
{
    if (mutex == NULL || !mutex->initialized)
        return LV_RESULT_INVALID;

    vSemaphoreDelete(mutex->xHandle);
    mutex->xHandle = NULL;
    mutex->initialized = false;
    return LV_RESULT_OK;
}

/* ============================================================================
 * THREAD SYNC — Task Notifications (condition variable simplificado)
 * ========================================================================== */

/**
 * @brief Inicializa el objeto de sincronización.
 *
 * Solo marca el objeto como inicializado; el task handle se llena
 * en lv_thread_sync_wait() cuando el task se registra para esperar.
 */
lv_result_t lv_thread_sync_init(lv_thread_sync_t *sync)
{
    if (sync == NULL)
        return LV_RESULT_INVALID;

    sync->xTaskToNotify = NULL;
    sync->initialized = true;
    return LV_RESULT_OK;
}

/**
 * @brief Bloquea el hilo actual hasta recibir una señal.
 *
 * Registra el task handle actual en xTaskToNotify y espera con
 * ulTaskNotifyTake. Si ya había una notificación pendiente (señal
 * enviada antes del wait), retorna inmediatamente sin bloquear.
 *
 * @note  No es seguro llamar wait() desde dos hilos distintos sobre el
 *        mismo sync object (undefined behavior en task notifications).
 */
lv_result_t lv_thread_sync_wait(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    /* Registrar este task como el receptor de la señal. */
    sync->xTaskToNotify = xTaskGetCurrentTaskHandle();

    /* Bloquear hasta que llegue una notificación (pdTRUE = auto-clear). */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    return LV_RESULT_OK;
}

/**
 * @brief Envía señal desde contexto de hilo.
 *
 * Si no hay ningún task esperando (xTaskToNotify == NULL), la notificación
 * se envía de todas formas: quedará acumulada en el TCB del task y el
 * próximo wait() la consumirá sin bloquearse.
 */
lv_result_t lv_thread_sync_signal(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    if (sync->xTaskToNotify != NULL)
    {
        xTaskNotifyGive(sync->xTaskToNotify);
    }

    return LV_RESULT_OK;
}

/**
 * @brief Envía señal desde ISR.
 *
 * vTaskNotifyGiveFromISR es ISR-seguro en FreeRTOS: no bloquea y hace
 * portYIELD_FROM_ISR si corresponde despertar un task de mayor prioridad.
 */
lv_result_t lv_thread_sync_signal_isr(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    if (sync->xTaskToNotify != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(sync->xTaskToNotify, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return LV_RESULT_OK;
}

/**
 * @brief Destruye el objeto de sincronización.
 */
lv_result_t lv_thread_sync_delete(lv_thread_sync_t *sync)
{
    if (sync == NULL || !sync->initialized)
        return LV_RESULT_INVALID;

    sync->xTaskToNotify = NULL;
    sync->initialized = false;
    return LV_RESULT_OK;
}

/* ============================================================================
 * THREADS — FreeRTOS Tasks
 * ========================================================================== */

/**
 * @brief Crea un hilo LVGL usando xTaskCreate de FreeRTOS.
 *
 * En la arquitectura estándar LVGL no llama esta función (el hilo GUI es
 * ui_ao_thread_entry). Está implementada para features opcionales (renderizado
 * paralelo, etc.) por si se habilitan en el futuro.
 *
 * @note  El stack se asigna dinámicamente desde el heap FreeRTOS (pvPortMalloc).
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

    /* Mapear prioridad LVGL a FreeRTOS (valor mayor = mayor prioridad). */
    static const UBaseType_t prio_map[] = {
        [LV_THREAD_PRIO_LOWEST] = tskIDLE_PRIORITY + 1U,
        [LV_THREAD_PRIO_LOW] = tskIDLE_PRIORITY + 2U,
        [LV_THREAD_PRIO_MID] = LV_FREERTOS_DEFAULT_PRIORITY,
        [LV_THREAD_PRIO_HIGH] = tskIDLE_PRIORITY + 5U,
        [LV_THREAD_PRIO_HIGHEST] = tskIDLE_PRIORITY + 7U,
    };
    UBaseType_t ux_prio = (prio <= LV_THREAD_PRIO_HIGHEST)
                              ? prio_map[prio]
                              : prio_map[LV_THREAD_PRIO_HIGHEST];

    thread->entry = callback;
    thread->arg = user_data;

    /* stack_size en bytes → words FreeRTOS */
    uint32_t stack_words = (uint32_t)(stack_size / sizeof(StackType_t));
    if (stack_words < LV_FREERTOS_MIN_STACK_WORDS)
        stack_words = LV_FREERTOS_MIN_STACK_WORDS;

    BaseType_t ret = xTaskCreate(
        thread_entry_wrapper,
        (name != NULL) ? name : "lv_thread",
        (uint16_t)stack_words,
        (void *)thread,
        ux_prio,
        &thread->xHandle);

    if (ret != pdPASS)
    {
        thread->xHandle = NULL;
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

    if (thread->xHandle != NULL)
    {
        vTaskDelete(thread->xHandle);
        thread->xHandle = NULL;
    }

    return LV_RESULT_OK;
}

/* ============================================================================
 * SLEEP y ESTADÍSTICAS
 * ========================================================================== */

/**
 * @brief Duerme el hilo actual N milisegundos usando vTaskDelay.
 */
void lv_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/**
 * @brief Retorna el porcentaje de idle (no disponible sin hook FreeRTOS).
 *
 * FreeRTOS no expone idle% sin instrumentar configUSE_IDLE_HOOK o
 * traceTASK_SWITCHED_IN/OUT. Se retorna 0 para no bloquear dashboards
 * de sysmon LVGL.
 *
 * @return  0 (estadística no disponible).
 */
uint32_t lv_os_get_idle_percent(void)
{
    return 0U;
}
