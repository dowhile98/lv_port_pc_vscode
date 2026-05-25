/**
 * @file hourmeter_ao.h
 * @brief Hourmeter Active Object — timer-driven accumulation + batch EEPROM writes.
 *
 * @details
 * Owns a 1-minute periodic timer that posts ACCUMULATE messages to its queue.
 * Accumulates in RAM via Hourmeter_AccumulateMinute() and batch-saves to EEPROM
 * every HOURMETER_SAVE_INTERVAL_MS (5 minutes) or on FORCE_SAVE.
 *
 * Dependencies (all injected via HourmeterAOConfig_t):
 *   - I_EXT_EEPROM: EEPROM interface for persistence
 *   - Condition callbacks: gps_has_fix, relay_is_active, in_time_window
 *
 * @note Thread-safe: GetData and GetHours acquire mutex.
 */

#ifndef HOURMETER_AO_H
#define HOURMETER_AO_H

#include "common/hourmeter_types.h"
#include "domain/services/hourmeter_service.h"
#include "infrastructure/adapters/storage/hourmeter_eeprom_storage.h"
#include "interfaces/i_ext_eeprom.h"
#include "hal/hal_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Constants ───────────────────────────────────────────────────────────── */

#define HOURMETER_AO_STACK_SIZE 2048U      /**< Thread stack (bytes) — 512 was insufficient: I2C call chain (hourmeter_thread→HAL_I2C) requires ~470 bytes, leaving <42 bytes margin */
#define HOURMETER_AO_QUEUE_DEPTH 8U        /**< Max queued messages */
#define HOURMETER_TIMER_INTERVAL_MS 60000U /**< 1-minute accumulation timer */
#define HOURMETER_SAVE_INTERVAL_MS 300000U /**< 5-minute batch save */

    /* ── Message Types ───────────────────────────────────────────────────────── */

    typedef enum
    {
        HOURMETER_MSG_NONE = 0,       /**< No-op / uninitialized                          */
        HOURMETER_MSG_START = 1,      /**< Reserved — posted by Start()                   */
        HOURMETER_MSG_STOP = 2,       /**< Reserved — posted by Stop()                    */
        HOURMETER_MSG_TERMINATE = 3,  /**< Reserved — posted by Deinit()                  */
        HOURMETER_MSG_ACCUMULATE = 4, /**< Explicit tick (testability + manual trigger)  */
        HOURMETER_MSG_FORCE_SAVE = 5, /**< Persist immediately (e.g. before shutdown)    */
        HOURMETER_MSG_RESET = 6,      /**< Factory reset all accumulators                */
    } HourmeterMsgType_t;

    typedef struct
    {
        HourmeterMsgType_t type;
    } HourmeterMsg_t;

    /* ── Condition Callback ──────────────────────────────────────────────────── */

    /**
     * @brief Callback to query a boolean system condition.
     * @param context  Opaque pointer set during Init.
     * @return true if the condition is active.
     */
    typedef bool (*HourmeterConditionFn_t)(void *context);

    /* ── Configuration ───────────────────────────────────────────────────────── */

    typedef struct
    {
        I_EXT_EEPROM *eeprom;                /**< EEPROM interface (required) */
        I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM device handle */

        HourmeterConditionFn_t gps_has_fix; /**< GPS fix query (optional, NULL = never) */
        void *gps_context;                  /**< Context for gps_has_fix */

        HourmeterConditionFn_t relay_active; /**< Relay active query (optional) */
        void *relay_context;                 /**< Context for relay_active */

        HourmeterConditionFn_t in_window; /**< Time window query (optional) */
        void *window_context;             /**< Context for in_window */
    } HourmeterAOConfig_t;

    /* ── Active Object Instance ──────────────────────────────────────────────── */

    typedef struct HourmeterAO
    {
        /* OSAL resources */
        os_thread_t thread;
        os_queue_t queue;
        os_mutex_t mutex;
        os_semaphore_t stopped_sem; /**< WaitStopped + Deinit gate (init=0) */
        uint8_t stack[HOURMETER_AO_STACK_SIZE];
        uint8_t queue_buffer[HOURMETER_AO_QUEUE_DEPTH * sizeof(HourmeterMsg_t)];
        volatile bool running;   /**< Inner loop control (Start/Stop)    */
        volatile bool terminate; /**< Outer loop exit signal (Deinit)    */

        /* Domain service (RAM accumulation) */
        Hourmeter_t service;

        /* EEPROM persistence */
        HourmeterEepromStorage_t storage;

        /* Condition callbacks */
        HourmeterConditionFn_t gps_has_fix;
        void *gps_context;
        HourmeterConditionFn_t relay_active;
        void *relay_context;
        HourmeterConditionFn_t in_window;
        void *window_context;

        /* Batch-save state */
        bool pending_save;
        uint32_t last_save_tick;
    } HourmeterAO_t;

    /* ── Public API ──────────────────────────────────────────────────────────── */

    /**
     * @brief  Initialize the HourmeterAO. Loads latest data from EEPROM.
     */
    Result_t HourmeterAO_Init(HourmeterAO_t *self, const HourmeterAOConfig_t *config);

    /**
     * @brief  Start the AO thread and periodic timer.
     */
    Result_t HourmeterAO_Start(HourmeterAO_t *self);

    /**
     * @brief  Stop the AO (non-blocking, posts MSG_STOP). Call WaitStopped() to synchronize.
     */
    Result_t HourmeterAO_Stop(HourmeterAO_t *self);

    /**
     * @brief  Block until the AO thread completes its stop sequence (final EEPROM persist done).
     * @param[in] timeout_ms  Max wait in ms. Pass OS_WAIT_FOREVER for unconditional wait.
     */
    Result_t HourmeterAO_WaitStopped(HourmeterAO_t *self, uint32_t timeout_ms);

    /**
     * @brief  Deinitialize the AO. Must only be called after Stop() + WaitStopped().
     *         Posts MSG_TERMINATE, waits for thread to exit outer loop, frees OSAL resources.
     */
    Result_t HourmeterAO_Deinit(HourmeterAO_t *self);

    /**
     * @brief  Get a thread-safe snapshot of the current hourmeter data.
     */
    Result_t HourmeterAO_GetData(HourmeterAO_t *self, HourmeterData_t *out_data);

    /**
     * @brief  Get hours/minutes for a specific accumulator (thread-safe).
     */
    Result_t HourmeterAO_GetHours(HourmeterAO_t *self,
                                  HourmeterType_t type,
                                  uint32_t *hours,
                                  uint32_t *minutes);

    /**
     * @brief  Force an immediate EEPROM save (e.g., before shutdown).
     */
    Result_t HourmeterAO_ForceSave(HourmeterAO_t *self);

    /**
     * @brief  Reset all accumulators to zero.
     */
    Result_t HourmeterAO_Reset(HourmeterAO_t *self);

    /**
     * @brief  Process one message from the queue (for unit testing).
     *
     * @note  In production, the AO thread calls this in a loop.
     *        In tests, call directly to step through messages deterministically.
     */
    Result_t HourmeterAO_ProcessOne(HourmeterAO_t *self);

#ifdef __cplusplus
}
#endif

#endif /* HOURMETER_AO_H */
