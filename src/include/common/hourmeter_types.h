/**
 * @file hourmeter_types.h
 * @brief Hourmeter data types for persistent accumulation of operating hours.
 *
 * @details
 * Defines HourmeterData_t (64 bytes, packed) used for EEPROM persistence
 * with 4 wear-leveling slots. Layout v4.0 places hourmeter at 0x0B00.
 *
 * Accumulators:
 *   - power_on:       Always running while system is powered.
 *   - gps_connected:  Running when GPS has 3D fix.
 *   - relay_active:   Running when relay is in ON cycle.
 *   - time_window:    Running when inside configured time window.
 *
 * @version 1.0.0
 * @date    2026-03-23
 */

#ifndef HOURMETER_TYPES_H
#define HOURMETER_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ── Constants ───────────────────────────────────────────────────────────── */

#define HOURMETER_MAGIC 0xC1C10800U    /**< CICX1 Hourmeter v1.0 */
#define HOURMETER_DATA_SIZE 56U        /**< Payload size (total - header) */
#define HOURMETER_MINUTES_PER_HOUR 60U /**< Rollover threshold */

    /* ── Accumulator Selector ────────────────────────────────────────────────── */

    /**
     * @brief Selects which hourmeter accumulator to query.
     */
    typedef enum
    {
        HOURMETER_TYPE_POWER_ON = 0,  /**< Total power-on time */
        HOURMETER_TYPE_GPS_CONNECTED, /**< Time with GPS 3D fix */
        HOURMETER_TYPE_RELAY_ACTIVE,  /**< Time with relay active */
        HOURMETER_TYPE_TIME_WINDOW,   /**< Time inside configured window */
        HOURMETER_TYPE_COUNT          /**< Number of accumulator types */
    } HourmeterType_t;

    /* ── Persistent Data Structure ───────────────────────────────────────────── */

    /**
     * @brief Hourmeter persistent data (64 bytes, packed).
     *
     * @note  Stored in EEPROM with 4-slot wear leveling at 0x0B00.
     *        CRC16-CCITT covers bytes [8..63] (everything after header).
     */
    typedef struct __attribute__((packed))
    {
        /* ── Header (8 bytes) ─────────────────────────────────────────────── */
        uint32_t magic;    /**< HOURMETER_MAGIC (0xC1C10800) */
        uint16_t size;     /**< Payload size (HOURMETER_DATA_SIZE = 56) */
        uint16_t checksum; /**< CRC16-CCITT over data [8..63] */

        /* ── Accumulators (32 bytes) ──────────────────────────────────────── */
        uint32_t power_on_hours;   /**< Total hours powered on (0–1,193,046 = 136 yr) */
        uint32_t power_on_minutes; /**< Additional minutes (0–59) */

        uint32_t gps_connected_hours;   /**< Hours with GPS 3D fix */
        uint32_t gps_connected_minutes; /**< Additional minutes (0–59) */

        uint32_t relay_active_hours;   /**< Hours relay in ON cycle */
        uint32_t relay_active_minutes; /**< Additional minutes (0–59) */

        uint32_t time_window_hours;   /**< Hours inside time window */
        uint32_t time_window_minutes; /**< Additional minutes (0–59) */

        /* ── Metadata (8 bytes) ───────────────────────────────────────────── */
        uint32_t last_update_timestamp; /**< Epoch UTC of last update */
        uint16_t write_count;           /**< EEPROM write counter (wear tracking) */
        uint8_t reserved[2];            /**< Reserved for expansion */

        /* ── Expansion (16 bytes) ─────────────────────────────────────────── */
        uint8_t _expansion[16]; /**< Future: daily history, etc. */

    } HourmeterData_t;

    _Static_assert(sizeof(HourmeterData_t) == 64, "HourmeterData_t must be exactly 64 bytes");

    /* ── Inline Helpers ──────────────────────────────────────────────────────── */

    /**
     * @brief Initialize a HourmeterData_t to default (zeroed, with magic + size).
     */
    static inline void HourmeterData_InitDefaults(HourmeterData_t *data)
    {
        if (!data)
        {
            return;
        }
        uint8_t *p = (uint8_t *)data;
        for (uint32_t i = 0; i < sizeof(HourmeterData_t); i++)
        {
            p[i] = 0;
        }
        data->magic = HOURMETER_MAGIC;
        data->size = HOURMETER_DATA_SIZE;
    }

#ifdef __cplusplus
}
#endif

#endif /* HOURMETER_TYPES_H */
