/**
 * @file gps_antenna_auto_switch_service.c
 * @brief GPS Antenna Auto-Switch Service — automatically switches antenna when GPS loses fix.
 *
 * ## State Machine
 * ```
 * IDLE (GPS has fix)
 *   ↓ GPS loses fix
 * WAITING (timeout counting)
 *   ↓ timeout expires
 * SWITCHING (toggle antenna → save → apply HW → reset GPS)
 *   ↓ switch complete
 * IDLE (restart cycle)
 *
 * If 4 switches in 1 hour:
 *   → COOLDOWN (1 hour) → IDLE
 * ```
 *
 * ## Anti Flip-Flop Logic
 * - Tracks timestamps of last 4 switches.
 * - If all 4 timestamps are within 1 hour → enter COOLDOWN.
 * - Cooldown lasts 1 hour, then resets counter.
 *
 * @author Tecna Smart Lab
 * @date 2026-04-15
 */

#include "application/services/gps_antenna_auto_switch_service.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_gps_control.h"
#include "interfaces/i_config_storage.h"
#include "common/gps_types.h"
#include "osal/osal.h"
#include <string.h>

/*============================================================================*
 * CONSTANTS
 *============================================================================*/

#define MAX_SWITCHES_PER_HOUR 4U        /**< Max antenna switches per hour (anti flip-flop) */
#define COOLDOWN_DURATION_S (60U * 60U) /**< Cooldown duration: 1 hour */
#define ONE_HOUR_S (60U * 60U)          /**< 1 hour in seconds */

/*============================================================================*
 * PRIVATE HELPERS
 *============================================================================*/

/**
 * @brief Get current system time in seconds (for timestamp comparison).
 * @return System uptime in seconds.
 */
static inline uint32_t get_current_time_s(void)
{
    return os_ticks_get() / 1000U; /* Convert ms to seconds */
}

/**
 * @brief Check if GPS currently has valid fix (2D or 3D).
 * @param[in] self  Service instance.
 * @return true if GPS has fix, false otherwise.
 */
static bool has_gps_fix(const GpsAntennaAutoSwitchService_t *self)
{
    GPSFixStatus_t fix_status = GPS_FIX_NONE;
    if (GPS_Source_GetFixStatus(self->gps_source, &fix_status) == ERR_OK)
    {
        return (fix_status == GPS_FIX_2D || fix_status == GPS_FIX_3D);
    }
    return false;
}

/**
 * @brief Load current GPSConfig from storage and cache relevant fields.
 * @param[in,out] self  Service instance.
 */
static void load_config(GpsAntennaAutoSwitchService_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    if (ConfigStorage_LoadGPSConfig(self->config_storage, &cfg) == ERR_OK)
    {
        self->current_antenna_type = cfg.antenna_type;

        /* Convert timeout from minutes to seconds; clamp to 60 min max */
        if (cfg.antenna_switch_timeout_min == 0U)
        {
            self->configured_timeout_s = 0U; /* Disabled */
        }
        else if (cfg.antenna_switch_timeout_min > 60U)
        {
            self->configured_timeout_s = 60U * 60U; /* clamp to max 60 min */
        }
        else
        {
            self->configured_timeout_s = cfg.antenna_switch_timeout_min * 60U;
        }
    }
    else
    {
        /* Fallback defaults if load fails */
        self->current_antenna_type = GPS_ANTENNA_INTERNAL;
        self->configured_timeout_s = 0U; /* Disabled by default if load fails */
    }
}

/**
 * @brief Record a new antenna switch timestamp (for flip-flop detection).
 * @param[in,out] self  Service instance.
 */
static void record_switch_timestamp(GpsAntennaAutoSwitchService_t *self)
{
    const uint32_t now_s = get_current_time_s();

    /* Shift old timestamps and insert new one at index 0 */
    for (uint8_t i = MAX_SWITCHES_PER_HOUR - 1U; i > 0U; i--)
    {
        self->switch_timestamps_s[i] = self->switch_timestamps_s[i - 1U];
    }
    self->switch_timestamps_s[0] = now_s;

    if (self->switch_count < MAX_SWITCHES_PER_HOUR)
    {
        self->switch_count++;
    }
}

/**
 * @brief Check if flip-flop limit has been reached (4 switches in last hour).
 * @param[in] self  Service instance.
 * @return true if limit reached, false otherwise.
 */
static bool flip_flop_limit_reached(const GpsAntennaAutoSwitchService_t *self)
{
    if (self->switch_count < MAX_SWITCHES_PER_HOUR)
    {
        return false; /* Not enough switches yet */
    }

    const uint32_t now_s = get_current_time_s();
    const uint32_t oldest_switch_s = self->switch_timestamps_s[MAX_SWITCHES_PER_HOUR - 1U];

    /* If oldest switch is within 1 hour → limit reached */
    return ((now_s - oldest_switch_s) < ONE_HOUR_S);
}

/**
 * @brief Reset the flip-flop counter (after cooldown or power cycle).
 * @param[in,out] self  Service instance.
 */
static void reset_flip_flop_counter(GpsAntennaAutoSwitchService_t *self)
{
    memset(self->switch_timestamps_s, 0, sizeof(self->switch_timestamps_s));
    self->switch_count = 0U;
}

/**
 * @brief Perform antenna switch: toggle type → save → apply HW → reset GPS.
 * @param[in,out] self  Service instance.
 * @param[in] count_for_flip_flop  true = count this switch, false = manual override (exempt).
 * @return ERR_OK on success.
 *
 * @note Blocking (~1.1s for GPS reset).
 */
static Result_t perform_antenna_switch(GpsAntennaAutoSwitchService_t *self, bool count_for_flip_flop)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    /* Load current config */
    Result_t res = ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Toggle antenna type */
    cfg.antenna_type = (cfg.antenna_type == GPS_ANTENNA_INTERNAL)
                           ? GPS_ANTENNA_EXTERNAL
                           : GPS_ANTENNA_INTERNAL;

    /* Save to storage */
    res = ConfigStorage_SaveGPSConfig(self->config_storage, &cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Update cached value */
    self->current_antenna_type = cfg.antenna_type;

    /* Apply hardware configuration (GPIO changes) */
    res = GPS_Control_ApplyHardwareConfig(self->gps_control);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Reset GPS hardware (blocking ~1.1s) */
    res = GPS_Control_Reset(self->gps_control);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Record timestamp for flip-flop detection (if not manual) */
    if (count_for_flip_flop)
    {
        record_switch_timestamp(self);
    }

    return ERR_OK;
}

/**
 * @brief Change state and record entry timestamp.
 * @param[in,out] self      Service instance.
 * @param[in]     new_state New state to enter.
 */
static void change_state(GpsAntennaAutoSwitchService_t *self, GpsAutoSwitchState_t new_state)
{
    self->state = new_state;
    self->state_entry_tick = os_ticks_get();
}

/*============================================================================*
 * PUBLIC API
 *============================================================================*/

Result_t GpsAntennaAutoSwitchService_Init(
    GpsAntennaAutoSwitchService_t *self,
    const GpsAntennaAutoSwitchServiceDeps_t *deps)
{
    if (!self || !deps)
    {
        return ERR_NULL_POINTER;
    }
    if (!deps->gps_source || !deps->gps_control || !deps->config_storage)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));

    /* Inject dependencies */
    self->gps_source = deps->gps_source;
    self->gps_control = deps->gps_control;
    self->config_storage = deps->config_storage;

    /* Load initial config */
    load_config(self);

    /* Start in IDLE state */
    change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);

    self->initialized = true;

    return ERR_OK;
}

Result_t GpsAntennaAutoSwitchService_Update(GpsAntennaAutoSwitchService_t *self)
{
    if (!self || !self->initialized)
    {
        return ERR_NULL_POINTER;
    }

    const uint32_t now_tick = os_ticks_get();
    const uint32_t elapsed_ms = now_tick - self->state_entry_tick;
    const bool gps_has_fix = has_gps_fix(self);

    switch (self->state)
    {
    case GPS_AUTO_SWITCH_STATE_IDLE:
        /* GPS has fix → stay idle */
        if (!gps_has_fix && self->configured_timeout_s > 0U)
        {
            /* GPS lost fix and auto-switch enabled → start timeout */
            change_state(self, GPS_AUTO_SWITCH_STATE_WAITING);
        }
        break;

    case GPS_AUTO_SWITCH_STATE_WAITING:
        /* GPS recovered fix → return to idle */
        if (gps_has_fix)
        {
            change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);
        }
        /* Timeout expired → switch antenna */
        else if (elapsed_ms >= (self->configured_timeout_s * 1000U))
        {
            /* Check flip-flop limit before switching */
            if (flip_flop_limit_reached(self))
            {
                /* Too many switches → cooldown */
                change_state(self, GPS_AUTO_SWITCH_STATE_COOLDOWN);
            }
            else
            {
                /* Perform switch */
                change_state(self, GPS_AUTO_SWITCH_STATE_SWITCHING);
            }
        }
        break;

    case GPS_AUTO_SWITCH_STATE_SWITCHING:
        /* Execute antenna switch (blocking) */
        (void)perform_antenna_switch(self, true); /* count for flip-flop */

        /* Reload config in case timeout was changed during switch */
        load_config(self);

        /* Return to IDLE and restart cycle */
        change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);
        break;

    case GPS_AUTO_SWITCH_STATE_COOLDOWN:
        /* Wait for 1 hour before resetting counter */
        if (elapsed_ms >= (COOLDOWN_DURATION_S * 1000U))
        {
            reset_flip_flop_counter(self);
            change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);
        }
        break;

    default:
        /* Invalid state → reset to IDLE */
        change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);
        break;
    }

    return ERR_OK;
}

Result_t GpsAntennaAutoSwitchService_GetStatus(
    const GpsAntennaAutoSwitchService_t *self,
    GpsAntennaAutoSwitchStatus_t *out_status)
{
    if (!self || !out_status)
    {
        return ERR_NULL_POINTER;
    }

    const uint32_t elapsed_ms = os_ticks_get() - self->state_entry_tick;

    out_status->state = self->state;
    out_status->current_antenna_type = self->current_antenna_type;
    out_status->time_since_state_entry_s = elapsed_ms / 1000U;
    out_status->switch_count_last_hour = self->switch_count;
    out_status->auto_switch_enabled = (self->configured_timeout_s > 0U);
    out_status->configured_timeout_min = self->configured_timeout_s / 60U;

    return ERR_OK;
}

Result_t GpsAntennaAutoSwitchService_ForceSwitch(GpsAntennaAutoSwitchService_t *self)
{
    if (!self || !self->initialized)
    {
        return ERR_NULL_POINTER;
    }

    /* Don't allow force switch during active switch */
    if (self->state == GPS_AUTO_SWITCH_STATE_SWITCHING)
    {
        return ERR_BUSY;
    }

    /* Perform switch without counting for flip-flop (manual override exempt) */
    const Result_t res = perform_antenna_switch(self, false);

    if (res == ERR_OK)
    {
        /* Reload config */
        load_config(self);

        /* Return to IDLE */
        change_state(self, GPS_AUTO_SWITCH_STATE_IDLE);
    }

    return res;
}
