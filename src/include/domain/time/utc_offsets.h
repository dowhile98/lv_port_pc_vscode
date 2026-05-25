/**
 * @file utc_offsets.h
 * @brief UTC timezone offset array (ACTION-007 Phase 4).
 *
 * Domain Layer - Contains UTC offset constants for global timezone configuration.
 */

#ifndef UTC_OFFSETS_H
#define UTC_OFFSETS_H

#define UTC_OFFSET_COUNT 38

/**
 * @brief UTC offset array (hours).
 * @note Index 14 = UTC+0 (Greenwich Mean Time).
 *
 * Usage example:
 *   float offset_hours = UTC_OFFSETS[utc_offset_index];
 *   int32_t offset_seconds = (int32_t)(offset_hours * 3600.0f);
 */
extern const float UTC_OFFSETS[UTC_OFFSET_COUNT];

#endif /* UTC_OFFSETS_H */
