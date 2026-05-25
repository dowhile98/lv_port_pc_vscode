/**
 * @file eventlog_ao.h
 * @brief EventLog Active Object — Pattern B (Message-Driven AO)
 *
 * @details
 * Asynchronously persists system events to EEPROM, eliminating blocking delays
 * in event producers. Implements Pattern B from ACTIVE_OBJECT_ARCHITECTURE.md:
 *   - Outer/inner loop with control messages (MSG_START, MSG_STOP, MSG_TERMINATE)
 *   - Queue as gate mechanism (NO run_sem)
 *   - stopped_sem for WaitStopped() synchronization
 *   - Lifecycle: Init→Start→Stop→WaitStopped→Deinit
 *
 * Features:
 *   - Non-blocking Append (<5μs queue post)
 *   - Hybrid cache (lazy load): 1μs reads on cached events
 *   - Observer pattern for event notifications
 *   - Thread-safe metadata/cache access
 *
 * @note Pattern B Compliance: See ACTIVE_OBJECT_ARCHITECTURE.md Section 3.2
 * @note Design: docs/architecture/EVENTLOG_AO_DESIGN.md
 */

#ifndef INCLUDE_APPLICATION_ACTIVEOBJECTS_EVENTLOG_AO_H_
#define INCLUDE_APPLICATION_ACTIVEOBJECTS_EVENTLOG_AO_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_types.h"
#include "infrastructure/osal/osal.h"
#include "interfaces/i_event_log_storage.h"
#include "interfaces/i_ext_eeprom.h"
#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Constants ───────────────────────────────────────────────────────────── */

#ifndef EVENTLOG_AO_STACK_SIZE
#define EVENTLOG_AO_STACK_SIZE (1024 * 6U) /**< Worker thread stack (bytes) — increased from 4KB: EEPROM/I2C call chain safety margin */
#endif

#ifndef EVENTLOG_AO_PRIORITY
#define EVENTLOG_AO_PRIORITY (6U) /**< Priority: below Network(5), above UI(10) */
#endif

#ifndef EVENTLOG_AO_QUEUE_DEPTH
#define EVENTLOG_AO_QUEUE_DEPTH (16U) /**< Max pending messages (burst tolerance) */
#endif

#ifndef EVENT_LOG_SUBSCRIBERS_MAX
#define EVENT_LOG_SUBSCRIBERS_MAX (4U) /**< Max observer callbacks */
#endif

#ifndef EVENT_LOG_CACHE_SIZE
#define EVENT_LOG_CACHE_SIZE (500U) /**< Partial cache: last 500 of 11,340 events (5 KB RAM) */
#endif

    /* ── Message Types (Pattern B — Control Messages Reserved) ───────────────── */

    /**
     * @brief Message types for EventLogAO.
     *
     * @note Pattern B Rule: Values 1-3 reserved for control messages.
     *       Application messages start from 4.
     */
    typedef enum
    {
        EVENTLOG_MSG_NONE = 0,      /**< Invalid message */
        EVENTLOG_MSG_START = 1,     /**< Reserved — posted by Start() */
        EVENTLOG_MSG_STOP = 2,      /**< Reserved — posted by Stop() */
        EVENTLOG_MSG_TERMINATE = 3, /**< Reserved — posted by Deinit() */
        /* Application events start here */
        EVENTLOG_MSG_APPEND = 4, /**< Write event to EEPROM */
        EVENTLOG_MSG_CLEAR = 5,  /**< Reset all events */
    } EventLogMsgType_t;

    /**
     * @brief Message structure for EventLogAO queue.
     *
     * @note Uses value semantics (EventLogEntry_t copied by value, 16 bytes)
     *       to avoid pointer lifetime issues.
     */
    typedef struct
    {
        EventLogMsgType_t type;
        EventLogEntry_t event; /**< Payload for MSG_APPEND (copied by value) */
    } EventLogMsg_t;

    /* ── Observer Pattern (Phase 4.9) ────────────────────────────────────────── */

    /**
     * @brief Callback invoked when new event written (after EEPROM success).
     *
     * @warning Executes in EventLogAO worker thread. Must be <500μs, no blocking.
     * @param context User context (typically an adapter instance).
     * @param event   Newly written event (valid only during callback).
     */
    typedef void (*EventLogCallback_t)(void *context, const EventLogEntry_t *event);

    /**
     * @brief Subscriber entry (callback + context).
     */
    typedef struct
    {
        EventLogCallback_t callback; /**< Callback function (NULL = slot free) */
        void *context;               /**< User context */
    } EventLogSubscriber_t;

    /* ── Configuration ───────────────────────────────────────────────────────── */

    /**
     * @brief Configuration for EventLogAO initialization.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;                /**< EEPROM interface (required) */
        I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM device handle */
        const ILogger *logger;               /**< Error logger (optional, NULL ok) */
    } EventLogAOConfig_t;

    /* ── Active Object Instance ──────────────────────────────────────────────── */

    /**
     * @brief EventLog Active Object instance (Pattern B).
     *
     * @note Pattern B: Queue is the gate (NO run_sem). stopped_sem signals
     *       inner loop exit (WaitStopped) and outer loop exit (Deinit).
     */
    typedef struct EventLogAO
    {
        /* ── OSAL resources (Pattern B) ───────────────────────────────────── */
        os_thread_t thread;
        os_queue_t queue;           /**< Gate + work channel (NO run_sem) */
        os_semaphore_t stopped_sem; /**< WaitStopped + Deinit gate (binary, init=0) */
        uint8_t stack[EVENTLOG_AO_STACK_SIZE];
        uint8_t queue_buffer[EVENTLOG_AO_QUEUE_DEPTH * sizeof(EventLogMsg_t)];

        /* ── Thread state (Pattern B canonical) ───────────────────────────── */
        volatile bool running; /**< Inner loop control */
        bool initialized;

        /* ── Dependencies (injected) ──────────────────────────────────────── */
        I_EXT_EEPROM *eeprom;
        I_EXT_EEPROM_Handle_t eeprom_handle;
        const ILogger *logger; /**< Optional logger (NULL ok) */

        /* ── Cached metadata (mutex-protected) ────────────────────────────── */
        os_mutex_t metadata_mutex; /**< Protects metadata below */
        EventLogMeta_t metadata;   /**< 6 bytes: head, count, checksum */

        /* ── Event cache (hybrid lazy load) ───────────────────────────────── */
        os_mutex_t cache_mutex;                      /**< Protects cache + cache_valid */
        EventLogEntry_t cache[EVENT_LOG_CACHE_SIZE]; /**< 5,000 bytes (500 events) */
        bool cache_valid[EVENT_LOG_CACHE_SIZE];      /**< 500 bytes bitmap */
        uint16_t cache_index[EVENT_LOG_CACHE_SIZE];  /**< 1,000 bytes: original index for each slot */

        /* ── Observer pattern ─────────────────────────────────────────────── */
        EventLogSubscriber_t subscribers[EVENT_LOG_SUBSCRIBERS_MAX]; /**< 32 bytes */

        /* ── IEventLogStorage interface exposure ──────────────────────────── */
        IEventLogStorage interface; /**< V-Table for DI Container */

    } EventLogAO_t;

    /* ── Public API (Lifecycle) ──────────────────────────────────────────────── */

    /**
     * @brief  Initialize the EventLogAO.
     *
     * @details Creates OSAL thread, queue, semaphores, mutexes. Thread blocks on
     *          queue in outer loop waiting for MSG_START. Loads metadata from EEPROM.
     *          Cache starts empty (lazy load).
     *
     * @param[in] self   Pointer to EventLogAO instance (not NULL).
     * @param[in] config Configuration (all fields validated).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER / ERR_ERROR on failure.
     */
    Result_t EventLogAO_Init(EventLogAO_t *self, const EventLogAOConfig_t *config);

    /**
     * @brief  Start the EventLogAO.
     *
     * @details Posts MSG_START to queue, waking outer loop → enters inner loop.
     *          Worker thread begins processing MSG_APPEND/MSG_CLEAR.
     *
     * @param[in] self Pointer to EventLogAO instance (not NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER / ERR_ERROR / ERR_BUSY if already running.
     */
    Result_t EventLogAO_Start(EventLogAO_t *self);

    /**
     * @brief  Stop the EventLogAO.
     *
     * @details Sets running=false, posts MSG_STOP to queue. Worker thread exits
     *          inner loop and signals stopped_sem.
     *
     * @param[in] self Pointer to EventLogAO instance (not NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER on failure.
     */
    Result_t EventLogAO_Stop(EventLogAO_t *self);

    /**
     * @brief  Wait for EventLogAO to stop.
     *
     * @details Blocks caller until worker thread exits inner loop (after Stop()).
     *          Uses stopped_sem.
     *
     * @param[in] self       Pointer to EventLogAO instance (not NULL).
     * @param[in] timeout_ms Timeout in milliseconds (OS_WAIT_FOREVER = infinite).
     *
     * @return ERR_OK if stopped, ERR_TIMEOUT if timeout, ERR_NULL_POINTER if self NULL.
     */
    Result_t EventLogAO_WaitStopped(EventLogAO_t *self, uint32_t timeout_ms);

    /**
     * @brief  Deinitialize the EventLogAO.
     *
     * @details Posts MSG_TERMINATE to queue, waking outer loop → thread exits.
     *          Waits for stopped_sem, then destroys OSAL resources.
     *
     * @warning Must call Stop() + WaitStopped() before Deinit().
     *
     * @param[in] self Pointer to EventLogAO instance (not NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER / ERR_BUSY if still running.
     */
    Result_t EventLogAO_Deinit(EventLogAO_t *self);

    /* ── Public API (Write Operations — Non-blocking) ────────────────────────── */

    /**
     * @brief  Append event to log (non-blocking).
     *
     * @details Copies event to message, posts to queue with OS_NO_WAIT.
     *          Returns immediately (<5μs). Worker thread handles EEPROM write (~20ms).
     *
     * @param[in] self  Pointer to EventLogAO instance (not NULL).
     * @param[in] event Event to append (not NULL).
     *
     * @return ERR_OK if queued, ERR_BUSY if queue full, ERR_NULL_POINTER / ERR_ERROR otherwise.
     */
    Result_t EventLogAO_Append(EventLogAO_t *self, const EventLogEntry_t *event);

    /**
     * @brief  Clear all events (non-blocking).
     *
     * @details Posts MSG_CLEAR to queue with OS_NO_WAIT. Worker thread resets
     *          metadata and invalidates cache.
     *
     * @param[in] self Pointer to EventLogAO instance (not NULL).
     *
     * @return ERR_OK if queued, ERR_BUSY if queue full, ERR_NULL_POINTER / ERR_ERROR otherwise.
     */
    Result_t EventLogAO_Clear(EventLogAO_t *self);

    /* ── Public API (Read Operations — Mutex-protected) ──────────────────────── */

    /**
     * @brief  Get count of events in log (mutex-protected RAM read).
     *
     * @details Thread-safe, returns cached value from RAM. No EEPROM access.
     *
     * @param[in]  self      Pointer to EventLogAO instance (not NULL).
     * @param[out] out_count Pointer to store event count (not NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER if arguments NULL.
     */
    Result_t EventLogAO_GetCount(EventLogAO_t *self, uint16_t *out_count);

    /**
     * @brief  Get metadata snapshot (mutex-protected RAM read).
     *
     * @details Thread-safe, returns cached metadata from RAM. No EEPROM access.
     *
     * @param[in]  self     Pointer to EventLogAO instance (not NULL).
     * @param[out] out_meta Pointer to store metadata (not NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER if arguments NULL.
     */
    Result_t EventLogAO_GetMetadata(EventLogAO_t *self, EventLogMeta_t *out_meta);

    /**
     * @brief  Read event by index (0 = oldest).
     *
     * @details Uses hybrid cache: instant if cached (1μs), lazy load if not (15ms).
     *          Thread-safe. First access may block ~15ms, subsequent <1μs.
     *          Events written via Append() are auto-cached (always instant read).
     *
     * @param[in]  self  Pointer to EventLogAO instance (not NULL).
     * @param[in]  index Event index (0 = oldest, must be < count).
     * @param[out] out   Pointer to store event (not NULL).
     *
     * @return ERR_OK on success, ERR_INVALID_PARAM if index >= count, ERR_NULL_POINTER if args NULL.
     */
    Result_t EventLogAO_ReadByIndex(EventLogAO_t *self, uint16_t index, EventLogEntry_t *out);

    /* ── Public API (Observer Pattern) ───────────────────────────────────────── */

    /**
     * @brief  Subscribe to event notifications.
     *
     * @details Callback invoked after EEPROM write success. Executes in worker thread.
     *
     * @warning Callback must be <500μs, no blocking.
     *
     * @param[in] self     Pointer to EventLogAO instance (not NULL).
     * @param[in] callback Callback function (not NULL).
     * @param[in] context  User context (optional, NULL ok).
     *
     * @return ERR_OK on success, ERR_BUSY if all slots full, ERR_NULL_POINTER if args NULL.
     */
    Result_t EventLogAO_Subscribe(EventLogAO_t *self,
                                  EventLogCallback_t callback,
                                  void *context);

    /**
     * @brief  Unsubscribe from event notifications.
     *
     * @param[in] self     Pointer to EventLogAO instance (not NULL).
     * @param[in] callback Callback function to remove (not NULL).
     *
     * @return ERR_OK on success, ERR_ERROR if not found, ERR_NULL_POINTER if args NULL.
     */
    Result_t EventLogAO_Unsubscribe(EventLogAO_t *self, EventLogCallback_t callback);

    /* ── Interface Exposure (DI Container) ───────────────────────────────────── */

    /**
     * @brief  Get IEventLogStorage interface for DI container.
     *
     * @param[in] self Pointer to EventLogAO instance (not NULL).
     *
     * @return Pointer to IEventLogStorage interface, or NULL if self is NULL.
     */
    IEventLogStorage *EventLogAO_GetInterface(EventLogAO_t *self);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_APPLICATION_ACTIVEOBJECTS_EVENTLOG_AO_H_ */
