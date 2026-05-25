/**
 * @file led_status_service.c
 * @brief LED Status Indicator Service — implementation.
 *
 * @details
 * Priority evaluation (highest first):
 *   1. relay.alarm_active           → LED_STATUS_ALARM_HIGH_TEMP
 *   2. license == RENTAL_EXPIRED    → LED_STATUS_LICENSE_EXPIRED
 *   3. relay.internal_state == ERROR→ LED_STATUS_RELAY_ERROR
 *   4. relay.internal_state == ACTIVE→ LED_STATUS_RELAY_ACTIVE
 *   5. relay.WAITING + is_synchronized → LED_STATUS_RELAY_WAITING
 *   6. gps fix != 3D (or gps==NULL) → LED_STATUS_NO_GPS_SYNC
 *   7. (fallback)                   → LED_STATUS_IDLE
 *
 * Blink patterns:
 *   ALARM_HIGH_TEMP  100/100ms continuous fast blink
 *   LICENSE_EXPIRED  3× 200ms pulse, 2 s pause
 *   RELAY_ERROR      2× 200ms pulse, 1 s pause
 *   RELAY_ACTIVE     200ms ON / 800ms OFF continuous
 *   RELAY_WAITING    Solid ON
 *   NO_GPS_SYNC      500ms ON / 1500ms OFF continuous
 *   IDLE             200ms ON / 3800ms OFF (heartbeat)
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "application/services/led_status_service.h"
#include "infrastructure/osal/osal.h" /* os_ticks_get() */
#include <string.h>

/*============================================================================*
 * PRIVATE TYPES
 *============================================================================*/

/**
 * @brief Encodes one blink pattern entry.
 *
 * @field on_ms     LED-ON duration per pulse (ms).  Ignored when solid == true.
 * @field off_ms    LED-OFF gap between pulses  (ms). Ignored when solid == true.
 * @field pulses    Pulses per burst. 0 = continuous (no grouping, no pause).
 * @field pause_ms  OFF duration after a completed burst (only when pulses > 0).
 * @field solid     true = LED always ON; blink machine bypassed.
 */
typedef struct
{
    uint16_t on_ms;
    uint16_t off_ms;
    uint8_t pulses;
    uint16_t pause_ms;
    bool solid;
} LedBlinkPattern_t;

/*============================================================================*
 * PRIVATE DATA — Pattern Table
 *============================================================================*/

/**
 * @brief Blink pattern for each LedStatusState_t (indexed by state value).
 *
 * @note Array is ROM-resident (const).
 */
/*
 * Pattern rationale (all periods clearly distinguishable by eye):
 *
 *  ALARM_HIGH_TEMP  — 50/50 ms  very fast strobe   (~10 Hz)  immediate panic signal
 *  LICENSE_EXPIRED  — 5× 100ms pulse burst, 3 s pause        counted pulses, long gap
 *  RELAY_ERROR      — 3× 200ms pulse burst, 2 s pause        3-blink SOS-style, long gap
 *  RELAY_ACTIVE     — 100ms ON / 400ms OFF continuous        rapid single flash (~2 Hz)
 *  RELAY_WAITING    — solid ON                                steady = ready
 *  NO_GPS_SYNC      — 600ms ON / 1400ms OFF continuous       slow double-beat (~0.5 Hz)
 *  IDLE             — 100ms ON / 4900ms OFF heartbeat        barely-there ~5 s period
 */
static const LedBlinkPattern_t s_patterns[LED_STATUS_STATE_COUNT] = {
    /* [LED_STATUS_ALARM_HIGH_TEMP] */ {.on_ms = 50U, .off_ms = 50U, .pulses = 0U, .pause_ms = 0U, .solid = false},
    /* [LED_STATUS_LICENSE_EXPIRED] */ {.on_ms = 100U, .off_ms = 150U, .pulses = 5U, .pause_ms = 3000U, .solid = false},
    /* [LED_STATUS_RELAY_ERROR]     */ {.on_ms = 200U, .off_ms = 200U, .pulses = 3U, .pause_ms = 2000U, .solid = false},
    /* [LED_STATUS_RELAY_ACTIVE]    */ {.on_ms = 100U, .off_ms = 400U, .pulses = 0U, .pause_ms = 0U, .solid = false},
    /* [LED_STATUS_RELAY_WAITING]   */ {.on_ms = 0U, .off_ms = 0U, .pulses = 0U, .pause_ms = 0U, .solid = true},
    /* [LED_STATUS_NO_GPS_SYNC]     */ {.on_ms = 600U, .off_ms = 1400U, .pulses = 0U, .pause_ms = 0U, .solid = false},
    /* [LED_STATUS_IDLE]            */ {.on_ms = 100U, .off_ms = 4900U, .pulses = 0U, .pause_ms = 0U, .solid = false},
};

/*============================================================================*
 * PRIVATE HELPER FUNCTIONS — GPIO
 *============================================================================*/

static void led_set_high(const LedStatusService_t *self)
{
    if (self->led_enabled && self->gpio_iface != NULL)
    {
        (void)GPIO_WritePin(self->gpio_iface, self->led_port, self->led_pin, I_GPIO_STATE_HIGH);
    }
}

static void led_set_low(const LedStatusService_t *self)
{
    if (self->led_enabled && self->gpio_iface != NULL)
    {
        (void)GPIO_WritePin(self->gpio_iface, self->led_port, self->led_pin, I_GPIO_STATE_LOW);
    }
}

/*============================================================================*
 * PRIVATE HELPER FUNCTIONS — Priority Computation
 *============================================================================*/

/**
 * @brief Evaluate all observable conditions and return the highest-priority state.
 *
 * @param[in] self  Initialized service instance.
 * @return Highest-matching LedStatusState_t.
 */
static LedStatusState_t compute_priority_state(const LedStatusService_t *self)
{
    RelayStatus_t relay_status;
    bool relay_ok = false;

    if (self->relay != NULL)
    {
        relay_ok = (RelayController_GetStatus(self->relay, &relay_status) == ERR_OK);
    }

    /* ── Priority 1: Alarm active (over-temperature safety) ─────────────── */
    if (relay_ok && relay_status.alarm_active)
    {
        return LED_STATUS_ALARM_HIGH_TEMP;
    }

    /* ── Priority 2: License expired ────────────────────────────────────── */
    if (self->license != NULL)
    {
        LicenseStatus_t ls;
        if (LicenseStatus_GetStatus(self->license, &ls) == ERR_OK)
        {
            if (ls.mode == LICENSE_MODE_RENTAL_EXPIRED)
            {
                return LED_STATUS_LICENSE_EXPIRED;
            }
        }
    }

    /* ── Priority 3: Relay FSM in unrecoverable error ───────────────────── */
    if (relay_ok && relay_status.internal_state == RELAY_STATE_ERROR)
    {
        return LED_STATUS_RELAY_ERROR;
    }

    /* ── Priority 4: Relay actively cycling (executing interruptions) ───── */
    if (relay_ok && relay_status.internal_state == RELAY_STATE_ACTIVE)
    {
        return LED_STATUS_RELAY_ACTIVE;
    }

    /* ── Priority 5: Relay waiting (synchronized, inside normal operation) ─ */
    if (relay_ok && relay_status.internal_state == RELAY_STATE_WAITING && relay_status.is_synchronized)
    {
        return LED_STATUS_RELAY_WAITING;
    }

    /* ── Priority 6: No valid GPS 3D fix ────────────────────────────────── */
    if (self->gps != NULL)
    {
        GPSFixStatus_t fix = GPS_FIX_NONE;
        if (GPS_Source_GetFixStatus(self->gps, &fix) == ERR_OK)
        {
            if (fix != GPS_FIX_3D)
            {
                return LED_STATUS_NO_GPS_SYNC;
            }
        }
        else
        {
            return LED_STATUS_NO_GPS_SYNC; /* Treat query failure as no fix. */
        }
    }
    else
    {
        return LED_STATUS_NO_GPS_SYNC; /* No GPS source injected → no fix assumed. */
    }

    return LED_STATUS_IDLE;
}

/*============================================================================*
 * PRIVATE HELPER FUNCTIONS — Blink State Machine
 *============================================================================*/

/**
 * @brief Arm the blink machine for a new state, starting the first pulse (LED ON).
 *
 * @param[in,out] self    Service instance.
 * @param[in]     state   New active state.
 * @param[in]     now_ms  Current tick counter (ms).
 */
static void arm_blink_machine(LedStatusService_t *self,
                              LedStatusState_t state,
                              uint32_t now_ms)
{
    const LedBlinkPattern_t *p = &s_patterns[state];

    self->current_state = state;
    self->pulse_count = 0U;

    if (p->solid)
    {
        self->blink_phase = LED_BLINK_PHASE_ON;
        self->phase_deadline_ms = UINT32_MAX; /* Effectively never */
        led_set_high(self);
    }
    else
    {
        self->blink_phase = LED_BLINK_PHASE_ON;
        self->phase_deadline_ms = now_ms + p->on_ms;
        led_set_high(self);
    }
}

/**
 * @brief Advance the blink machine by one tick; apply GPIO changes when deadline fires.
 *
 * @param[in,out] self    Service instance.
 * @param[in]     now_ms  Current tick counter (ms).
 */
static void tick_blink_machine(LedStatusService_t *self, uint32_t now_ms)
{
    const LedBlinkPattern_t *p = &s_patterns[self->current_state];

    if (p->solid)
    {
        led_set_high(self); /* Re-assert in case of glitch */
        return;
    }

    /* Cast to signed for wraparound-safe comparison */
    if ((int32_t)(now_ms - self->phase_deadline_ms) < 0)
    {
        return; /* Deadline has not arrived yet */
    }

    switch (self->blink_phase)
    {
    case LED_BLINK_PHASE_ON:
    {
        led_set_low(self);

        if (p->pulses == 0U)
        {
            /* Continuous blink: transition directly to OFF phase */
            self->blink_phase = LED_BLINK_PHASE_OFF;
            self->phase_deadline_ms = now_ms + p->off_ms;
        }
        else
        {
            self->pulse_count++;
            if (self->pulse_count < p->pulses)
            {
                /* More pulses remain in this burst */
                self->blink_phase = LED_BLINK_PHASE_OFF;
                self->phase_deadline_ms = now_ms + p->off_ms;
            }
            else
            {
                /* Burst complete: enter long pause */
                self->pulse_count = 0U;
                self->blink_phase = LED_BLINK_PHASE_PAUSE;
                self->phase_deadline_ms = now_ms + p->pause_ms;
            }
        }
        break;
    }

    case LED_BLINK_PHASE_OFF:
    {
        /* Start the next pulse */
        led_set_high(self);
        self->blink_phase = LED_BLINK_PHASE_ON;
        self->phase_deadline_ms = now_ms + p->on_ms;
        break;
    }

    case LED_BLINK_PHASE_PAUSE:
    {
        /* Pause over: start a new burst */
        led_set_high(self);
        self->pulse_count = 0U;
        self->blink_phase = LED_BLINK_PHASE_ON;
        self->phase_deadline_ms = now_ms + p->on_ms;
        break;
    }

    default:
        break;
    }
}

/*============================================================================*
 * PUBLIC API
 *============================================================================*/

Result_t LedStatusService_Init(LedStatusService_t *self, const LedStatusServiceConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->led_enabled && config->gpio_iface == NULL)
    {
        return ERR_INVALID_PARAM;
    }

    (void)memset(self, 0, sizeof(*self));

    self->gpio_iface = config->gpio_iface;
    self->led_port = config->led_port;
    self->led_pin = config->led_pin;
    self->led_enabled = config->led_enabled;
    self->relay = config->relay;
    self->gps = config->gps;
    self->license = config->license;
    self->logger = config->logger; /* Can be NULL */

    /* Start with LED off */
    led_set_low(self);

    /* Arm state machine in IDLE state */
    arm_blink_machine(self, LED_STATUS_IDLE, os_ticks_get());

    self->is_initialized = true;

    return ERR_OK;
}

void LedStatusService_Update(LedStatusService_t *self)
{
    if (self == NULL || !self->is_initialized)
    {
        return;
    }

    uint32_t now_ms = os_ticks_get();

    LedStatusState_t new_state = compute_priority_state(self);

    if (new_state != self->current_state)
    {
        /* Priority changed: reset blink machine for the new state */
        if (self->logger)
        {
            static const char *state_names[] = {
                "ALARM_HIGH_TEMP", "LICENSE_EXPIRED", "RELAY_ERROR",
                "RELAY_ACTIVE", "RELAY_WAITING", "NO_GPS_SYNC", "IDLE"};
            if (new_state < LED_STATUS_STATE_COUNT)
            {
                Logger_Log(self->logger, LOG_LEVEL_DEBUG, "LED_STATUS", __FILE__, __LINE__,
                           "State transition: %s -> %s",
                           (self->current_state < LED_STATUS_STATE_COUNT) ? state_names[self->current_state] : "UNKNOWN",
                           state_names[new_state]);
            }
        }
        arm_blink_machine(self, new_state, now_ms);
        return;
    }

    tick_blink_machine(self, now_ms);
}

LedStatusState_t LedStatusService_GetCurrentState(const LedStatusService_t *self)
{
    if (self == NULL || !self->is_initialized)
    {
        return LED_STATUS_IDLE;
    }
    return self->current_state;
}
