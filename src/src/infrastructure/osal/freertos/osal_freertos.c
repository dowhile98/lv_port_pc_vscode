/**
 * @file osal_freertos.c
 * @brief OSAL implementation for FreeRTOS.
 *
 * Maps the portable OSAL API (osal.h) to FreeRTOS primitives.
 * All OS objects are allocated dynamically via pvPortMalloc / FreeRTOS
 * internal allocators; no static buffers are required from callers.
 */

#include "infrastructure/osal/osal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"
#include "timers.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ========================================================================== *
 * Internal helper: wrapper stored as timer ID so the FreeRTOS callback can   *
 * forward to the OSAL callback with its user argument.                       *
 * ========================================================================== */
typedef struct
{
    os_timer_callback_t callback;
    void *arg;
} timer_wrapper_t;

static void s_timer_callback(TimerHandle_t xTimer)
{
    timer_wrapper_t *w = (timer_wrapper_t *)pvTimerGetTimerID(xTimer);
    if (w != NULL && w->callback != NULL)
    {
        w->callback(w->arg);
    }
}

/* ========================================================================== *
 * Memory                                                                      *
 * ========================================================================== */

Result_t os_init(void *memory_ptr)
{
    (void)memory_ptr; /* FreeRTOS manages its heap internally */
    return ERR_OK;
}

Result_t os_alloc(uint32_t size, void **out_ptr)
{
    if (out_ptr == NULL)
        return ERR_NULL_POINTER;
    if (size == 0U)
        return ERR_INVALID_PARAM;

    *out_ptr = pvPortMalloc((size_t)size);
    return (*out_ptr != NULL) ? ERR_OK : ERR_NO_MEM;
}

void *os_calloc(size_t num, size_t size)
{
    if (size != 0U && num > (SIZE_MAX / size))
        return NULL;

    size_t total = num * size;
    void *ptr = pvPortMalloc(total);
    if (ptr != NULL)
    {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *os_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
        return pvPortMalloc(new_size);

    if (new_size == 0U)
    {
        vPortFree(ptr);
        return NULL;
    }

    void *new_ptr = pvPortMalloc(new_size);
    if (new_ptr != NULL)
    {
        memcpy(new_ptr, ptr, new_size); /* Safe only when new_size <= original */
        vPortFree(ptr);
    }
    return new_ptr;
}

Result_t os_free(void *ptr)
{
    if (ptr == NULL)
        return ERR_NULL_POINTER;

    vPortFree(ptr);
    return ERR_OK;
}

/* ========================================================================== *
 * Tick helpers                                                                *
 * ========================================================================== */

uint32_t os_ms_to_ticks(uint32_t ms)
{
    return (uint32_t)pdMS_TO_TICKS(ms);
}

uint32_t os_ticks_get(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/* ========================================================================== *
 * Threads                                                                     *
 * ========================================================================== */

Result_t os_thread_create(os_thread_t *thread, const os_thread_config_t *config)
{
    if (thread == NULL || config == NULL || config->entry == NULL)
        return ERR_NULL_POINTER;

    /* Convert byte stack size to words as required by xTaskCreate */
    uint32_t stack_words = config->stack_size / sizeof(StackType_t);
    if (stack_words == 0U)
        stack_words = (uint32_t)configMINIMAL_STACK_SIZE;

    TaskHandle_t handle = NULL;
    BaseType_t ret = xTaskCreate(
        (TaskFunction_t)config->entry,
        (config->name != NULL) ? config->name : "task",
        (configSTACK_DEPTH_TYPE)stack_words,
        config->arg,
        (UBaseType_t)config->priority,
        &handle);

    if (ret != pdPASS || handle == NULL)
        return ERR_NO_MEM;

    /* FreeRTOS starts tasks immediately; suspend if caller does not want auto-start */
    if (!config->auto_start)
        vTaskSuspend(handle);

    *thread = (os_thread_t)handle;
    return ERR_OK;
}

Result_t os_thread_terminate(os_thread_t thread)
{
    if (thread == NULL)
        return ERR_NULL_POINTER;

    vTaskDelete((TaskHandle_t)thread);
    return ERR_OK;
}

Result_t os_thread_suspend(os_thread_t thread)
{
    if (thread == NULL)
        return ERR_NULL_POINTER;

    vTaskSuspend((TaskHandle_t)thread);
    return ERR_OK;
}

Result_t os_thread_resume(os_thread_t thread)
{
    if (thread == NULL)
        return ERR_NULL_POINTER;

    vTaskResume((TaskHandle_t)thread);
    return ERR_OK;
}

Result_t os_thread_sleep(uint32_t ms)
{
    vTaskDelay((TickType_t)pdMS_TO_TICKS(ms));
    return ERR_OK;
}

/* ========================================================================== *
 * Mutex (recursive, maps to xSemaphoreCreateMutex)                           *
 * ========================================================================== */

Result_t os_mutex_create(os_mutex_t *mutex, const char *name)
{
    (void)name; /* FreeRTOS mutex has no name */
    if (mutex == NULL)
        return ERR_NULL_POINTER;

    SemaphoreHandle_t h = xSemaphoreCreateMutex();
    if (h == NULL)
        return ERR_NO_MEM;

    *mutex = (os_mutex_t)h;
    return ERR_OK;
}

Result_t os_mutex_acquire(os_mutex_t mutex, uint32_t timeout_ms)
{
    if (mutex == NULL)
        return ERR_NULL_POINTER;

    TickType_t ticks = (timeout_ms == OS_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : (TickType_t)pdMS_TO_TICKS(timeout_ms);

    return (xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) == pdTRUE)
               ? ERR_OK
               : ERR_TIMEOUT;
}

Result_t os_mutex_release(os_mutex_t mutex)
{
    if (mutex == NULL)
        return ERR_NULL_POINTER;

    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdTRUE) ? ERR_OK : ERR_ERROR;
}

Result_t os_mutex_delete(os_mutex_t mutex)
{
    if (mutex == NULL)
        return ERR_NULL_POINTER;

    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return ERR_OK;
}

/* ========================================================================== *
 * Counting Semaphores                                                         *
 * ========================================================================== */

Result_t os_semaphore_create(os_semaphore_t *sem, const char *name, uint32_t initial_count)
{
    (void)name;
    if (sem == NULL)
        return ERR_NULL_POINTER;

    /* Max count set to a large value; adjust if bounded semaphore is required */
    SemaphoreHandle_t h = xSemaphoreCreateCounting(
        (UBaseType_t)0xFFFFFFFFUL, (UBaseType_t)initial_count);

    if (h == NULL)
        return ERR_NO_MEM;

    *sem = (os_semaphore_t)h;
    return ERR_OK;
}

Result_t os_semaphore_get(os_semaphore_t sem, uint32_t timeout_ms)
{
    if (sem == NULL)
        return ERR_NULL_POINTER;

    TickType_t ticks = (timeout_ms == OS_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : (TickType_t)pdMS_TO_TICKS(timeout_ms);

    return (xSemaphoreTake((SemaphoreHandle_t)sem, ticks) == pdTRUE) ? ERR_OK : ERR_TIMEOUT;
}

Result_t os_semaphore_put(os_semaphore_t sem)
{
    if (sem == NULL)
        return ERR_NULL_POINTER;

    return (xSemaphoreGive((SemaphoreHandle_t)sem) == pdTRUE) ? ERR_OK : ERR_ERROR;
}

Result_t os_semaphore_delete(os_semaphore_t sem)
{
    if (sem == NULL)
        return ERR_NULL_POINTER;

    vSemaphoreDelete((SemaphoreHandle_t)sem);
    return ERR_OK;
}

/* ========================================================================== *
 * Event Flags                                                                 *
 *                                                                             *
 * OS_FLAGS_* → FreeRTOS xEventGroupWaitBits parameters:                      *
 *   OS_FLAGS_AND_CLEAR (0x00): xWaitForAll=true,  xClearOnExit=true          *
 *   OS_FLAGS_AND       (0x01): xWaitForAll=true,  xClearOnExit=false         *
 *   OS_FLAGS_OR        (0x02): xWaitForAll=false, xClearOnExit=false         *
 *   OS_FLAGS_OR_CLEAR  (0x03): xWaitForAll=false, xClearOnExit=true          *
 * ========================================================================== */

Result_t os_event_flags_create(os_event_flags_t *flags, const char *name)
{
    (void)name;
    if (flags == NULL)
        return ERR_NULL_POINTER;

    EventGroupHandle_t h = xEventGroupCreate();
    if (h == NULL)
        return ERR_NO_MEM;

    *flags = (os_event_flags_t)h;
    return ERR_OK;
}

Result_t os_event_flags_delete(os_event_flags_t flags)
{
    if (flags == NULL)
        return ERR_NULL_POINTER;

    vEventGroupDelete((EventGroupHandle_t)flags);
    return ERR_OK;
}

Result_t os_event_flags_set(os_event_flags_t flags, uint32_t mask, uint32_t option)
{
    (void)option; /* FreeRTOS only supports OR-set; AND-set has no equivalent */
    if (flags == NULL)
        return ERR_NULL_POINTER;

    xEventGroupSetBits((EventGroupHandle_t)flags, (EventBits_t)mask);
    return ERR_OK;
}

Result_t os_event_flags_get(os_event_flags_t flags,
                            uint32_t mask,
                            uint32_t option,
                            uint32_t *actual_flags,
                            uint32_t timeout_ms)
{
    if (flags == NULL || actual_flags == NULL)
        return ERR_NULL_POINTER;

    /* bit1 of option: 0=AND, 1=OR */
    BaseType_t wait_for_all = ((option & 0x02U) == 0U) ? pdTRUE : pdFALSE;
    /* clear if option is OR_CLEAR (0x03) or AND_CLEAR (0x00) */
    BaseType_t clear_on_exit =
        ((option == OS_FLAGS_OR_CLEAR) || (option == OS_FLAGS_AND_CLEAR)) ? pdTRUE : pdFALSE;

    TickType_t ticks = (timeout_ms == OS_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : (TickType_t)pdMS_TO_TICKS(timeout_ms);

    EventBits_t bits = xEventGroupWaitBits(
        (EventGroupHandle_t)flags,
        (EventBits_t)mask,
        clear_on_exit,
        wait_for_all,
        ticks);

    *actual_flags = (uint32_t)bits;

    if (wait_for_all == pdTRUE)
        return ((bits & (EventBits_t)mask) == (EventBits_t)mask) ? ERR_OK : ERR_TIMEOUT;
    else
        return ((bits & (EventBits_t)mask) != 0U) ? ERR_OK : ERR_TIMEOUT;
}

/* ========================================================================== *
 * Message Queues                                                              *
 *                                                                             *
 * FreeRTOS xQueueCreate allocates both the queue structure and its buffer    *
 * internally; config->buffer is ignored on FreeRTOS.                         *
 * ========================================================================== */

Result_t os_queue_create(os_queue_t *queue, const os_queue_config_t *config)
{
    if (queue == NULL || config == NULL)
        return ERR_NULL_POINTER;
    if (config->item_size == 0U || config->buffer_size == 0U)
        return ERR_INVALID_PARAM;

    UBaseType_t length = (UBaseType_t)(config->buffer_size / config->item_size);
    if (length == 0U)
        return ERR_INVALID_PARAM;

    QueueHandle_t h = xQueueCreate(length, (UBaseType_t)config->item_size);
    if (h == NULL)
        return ERR_NO_MEM;

    *queue = (os_queue_t)h;
    return ERR_OK;
}

Result_t os_queue_delete(os_queue_t queue)
{
    if (queue == NULL)
        return ERR_NULL_POINTER;

    vQueueDelete((QueueHandle_t)queue);
    return ERR_OK;
}

Result_t os_queue_send(os_queue_t queue, const void *item, uint32_t timeout_ms)
{
    if (queue == NULL || item == NULL)
        return ERR_NULL_POINTER;

    TickType_t ticks = (timeout_ms == OS_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : (TickType_t)pdMS_TO_TICKS(timeout_ms);

    BaseType_t ret = xQueueSend((QueueHandle_t)queue, item, ticks);
    return (ret == pdTRUE) ? ERR_OK : ERR_TIMEOUT;
}

Result_t os_queue_receive(os_queue_t queue, void *item, uint32_t timeout_ms)
{
    if (queue == NULL || item == NULL)
        return ERR_NULL_POINTER;

    TickType_t ticks = (timeout_ms == OS_WAIT_FOREVER)
                           ? portMAX_DELAY
                           : (TickType_t)pdMS_TO_TICKS(timeout_ms);

    BaseType_t ret = xQueueReceive((QueueHandle_t)queue, item, ticks);
    return (ret == pdTRUE) ? ERR_OK : ERR_TIMEOUT;
}

/* ========================================================================== *
 * Software Timers                                                             *
 *                                                                             *
 * A timer_wrapper_t is heap-allocated and stored as the timer ID so that     *
 * s_timer_callback can forward the call with the correct user argument.       *
 * The wrapper is freed in os_timer_delete.                                   *
 * ========================================================================== */

Result_t os_timer_create(os_timer_t *timer, const os_timer_config_t *config)
{
    if (timer == NULL || config == NULL || config->callback == NULL)
        return ERR_NULL_POINTER;

    timer_wrapper_t *w = (timer_wrapper_t *)pvPortMalloc(sizeof(timer_wrapper_t));
    if (w == NULL)
        return ERR_NO_MEM;

    w->callback = config->callback;
    w->arg = config->arg;

    UBaseType_t auto_reload = (config->reschedule_ticks > 0U) ? pdTRUE : pdFALSE;
    /* FreeRTOS requires period > 0 */
    TickType_t period = (config->initial_ticks > 0U)
                            ? (TickType_t)config->initial_ticks
                            : ((config->reschedule_ticks > 0U)
                                   ? (TickType_t)config->reschedule_ticks
                                   : 1U);

    TimerHandle_t h = xTimerCreate(
        (config->name != NULL) ? config->name : "timer",
        period,
        auto_reload,
        (void *)w,
        s_timer_callback);

    if (h == NULL)
    {
        vPortFree(w);
        return ERR_NO_MEM;
    }

    *timer = (os_timer_t)h;
    return ERR_OK;
}

Result_t os_timer_delete(os_timer_t timer)
{
    if (timer == NULL)
        return ERR_NULL_POINTER;

    /* Free the wrapper before deleting the timer */
    timer_wrapper_t *w = (timer_wrapper_t *)pvTimerGetTimerID((TimerHandle_t)timer);
    BaseType_t ret = xTimerDelete((TimerHandle_t)timer, portMAX_DELAY);
    if (w != NULL)
        vPortFree(w);

    return (ret == pdTRUE) ? ERR_OK : ERR_ERROR;
}

Result_t os_timer_activate(os_timer_t timer)
{
    if (timer == NULL)
        return ERR_NULL_POINTER;

    return (xTimerStart((TimerHandle_t)timer, portMAX_DELAY) == pdTRUE) ? ERR_OK : ERR_ERROR;
}

Result_t os_timer_deactivate(os_timer_t timer)
{
    if (timer == NULL)
        return ERR_NULL_POINTER;

    return (xTimerStop((TimerHandle_t)timer, portMAX_DELAY) == pdTRUE) ? ERR_OK : ERR_ERROR;
}

Result_t os_timer_change(os_timer_t timer, uint32_t initial_ticks, uint32_t reschedule_ticks)
{
    if (timer == NULL)
        return ERR_NULL_POINTER;

    (void)reschedule_ticks; /* auto_reload is fixed at creation time */
    TickType_t period = (initial_ticks > 0U) ? (TickType_t)initial_ticks : 1U;

    return (xTimerChangePeriod((TimerHandle_t)timer, period, portMAX_DELAY) == pdTRUE)
               ? ERR_OK
               : ERR_ERROR;
}
