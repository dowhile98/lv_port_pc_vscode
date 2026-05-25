/**
 * @file information_show_screen_presenter.c
 * @brief Unified presenter for the information_show screen.
 *
 * Consolidates four information screens into one SCREEN_ID_INFORMATION_SHOW (29).
 * The active provider is chosen by reading *pending_item_ptr at OnEnter time.
 *
 * Provider table:
 *   idx 0 — Device Status           (time_source, relay, hourmeter)
 *   idx 1 — Interrupt Configuration  (config_storage, time_source)
 *   idx 2 — GPS Status               (config_storage, gps_source)
 *   idx 3 — Battery Status           (battery)
 *   idx 4 — Batch                    (device_identity)
 *   idx 5 — FW & HW Version          (none — compile-time constants)
 *   idx 6 — (reserved)               (none)
 *   idx 7 — License Status           (license_status)
 *
 * @note NO #include "lvgl.h" — intentional (PC-testable).
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/information_show/information_show_screen_presenter.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_battery_monitor.h"
#include "interfaces/i_device_identity.h"
#include "interfaces/i_license_status.h"
#include "interfaces/i_wifi_status_source.h"
#include "domain/services/event_log_formatter.h"
#include "common/wifi_types.h"
#include "osal/osal.h"
#include "infrastructure/config/version.h"
#include "domain/time/utc_offsets.h"
#include "lwprintf/lwprintf.h"
#include <string.h>
#include <inttypes.h>

/* ══════════════════════════════════════════════════════════════════════════
 * Shared macros used by all build_xxx functions
 * ══════════════════════════════════════════════════════════════════════════ */
#define APPEND(buf, size, s) strncat((buf), (s), (size_t)((size) - strlen(buf) - 1U))
#define APPENDL(line, buf, size, l) \
    do                              \
    {                               \
        (void)lwprintf_snprintf l;  \
        APPEND(buf, size, line);    \
    } while (0)

/* ══════════════════════════════════════════════════════════════════════════
 * Shared static buffers for WiFi providers
 *
 * All five WiFi providers (idx 8-12) are invoked one-at-a-time from
 * refresh_view().  Making these file-scope statics eliminates ~400 B of
 * per-call stack pressure:
 *   WifiConfig_t  ≈ 248 B  (v3.0 layout)
 *   uid_buf       =  32 B  (DEVICE_IDENTITY_UID_STR_LEN)
 *   line          =  80 B  (single-line scratch buffer)
 * ══════════════════════════════════════════════════════════════════════════ */
static WifiConfig_t s_wifi_cfg; /**< Shared WiFi config (WiFi providers). */
static char s_line[80];         /**< Shared single-line scratch buffer.    */
static char s_buf[INFORMATION_SHOW_CONTENT_BUF_SIZE];
/* ══════════════════════════════════════════════════════════════════════════
 * Provider 0 — Device Status
 * ══════════════════════════════════════════════════════════════════════════ */

static const char *relay_mode_str(RelayInternalState_t state)
{
    switch (state)
    {
    case RELAY_STATE_IDLE:
        return "Idle";
    case RELAY_STATE_WAITING:
        return "Waiting";
    case RELAY_STATE_ACTIVE:
        return "Active";
    case RELAY_STATE_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

static void build_device_status(const InformationShowScreenPresenter_t *self,
                                char *buf,
                                uint32_t size)
{
    char line[80];
    buf[0] = '\0';

    static const struct
    {
        HourmeterType_t type;
        const char *label;
    } k_hourmeters[] = {
        {HOURMETER_TYPE_POWER_ON, "Hourmeter Power ON"},
        {HOURMETER_TYPE_GPS_CONNECTED, "Hourmeter GPS Conn"},
        {HOURMETER_TYPE_RELAY_ACTIVE, "Hourmeter Contact "},
        {HOURMETER_TYPE_TIME_WINDOW, "Hourmeter Time Win"},
    };

    /* 1. High Temperature Alarm */
    if (self->relay != NULL)
    {
        RelayStatus_t status;
        memset(&status, 0, sizeof(status));
        if (RelayController_GetStatus(self->relay, &status) == ERR_OK)
        {
            APPENDL(line, buf, size,
                    (line, sizeof(line), "High Temp Alarm: %s\n",
                     status.alarm_active ? "Active" : "Off"));
        }
        else
        {
            APPEND(buf, size, "High Temp Alarm: --\n");
        }
    }
    else
    {
        APPEND(buf, size, "High Temp Alarm: --\n");
    }

    /* 2–5. Hourmeter counters */
    for (uint8_t i = 0U; i < 4U; i++)
    {
        if (self->hourmeter != NULL)
        {
            uint32_t hours = 0U;
            uint32_t minutes = 0U;
            if (HourmeterAO_GetHours(self->hourmeter,
                                     k_hourmeters[i].type,
                                     &hours,
                                     &minutes) == ERR_OK)
            {
                APPENDL(line, buf, size,
                        (line, sizeof(line), "%s: %02" PRIu32 ":%02" PRIu32 ":00\n",
                         k_hourmeters[i].label, hours, minutes));
            }
            else
            {
                APPENDL(line, buf, size,
                        (line, sizeof(line), "%s: 00:00:00\n", k_hourmeters[i].label));
            }
        }
        else
        {
            APPENDL(line, buf, size,
                    (line, sizeof(line), "%s: 00:00:00\n", k_hourmeters[i].label));
        }
    }

    /* 6–7. Date & Time */
    DateTime_t dt;
    memset(&dt, 0, sizeof(dt));
    if (self->time_source != NULL &&
        TimeSource_GetTime(self->time_source, &dt) == ERR_OK)
    {
        APPENDL(line, buf, size,
                (line, sizeof(line), "Date: %02u/%02u/%04u\n",
                 (unsigned)dt.day, (unsigned)dt.month, (unsigned)dt.year));
        APPENDL(line, buf, size,
                (line, sizeof(line), "Time: %02u:%02u:%02u\n",
                 (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second));
    }
    else
    {
        APPEND(buf, size, "Date: --\n");
        APPEND(buf, size, "Time: --\n");
    }

    /* 8. Contact State */
    if (self->relay != NULL)
    {
        RelayContactState_t contact;
        if (RelayController_GetState(self->relay, &contact) == ERR_OK)
        {
            APPENDL(line, buf, size,
                    (line, sizeof(line), "Contact State: %s\n",
                     (contact != RELAY_CONTACT_CLOSED) ? "Closed" : "Open"));
        }
        else
        {
            APPEND(buf, size, "Contact State: --\n");
        }
    }
    else
    {
        APPEND(buf, size, "Contact State: --\n");
    }

    /* 9. Relay Mode */
    if (self->relay != NULL)
    {
        RelayStatus_t status;
        memset(&status, 0, sizeof(status));
        if (RelayController_GetStatus(self->relay, &status) == ERR_OK)
        {
            const char *mode = status.alarm_active
                                   ? "Forced Open"
                                   : relay_mode_str(status.internal_state);
            APPENDL(line, buf, size,
                    (line, sizeof(line), "Contact Mode: %s\n", mode));
        }
        else
        {
            APPEND(buf, size, "Relay Mode: --\n");
        }
    }
    else
    {
        APPEND(buf, size, "Relay Mode: --\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Provider 1 — Interrupt Configuration
 * ══════════════════════════════════════════════════════════════════════════ */

static const char *const k_day_abbr[7] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

static uint8_t get_multicycle_idx(const MultiCycleConfig_t *mc, ITimeSource *ts)
{
    DateTime_t now;
    memset(&now, 0, sizeof(now));
    if (ts == NULL || TimeSource_GetTime(ts, &now) != ERR_OK)
    {
        return 0U;
    }
    for (uint8_t i = 0U; i < (uint8_t)RELAY_MAX_CYCLES; i++)
    {
        const DateTime_t *d = &mc->boundary_dates[i];
        bool after = false;
        if (now.year > d->year)
        {
            after = true;
        }
        else if (now.year < d->year)
        {
            after = false;
        }
        else if (now.month > d->month)
        {
            after = true;
        }
        else if (now.month < d->month)
        {
            after = false;
        }
        else
        {
            after = (now.day > d->day);
        }
        if (!after)
        {
            return i;
        }
    }
    return (uint8_t)(RELAY_MAX_CYCLES - 1U);
}

static void build_interrupt_config(const InformationShowScreenPresenter_t *self,
                                   char *buf,
                                   uint32_t size)
{
    char line[96];

    if (self->config_storage == NULL)
    {
        strncpy(buf, "Interruption state: --\nStart Time: --\n"
                     "Stop Time: --\nCycling Days: --\n"
                     "Period Mode: --\nCycle start: --\n"
                     "On Time: --\nOff Time: --\n",
                (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (ConfigStorage_LoadRelayConfig(self->config_storage, &cfg) != ERR_OK)
    {
        strncpy(buf, "Interruption state: --\nStart Time: --\n"
                     "Stop Time: --\nCycling Days: --\n"
                     "Period Mode: --\nCycle start: --\n"
                     "On Time: --\nOff Time: --\n",
                (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    buf[0] = '\0';

    /* 1. State */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Interruption state: %s\n",
             cfg.enabled ? "Enabled" : "Disabled"));

    /* 2. Start Time */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Start Time: %02u:%02u:%02u\n",
             (unsigned)cfg.time_window.start_time.hour,
             (unsigned)cfg.time_window.start_time.minute,
             (unsigned)cfg.time_window.start_time.second));

    /* 3. Stop Time */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Stop Time: %02u:%02u:%02u\n",
             (unsigned)cfg.time_window.stop_time.hour,
             (unsigned)cfg.time_window.stop_time.minute,
             (unsigned)cfg.time_window.stop_time.second));

    /* 4. Cycling Days */
    {
        char days_str[32];
        days_str[0] = '\0';
        bool any = false;
        for (uint8_t bit = 0U; bit < 7U; bit++)
        {
            if ((cfg.time_window.weekday_mask >> bit) & 0x01U)
            {
                if (any)
                {
                    strncat(days_str, " ", sizeof(days_str) - strlen(days_str) - 1U);
                }
                strncat(days_str, k_day_abbr[bit],
                        sizeof(days_str) - strlen(days_str) - 1U);
                any = true;
            }
        }
        if (!any)
        {
            strncat(days_str, "--", sizeof(days_str) - strlen(days_str) - 1U);
        }
        APPENDL(line, buf, size,
                (line, sizeof(line), "Cycling Days: %s\n", days_str));
    }

    /* 5. Period Mode */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Period Mode: %s\n",
             cfg.multicycle.enabled ? "Multiple" : "Single"));

    /* 6. Cycle start */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Cycle start: %s\n",
             cfg.start_with_on ? "ON" : "OFF"));

    /* 7 & 8. On / Off Time */
    {
        uint32_t ton, toff;
        if (cfg.multicycle.enabled)
        {
            uint8_t idx = get_multicycle_idx(&cfg.multicycle, self->time_source);
            ton = cfg.multicycle.ton[idx];
            toff = cfg.multicycle.toff[idx];
        }
        else
        {
            ton = cfg.simple_cycle.ton;
            toff = cfg.simple_cycle.toff;
        }
        APPENDL(line, buf, size,
                (line, sizeof(line), "On Time: %.3fs\n", (double)ton / 1000.0));
        APPENDL(line, buf, size,
                (line, sizeof(line), "Off Time: %.3fs\n", (double)toff / 1000.0));
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Provider 2 — GPS Status
 * ══════════════════════════════════════════════════════════════════════════ */

static void format_utc_label(char *buf, uint32_t buf_size, float hours)
{
    int32_t total_q;
    if (hours < 0.0f)
    {
        total_q = -(int32_t)((-hours) * 4.0f + 0.001f);
    }
    else
    {
        total_q = (int32_t)(hours * 4.0f + 0.001f);
    }
    const int32_t abs_q = (total_q < 0) ? -total_q : total_q;
    const int32_t h = abs_q / 4;
    const int32_t frac_q = abs_q % 4;
    const char sign_c = (total_q < 0) ? '-' : '+';

    switch (frac_q)
    {
    case 1:
        (void)lwprintf_snprintf(buf, (size_t)buf_size, "UTC%c%d.25", sign_c, (int)h);
        break;
    case 2:
        (void)lwprintf_snprintf(buf, (size_t)buf_size, "UTC%c%d.5", sign_c, (int)h);
        break;
    case 3:
        (void)lwprintf_snprintf(buf, (size_t)buf_size, "UTC%c%d.75", sign_c, (int)h);
        break;
    default:
        (void)lwprintf_snprintf(buf, (size_t)buf_size, "UTC%c%d.0", sign_c, (int)h);
        break;
    }
}

static void format_coord(char *buf, uint32_t buf_size, float val)
{
    char sign_c;
    if (val < 0.0f)
    {
        sign_c = '-';
        val = -val;
    }
    else
    {
        sign_c = '+';
    }
    const int32_t integer_part = (int32_t)val;
    const uint32_t frac_part = (uint32_t)((val - (float)integer_part) * 1000000.0f + 0.5f);
    (void)lwprintf_snprintf(buf, (size_t)buf_size,
                            "%c%d.%06u", sign_c, (int)integer_part, (unsigned)frac_part);
}

static void format_altitude(char *buf, uint32_t buf_size, float val)
{
    char sign_c;
    if (val < 0.0f)
    {
        sign_c = '-';
        val = -val;
    }
    else
    {
        sign_c = ' ';
    }
    const int32_t integer_part = (int32_t)val;
    uint32_t frac_part = (uint32_t)((val - (float)integer_part) * 100.0f + 0.5f);
    if (frac_part >= 100U)
    {
        frac_part = 99U;
    }
    (void)lwprintf_snprintf(buf, (size_t)buf_size,
                            "%c%d.%02u m", sign_c, (int)integer_part, (unsigned)frac_part);
}

static void build_gps_status(const InformationShowScreenPresenter_t *self,
                             char *buf,
                             uint32_t size)
{
    static const char k_gps_fallback[] =
        "Antenna: --\nTime Offset: --\nUTC: --\n"
        "Satellites in use: --\nGPS status: --\n"
        "Latitude: --\nLongitude: --\nAltitude: --\nDate: --\n";

    if (self->config_storage == NULL || self->gps_source == NULL)
    {
        strncpy(buf, k_gps_fallback, (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    GPSConfig_t cfg;
    GPSPosition_t pos;
    GPSFixStatus_t fix = GPS_FIX_NONE;
    DateTime_t gps_date;
    bool have_date = false;

    memset(&cfg, 0, sizeof(cfg));
    memset(&pos, 0, sizeof(pos));
    memset(&gps_date, 0, sizeof(gps_date));

    if (ConfigStorage_LoadGPSConfig(self->config_storage, &cfg) != ERR_OK)
    {
        strncpy(buf, k_gps_fallback, (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    (void)GPS_Source_GetPosition(self->gps_source, &pos);
    (void)GPS_Source_GetFixStatus(self->gps_source, &fix);
    if (GPS_Source_GetTimeUTC(self->gps_source, &gps_date) == ERR_OK)
    {
        have_date = true;
    }

    char line[96];
    buf[0] = '\0';

    /* 1. Antenna */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Antenna: %s\n",
             (cfg.antenna_type == 1U) ? "External" : "Internal"));

    /* 2. Time Offset */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Time Offset: %+d s\n", (int)cfg.time_offset));

    /* 3. UTC */
    {
        char utc_str[16];
        utc_str[0] = '\0';
        if (cfg.utc_offset_index < (uint8_t)UTC_OFFSET_COUNT)
        {
            format_utc_label(utc_str, (uint32_t)sizeof(utc_str),
                             UTC_OFFSETS[cfg.utc_offset_index]);
        }
        else
        {
            strncat(utc_str, "UTC--", sizeof(utc_str) - 1U);
        }
        APPENDL(line, buf, size, (line, sizeof(line), "UTC: %s\n", utc_str));
    }

    /* 4. Satellites in use */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Satellites in use: %u\n",
             (unsigned)pos.satellites_used));

    /* 5. GPS status */
    {
        const char *fix_str =
            (fix == GPS_FIX_3D) ? "Fix 3D" : (fix == GPS_FIX_2D) ? "Fix 2D"
                                                                 : "No Fix";
        APPENDL(line, buf, size, (line, sizeof(line), "GPS status: %s\n", fix_str));
    }

    /* 6. Latitude */
    {
        char coord[24];
        format_coord(coord, (uint32_t)sizeof(coord), pos.latitude);
        APPENDL(line, buf, size, (line, sizeof(line), "Latitude: %s\n", coord));
    }

    /* 7. Longitude */
    {
        char coord[24];
        format_coord(coord, (uint32_t)sizeof(coord), pos.longitude);
        APPENDL(line, buf, size, (line, sizeof(line), "Longitude: %s\n", coord));
    }

    /* 8. Altitude */
    {
        char alt_str[24];
        format_altitude(alt_str, (uint32_t)sizeof(alt_str), pos.altitude);
        APPENDL(line, buf, size, (line, sizeof(line), "Altitude: %s\n", alt_str));
    }

    /* 9. Date */
    if (have_date)
    {
        APPENDL(line, buf, size,
                (line, sizeof(line), "Date: %02u/%02u/%04u %02u:%02u:%02u\n",
                 (unsigned)gps_date.day, (unsigned)gps_date.month,
                 (unsigned)gps_date.year,
                 (unsigned)gps_date.hour, (unsigned)gps_date.minute,
                 (unsigned)gps_date.second));
    }
    else
    {
        APPEND(buf, size, "Date: --\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Provider 3 — Battery Status
 * ══════════════════════════════════════════════════════════════════════════ */

static const char *battery_status_to_string(BatteryStatus_t status)
{
    if (status & BATTERY_STATUS_ERROR)
    {
        return "Error";
    }
    if (status & BATTERY_STATUS_CRITICAL_SOC)
    {
        return "Critical";
    }
    if (status & BATTERY_STATUS_LOW_SOC)
    {
        return "Low";
    }
    if (status & BATTERY_STATUS_FULL)
    {
        return "Full";
    }
    if (status & BATTERY_STATUS_CHARGING)
    {
        return "Charging";
    }
    if (status & BATTERY_STATUS_DISCHARGING)
    {
        return "Discharging";
    }
    return "Unknown";
}

static void build_battery_status(const InformationShowScreenPresenter_t *self,
                                 char *buf,
                                 uint32_t size)
{
    static const char k_batt_fallback[] =
        "Voltage:   --\nCurrent:   --\nTemp:      --\n"
        "Charge:    --\nHealth:    --\nRemaining: --\n"
        "Full Cap:  --\nAvg Power: --\nStatus:    --\n";

    if (self->battery == NULL)
    {
        strncpy(buf, k_batt_fallback, (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    BatteryData_t data;
    memset(&data, 0, sizeof(data));
    if (BatteryMonitor_GetData(self->battery, &data) != ERR_OK)
    {
        strncpy(buf, k_batt_fallback, (size_t)size - 1U);
        buf[size - 1U] = '\0';
        return;
    }

    char line[64];
    buf[0] = '\0';

    /* 1. Voltage */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Voltage:   %u mV\n", (unsigned)data.voltage_mv));

    /* 2. Current (signed) */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Current:   %+d mA\n", (int)data.current_ma));

    /* 3. Temperature — temperature_decideg_c is in 0.1°C */
    {
        int32_t raw = (int32_t)data.temperature_decideg_c;
        char sign_c;
        if (raw < 0)
        {
            sign_c = '-';
            raw = -raw;
        }
        else
        {
            sign_c = ' ';
        }
        int32_t int_c = raw / 10;
        int32_t dec_c = raw % 10;
        APPENDL(line, buf, size,
                (line, sizeof(line), "Temp:      %c%d.%d C\n",
                 sign_c, (int)int_c, (int)dec_c));
    }

    /* 4. State of Charge */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Charge:    %u %%\n",
             (unsigned)data.state_of_charge_percent));

    /* 5. State of Health */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Health:    %u %%\n",
             (unsigned)data.state_of_health_percent));

    /* 6. Remaining capacity */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Remaining: %u mAh\n",
             (unsigned)data.remaining_capacity_mah));

    /* 7. Full charge capacity */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Full Cap:  %u mAh\n",
             (unsigned)data.full_capacity_mah));

    /* 8. Average power (signed, mW) */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Avg Power: %+d mW\n", (int)data.average_power_mw));

    /* 9. Status */
    APPENDL(line, buf, size,
            (line, sizeof(line), "Status:    %s\n",
             battery_status_to_string(data.status)));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Provider 4 — Batch
 * ══════════════════════════════════════════════════════════════════════════ */

/** Compile-time product strings — update per release. */
#define BATCH_MODEL_STR "TCS-CICX1 V5"
#define BATCH_BATCH_STR "290229-01-V5"
#define BATCH_COMPANY_STR "TECNA S.A."

static void build_batch(const InformationShowScreenPresenter_t *self,
                        char *buf,
                        uint32_t size)
{
    char uid[DEVICE_IDENTITY_UID_STR_LEN];
    uid[0] = '\0';
    if (self->device_identity != NULL)
    {
        (void)DeviceIdentity_GetUidString(self->device_identity, uid, (uint32_t)sizeof(uid));
    }
    else
    {
        uid[0] = '-';
        uid[1] = '-';
        uid[2] = '\0';
    }

    char line[100];
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPENDL(line, buf, size, (line, sizeof(line), "-> MODEL   %s\n", BATCH_MODEL_STR));
    APPENDL(line, buf, size, (line, sizeof(line), "-> BATCH   %s\n", BATCH_BATCH_STR));
    APPENDL(line, buf, size, (line, sizeof(line), "-> ID        %s\n", uid));
    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPEND(buf, size, "\n");
    APPENDL(line, buf, size, (line, sizeof(line), "                    %s\n", BATCH_COMPANY_STR));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Provider 5 — Firmware & Hardware Version
 * ══════════════════════════════════════════════════════════════════════════ */

static void build_firmware_version(const InformationShowScreenPresenter_t *self,
                                   char *buf,
                                   uint32_t size)
{
    (void)self;
    char line[80];
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPENDL(line, buf, size, (line, sizeof(line), "-> Firmware    %s\n", FW_VERSION_STRING));
    APPENDL(line, buf, size, (line, sizeof(line), "-> Hardware    %s\n", HW_VERSION_STRING));
    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPEND(buf, size, "\n");
    APPENDL(line, buf, size, (line, sizeof(line), "Build date:  %s\n", __DATE__));
    APPENDL(line, buf, size, (line, sizeof(line), "Build time:  %s\n", __TIME__));
}

/* ══════════════════════════════════════════════════════════════════════════ * Provider 6 — Reserved placeholder
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_reserved(const InformationShowScreenPresenter_t *self,
                           char *buf,
                           uint32_t size)
{
    (void)self;
    (void)size;
    buf[0] = '\0'; /* No content — reserved for future use */
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 7 — License Status
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_license_status(const InformationShowScreenPresenter_t *self,
                                 char *buf,
                                 uint32_t size)
{
    char line[80];
    buf[0] = '\0';

    if (self->license_status == NULL)
    {
        APPEND(buf, size, "License service unavailable\n");
        return;
    }

    LicenseStatus_t status;
    if (LicenseStatus_GetStatus(self->license_status, &status) != ERR_OK)
    {
        APPEND(buf, size, "License status read error\n");
        return;
    }

    APPEND(buf, size, "---------------------------------------------------------------\n");

    switch (status.mode)
    {
    case LICENSE_MODE_FREE:
        APPEND(buf, size, "Mode:     Free Operation\n");
        APPEND(buf, size, "          No license restrictions.\n");
        break;

    case LICENSE_MODE_RENTAL_ACTIVE:
        APPEND(buf, size, "Mode:     Rental (Active)\n");
        APPENDL(line, buf, size,
                (line, sizeof(line), "Days left: %ld\n", (long)status.days_remaining));
        break;

    case LICENSE_MODE_RENTAL_EXPIRED:
        APPEND(buf, size, "Mode:     Rental (EXPIRED)\n");
        APPEND(buf, size, "Status:   ACCESS BLOCKED\n");
        APPEND(buf, size, "          Contact distributor to renew.\n");
        break;

    default:
        APPEND(buf, size, "Mode:     Unknown\n");
        break;
    }

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Helpers shared by WiFi providers
 * ════════════════════════════════════════════════════════════════════════════ */

static const char *wifi_mode_str(uint8_t mode)
{
    switch ((WifiMode_t)mode)
    {
    case WIFI_MODE_DISABLED:
        return "Disabled";
    case WIFI_MODE_STA:
        return "STA (Station)";
    case WIFI_MODE_AP:
        return "AP (Access Point)";
    case WIFI_MODE_APSTA:
        return "AP+STA";
    default:
        return "Unknown";
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 8 — AP Information
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_wifi_ap_info(const InformationShowScreenPresenter_t *self,
                               char *buf,
                               uint32_t size)
{
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");

    if (self->config_storage == NULL)
    {
        APPEND(buf, size, "Config storage unavailable\n");
        return;
    }

    memset(&s_wifi_cfg, 0, sizeof(s_wifi_cfg));
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &s_wifi_cfg) != ERR_OK)
    {
        APPEND(buf, size, "WiFi config read error\n");
        return;
    }

    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Mode:       %s\n", wifi_mode_str(s_wifi_cfg.mode)));
    /* SSID: always read from stored config — source of truth after first-boot DI init. */
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "SSID:  %.32s\n", s_wifi_cfg.ap_ssid));

    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Password:   %.32s\n", s_wifi_cfg.ap_pwd));
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Channel:    %u\n", (unsigned)s_wifi_cfg.ap_channel));
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Max Clients:%u\n", (unsigned)s_wifi_cfg.ap_max_conn));

    /* Live AP IP from network stack when available */
    if (self->ap_port != NULL)
    {
        NetworkPortStatus_t status;
        memset(&status, 0, sizeof(status));
        if (NetworkPort_GetStatus(self->ap_port, &status) == ERR_OK && status.has_ip_address)
        {
            const uint8_t *ip = status.ip_addr.octet;
            const uint8_t *msk = status.subnet_mask.octet;
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "IP:         %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Mask:       %d.%d.%d.%d\n", msk[0], msk[1], msk[2], msk[3]));
        }
        else
        {
            APPEND(buf, size, "IP:         --\n");
            APPEND(buf, size, "Mask:       --\n");
        }
    }
    else
    {
        APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "IP:         %.16s\n", s_wifi_cfg.ap_ip));
        APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Mask:       %.16s\n", s_wifi_cfg.ap_mask));
    }

    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "DHCP Server:%s\n", s_wifi_cfg.ap_use_dhcp_server ? "Enabled" : "Disabled"));

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 9 — STA Information
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_wifi_sta_info(const InformationShowScreenPresenter_t *self,
                                char *buf,
                                uint32_t size)
{
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");

    bool connected = WifiStatusSource_IsConnected(self->wifi_status);
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Status:     %s\n", connected ? "Connected" : "Disconnected"));

    if (self->config_storage == NULL)
    {
        APPEND(buf, size, "Config storage unavailable\n");
        return;
    }

    memset(&s_wifi_cfg, 0, sizeof(s_wifi_cfg));
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &s_wifi_cfg) != ERR_OK)
    {
        APPEND(buf, size, "WiFi config read error\n");
        return;
    }

    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "SSID: %.32s\n", s_wifi_cfg.sta_ssid));
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "IP Mode:    %s\n", s_wifi_cfg.sta_use_dhcp ? "DHCP" : "Static"));

    /* Live IP / Mask / Gateway — always shown from network stack when available */
    if (self->sta_port != NULL)
    {
        NetworkPortStatus_t status;
        memset(&status, 0, sizeof(status));
        if (NetworkPort_GetStatus(self->sta_port, &status) == ERR_OK && status.has_ip_address)
        {
            const uint8_t *ip = status.ip_addr.octet;
            const uint8_t *msk = status.subnet_mask.octet;
            const uint8_t *gw = status.gateway.octet;
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "IP:         %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Mask:       %d.%d.%d.%d\n", msk[0], msk[1], msk[2], msk[3]));
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Gateway:    %d.%d.%d.%d\n", gw[0], gw[1], gw[2], gw[3]));
        }
        else
        {
            APPEND(buf, size, "IP:         --\n");
            APPEND(buf, size, "Mask:       --\n");
            APPEND(buf, size, "Gateway:    --\n");
        }
    }
    else if (!s_wifi_cfg.sta_use_dhcp)
    {
        /* Fallback to config values when no live port is injected (e.g. unit tests) */
        APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "IP:         %.16s\n", s_wifi_cfg.sta_ip));
        APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Mask:       %.16s\n", s_wifi_cfg.sta_mask));
        APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Gateway:    %.16s\n", s_wifi_cfg.sta_gateway));
    }

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 10 — Connect to AP
 *   Shows the device AP credentials so the user can join the device's network.
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_wifi_connect_ap(const InformationShowScreenPresenter_t *self,
                                  char *buf,
                                  uint32_t size)
{
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPEND(buf, size, "Join the device's AP network:\n\n");

    if (self->config_storage == NULL)
    {
        APPEND(buf, size, "Config storage unavailable\n");
        return;
    }

    memset(&s_wifi_cfg, 0, sizeof(s_wifi_cfg));
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &s_wifi_cfg) != ERR_OK)
    {
        APPEND(buf, size, "WiFi config read error\n");
        return;
    }

    /* Network name: always read from stored config — source of truth after first-boot DI init. */
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Network:    %.32s\n", s_wifi_cfg.ap_ssid));
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Password:   %.32s\n", s_wifi_cfg.ap_pwd));
    APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "Channel:    %u\n", (unsigned)s_wifi_cfg.ap_channel));
    APPEND(buf, size, "\nConnect to the network above,\nthen open the web interface.\n");

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 11 — Web Server (AP mode)
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_wifi_webserver_ap(const InformationShowScreenPresenter_t *self,
                                    char *buf,
                                    uint32_t size)
{
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPEND(buf, size, "Web interface via AP network:\n\n");

    /* Live AP IP from network stack */
    if (self->ap_port != NULL)
    {
        NetworkPortStatus_t status;
        memset(&status, 0, sizeof(status));
        if (NetworkPort_GetStatus(self->ap_port, &status) == ERR_OK && status.has_ip_address)
        {
            const uint8_t *ip = status.ip_addr.octet;
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "URL:  http://%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
        }
        else
        {
            APPEND(buf, size, "URL:  http://--\n");
        }
    }
    else
    {
        APPENDL(s_line, buf, size,
                (s_line, sizeof(s_line), "URL:  http://%s\n", WIFI_DEFAULT_AP_IP));
    }

    APPEND(buf, size, "\n1. Connect to device AP\n");
    APPEND(buf, size, "2. Open URL in browser\n");

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 12 — Web Server (STA mode)
 * ════════════════════════════════════════════════════════════════════════════ */

static void build_wifi_webserver_sta(const InformationShowScreenPresenter_t *self,
                                     char *buf,
                                     uint32_t size)
{
    buf[0] = '\0';

    APPEND(buf, size, "---------------------------------------------------------------\n");
    APPEND(buf, size, "Web interface via LAN (STA):\n\n");

    /* Live STA IP from network stack */
    if (self->sta_port != NULL)
    {
        NetworkPortStatus_t status;
        memset(&status, 0, sizeof(status));
        if (NetworkPort_GetStatus(self->sta_port, &status) == ERR_OK && status.has_ip_address)
        {
            const uint8_t *ip = status.ip_addr.octet;
            APPENDL(s_line, buf, size, (s_line, sizeof(s_line), "URL:  http://%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
        }
        else
        {
            APPEND(buf, size, "URL:  http://--\n");
        }
    }
    else
    {
        APPENDL(s_line, buf, size,
                (s_line, sizeof(s_line), "URL:  http://%s\n", WIFI_DEFAULT_STA_IP));
    }

    APPEND(buf, size, "\n1. Connect PC to same LAN as device\n");
    APPEND(buf, size, "2. Open URL in browser\n");

    APPEND(buf, size, "---------------------------------------------------------------\n");
}

/* ════════════════════════════════════════════════════════════════════════════ * Provider dispatch table
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct
{
    const char *title;
    void (*build_fn)(const InformationShowScreenPresenter_t *self,
                     char *buf, uint32_t size);
} InformationProvider_t;

static const InformationProvider_t k_providers[] = {
    {"Device Status", build_device_status},              /* idx  0 */
    {"Interrupt Configuration", build_interrupt_config}, /* idx  1 */
    {"GPS Status", build_gps_status},                    /* idx  2 */
    {"Battery Status", build_battery_status},            /* idx  3 */
    {"Batch", build_batch},                              /* idx  4 */
    {"FW & HW Version", build_firmware_version},         /* idx  5 */
    {"License Status", build_license_status},            /* idx  6 */
    {"---", build_reserved},                             /* idx  7 (reserved) */
    {"AP Information", build_wifi_ap_info},              /* idx  8 */
    {"STA Information", build_wifi_sta_info},            /* idx  9 */
    {"Connect to AP", build_wifi_connect_ap},            /* idx 10 */
    {"Web Server (AP)", build_wifi_webserver_ap},        /* idx 11 */
    {"Web Server (STA)", build_wifi_webserver_sta},      /* idx 12 */
};

/* ────────────────────────────────────────────────────────────────────────────
 * Provider 13 — Historical Events
 * ════════════════════════════════════════════════════════════════════════════ */

#define INFORMATION_SHOW_ITEM_EVENT_LOG 13U
#define INFORMATION_SHOW_ITEM_ALARM 14U
#define INFORMATION_SHOW_ITEM_CONFIRMATION 15U

/**
 * @brief Build content for Historical Events provider.
 *
 * Displays a single EventLogEntry_t formatted by EventLogFormatter.
 * Header line: [idx+1 / count] (1-based for user display).
 * Circular navigation is controlled by on_key_event.
 */
static void build_event_log(const InformationShowScreenPresenter_t *self,
                            char *buf, uint32_t size)
{
    if (buf == NULL || size == 0U)
    {
        return;
    }
    buf[0] = '\0';

    if (!event_log_storage_is_valid(self->event_log_storage))
    {
        strncat(buf, "No storage\n", (size_t)(size - strlen(buf) - 1U));
        return;
    }

    uint16_t count = 0U;
    if (EventLogStorage_GetCount(self->event_log_storage, &count) != ERR_OK)
    {
        strncat(buf, "Read error\n", (size_t)(size - strlen(buf) - 1U));
        return;
    }

    if (count == 0U)
    {
        strncat(buf, "No events\n", (size_t)(size - strlen(buf) - 1U));
        return;
    }

    /* header: [idx+1 / count] */
    char header[32];
    (void)lwprintf_snprintf(header, sizeof(header), "[%u/%u]\n",
                            (unsigned)(self->event_log_idx + 1U),
                            (unsigned)count);
    strncat(buf, header, (size_t)(size - strlen(buf) - 1U));

    /* read and format the event */
    EventLogEntry_t entry;
    if (EventLogStorage_ReadByIndex(self->event_log_storage,
                                    self->event_log_idx, &entry) != ERR_OK)
    {
        strncat(buf, "Read error\n", (size_t)(size - strlen(buf) - 1U));
        return;
    }

    char formatted[128];
    (void)EventLogFormatter_Format(&entry, formatted, (uint32_t)sizeof(formatted));
    strncat(buf, formatted, (size_t)(size - strlen(buf) - 1U));
}

/* ────────────────────────────────────────────────────────────────────────────
 * Provider 14 — High Temperature Alarm
 * ════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Build content for High Temperature Alarm provider.
 */
static void build_alarm(const InformationShowScreenPresenter_t *self,
                        char *buf, uint32_t size)
{
    (void)self;
    if (buf == NULL || size == 0U)
    {
        return;
    }
    buf[0] = '\0';
    strncat(buf, "WARNING: High temperature\ndetected!\n\nCheck power unit\nand ventilation.",
            (size_t)(size - 1U));
}

/* ────────────────────────────────────────────────────────────────────────────
 * Provider 15 — Action Confirmation
 * ════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Build content for Action Confirmation provider (item 15).
 *
 * Displays the pending_confirmation_msg string.  Auto-dismissed after 1 s
 * via logic in on_update().
 */
static void build_confirmation(const InformationShowScreenPresenter_t *self,
                               char *buf, uint32_t size)
{
    if (buf == NULL || size == 0U)
    {
        return;
    }
    buf[0] = '\0';
    const char *msg = (self->pending_confirmation_msg != NULL)
                          ? self->pending_confirmation_msg
                          : "Done";
    strncat(buf, msg, (size_t)(size - 1U));
}

#define INFORMATION_PROVIDER_COUNT \
    ((uint8_t)(sizeof(k_providers) / sizeof(k_providers[0])))

#define INFORMATION_PROVIDER_EVENT_LOG_IDX 13U

/* ══════════════════════════════════════════════════════════════════════════
 * Shared refresh logic
 * ══════════════════════════════════════════════════════════════════════════ */

static void refresh_view(InformationShowScreenPresenter_t *self)
{
    uint8_t idx = *self->pending_item_ptr;

    const char *title;
    if (idx == INFORMATION_PROVIDER_EVENT_LOG_IDX)
    {
        title = "Historical Events";
        build_event_log(self, s_buf, (uint32_t)sizeof(s_buf));
    }
    else if (idx == INFORMATION_SHOW_ITEM_ALARM)
    {
        title = "High Temp Alarm";
        build_alarm(self, s_buf, (uint32_t)sizeof(s_buf));
    }
    else if (idx == INFORMATION_SHOW_ITEM_CONFIRMATION)
    {
        title = "Done";
        build_confirmation(self, s_buf, (uint32_t)sizeof(s_buf));
    }
    else if (idx < INFORMATION_PROVIDER_COUNT)
    {
        title = k_providers[idx].title;
        k_providers[idx].build_fn(self, s_buf, (uint32_t)sizeof(s_buf));
    }
    else
    {
        title = "---";
        s_buf[0] = '\0';
    }

    IInformationShowScreenView_SetTitle(self->view, title);
    IInformationShowScreenView_SetContent(self->view, s_buf);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Lifecycle vtable
 * ══════════════════════════════════════════════════════════════════════════ */

static void on_enter(IScreen_t *base)
{
    InformationShowScreenPresenter_t *self = (InformationShowScreenPresenter_t *)base;
    self->active = true;

    /* For event log provider: position at most recent event on each enter */
    if (*self->pending_item_ptr == INFORMATION_PROVIDER_EVENT_LOG_IDX &&
        event_log_storage_is_valid(self->event_log_storage))
    {
        uint16_t count = 0U;
        if (EventLogStorage_GetCount(self->event_log_storage, &count) == ERR_OK && count > 0U)
        {
            self->event_log_idx = (uint16_t)(count - 1U);
        }
        else
        {
            self->event_log_idx = 0U;
        }
    }

    /* For confirmation provider: record entry time for auto-dismiss */
    if (*self->pending_item_ptr == INFORMATION_SHOW_ITEM_CONFIRMATION)
    {
        self->confirmation_start_ticks = os_ticks_get();
    }

    refresh_view(self);
}

static void _on_exit(IScreen_t *base)
{
    InformationShowScreenPresenter_t *self = (InformationShowScreenPresenter_t *)base;
    self->active = false;
}

static void on_update(IScreen_t *base)
{
    InformationShowScreenPresenter_t *self = (InformationShowScreenPresenter_t *)base;
    if (!self->active)
    {
        return;
    }
    refresh_view(self);

    /* ── Auto-dismiss alarm screen when overtemp clears ───────────── */
    if (*self->pending_item_ptr == INFORMATION_SHOW_ITEM_ALARM &&
        self->di_source != NULL && self->router != NULL)
    {
        bool overtemp = false;
        (void)DigitalInputSource_ReadInput(self->di_source, DI_ID_ALARM_OVERTEMP, &overtemp);
        if (!overtemp)
        {
            uint8_t dest = (self->back_screen_id_ptr != NULL)
                               ? *self->back_screen_id_ptr
                               : self->back_screen_id;
            (void)IScreenRouter_NavigateTo(self->router, dest);
        }
    }

    /* ── Auto-dismiss confirmation screen after 1 second ─────────── */
#define CONFIRMATION_AUTO_DISMISS_MS 1000U
    if (*self->pending_item_ptr == INFORMATION_SHOW_ITEM_CONFIRMATION &&
        self->router != NULL)
    {
        uint32_t now = os_ticks_get();
        if ((now - self->confirmation_start_ticks) >= CONFIRMATION_AUTO_DISMISS_MS)
        {
            uint8_t dest = (self->back_screen_id_ptr != NULL)
                               ? *self->back_screen_id_ptr
                               : self->back_screen_id;
            (void)IScreenRouter_NavigateTo(self->router, dest);
        }
    }
#undef CONFIRMATION_AUTO_DISMISS_MS
}

static void on_back_pressed(IScreen_t *base)
{
    InformationShowScreenPresenter_t *self = (InformationShowScreenPresenter_t *)base;
    if (self->router != NULL)
    {
        uint8_t dest = (self->back_screen_id_ptr != NULL)
                           ? *self->back_screen_id_ptr
                           : self->back_screen_id;
        (void)IScreenRouter_NavigateTo(self->router, dest);
    }
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    InformationShowScreenPresenter_t *self = (InformationShowScreenPresenter_t *)base;
    (void)event; /* PRESS and KEEPALIVE both advance the log index */

    if (*self->pending_item_ptr != INFORMATION_PROVIDER_EVENT_LOG_IDX)
    {
        return;
    }
    if (!event_log_storage_is_valid(self->event_log_storage))
    {
        return;
    }

    uint16_t count = 0U;
    if (EventLogStorage_GetCount(self->event_log_storage, &count) != ERR_OK || count == 0U)
    {
        return;
    }
    if(event != SCREEN_KEY_EVENT_PRESS)
    {
    	/*ignore event*/
    	return;
    }

    if (key == (uint8_t)SCREEN_KEY_DOWN)
    {
        /* DOWN → toward past (decrement, circular) */
        self->event_log_idx = (self->event_log_idx == 0U)
                                  ? (uint16_t)(count - 1U)
                                  : (uint16_t)(self->event_log_idx - 1U);
    }
    else if (key == (uint8_t)SCREEN_KEY_UP)
    {
        /* UP → toward recent (increment, circular) */
        self->event_log_idx = (self->event_log_idx >= (uint16_t)(count - 1U))
                                  ? 0U
                                  : (uint16_t)(self->event_log_idx + 1U);
    }
    else
    {
        return;
    }

    refresh_view(self);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialise the unified information_show presenter.
 */
Result_t InformationShowScreenPresenter_Init(InformationShowScreenPresenter_t *self,
                                             const InformationShowScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->pending_item_ptr == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));

    self->base.OnEnter = on_enter;
    self->base.OnExit = _on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = on_key_event;

    self->view = deps->view;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;
    self->pending_item_ptr = deps->pending_item_ptr;
    self->time_source = deps->time_source;
    self->relay = deps->relay;
    self->hourmeter = deps->hourmeter;
    self->config_storage = deps->config_storage;
    self->gps_source = deps->gps_source;
    self->battery = deps->battery;
    self->device_identity = deps->device_identity;
    self->license_status = deps->license_status;
    self->wifi_status = deps->wifi_status;
    self->sta_port = deps->sta_port;
    self->ap_port = deps->ap_port;
    self->back_screen_id_ptr = deps->back_screen_id_ptr;
    self->event_log_storage = deps->event_log_storage;
    self->di_source = deps->di_source;
    self->pending_confirmation_msg = deps->pending_confirmation_msg;
    self->event_log_idx = 0U;
    self->confirmation_start_ticks = 0U;

    return ERR_OK;
}

#undef APPEND
#undef APPENDL
