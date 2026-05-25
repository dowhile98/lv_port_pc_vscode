/**
 * @file eventlog_ao.c
 * @brief EventLog Active Object implementation (Pattern B - Message-Driven).
 *
 * @details
 * Pattern B implementation from ACTIVE_OBJECT_ARCHITECTURE.md:
 * - Queue is the gate (NO run_sem)
 * - Outer loop blocks on MSG_START
 * - Inner loop processes application messages (MSG_APPEND, MSG_CLEAR)
 * - stopped_sem signals Stop() completion
 *
 * @note Thread-safe: Appends serialized via queue, Reads protected by mutexes
 * @note No malloc: Static allocation for all resources
 */

#include "application/activeobjects/eventlog_ao.h"
#include "infrastructure/osal/osal.h"
#include "infrastructure/storage/eeprom_layout.h"
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════
 *  Forward Declarations
 * ══════════════════════════════════════════════════════════════════════════ */

static void eventlog_ao_thread_entry(void *ctx);
static void dispatch_message(EventLogAO_t *self, const EventLogMsg_t *msg);
static void notify_subscribers(EventLogAO_t *self, const EventLogEntry_t *event);
static Result_t load_metadata_from_eeprom(EventLogAO_t *self);
static Result_t save_metadata_to_eeprom(EventLogAO_t *self);
static Result_t write_event_to_eeprom(EventLogAO_t *self, uint16_t index, const EventLogEntry_t *event);
static Result_t read_event_from_eeprom(EventLogAO_t *self, uint16_t index, EventLogEntry_t *out);
static uint16_t calculate_metadata_checksum(uint16_t head, uint16_t count);

/* ── V-Table Wrappers ─────────────────────────────────────────────────── */
static Result_t vtable_Append(void *self, const EventLogEntry_t *event);
static Result_t vtable_ReadByIndex(void *self, uint16_t index, EventLogEntry_t *out_event);
static Result_t vtable_GetCount(void *self, uint16_t *out_count);
static Result_t vtable_GetMetadata(void *self, EventLogMeta_t *out_meta);
static Result_t vtable_Clear(void *self);
/* ── Static V-Table Instance (const) ───────────────────────────────────────── */
static const IEventLogStorage_Vtable s_eventlog_vtable = {
    .Append = vtable_Append,
    .ReadByIndex = vtable_ReadByIndex,
    .GetCount = vtable_GetCount,
    .GetMetadata = vtable_GetMetadata,
    .Clear = vtable_Clear,
};

static const char *TAG = "AO_EVT";
/* ══════════════════════════════════════════════════════════════════════════
 *  Lifecycle Functions (Init, Start, Stop, WaitStopped, Deinit)
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t EventLogAO_Init(EventLogAO_t *self, const EventLogAOConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->eeprom == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (self->initialized)
    {
        return ERR_ERROR; /* Already initialized */
    }

    /* Zero out struct */
    memset(self, 0, sizeof(EventLogAO_t));

    /* Store configuration */
    self->eeprom = config->eeprom;
    self->eeprom_handle = config->eeprom_handle;
    self->logger = config->logger;

    /* ── Create OSAL resources ───────────────────────────────────────────── */

    /* Create stopped semaphore (binary, initial count = 0) */
    Result_t res = os_semaphore_create(&self->stopped_sem, "EventLogAO_stopped", 0);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Create metadata mutex */
    res = os_mutex_create(&self->metadata_mutex, "EventLogAO_meta");
    if (res != ERR_OK)
    {
        os_semaphore_delete(self->stopped_sem);
        return ERR_ERROR;
    }

    /* Create cache mutex */
    res = os_mutex_create(&self->cache_mutex, "EventLogAO_cache");
    if (res != ERR_OK)
    {
        os_mutex_delete(self->metadata_mutex);
        os_semaphore_delete(self->stopped_sem);
        return ERR_ERROR;
    }

    /* Create message queue */
    os_queue_config_t queue_cfg = {
        .name = "EventLogAO_queue",
        .buffer = self->queue_buffer,
        .buffer_size = sizeof(self->queue_buffer),
        .item_size = sizeof(EventLogMsg_t),
    };
    res = os_queue_create(&self->queue, &queue_cfg);
    if (res != ERR_OK)
    {
        os_mutex_delete(self->cache_mutex);
        os_mutex_delete(self->metadata_mutex);
        os_semaphore_delete(self->stopped_sem);
        return ERR_ERROR;
    }

    /* Create worker thread */
    os_thread_config_t thread_cfg = {
        .name = "EventLogAO",
        .entry = eventlog_ao_thread_entry,
        .arg = self,
        .stack_ptr = self->stack,
        .stack_size = EVENTLOG_AO_STACK_SIZE,
        .priority = EVENTLOG_AO_PRIORITY,
        .auto_start = true, /* Thread starts immediately, blocks on queue */
    };

    res = os_thread_create(&self->thread, &thread_cfg);
    if (res != ERR_OK)
    {
        os_queue_delete(self->queue);
        os_mutex_delete(self->cache_mutex);
        os_mutex_delete(self->metadata_mutex);
        os_semaphore_delete(self->stopped_sem);
        return ERR_ERROR;
    }

    /* ── Load metadata from EEPROM ───────────────────────────────────────── */
    res = load_metadata_from_eeprom(self);
    if (res != ERR_OK)
    {
        /* If metadata corrupted, reset to defaults */
        LOG_WARN(self->logger, TAG, "Event log metadata corrupted or invalid, resetting to defaults");
        self->metadata.head = 0;
        self->metadata.count = 0;
        self->metadata.checksum = 0;
        save_metadata_to_eeprom(self); /* Write defaults */
        LOG_INFO(self->logger, TAG, "Event log metadata reset: head=%u, count=%u",
                 self->metadata.head, self->metadata.count);
    }
    else
    {
        LOG_INFO(self->logger, TAG, "Event log metadata loaded: head=%u, count=%u, checksum=0x%04X",
                 self->metadata.head, self->metadata.count, self->metadata.checksum);
    }

    /* ── Initialize cache (all invalid) ──────────────────────────────────── */
    memset(self->cache_valid, false, sizeof(self->cache_valid));
    memset(self->cache_index, 0xFF, sizeof(self->cache_index)); /* 0xFFFF = invalid index */
    /* ── Initialize subscribers ──────────────────────────────────────────── */
    for (uint8_t i = 0; i < EVENT_LOG_SUBSCRIBERS_MAX; i++)
    {
        self->subscribers[i].callback = NULL;
        self->subscribers[i].context = NULL;
    }

    /* ── Initialize V-Table interface ────────────────────────────────────── */
    self->interface.vtable = &s_eventlog_vtable;
    self->interface.impl = self;

    self->initialized = true;
    self->running = false;

    return ERR_OK;
}

Result_t EventLogAO_Start(EventLogAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR; /* Not initialized */
    }

    if (self->running)
    {
        return ERR_OK; /* Already running */
    }

    self->running = true;

    /* Post MSG_START to wake outer loop */
    EventLogMsg_t msg = {
        .type = EVENTLOG_MSG_START,
    };

    Result_t res = os_queue_send(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        self->running = false;
        return ERR_ERROR;
    }

    return ERR_OK;
}

Result_t EventLogAO_Stop(EventLogAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    if (!self->running)
    {
        return ERR_OK; /* Already stopped */
    }

    self->running = false;

    /* Post MSG_STOP to break inner loop */
    EventLogMsg_t msg = {
        .type = EVENTLOG_MSG_STOP,
    };

    Result_t res = os_queue_send(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    return ERR_OK;
}

Result_t EventLogAO_WaitStopped(EventLogAO_t *self, uint32_t timeout_ms)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    /* Wait for stopped_sem (signaled by worker thread on inner loop exit) */
    Result_t res = os_semaphore_get(self->stopped_sem, timeout_ms);
    if (res != ERR_OK)
    {
        return ERR_TIMEOUT;
    }

    return ERR_OK;
}

Result_t EventLogAO_Deinit(EventLogAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    if (self->running)
    {
        return ERR_BUSY; /* Must call Stop() + WaitStopped() first */
    }

    /* Post MSG_TERMINATE to break outer loop */
    EventLogMsg_t msg = {
        .type = EVENTLOG_MSG_TERMINATE,
    };

    Result_t res = os_queue_send(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Wait for thread to exit outer loop */
    res = os_semaphore_get(self->stopped_sem, 5000); /* 5 second timeout */
    if (res != ERR_OK)
    {
        return ERR_TIMEOUT;
    }

    /* Destroy OSAL resources */
    os_thread_terminate(self->thread);
    os_queue_delete(self->queue);
    os_mutex_delete(self->cache_mutex);
    os_mutex_delete(self->metadata_mutex);
    os_semaphore_delete(self->stopped_sem);

    self->initialized = false;

    return ERR_OK;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Write Operations (Non-Blocking)
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t EventLogAO_Append(EventLogAO_t *self, const EventLogEntry_t *event)
{
    if (self == NULL || event == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized || !self->running)
    {
        return ERR_ERROR;
    }

    /* Copy event to message (value semantics) */
    EventLogMsg_t msg = {
        .type = EVENTLOG_MSG_APPEND,
        .event = *event,
    };

    /* Post to queue (non-blocking) */
    Result_t res = os_queue_send(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return ERR_BUSY; /* Queue full */
    }

    return ERR_OK;
}

Result_t EventLogAO_Clear(EventLogAO_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized || !self->running)
    {
        return ERR_ERROR;
    }

    /* Post MSG_CLEAR to queue */
    EventLogMsg_t msg = {
        .type = EVENTLOG_MSG_CLEAR,
    };

    Result_t res = os_queue_send(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return ERR_BUSY;
    }

    return ERR_OK;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Read Operations (Mutex-Protected)
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t EventLogAO_GetCount(EventLogAO_t *self, uint16_t *out_count)
{
    if (self == NULL || out_count == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    /* Lock metadata mutex */
    Result_t res = os_mutex_acquire(self->metadata_mutex, OS_WAIT_FOREVER);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    *out_count = self->metadata.count;

    os_mutex_release(self->metadata_mutex);

    return ERR_OK;
}

Result_t EventLogAO_GetMetadata(EventLogAO_t *self, EventLogMeta_t *out_meta)
{
    if (self == NULL || out_meta == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    /* Lock metadata mutex */
    Result_t res = os_mutex_acquire(self->metadata_mutex, OS_WAIT_FOREVER);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    *out_meta = self->metadata;

    os_mutex_release(self->metadata_mutex);

    return ERR_OK;
}

Result_t EventLogAO_ReadByIndex(EventLogAO_t *self, uint16_t index, EventLogEntry_t *out)
{
    if (self == NULL || out == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    /* Validate index */
    uint16_t count;
    Result_t res = EventLogAO_GetCount(self, &count);
    if (res != ERR_OK)
    {
        return res;
    }

    if (index >= count)
    {
        return ERR_INVALID_PARAM;
    }

    /* ── Phase 3.5: Hybrid Cache ─────────────────────────────────────────── */
    /* Check cache first */
    uint16_t cache_slot = index % EVENT_LOG_CACHE_SIZE;

    res = os_mutex_acquire(self->cache_mutex, OS_WAIT_FOREVER);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Cache hit: instant read (1μs) - verify exact index match */
    if (self->cache_valid[cache_slot] && self->cache_index[cache_slot] == index)
    {
        *out = self->cache[cache_slot];
        os_mutex_release(self->cache_mutex);
        return ERR_OK;
    }

    os_mutex_release(self->cache_mutex);

    /* Cache miss: Read from EEPROM (blocking 15ms) */
    res = read_event_from_eeprom(self, index, out);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Populate cache on miss */
    res = os_mutex_acquire(self->cache_mutex, OS_WAIT_FOREVER);
    if (res == ERR_OK)
    {
        cache_slot = index % EVENT_LOG_CACHE_SIZE;
        self->cache[cache_slot] = *out;
        self->cache_valid[cache_slot] = true;
        self->cache_index[cache_slot] = index;
        os_mutex_release(self->cache_mutex);
    }

    return ERR_OK;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Observer Pattern (Subscribe/Unsubscribe)
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t EventLogAO_Subscribe(EventLogAO_t *self, EventLogCallback_t callback, void *context)
{
    if (self == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->initialized)
    {
        return ERR_ERROR;
    }

    /* Find free slot */
    for (uint8_t i = 0; i < EVENT_LOG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback == NULL)
        {
            self->subscribers[i].callback = callback;
            self->subscribers[i].context = context;
            return ERR_OK;
        }
    }

    return ERR_ERROR; /* No free slots */
}

Result_t EventLogAO_Unsubscribe(EventLogAO_t *self, EventLogCallback_t callback)
{
    if (self == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Find and clear subscription */
    for (uint8_t i = 0; i < EVENT_LOG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback == callback)
        {
            self->subscribers[i].callback = NULL;
            self->subscribers[i].context = NULL;
            return ERR_OK;
        }
    }

    return ERR_ERROR; /* Not found */
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Interface Exposure (DI Container)
 * ══════════════════════════════════════════════════════════════════════════ */

IEventLogStorage *EventLogAO_GetInterface(EventLogAO_t *self)
{
    if (self == NULL || !self->initialized)
    {
        return NULL;
    }

    return &self->interface;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  V-Table Wrappers (Type-Safe Casting)
 * ══════════════════════════════════════════════════════════════════════════ */

static Result_t vtable_Append(void *self, const EventLogEntry_t *event)
{
    return EventLogAO_Append((EventLogAO_t *)self, event);
}

static Result_t vtable_ReadByIndex(void *self, uint16_t index, EventLogEntry_t *out_event)
{
    return EventLogAO_ReadByIndex((EventLogAO_t *)self, index, out_event);
}

static Result_t vtable_GetCount(void *self, uint16_t *out_count)
{
    return EventLogAO_GetCount((EventLogAO_t *)self, out_count);
}

static Result_t vtable_GetMetadata(void *self, EventLogMeta_t *out_meta)
{
    return EventLogAO_GetMetadata((EventLogAO_t *)self, out_meta);
}

static Result_t vtable_Clear(void *self)
{
    return EventLogAO_Clear((EventLogAO_t *)self);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Worker Thread (Pattern B: Outer/Inner Loop)
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Worker thread entry point (Pattern B canonical implementation).
 *
 * @details
 * Outer loop: Blocks on queue waiting for MSG_START (gate mechanism, NO run_sem).
 * Inner loop: Processes application messages (MSG_APPEND, MSG_CLEAR).
 *
 * Lifecycle:
 * 1. Thread created in Init() → blocks on queue in outer loop
 * 2. Start() posts MSG_START → outer loop wakes → enters inner loop
 * 3. Stop() posts MSG_STOP → inner loop breaks → signals stopped_sem
 * 4. Outer loop waits for next MSG_START (reusable)
 * 5. Deinit() posts MSG_TERMINATE → outer loop breaks → thread exits
 *
 * @param[in] ctx EventLogAO_t* context
 */
static void eventlog_ao_thread_entry(void *ctx)
{
    EventLogAO_t *self = (EventLogAO_t *)ctx;
    EventLogMsg_t msg;
    bool should_exit = false;

    LOG_INFO(self->logger, TAG, "Worker thread started, waiting for MSG_START...");
    /* ── Outer loop: Wait for MSG_START (queue is gate, NO run_sem) ────── */
    while (!should_exit)
    {
        /* Block indefinitely on queue */
        if (os_queue_receive(self->queue, &msg, OS_WAIT_FOREVER) != ERR_OK)
        {
            continue; /* Spurious wakeup */
        }

        /* Handle control messages */
        if (msg.type == EVENTLOG_MSG_TERMINATE)
        {
            LOG_INFO(self->logger, TAG, "MSG_TERMINATE received, exiting thread...");
            break; /* Exit outer loop → thread exits */
        }

        if (msg.type != EVENTLOG_MSG_START)
        {
            LOG_INFO(self->logger, TAG, "Received non-START message while waiting for START, ignoring...");
            continue; /* Discard non-START messages before started */
        }

        /* MSG_START received → enter inner loop */

        /* ── Inner loop: Process application messages ────────────────────── */
        while (self->running)
        {
            if (os_queue_receive(self->queue, &msg, OS_WAIT_FOREVER) != ERR_OK)
            {
                continue;
            }

            /* Handle control messages */
            if (msg.type == EVENTLOG_MSG_STOP)
            {
                LOG_INFO(self->logger, TAG, "MSG_STOP received, stopping inner loop...");
                break; /* Exit inner loop */
            }

            if (msg.type == EVENTLOG_MSG_TERMINATE)
            {
                LOG_INFO(self->logger, TAG, "MSG_TERMINATE received, exiting thread...");
                should_exit = true;
                break; /* Exit inner loop + outer loop */
            }

            /* Dispatch application messages */
            dispatch_message(self, &msg);
        }

        /* Inner loop exited → stopped */
        self->running = false;

        if (!should_exit)
        {
            /* Signal WaitStopped() that inner loop has stopped */

            os_semaphore_put(self->stopped_sem);
            /* Return to outer loop → wait for next MSG_START */
        }
    }

    /* Outer loop exited → signal Deinit() that thread is terminating */
    os_semaphore_put(self->stopped_sem);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Message Dispatcher (Application Messages)
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Dispatch application messages (MSG_APPEND, MSG_CLEAR).
 *
 * @details
 * Executed in worker thread context.
 *
 * MSG_APPEND (20ms total):
 *   1. Write event to EEPROM (~10ms)
 *   2. Update metadata in RAM (mutex)
 *   3. Write metadata to EEPROM (~10ms)
 *   4. Update cache (mutex) [Phase 3.5]
 *   5. Notify subscribers
 *
 * MSG_CLEAR (10ms):
 *   1. Reset metadata in RAM (mutex)
 *   2. Write metadata to EEPROM (~10ms)
 *   3. Invalidate cache (mutex)
 *
 * @param[in] self EventLogAO instance
 * @param[in] msg  Application message
 */
static void dispatch_message(EventLogAO_t *self, const EventLogMsg_t *msg)
{
    switch (msg->type)
    {
    case EVENTLOG_MSG_APPEND:
    {
        /* ── Step 1: Write event to EEPROM (~10ms) ──────────────────── */
        uint16_t write_index = self->metadata.head;
        Result_t res = write_event_to_eeprom(self, write_index, &msg->event);
        if (res != ERR_OK)
        {
            LOG_ERROR(self->logger, TAG, "Failed to write event to EEPROM, error: %d", res);
            break;
        }

        /* ── Step 2: Update metadata in RAM (mutex) ─────────────────── */
        os_mutex_acquire(self->metadata_mutex, OS_WAIT_FOREVER);

        self->metadata.head = (self->metadata.head + 1) % EVENT_LOG_MAX_SIZE;
        if (self->metadata.count < EVENT_LOG_MAX_SIZE)
        {
            self->metadata.count++;
        }

        os_mutex_release(self->metadata_mutex);

        /* ── Step 3: Write metadata to EEPROM (~10ms) ───────────────── */
        res = save_metadata_to_eeprom(self);
        if (res != ERR_OK)
        {
            LOG_ERROR(self->logger, TAG, "Failed to save metadata to EEPROM, error: %d", res);
            break;
        }

        /* ── Step 4: Update cache (mutex) — Auto-populate ──────────── */
        uint16_t cache_slot = write_index % EVENT_LOG_CACHE_SIZE;
        os_mutex_acquire(self->cache_mutex, OS_WAIT_FOREVER);
        self->cache[cache_slot] = msg->event;
        self->cache_valid[cache_slot] = true;
        self->cache_index[cache_slot] = write_index;
        os_mutex_release(self->cache_mutex);

        LOG_INFO(self->logger, TAG, "Appended event at index %d, total count: %d", write_index, self->metadata.count);
        /* ── Step 5: Notify subscribers ─────────────────────────────── */
        notify_subscribers(self, &msg->event);

        break;
    }

    case EVENTLOG_MSG_CLEAR:
    {
        /* ── Step 1: Invalidate cache (mutex) ───────────────────────── */
        os_mutex_acquire(self->cache_mutex, OS_WAIT_FOREVER);
        memset(self->cache_valid, false, sizeof(self->cache_valid));
        memset(self->cache_index, 0xFF, sizeof(self->cache_index)); /* 0xFFFF = invalid */
        os_mutex_release(self->cache_mutex);

        /* ── Step 2: Reset metadata in RAM (mutex) ──────────────────── */
        os_mutex_acquire(self->metadata_mutex, OS_WAIT_FOREVER);
        self->metadata.head = 0;
        self->metadata.count = 0;
        os_mutex_release(self->metadata_mutex);

        /* ── Step 3: Write reset metadata to EEPROM (~10ms) ─────────── */
        Result_t res = save_metadata_to_eeprom(self);
        if (res != ERR_OK)
        {
            LOG_ERROR(self->logger, TAG, "Failed to save metadata to EEPROM, error: %d", res);
        }

        break;
    }

    default:
        /* Unknown message type */
        break;
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Observer Notification
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Notify all subscribers of new event.
 *
 * @details
 * Executed in worker thread context after EEPROM write success.
 *
 * @warning Callbacks must be <500μs and non-blocking.
 *
 * @param[in] self  EventLogAO instance
 * @param[in] event New event
 */
static void notify_subscribers(EventLogAO_t *self, const EventLogEntry_t *event)
{
    for (uint8_t i = 0; i < EVENT_LOG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback != NULL)
        {
            self->subscribers[i].callback(self->subscribers[i].context, event);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  EEPROM Backend Helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Load metadata from EEPROM.
 *
 * @param[in] self EventLogAO instance
 * @return ERR_OK on success, error code on failure
 */
static Result_t load_metadata_from_eeprom(EventLogAO_t *self)
{
    EventLogMeta_t meta;

    Result_t res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        EEPROM_EVENT_LOG_META_OFFSET,
        (uint8_t *)&meta,
        sizeof(EventLogMeta_t));

    if (res != ERR_OK)
    {
        return res;
    }

    /* Validate checksum */
    uint16_t expected_checksum = calculate_metadata_checksum(meta.head, meta.count);
    if (meta.checksum != expected_checksum)
    {
        LOG_ERROR(self->logger, TAG, "Metadata checksum mismatch: expected 0x%04X, got 0x%04X",
                  expected_checksum, meta.checksum);
        return ERR_ERROR; /* Corrupted metadata */
    }

    self->metadata = meta;

    return ERR_OK;
}

/**
 * @brief Save metadata to EEPROM.
 *
 * @param[in] self EventLogAO instance
 * @return ERR_OK on success, error code on failure
 */
static Result_t save_metadata_to_eeprom(EventLogAO_t *self)
{
    /* Calculate checksum over head and count fields */
    self->metadata.checksum = calculate_metadata_checksum(self->metadata.head, self->metadata.count);

    Result_t res = EXT_EEPROM_WriteData(
        self->eeprom,
        self->eeprom_handle,
        EEPROM_EVENT_LOG_META_OFFSET,
        (const uint8_t *)&self->metadata,
        sizeof(EventLogMeta_t));

    return res;
}

/**
 * @brief Write event to EEPROM at given index.
 *
 * @param[in] self  EventLogAO instance
 * @param[in] index Event index (0-99)
 * @param[in] event Event data
 * @return ERR_OK on success, error code on failure
 */
static Result_t write_event_to_eeprom(EventLogAO_t *self, uint16_t index, const EventLogEntry_t *event)
{
    if (index >= EVENT_LOG_MAX_SIZE)
    {
        return ERR_INVALID_PARAM;
    }

    uint16_t address = EEPROM_EVENT_LOG_ENTRIES_OFFSET + (index * sizeof(EventLogEntry_t));

    Result_t res = EXT_EEPROM_WriteData(
        self->eeprom,
        self->eeprom_handle,
        address,
        (const uint8_t *)event,
        sizeof(EventLogEntry_t));

    return res;
}

/**
 * @brief Read event from EEPROM at given index.
 *
 * @param[in]  self  EventLogAO instance
 * @param[in]  index Event index (0-99)
 * @param[out] out   Output event
 * @return ERR_OK on success, error code on failure
 */
static Result_t read_event_from_eeprom(EventLogAO_t *self, uint16_t index, EventLogEntry_t *out)
{
    if (index >= EVENT_LOG_MAX_SIZE)
    {
        return ERR_INVALID_PARAM;
    }

    uint16_t address = EEPROM_EVENT_LOG_ENTRIES_OFFSET + (index * sizeof(EventLogEntry_t));

    Result_t res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        address,
        (uint8_t *)out,
        sizeof(EventLogEntry_t));

    return res;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Checksum Calculation
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Calculate checksum for metadata to detect corruption.
 *
 * @details
 * Uses rotate-and-XOR algorithm for good bit mixing.
 * Checksum covers head and count fields only (checksum field excluded).
 *
 * @param[in] head  Head index (0-EVENT_LOG_MAX_SIZE)
 * @param[in] count Event count (0-EVENT_LOG_MAX_SIZE)
 * @return uint16_t Calculated 16-bit checksum value
 */
static uint16_t calculate_metadata_checksum(uint16_t head, uint16_t count)
{
    uint16_t checksum = 0x5A5A; /* Magic seed for initial mixing */

    /* Rotate left 7 bits and XOR with head (promotes bit diffusion) */
    checksum = (checksum << 7) | (checksum >> 9);
    checksum ^= head;

    /* Rotate left 7 bits and XOR with count */
    checksum = (checksum << 7) | (checksum >> 9);
    checksum ^= count;

    return checksum;
}
