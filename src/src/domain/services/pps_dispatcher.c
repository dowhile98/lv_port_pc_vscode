/**
 * @file pps_dispatcher.c
 * @brief PPS Event Dispatcher Implementation
 * @author TCS Development Team
 * @date 2026-01-29
 */

#include "domain/services/pps_dispatcher.h"
#include <string.h>

/**
 * @brief Initialize PPS dispatcher.
 *
 * Clears all subscriber slots and resets counter to zero.
 * Uses memset for efficient bulk initialization.
 *
 * @param[in,out] self Dispatcher instance
 *
 * @note Assumes self is not NULL (caller responsibility during init).
 * @note Execution time: ~5μs on STM32U5 @ 160 MHz.
 */
void PPSDispatcher_Init(PPSDispatcher_t *self)
{
    if (self == NULL)
    {
        return;
    }

    /* Zero entire structure (efficient bulk clear) */
    memset(self, 0, sizeof(PPSDispatcher_t));

    /* Explicitly mark all slots as inactive (redundant but explicit) */
    for (uint8_t i = 0; i < PPS_DISPATCHER_MAX_SUBSCRIBERS; i++)
    {
        self->subscribers[i].is_active = false;
        self->subscribers[i].callback = NULL;
        self->subscribers[i].context = NULL;
    }

    self->subscriber_count = 0;
}

/**
 * @brief Register a PPS event subscriber.
 *
 * Finds first inactive slot, stores callback + context, marks active.
 * Uses linear search (acceptable for small MAX_SUBSCRIBERS).
 *
 * @param[in,out] self Dispatcher instance
 * @param[in] callback Function to call on PPS event
 * @param[in] context User context (can be NULL)
 *
 * @return ERR_OK on success
 * @return ERR_NULL_POINTER if self or callback is NULL
 * @return ERR_BUSY if table is full
 *
 * @note Time complexity: O(n) where n = MAX_SUBSCRIBERS (typically 4)
 * @note Execution time: ~2μs on STM32U5
 */
Result_t PPSDispatcher_Subscribe(PPSDispatcher_t *self,
                                 PPSSubscriberCallback_t callback,
                                 void *context)
{
    /* Validate inputs */
    if (self == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Check if table is full (quick optimization) */
    if (self->subscriber_count >= PPS_DISPATCHER_MAX_SUBSCRIBERS)
    {
        return ERR_BUSY;
    }

    /* Find first inactive slot */
    for (uint8_t i = 0; i < PPS_DISPATCHER_MAX_SUBSCRIBERS; i++)
    {
        if (!self->subscribers[i].is_active)
        {
            /* Slot found - store subscriber */
            self->subscribers[i].callback = callback;
            self->subscribers[i].context = context;
            self->subscribers[i].is_active = true;
            self->subscriber_count++;

            return ERR_OK;
        }
    }

    /* Should never reach here (count check should prevent) */
    return ERR_BUSY;
}

/**
 * @brief Dispatch PPS event to all active subscribers.
 *
 * Iterates through subscriber table and invokes callbacks sequentially.
 * CRITICAL: No validation performed for ISR speed requirements.
 *
 * @param[in] self Dispatcher instance (MUST be valid)
 * @param[in] gps_time Current GPS time (can be NULL)
 *
 * @warning Called from EXTI ISR context. NO blocking operations.
 * @warning Assumes self is valid (caller responsibility).
 * @warning If callback crashes, system hangs in ISR context.
 *
 * @note Time complexity: O(n) where n = subscriber_count
 * @note Execution time: ~10μs per subscriber @ 160 MHz
 * @note Total budget: 4 subscribers × 10μs = ~40μs (within 50μs limit)
 *
 * @implementation Optimizations applied:
 * - No NULL checks (ISR speed critical)
 * - Iterate only active subscribers (use subscriber_count)
 * - Simple loop with minimal branching
 * - Inline-friendly (no function calls except callbacks)
 */
void PPSDispatcher_Dispatch(PPSDispatcher_t *self, const DateTime_t *gps_time)
{
    /* NO validation - ISR context requires maximum speed */
    /* Caller MUST ensure self is valid */

    /* Early exit if no subscribers (optimization) */
    if (self->subscriber_count == 0)
    {
        return;
    }

    /* Iterate through subscriber table */
    uint8_t called_count = 0;
    for (uint8_t i = 0; i < PPS_DISPATCHER_MAX_SUBSCRIBERS; i++)
    {
        /* Check if slot is active */
        if (self->subscribers[i].is_active)
        {
            /* Invoke callback with context and time */
            self->subscribers[i].callback(self->subscribers[i].context, gps_time);

            /* Optimization: exit early if all active subscribers called */
            called_count++;
            if (called_count >= self->subscriber_count)
            {
                break;
            }
        }
    }
}
