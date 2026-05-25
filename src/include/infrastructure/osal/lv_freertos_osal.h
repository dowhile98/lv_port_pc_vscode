/**
 * @file lv_freertos_osal.h
 * @brief Tipos para el bridge LVGL OS custom → FreeRTOS.
 *
 * Este header es el único punto donde LVGL ve los tipos de concurrencia
 * FreeRTOS.  Se activa vía:
 *
 *   lv_conf.h:
 *     #define LV_USE_OS          LV_OS_CUSTOM
 *     #define LV_OS_CUSTOM_INCLUDE <infrastructure/osal/lv_freertos_osal.h>
 *
 * Sólo debe incluirse desde LVGL internals (vía LV_OS_CUSTOM_INCLUDE) y
 * desde lv_freertos_osal.c. NUNCA desde capas Application/Domain.
 *
 * ## Mapeo de primitivas
 *
 * | LVGL              | FreeRTOS                    | Nota                          |
 * |-------------------|-----------------------------|-------------------------------|
 * | lv_mutex_t        | SemaphoreHandle_t (recursive)| Re-entrante. NO usar desde ISR.|
 * | lv_thread_sync_t  | Task Notification           | ISR-seguro (FromISR variant). |
 * | lv_thread_t       | TaskHandle_t                | Solo para features opcionales.|
 *
 * ## Nota sobre lv_mutex_lock_isr()
 *
 * xSemaphoreTakeFromISR no funciona con mutexes recursivos en FreeRTOS.
 * Esta función retorna LV_RESULT_INVALID. La condición de funcionamiento
 * correcto es que lv_display_flush_ready() NUNCA se llame desde ISR →
 * asegurado por el patrón flush_wait_cb en ui_port_disp.c.
 *
 * @author  Tecna Smart Lab
 * @date    25 de Mayo 2026
 */

#ifndef LV_FREERTOS_OSAL_H
#define LV_FREERTOS_OSAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* FreeRTOS headers: permitidos aquí, este archivo vive en Infrastructure.
 * Application/Domain NUNCA incluye este header directamente. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdbool.h>
#include <stddef.h>

    /* ============================================================================
     * lv_mutex_t — Recursive mutex (SemaphoreHandle_t)
     * ========================================================================== */

    /**
     * @brief Mutex LVGL respaldado por mutex recursivo FreeRTOS.
     *
     * Se usa mutex recursivo para que el general_mutex de LVGL pueda ser tomado
     * desde el mismo hilo más de una vez sin deadlock.
     */
    typedef struct
    {
        SemaphoreHandle_t xHandle; /**< Handle del mutex recursivo FreeRTOS.    */
        bool initialized;          /**< true después de xSemaphoreCreateRecursiveMutex exitoso. */
    } lv_mutex_t;

    /* ============================================================================
     * lv_thread_sync_t — Task Notification (condition variable simplificado)
     * ========================================================================== */

    /**
     * @brief Objeto de sincronización LVGL usando Task Notifications FreeRTOS.
     *
     * Las Task Notifications son más ligeras que los semáforos y son ISR-seguras:
     *  - lv_thread_sync_wait()       → ulTaskNotifyTake(pdTRUE, portMAX_DELAY)
     *  - lv_thread_sync_signal()     → xTaskNotifyGive()        [hilo]
     *  - lv_thread_sync_signal_isr() → vTaskNotifyGiveFromISR() [ISR — seguro]
     *
     * Si la señal llega antes del wait, la notificación queda pendiente en el
     * TCB del task y el siguiente wait retorna de inmediato (sin señal perdida).
     */
    typedef struct
    {
        volatile TaskHandle_t xTaskToNotify; /**< Task esperando la señal; NULL si no hay. */
        bool initialized;                    /**< true después de lv_thread_sync_init. */
    } lv_thread_sync_t;

    /* ============================================================================
     * lv_thread_t — FreeRTOS Task
     * ========================================================================== */

    /**
     * @brief Hilo LVGL respaldado por FreeRTOS Task.
     *
     * En la arquitectura estándar LVGL no crea sus propios hilos (ui_ao_thread_entry
     * es el único hilo GUI). Esta struct existe para compatibilidad con features
     * opcionales de LVGL (ej. renderizado paralelo).
     */
    typedef struct
    {
        TaskHandle_t xHandle;  /**< Handle del task FreeRTOS.              */
        void (*entry)(void *); /**< Función de entrada del hilo.           */
        void *arg;             /**< Argumento pasado al hilo.              */
    } lv_thread_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_FREERTOS_OSAL_H */
