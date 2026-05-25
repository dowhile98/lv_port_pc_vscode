/**
 * @file gps_antenna_auto_switch_service.h
 * @brief GPS Antenna Auto-Switch Service — automatically switches antenna when GPS loses fix.
 *
 * ## Behavior
 * - **IDLE:** GPS has fix → no action.
 * - **WAITING:** GPS lost fix → countdown timer active (configurable 1-30 min).
 * - **SWITCHING:** Timeout expired → toggle antenna (INTERNAL ↔ EXTERNAL), save config, apply HW, reset GPS.
 * - **COOLDOWN:** Too many switches in 1 hour → pause for 1 hour before retrying.
 *
 * ## Anti Flip-Flop
 * - Max 4 antenna switches per hour.
 * - If limit reached → enter COOLDOWN (1 hour).
 * - Counter resets on power cycle (not persisted).
 *
 * ## Thread Safety
 * - Call `GpsAntennaAutoSwitchService_Update()` from a single thread (e.g., main loop or control thread).
 * - Polling frequency: 1000 ms (1 Hz) is sufficient.
 *
 * @author Tecna Smart Lab
 * @date 2026-04-15
 */

#ifndef GPS_ANTENNA_AUTO_SWITCH_SERVICE_H
#define GPS_ANTENNA_AUTO_SWITCH_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_gps_control.h"
#include "interfaces/i_config_storage.h"

    /**
     * @brief Service state machine.
     */
    typedef enum
    {
        GPS_AUTO_SWITCH_STATE_IDLE = 0,      /**< GPS has fix → timeout inactive */
        GPS_AUTO_SWITCH_STATE_WAITING = 1,   /**< No fix → counting timeout */
        GPS_AUTO_SWITCH_STATE_SWITCHING = 2, /**< Timeout expired → switching antenna */
        GPS_AUTO_SWITCH_STATE_COOLDOWN = 3   /**< Too many switches → pause 1 hour */
    } GpsAutoSwitchState_t;

    /**
     * @brief Service status (for telemetry/debugging).
     */
    typedef struct
    {
        GpsAutoSwitchState_t state;        /**< Current state */
        uint8_t current_antenna_type;      /**< 0=Internal, 1=External */
        uint32_t time_since_state_entry_s; /**< Seconds in current state */
        uint8_t switch_count_last_hour;    /**< Number of switches in last hour */
        bool auto_switch_enabled;          /**< false if timeout=0 (disabled) */
        uint16_t configured_timeout_min;   /**< Configured timeout in minutes */
    } GpsAntennaAutoSwitchStatus_t;

    /**
     * @brief Dependency bundle for service initialization.
     */
    typedef struct
    {
        IGPSSource *gps_source;         /**< Read fix status */
        IGPSControl *gps_control;       /**< Apply HW config + reset */
        IConfigStorage *config_storage; /**< Load/save GPSConfig_t */
    } GpsAntennaAutoSwitchServiceDeps_t;

    /**
     * @brief Service instance structure.
     *
     * @note Public definition required for static allocation in DI container.
     */
    typedef struct GpsAntennaAutoSwitchService_t
    {
        /* Dependencies */
        IGPSSource *gps_source;
        IGPSControl *gps_control;
        IConfigStorage *config_storage;

        /* State */
        GpsAutoSwitchState_t state;
        uint32_t state_entry_tick;     /**< os_ticks_get() when entering current state */
        uint8_t current_antenna_type;  /**< Cached from GPSConfig_t */
        uint16_t configured_timeout_s; /**< Timeout in seconds (converted from minutes) */

        /* Anti flip-flop */
        uint32_t switch_timestamps_s[4]; /**< Ring buffer of switch times (MAX_SWITCHES_PER_HOUR) */
        uint8_t switch_count;            /**< Number of valid entries in ring buffer */

        /* Flags */
        bool initialized;
    } GpsAntennaAutoSwitchService_t;

    /**
     * @brief Initialize the GPS antenna auto-switch service.
     *
     * @param[in,out] self  Service instance (must be zeroed before first init).
     * @param[in]     deps  Dependencies (all pointers must be valid).
     * @return ERR_OK on success, ERR_NULL_POINTER if deps invalid.
     */
    Result_t GpsAntennaAutoSwitchService_Init(
        GpsAntennaAutoSwitchService_t *self,
        const GpsAntennaAutoSwitchServiceDeps_t *deps);

    /**
     * @brief Update service logic (call periodically, e.g., 1 Hz from main loop).
     *
     * @param[in,out] self  Service instance.
     * @return ERR_OK on success.
     *
     * @note NOT thread-safe — call from a single thread only.
     * @note Blocking during SWITCHING state (~1.1s for GPS reset).
     */
    Result_t GpsAntennaAutoSwitchService_Update(GpsAntennaAutoSwitchService_t *self);

    /**
     * @brief Get current service status (for telemetry/UI).
     *
     * @param[in]  self       Service instance.
     * @param[out] out_status Status structure.
     * @return ERR_OK on success, ERR_NULL_POINTER if self/out_status NULL.
     */
    Result_t GpsAntennaAutoSwitchService_GetStatus(
        const GpsAntennaAutoSwitchService_t *self,
        GpsAntennaAutoSwitchStatus_t *out_status);

    /**
     * @brief Force an immediate antenna switch (manual override).
     *
     * @param[in,out] self  Service instance.
     * @return ERR_OK on success, ERR_BUSY if already switching.
     *
     * @note Does NOT count against flip-flop limit (manual actions exempt).
     * @note Blocking (~1.1s for GPS reset).
     */
    Result_t GpsAntennaAutoSwitchService_ForceSwitch(GpsAntennaAutoSwitchService_t *self);

#ifdef __cplusplus
}
#endif

#endif /* GPS_ANTENNA_AUTO_SWITCH_SERVICE_H */
