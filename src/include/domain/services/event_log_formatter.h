/**
 * @file event_log_formatter.h
 * @brief Portable domain service: translates EventLogEntry_t to human-readable text.
 *
 * No LVGL, no HAL — fully testable on PC.
 * Uses DateTime_FromUnix() for timestamp conversion and lwprintf_snprintf for formatting.
 *
 * ## Output format (EventLogFormatter_Format)
 * @code
 *   "DD/MM/YYYY HH:MM:SS\n[SEVERITY] TYPE_NAME\ndetalle extra\n"
 * @endcode
 *
 * @author Tecna Smart Lab
 * @date   8 de Abril 2026
 */
#ifndef EVENT_LOG_FORMATTER_H
#define EVENT_LOG_FORMATTER_H

#include "hal/hal_types.h"
#include "interfaces/i_event_log_storage.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Formats an EventLogEntry_t to a human-readable string.
     *
     * Output format:
     * @code
     *   "DD/MM/YYYY HH:MM:SS\n[SEVERITY] TYPE_NAME\n"
     * @endcode
     *
     * Severity strings:
     *   - EVENT_SEVERITY_INFO     → "INFO"
     *   - EVENT_SEVERITY_WARNING  → "WARN"
     *   - EVENT_SEVERITY_ERROR    → "ERR "
     *   - EVENT_SEVERITY_CRITICAL → "CRIT"
     *
     * @param[in]  entry    Event to format (must not be NULL).
     * @param[out] buf      Destination buffer (must not be NULL).
     * @param[in]  buf_len  Buffer length in bytes.
     *
     * @return ERR_OK always (text is truncated if buf_len is insufficient).
     * @return ERR_NULL_POINTER if entry or buf is NULL.
     */
    Result_t EventLogFormatter_Format(const EventLogEntry_t *entry,
                                      char *buf,
                                      uint32_t buf_len);

    /**
     * @brief Converts a Unix timestamp to "DD/MM/YYYY HH:MM:SS" string.
     *
     * Uses DateTime_FromUnix() + lwprintf_snprintf. No UTC offset applied.
     *
     * @param[in]  ts_unix  Unix timestamp (seconds since 1970-01-01 UTC).
     * @param[out] buf      Destination buffer (must not be NULL).
     * @param[in]  buf_len  Buffer length in bytes.
     */
    void EventLogFormatter_TimestampToStr(uint32_t ts_unix,
                                          char *buf,
                                          uint32_t buf_len);

    /**
     * @brief Returns a human-readable string for an EventType_t value.
     * @param type  Raw EventType_t cast to uint8_t.
     * @return Null-terminated string literal (never NULL).
     */
    const char *EventLogFormatter_GetEventTypeStr(uint8_t type);

    /**
     * @brief Returns a human-readable string for an EventSeverity_t value.
     * @param severity  Raw EventSeverity_t cast to uint8_t.
     * @return Null-terminated string literal (never NULL).
     */
    const char *EventLogFormatter_GetSeverityStr(uint8_t severity);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_LOG_FORMATTER_H */
