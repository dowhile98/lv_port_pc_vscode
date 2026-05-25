/**
 * @file pps_dispatcher.h
 * @brief PPS Event Dispatcher - Multi-Subscriber Observer Pattern
 * @author TCS Development Team
 * @date 2026-01-29
 *
 * @section purpose Purpose
 * Distributes GPS PPS (Pulse Per Second) events to multiple subscribers
 * without modifying the GPS adapter. Implements Observer Pattern with
 * ISR-safe dispatch mechanism.
 *
 * @section architecture Architecture
 * @code
 * EXTI ISR → GPSAdapter → PPSDispatcher
 *                              ├─→ TimeAdapter (RTC sync)
 *                              ├─→ RelayAO (cycle control)
 *                              └─→ Logger (optional)
 * @endcode
 *
 * @section constraints Constraints
 * - Static allocation only (NO malloc)
 * - ISR-safe dispatch (<50μs total execution)
 * - Deterministic iteration order (registration order)
 * - No mutex/semaphore (called from ISR context)
 *
 * @section solid SOLID Principles
 * - SRP: Only manages subscriber list and dispatch
 * - OCP: Extensible via callback registration
 * - LSP: Compatible with PPSCallback_t signature
 * - ISP: Single focused interface (3 functions)
 * - DIP: Depends on callback abstraction, not concrete implementations
 */

#ifndef PPS_DISPATCHER_H
#define PPS_DISPATCHER_H

#include "hal_types.h"
#include "common/date_time.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Maximum number of PPS subscribers.
 * @note Increase if more consumers needed (e.g., Logger, UI, Diagnostics).
 *       Current allocation: 4 slots × ~12 bytes = 48 bytes RAM.
 */
#ifndef PPS_DISPATCHER_MAX_SUBSCRIBERS
#define PPS_DISPATCHER_MAX_SUBSCRIBERS 4
#endif
/**
 * @brief PPS subscriber callback function type.
 *
 * @warning MUST execute in <10μs. Called from EXTI ISR context.
 * @warning NO blocking operations (mutex, I/O, printf, etc.).
 * @warning Callbacks execute sequentially in registration order.
 *
 * @param[in] context User-defined context pointer (passed during Subscribe)
 * @param[in] gps_time Current GPS time (CAN be NULL if time unavailable)
 *
 * @note If gps_time is NULL, subscriber should fetch time from its own source
 *       or skip time-dependent operations.
 */
typedef void (*PPSSubscriberCallback_t)(void *context, const DateTime_t *gps_time);

/**
 * @brief PPS subscriber entry (internal structure).
 * @private
 */
typedef struct
{
    PPSSubscriberCallback_t callback; /**< Callback function pointer */
    void *context;                    /**< User context (opaque) */
    bool is_active;                   /**< Slot occupied flag */
} PPSSubscriber_t;

/**
 * @brief PPS Dispatcher state machine.
 *
 * @note Statically allocated, no dynamic memory.
 * @note Thread-safe for single writer (init) + multiple readers (dispatch ISR).
 */
typedef struct
{
    PPSSubscriber_t subscribers[PPS_DISPATCHER_MAX_SUBSCRIBERS]; /**< Subscriber table */
    uint8_t subscriber_count;                                    /**< Active subscriber count (optimization) */
} PPSDispatcher_t;

/**
 * @brief Initialize PPS dispatcher.
 *
 * Clears all subscriber slots and resets counter.
 *
 * @pre Must be called during system initialization (before kernel start).
 * @pre Dispatcher must not be in use (no concurrent dispatch).
 * @post All subscriber slots marked as inactive.
 * @post subscriber_count == 0.
 *
 * @param[in,out] self Dispatcher instance (must not be NULL)
 *
 * @note NOT thread-safe. Call only from main thread during init.
 */
void PPSDispatcher_Init(PPSDispatcher_t *self);

/**
 * @brief Register a PPS event subscriber.
 *
 * Adds a callback to the subscriber list. Callbacks are invoked in
 * registration order (FIFO) during dispatch.
 *
 * @pre Dispatcher initialized via PPSDispatcher_Init().
 * @pre callback not NULL.
 * @pre Fewer than PPS_DISPATCHER_MAX_SUBSCRIBERS already registered.
 *
 * @param[in,out] self Dispatcher instance
 * @param[in] callback Function to call on PPS event (ISR context)
 * @param[in] context User context passed to callback (can be NULL)
 *
 * @return ERR_OK on success
 * @return ERR_NULL_POINTER if self or callback is NULL
 * @return ERR_BUSY if subscriber table is full (MAX_SUBSCRIBERS reached)
 *
 * @note NOT thread-safe. Register all subscribers during system init.
 * @note Same callback can be registered multiple times with different contexts.
 */
Result_t PPSDispatcher_Subscribe(PPSDispatcher_t *self,
                                 PPSSubscriberCallback_t callback,
                                 void *context);

/**
 * @brief Dispatch PPS event to all active subscribers.
 *
 * Iterates through subscriber list and invokes each callback sequentially.
 * Executes in ISR context with strict timing constraints.
 *
 * @warning Called from EXTI ISR context. MUST complete in <50μs.
 * @warning NO validation performed (for speed). Caller must ensure self is valid.
 * @warning If a callback crashes, system hangs (ISR context). Callbacks MUST be robust.
 *
 * @pre Dispatcher initialized and subscribers registered.
 * @pre Called from ISR context only.
 *
 * @param[in] self Dispatcher instance (must be valid, NOT checked)
 * @param[in] gps_time Current GPS time (can be NULL)
 *
 * @note Subscribers called in registration order (deterministic).
 * @note If gps_time is NULL, subscribers receive NULL and must handle gracefully.
 * @note Timing budget: ~10μs per subscriber × 4 = ~40μs total (safe margin).
 */
void PPSDispatcher_Dispatch(PPSDispatcher_t *self, const DateTime_t *gps_time);

#endif /* PPS_DISPATCHER_H */
