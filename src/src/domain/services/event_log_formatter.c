/**
 * @file event_log_formatter.c
 * @brief Portable domain service: translates EventLogEntry_t to human-readable text.
 *
 * No LVGL, no HAL — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   8 de Abril 2026
 */
#include "domain/services/event_log_formatter.h"
#include "common/date_time.h"
#include "lwprintf/lwprintf.h"
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════
 * Private helpers
 * ══════════════════════════════════════════════════════════════════════════ */

const char *EventLogFormatter_GetSeverityStr(uint8_t severity)
{
    switch ((EventSeverity_t)severity)
    {
    case EVENT_SEVERITY_INFO:
        return "INFO";
    case EVENT_SEVERITY_WARNING:
        return "WARN";
    case EVENT_SEVERITY_ERROR:
        return "ERR ";
    case EVENT_SEVERITY_CRITICAL:
        return "CRIT";
    default:
        return "????";
    }
}

const char *EventLogFormatter_GetEventTypeStr(uint8_t type)
{
    switch ((EventType_t)type)
    {
    case EVENT_TYPE_SYSTEM_BOOT:
        return "System Boot";
    case EVENT_TYPE_SYSTEM_FACTORY_RESET:
        return "Factory Reset";
    case EVENT_TYPE_SYSTEM_EVENTS_CLEARED:
        return "Events Cleared";
    case EVENT_TYPE_SYSTEM_HOURMETER_RESET:
        return "Hourmeter Reset";

    case EVENT_TYPE_CONFIG_CHANGED:
        return "Config Changed";
    case EVENT_TYPE_CONFIG_RELAY:
        return "Config Relay Changed";
    case EVENT_TYPE_CONFIG_GPS:
        return "Config GPS Changed";
    case EVENT_TYPE_CONFIG_GENERAL:
        return "Config General Changed";
    case EVENT_TYPE_CONFIG_WIFI:
        return "Config WiFi Changed";

    case EVENT_TYPE_GPS_LOCK_ACQUIRED:
        return "GPS Lock Acquired";
    case EVENT_TYPE_GPS_LOCK_LOST:
        return "GPS Lock Lost";
    case EVENT_TYPE_GPS_TIME_SYNC:
        return "GPS Time Sync";
    case EVENT_TYPE_GPS_TIME_SYNC_LOST:
        return "GPS Time Sync Lost";

    case EVENT_TYPE_RELAY_ENABLED:
        return "Relay Enabled";
    case EVENT_TYPE_RELAY_SYNC_FAIL:
        return "Relay Sync Fail";
    case EVENT_TYPE_RELAY_CYCLE_START:
        return "Relay Cycle Start";
    case EVENT_TYPE_RELAY_CYCLE_END:
        return "Relay Cycle End";
    case EVENT_TYPE_RELAY_FORCED_OPEN:
        return "Relay Forced Open";
    case EVENT_TYPE_RELAY_ERROR:
        return "Relay FSM Error";

    case EVENT_TYPE_ALARM_HIGH_TEMP_ACTIVE:
        return "High Temp Active";
    case EVENT_TYPE_ALARM_HIGH_TEMP_CLEARED:
        return "High Temp Cleared";

    case EVENT_TYPE_ERROR_EEPROM:
        return "Error EEPROM";
    case EVENT_TYPE_ERROR_GPS:
        return "Error GPS";
    case EVENT_TYPE_ERROR_STORAGE:
        return "Error Storage";

    case EVENT_TYPE_WIFI_RESET:
        return "WiFi Reset";
    case EVENT_TYPE_WIFI_CONNECTED:
        return "WiFi Connected";
    case EVENT_TYPE_WIFI_DISCONNECTED:
        return "WiFi Disconnected";

    case EVENT_TYPE_LICENSE_FREE_MODE:
        return "License Free Mode";
    case EVENT_TYPE_LICENSE_RENT_MODE:
        return "License Rent Mode";
    case EVENT_TYPE_LICENSE_RENT_ENDED:
        return "License Rent Ended";

    case EVENT_TYPE_WARNING_BATTERY_LOW:
        return "Warning Battery Low";

    default:
        return "Unknown";
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════════════════ */

void EventLogFormatter_TimestampToStr(uint32_t ts_unix, char *buf, uint32_t buf_len)
{
    if (buf == NULL || buf_len == 0U)
    {
        return;
    }
    DateTime_t dt;
    DateTime_FromUnix(ts_unix, &dt);
    (void)lwprintf_snprintf(buf, (size_t)buf_len,
                            "%02u/%02u/%04u %02u:%02u:%02u",
                            (unsigned)dt.day,
                            (unsigned)dt.month,
                            (unsigned)dt.year,
                            (unsigned)dt.hour,
                            (unsigned)dt.minute,
                            (unsigned)dt.second);
}

Result_t EventLogFormatter_Format(const EventLogEntry_t *entry,
                                  char *buf,
                                  uint32_t buf_len)
{
    if (entry == NULL || buf == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (buf_len == 0U)
    {
        return ERR_OK;
    }

    char ts_str[24]; /* "DD/MM/YYYY HH:MM:SS" = 19 chars + NUL */
    EventLogFormatter_TimestampToStr(entry->timestamp, ts_str, (uint32_t)sizeof(ts_str));

    (void)lwprintf_snprintf(buf, (size_t)buf_len,
                            "%s\n[%s] %s\n",
                            ts_str,
                            EventLogFormatter_GetSeverityStr(entry->severity),
                            EventLogFormatter_GetEventTypeStr(entry->type));

    return ERR_OK;
}
