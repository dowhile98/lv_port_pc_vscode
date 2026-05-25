/**
 * @file hourmeter_ao.c
 * @brief Hourmeter Active Object — timer-driven accumulation + batch EEPROM writes.
 */

#include "application/activeobjects/hourmeter_ao.h"
#include "infrastructure/osal/osal.h"
#include <string.h>

/* ── Private Helpers ─────────────────────────────────────────────────────── */

static bool query_condition(HourmeterConditionFn_t fn, void *ctx)
{
    if (fn)
    {
        return fn(ctx);
    }
    return false;
}

static void save_to_eeprom(HourmeterAO_t *self)
{
    HourmeterEepromStorage_Save(&self->storage, &self->service.data);
    self->pending_save = false;
    self->last_save_tick = os_ticks_get();
}

static void handle_accumulate(HourmeterAO_t *self)
{
    bool gps = query_condition(self->gps_has_fix, self->gps_context);
    bool rly = query_condition(self->relay_active, self->relay_context);
    bool win = query_condition(self->in_window, self->window_context);

    Hourmeter_AccumulateMinute(&self->service, gps, rly, win);
    self->pending_save = true;

    /* Batch save check */
    uint32_t now = os_ticks_get();
    uint32_t elapsed = now - self->last_save_tick;
    if (elapsed >= os_ms_to_ticks(HOURMETER_SAVE_INTERVAL_MS))
    {
        save_to_eeprom(self);
    }
}

static void handle_force_save(HourmeterAO_t *self)
{
    if (self->pending_save)
    {
        save_to_eeprom(self);
    }
}

static void handle_reset(HourmeterAO_t *self)
{
    Hourmeter_Reset(&self->service);
    save_to_eeprom(self);
}

/* ── Worker Thread ───────────────────────────────────────────────────────── */

/**
 * @brief AO thread — Pattern B (Message-Driven) per ACTIVE_OBJECT_ARCHITECTURE.md.
 *
 * Outer loop : blocks on queue waiting for MSG_START.
 *              The queue IS the gate — no run_sem needed for Pattern B.
 * Inner loop : uses os_queue_receive with timeout = HOURMETER_TIMER_INTERVAL_MS (60 s).
 *              When the receive times out (no message in 60 s) → accumulate 1 minute.
 *              Explicit ACCUMULATE messages also fire accumulation (unit tests / manual).
 */
static void hourmeter_thread_func(void *arg)
{
    HourmeterAO_t *self = (HourmeterAO_t *)arg;
    bool should_exit = false;

    /* ── Outer loop: blocked until MSG_START ────────────────────────────── */
    while (!should_exit)
    {
        HourmeterMsg_t msg;
        Result_t res = os_queue_receive(self->queue, &msg, OS_WAIT_FOREVER);
        if (res != ERR_OK)
        {
            continue;
        }

        if (msg.type == HOURMETER_MSG_TERMINATE)
        {
            break;
        }
        if (msg.type != HOURMETER_MSG_START)
        {
            continue; /* discard anything that arrives before Start() */
        }

        /* ── Inner loop: 60-second tick via queue timeout ───────────────── */
        while (self->running)
        {
            HourmeterMsg_t inner;
            /* Block up to 60 s. Timeout = no message → 1 minute has elapsed */
            Result_t r = os_queue_receive(self->queue, &inner,
                                          HOURMETER_TIMER_INTERVAL_MS);

            os_mutex_acquire(self->mutex, OS_WAIT_FOREVER);

            if (r != ERR_OK)
            {
                /* Timeout path: 60 seconds passed with no message → accumulate */
                handle_accumulate(self);
            }
            else
            {
                switch (inner.type)
                {
                case HOURMETER_MSG_STOP:
                    self->running = false;
                    break;
                case HOURMETER_MSG_TERMINATE:
                    self->running = false;
                    should_exit = true;
                    break;
                case HOURMETER_MSG_ACCUMULATE:
                    /* Explicit tick — unit tests and manual triggers */
                    handle_accumulate(self);
                    break;
                case HOURMETER_MSG_FORCE_SAVE:
                    handle_force_save(self);
                    break;
                case HOURMETER_MSG_RESET:
                    handle_reset(self);
                    break;
                default:
                    break;
                }
            }

            os_mutex_release(self->mutex);
        }

        /* Final persist before re-entering outer loop (Stop) or exiting (Deinit) */
        os_mutex_acquire(self->mutex, OS_WAIT_FOREVER);
        if (self->pending_save)
        {
            save_to_eeprom(self);
        }
        os_mutex_release(self->mutex);

        if (!should_exit)
        {
            os_semaphore_put(self->stopped_sem); /* → WaitStopped() */
        }
    }

    os_semaphore_put(self->stopped_sem); /* → Deinit() */
}

/* ══════════════════════════════════════════════════════════════════════════ *
 *  Public API                                                               *
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t HourmeterAO_Init(HourmeterAO_t *self, const HourmeterAOConfig_t *config)
{
    if (!self || !config || !config->eeprom)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(HourmeterAO_t));

    /* Store condition callbacks */
    self->gps_has_fix = config->gps_has_fix;
    self->gps_context = config->gps_context;
    self->relay_active = config->relay_active;
    self->relay_context = config->relay_context;
    self->in_window = config->in_window;
    self->window_context = config->window_context;

    /* Init EEPROM storage */
    Result_t res = HourmeterEepromStorage_Init(&self->storage,
                                               config->eeprom,
                                               config->eeprom_handle);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Load existing data from EEPROM */
    HourmeterData_t loaded;
    res = HourmeterEepromStorage_Load(&self->storage, &loaded);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Init domain service and restore data */
    Hourmeter_Init(&self->service);
    Hourmeter_LoadFromStorage(&self->service, &loaded);

    /* Create OSAL resources */
    res = os_mutex_create(&self->mutex, "hm_mtx");
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    os_queue_config_t q_cfg = {
        .name = "hm_q",
        .buffer = self->queue_buffer,
        .buffer_size = sizeof(self->queue_buffer),
        .item_size = sizeof(HourmeterMsg_t),
    };
    res = os_queue_create(&self->queue, &q_cfg);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* stopped_sem (binary, init=0): signalled by thread after Stop (WaitStopped) and Deinit */
    res = os_semaphore_create(&self->stopped_sem, "hm_stop", 0U);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Thread starts immediately → enters outer loop → blocks on queue receive (MSG_START gate) */
    os_thread_config_t th_cfg = {
        .name = "hourmeter",
        .entry = hourmeter_thread_func,
        .arg = self,
        .stack_ptr = self->stack,
        .stack_size = HOURMETER_AO_STACK_SIZE,
        .priority = 15,
        .auto_start = true,
    };
    res = os_thread_create(&self->thread, &th_cfg);
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    self->running = false;
    self->terminate = false;
    self->pending_save = false;
    self->last_save_tick = 0;

    return ERR_OK;
}

Result_t HourmeterAO_Start(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    if (self->running)
    {
        return ERR_BUSY;
    }

    self->running = true;
    self->last_save_tick = os_ticks_get();

    /* Post MSG_START — unblocks the thread outer loop (queue IS the gate, no run_sem needed) */
    HourmeterMsg_t msg = {.type = HOURMETER_MSG_START};
    return os_queue_send(self->queue, &msg, OS_NO_WAIT);
}

Result_t HourmeterAO_Stop(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    self->running = false;

    /* Post MSG_STOP — unblocks the inner loop immediately (no os_timer_deactivate needed).
     * The thread executes the final EEPROM persist before signalling stopped_sem.
     * Call HourmeterAO_WaitStopped() to synchronize before accessing storage. */
    HourmeterMsg_t msg = {.type = HOURMETER_MSG_STOP};
    (void)os_queue_send(self->queue, &msg, OS_NO_WAIT);

    return ERR_OK;
}

Result_t HourmeterAO_WaitStopped(HourmeterAO_t *self, uint32_t timeout_ms)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t HourmeterAO_Deinit(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    if (self->running)
    {
        return ERR_BUSY; /* Must call Stop() + WaitStopped() first */
    }

    self->terminate = true;
    HourmeterMsg_t msg = {.type = HOURMETER_MSG_TERMINATE};
    (void)os_queue_send(self->queue, &msg, OS_NO_WAIT);

    /* Wait for thread to exit the outer loop */
    os_semaphore_get(self->stopped_sem, OS_WAIT_FOREVER);

    os_semaphore_delete(self->stopped_sem);
    memset(self, 0, sizeof(HourmeterAO_t));
    return ERR_OK;
}

Result_t HourmeterAO_GetData(HourmeterAO_t *self, HourmeterData_t *out_data)
{
    if (!self || !out_data)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->mutex, OS_WAIT_FOREVER);
    Result_t res = Hourmeter_GetData(&self->service, out_data);
    os_mutex_release(self->mutex);

    return res;
}

Result_t HourmeterAO_GetHours(HourmeterAO_t *self,
                              HourmeterType_t type,
                              uint32_t *hours,
                              uint32_t *minutes)
{
    if (!self || !hours || !minutes)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->mutex, OS_WAIT_FOREVER);
    Result_t res = Hourmeter_GetHours(&self->service, type, hours, minutes);
    os_mutex_release(self->mutex);

    return res;
}

Result_t HourmeterAO_ForceSave(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    HourmeterMsg_t msg = {.type = HOURMETER_MSG_FORCE_SAVE};
    return os_queue_send(self->queue, &msg, OS_NO_WAIT);
}

Result_t HourmeterAO_Reset(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    HourmeterMsg_t msg = {.type = HOURMETER_MSG_RESET};
    return os_queue_send(self->queue, &msg, OS_NO_WAIT);
}

Result_t HourmeterAO_ProcessOne(HourmeterAO_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    HourmeterMsg_t msg;
    Result_t res = os_queue_receive(self->queue, &msg, OS_NO_WAIT);
    if (res != ERR_OK)
    {
        return res;
    }

    os_mutex_acquire(self->mutex, OS_WAIT_FOREVER);

    switch (msg.type)
    {
    case HOURMETER_MSG_ACCUMULATE:
        handle_accumulate(self);
        break;
    case HOURMETER_MSG_FORCE_SAVE:
        handle_force_save(self);
        break;
    case HOURMETER_MSG_RESET:
        handle_reset(self);
        break;
    case HOURMETER_MSG_START:
    case HOURMETER_MSG_STOP:
    case HOURMETER_MSG_TERMINATE:
        /* Control messages are no-ops in test context */
        break;
    default:
        break;
    }

    os_mutex_release(self->mutex);

    return ERR_OK;
}
