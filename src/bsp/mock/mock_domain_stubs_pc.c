/**
 * @file mock_domain_stubs_pc.c
 * @brief PC simulator stubs for domain/application symbols used by presenters.
 *
 * These are no-op / default-value implementations so the presenter layer
 * compiles and links without the real domain services.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "hal_types.h"

/* ---- DateTime ------------------------------------------------------------ */
#include "common/date_time.h"

uint32_t DateTime_ToUnix(const DateTime_t *dt)
{
    (void)dt;
    return 0U;
}

void DateTime_FromUnix(uint32_t unix_ts, DateTime_t *out_dt)
{
    if (out_dt == NULL)
    {
        return;
    }
    time_t t = (time_t)unix_ts;
    struct tm gmt;
    gmtime_r(&t, &gmt);
    out_dt->year = (uint16_t)(gmt.tm_year + 1900);
    out_dt->month = (uint8_t)(gmt.tm_mon + 1);
    out_dt->day = (uint8_t)(gmt.tm_mday);
    out_dt->hour = (uint8_t)(gmt.tm_hour);
    out_dt->minute = (uint8_t)(gmt.tm_min);
    out_dt->second = (uint8_t)(gmt.tm_sec);
}

/* ---- UTC offsets --------------------------------------------------------- */
#include "domain/time/utc_offsets.h"

const float UTC_OFFSETS[UTC_OFFSET_COUNT] = {
    -12.0f, -11.0f, -10.0f, -9.5f, -9.0f, -8.0f, -7.0f, -6.0f,
    -5.0f, -4.5f, -4.0f, -3.5f, -3.0f, -2.0f, -1.0f, 0.0f,
    1.0f, 2.0f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f,
    5.75f, 6.0f, 6.5f, 7.0f, 8.0f, 9.0f, 9.5f, 10.0f,
    10.5f, 11.0f, 12.0f, 12.75f, 13.0f, 14.0f};

/* ---- BuzzerNotificationService ------------------------------------------- */
#include "domain/services/buzzer_notification_service.h"

Result_t BuzzerNotificationService_NotifyEvent(BuzzerNotificationService_t *service,
                                               BuzzerEventType_t event_type)
{
    (void)service;
    (void)event_type;
    return ERR_OK;
}

/* ---- EventLogFormatter --------------------------------------------------- */
#include "domain/services/event_log_formatter.h"

static const char *event_type_str(uint8_t t)
{
    switch (t)
    {
    case 0:
        return "System Boot";
    case 1:
        return "Config Changed";
    case 2:
        return "GPS Lock Acquired";
    case 3:
        return "GPS Lock Lost";
    case 5:
        return "Relay Enabled";
    case 6:
        return "Relay Sync Fail";
    case 7:
        return "Relay Cycle Start";
    case 8:
        return "Relay Cycle End";
    case 9:
        return "Relay Forced Open";
    case 10:
        return "Error EEPROM";
    case 11:
        return "Error GPS";
    case 12:
        return "Error Storage";
    case 13:
        return "GPS Time Sync";
    case 14:
        return "GPS Sync Lost";
    case 15:
        return "Relay FSM Error";
    case 20:
        return "High Temp Active";
    case 21:
        return "High Temp Cleared";
    case 22:
        return "Config Relay";
    case 23:
        return "Config GPS";
    case 24:
        return "Config General";
    case 25:
        return "Config WiFi";
    case 30:
        return "Factory Reset";
    case 31:
        return "Events Cleared";
    case 32:
        return "Hourmeter Reset";
    case 40:
        return "WiFi Reset";
    case 41:
        return "WiFi Connected";
    case 42:
        return "WiFi Disconnected";
    case 50:
        return "License Free";
    case 51:
        return "License Rent";
    case 52:
        return "License Ended";
    case 60:
        return "Battery Low";
    default:
        return "Unknown";
    }
}
static const char *severity_str(uint8_t s)
{
    switch (s)
    {
    case 0:
        return "INFO";
    case 1:
        return "WARN";
    case 2:
        return "ERR ";
    case 3:
        return "CRIT";
    default:
        return "????";
    }
}
Result_t EventLogFormatter_Format(const EventLogEntry_t *entry,
                                  char *buf, uint32_t buf_len)
{
    if (entry == NULL || buf == NULL || buf_len == 0U)
    {
        return ERR_NULL_POINTER;
    }
    DateTime_t dt;
    DateTime_FromUnix(entry->timestamp, &dt);
    snprintf(buf, (size_t)buf_len,
             "%02u/%02u/%04u %02u:%02u:%02u\n[%s] %s\n",
             (unsigned)dt.day, (unsigned)dt.month, (unsigned)dt.year,
             (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second,
             severity_str(entry->severity),
             event_type_str(entry->type));
    return ERR_OK;
}

/* ---- GpsAntennaAutoSwitchService ---------------------------------------- */
#include "application/services/gps_antenna_auto_switch_service.h"

Result_t GpsAntennaAutoSwitchService_GetStatus(
    const GpsAntennaAutoSwitchService_t *self,
    GpsAntennaAutoSwitchStatus_t *out_status)
{
    (void)self;
    if (out_status)
    {
        *out_status = (GpsAntennaAutoSwitchStatus_t){0};
    }
    return ERR_OK;
}

/* ---- HourmeterAO --------------------------------------------------------- */
#include "application/activeobjects/hourmeter_ao.h"

Result_t HourmeterAO_GetHours(HourmeterAO_t *self, HourmeterType_t type,
                              uint32_t *hours, uint32_t *minutes)
{
    (void)self;
    (void)type;
    if (hours)
    {
        *hours = 0U;
    }
    if (minutes)
    {
        *minutes = 0U;
    }
    return ERR_OK;
}

/* ---- lwprintf_snprintf_ex ------------------------------------------------ */
#include "lwprintf/lwprintf.h"

int lwprintf_snprintf_ex(lwprintf_t *const lwobj, char *s, size_t n,
                         const char *format, ...)
{
    (void)lwobj;
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(s, n, format, args);
    va_end(args);
    return ret;
}
