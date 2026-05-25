/**
 * @file pc_event_log_storage.c
 * @brief In-memory IEventLogStorage for the PC simulator.
 *
 * Replaces the EEPROM event log (EventLogAO) with a simple circular array.
 * Pre-populated with representative events so the Historical Events screen
 * shows meaningful data on first launch.
 *
 * Thread-safety: all calls come from the UI-AO thread — no locking needed.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "pc_event_log_storage.h"

#include <string.h>
#include <stdint.h>

/* ============================================================================
 * CONSTANTS
 * ========================================================================== */

/** Maximum events kept in memory (same as firmware circular buffer). */
#define PC_EL_MAX_ENTRIES 100U

/* ============================================================================
 * INTERNAL STATE
 * ========================================================================== */

typedef struct
{
    EventLogEntry_t buf[PC_EL_MAX_ENTRIES];
    uint16_t count; /**< number of valid entries (0..PC_EL_MAX_ENTRIES) */
    uint16_t head;  /**< next write index (circular) */
    bool initialised;
} PcEventLogState_t;

static PcEventLogState_t s_state;

/* ============================================================================
 * SAMPLE DATA
 * ========================================================================== */

/*
 * Base timestamp: 2026-05-25 00:00:00 UTC  =  1748131200
 * Events are spaced ~1 h apart going backwards in time.
 */
#define BASE_TS 1748131200U
#define H1 3600U

static const EventLogEntry_t k_seed_events[] = {
    /* ts                          type  sev  info  params  crc */
    {BASE_TS - 10U * H1, 0, 0, 0, 0, 0}, /* System Boot        */
    {BASE_TS - 9U * H1, 2, 0, 0, 0, 0},  /* GPS Lock Acquired  */
    {BASE_TS - 8U * H1, 24, 0, 0, 0, 0}, /* Config General     */
    {BASE_TS - 7U * H1, 5, 0, 1, 0, 0},  /* Relay Enabled      */
    {BASE_TS - 6U * H1, 13, 0, 0, 0, 0}, /* GPS Time Sync      */
    {BASE_TS - 5U * H1, 7, 0, 0, 0, 0},  /* Relay Cycle Start  */
    {BASE_TS - 4U * H1, 8, 0, 0, 0, 0},  /* Relay Cycle End    */
    {BASE_TS - 3U * H1, 22, 0, 0, 0, 0}, /* Config Relay       */
    {BASE_TS - 2U * H1, 3, 1, 0, 0, 0},  /* GPS Lock Lost      */
    {BASE_TS - 1U * H1, 41, 0, 0, 0, 0}, /* WiFi Connected     */
};

#define SEED_COUNT ((uint16_t)(sizeof(k_seed_events) / sizeof(k_seed_events[0])))

/* ============================================================================
 * INITIALISATION
 * ========================================================================== */

static void init_state(void)
{
    if (s_state.initialised)
    {
        return;
    }
    memset(&s_state, 0, sizeof(s_state));
    uint16_t n = (SEED_COUNT < PC_EL_MAX_ENTRIES) ? SEED_COUNT : PC_EL_MAX_ENTRIES;
    for (uint16_t i = 0U; i < n; i++)
    {
        s_state.buf[i] = k_seed_events[i];
    }
    s_state.count = n;
    s_state.head = n % PC_EL_MAX_ENTRIES;
    s_state.initialised = true;
}

/* ============================================================================
 * VTABLE IMPLEMENTATIONS
 * ========================================================================== */

static Result_t pc_el_append(void *self, const EventLogEntry_t *event)
{
    (void)self;
    init_state();
    if (event == NULL)
    {
        return ERR_NULL_POINTER;
    }
    s_state.buf[s_state.head] = *event;
    s_state.head = (uint16_t)((s_state.head + 1U) % PC_EL_MAX_ENTRIES);
    if (s_state.count < PC_EL_MAX_ENTRIES)
    {
        s_state.count++;
    }
    return ERR_OK;
}

static Result_t pc_el_read_by_index(void *self, uint16_t index, EventLogEntry_t *out_event)
{
    (void)self;
    init_state();
    if (out_event == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (index >= s_state.count)
    {
        return ERR_INVALID_PARAM;
    }
    /* Index 0 = oldest.  The oldest entry is at:
     * (head - count + PC_EL_MAX_ENTRIES) % PC_EL_MAX_ENTRIES  */
    uint16_t start = (uint16_t)((s_state.head + PC_EL_MAX_ENTRIES - s_state.count) % PC_EL_MAX_ENTRIES);
    uint16_t actual = (uint16_t)((start + index) % PC_EL_MAX_ENTRIES);
    *out_event = s_state.buf[actual];
    return ERR_OK;
}

static Result_t pc_el_get_count(void *self, uint16_t *out_count)
{
    (void)self;
    init_state();
    if (out_count == NULL)
    {
        return ERR_NULL_POINTER;
    }
    *out_count = s_state.count;
    return ERR_OK;
}

static Result_t pc_el_get_metadata(void *self, EventLogMeta_t *out_meta)
{
    (void)self;
    init_state();
    if (out_meta == NULL)
    {
        return ERR_NULL_POINTER;
    }
    out_meta->head = s_state.head;
    out_meta->count = s_state.count;
    out_meta->checksum = 0U;
    return ERR_OK;
}

static Result_t pc_el_clear(void *self)
{
    (void)self;
    init_state();
    s_state.count = 0U;
    s_state.head = 0U;
    return ERR_OK;
}

/* ============================================================================
 * SINGLETON
 * ========================================================================== */

static const IEventLogStorage_Vtable k_vtable = {
    .Append = pc_el_append,
    .ReadByIndex = pc_el_read_by_index,
    .GetCount = pc_el_get_count,
    .GetMetadata = pc_el_get_metadata,
    .Clear = pc_el_clear,
};

static IEventLogStorage s_iface = {
    .vtable = &k_vtable,
    .impl = &s_state, /* non-NULL required by event_log_storage_is_valid() */
};

IEventLogStorage *PCEventLogStorage_GetInstance(void)
{
    init_state();
    return &s_iface;
}
