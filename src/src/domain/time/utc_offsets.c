/**
 * @file utc_offsets.c
 * @brief UTC timezone offset array implementation (ACTION-007 Phase 4).
 */

#include "domain/time/utc_offsets.h"

/**
 * @brief UTC offset array covering major timezones.
 *
 * Index mapping:
 *   0  = UTC-12 (Baker Island)
 *   4  = UTC-9  (Alaska)
 *   8  = UTC-5  (Eastern Time)
 *   14 = UTC+0  (GMT/UTC)
 *   22 = UTC+5  (Pakistan)
 *   30 = UTC+9  (Japan/Korea)
 *   34 = UTC+12 (New Zealand)
 */
const float UTC_OFFSETS[UTC_OFFSET_COUNT] = {
    -12.0f, -11.0f, -10.0f, -9.5f, -9.0f, -8.0f, -7.0f, -6.0f, -5.0f,
    -4.0f, -3.5f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f,
    3.5f, 4.0f, 4.5f, 5.0f, 5.5f, 5.75f, 6.0f, 6.5f, 7.0f,
    8.0f, 8.75f, 9.0f, 9.5f, 10.0f, 10.5f, 11.0f, 12.0f, 12.75f,
    13.0f, 14.0f};
