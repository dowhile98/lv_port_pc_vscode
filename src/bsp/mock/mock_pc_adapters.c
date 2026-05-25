/**
 * @file mock_pc_adapters.c
 * @brief PC simulator stub adapters for required HomeScreenPresenter deps.
 *
 * Provides minimal implementations of the four interfaces that
 * HomeScreenPresenter requires as non-NULL:
 *   - ITimeSource          → returns PC system time (via time.h)
 *   - IGPSSource           → returns "no fix" (static stub)
 *   - IWifiStatusSource_t  → returns disconnected
 *   - IConfigStorage       → returns default config on load, ignores saves
 *
 * Behavior on PC matches the device behaviour for a freshly powered unit:
 *   no GPS fix, no WiFi, default relay/GPS config, system clock as time.
 *
 * @note These adapters have zero hardware dependencies — they run on any POSIX
 *       host.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "hal_types.h"
#include "common/date_time.h"
#include "common/gps_types.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_wifi_status_source.h"
#include "interfaces/i_config_storage.h"

/* ============================================================================
 * ITimeSource — PC system clock
 * ========================================================================== */

static Result_t pc_time_get(void *self, DateTime_t *out_time)
{
    (void)self;
    if (out_time == NULL)
    {
        return ERR_NULL_POINTER;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t == NULL)
    {
        memset(out_time, 0, sizeof(*out_time));
        return ERR_ERROR;
    }
    out_time->year = (uint16_t)(t->tm_year + 1900);
    out_time->month = (uint8_t)(t->tm_mon + 1);
    out_time->day = (uint8_t)t->tm_mday;
    out_time->hour = (uint8_t)t->tm_hour;
    out_time->minute = (uint8_t)t->tm_min;
    out_time->second = (uint8_t)t->tm_sec;
    out_time->weekDay = (uint8_t)(t->tm_wday == 0 ? 7U : (uint8_t)t->tm_wday);
    return ERR_OK;
}

static Result_t pc_time_set(void *self, const DateTime_t *time)
{
    (void)self;
    (void)time;
    return ERR_OK; /* No-op on PC */
}

static bool pc_time_is_synchronized(void *self)
{
    (void)self;
    return true; /* PC always has valid time */
}

static const ITimeSource_Vtable s_pc_time_vtable = {
    .GetTime = pc_time_get,
    .SetTime = pc_time_set,
    .IsSynchronized = pc_time_is_synchronized,
};

static ITimeSource s_pc_time_source = {
    .vtable = &s_pc_time_vtable,
    .impl = (void *)1, /* non-NULL sentinel — impl not used */
};

ITimeSource *MockTimeSource_GetInstance(void)
{
    return &s_pc_time_source;
}

/* ============================================================================
 * IGPSSource — no fix stub
 * ========================================================================== */

static Result_t pc_gps_get_position(void *self, GPSPosition_t *out_pos)
{
    (void)self;
    if (out_pos == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out_pos, 0, sizeof(*out_pos));
    return ERR_OK;
}

static Result_t pc_gps_get_time_utc(void *self, DateTime_t *out_time)
{
    (void)self;
    if (out_time == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out_time, 0, sizeof(*out_time));
    return ERR_OK;
}

static Result_t pc_gps_get_fix_status(void *self, GPSFixStatus_t *out_status)
{
    (void)self;
    if (out_status == NULL)
    {
        return ERR_NULL_POINTER;
    }
    *out_status = GPS_FIX_NONE;
    return ERR_OK;
}

static Result_t pc_gps_register_pps(void *self, PPSCallback_t cb, void *ctx)
{
    (void)self;
    (void)cb;
    (void)ctx;
    return ERR_OK;
}

static const IGPSSource_Vtable s_pc_gps_vtable = {
    .GetPosition = pc_gps_get_position,
    .GetTimeUTC = pc_gps_get_time_utc,
    .GetFixStatus = pc_gps_get_fix_status,
    .RegisterPPSCallback = pc_gps_register_pps,
};

static IGPSSource s_pc_gps_source = {
    .vtable = &s_pc_gps_vtable,
    .impl = (void *)1,
};

IGPSSource *MockGpsSource_GetInstance(void)
{
    return &s_pc_gps_source;
}

/* ============================================================================
 * IWifiStatusSource_t — always disconnected
 * ========================================================================== */

static bool pc_wifi_is_connected(void *impl)
{
    (void)impl;
    return false;
}

static const IWifiStatusSource_Vtable s_pc_wifi_vtable = {
    .IsConnected = pc_wifi_is_connected,
};

static IWifiStatusSource_t s_pc_wifi_status = {
    .vtable = &s_pc_wifi_vtable,
    .impl = (void *)1,
};

IWifiStatusSource_t *MockWifiStatusSource_GetInstance(void)
{
    return &s_pc_wifi_status;
}

/* ============================================================================
 * IConfigStorage — default values, save is a no-op
 * ========================================================================== */

static Result_t pc_cfg_load_superuser(void *self, SuperUserConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    out->magic = SUPERUSER_CONFIG_MAGIC;
    out->size = (uint16_t)(sizeof(*out) - 8U);
    out->license_key = 0U; /* OPERATION_MODE_FREE */
    return ERR_OK;
}
static Result_t pc_cfg_save_superuser(void *s, const SuperUserConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}
static Result_t pc_cfg_reset(void *s)
{
    (void)s;
    return ERR_OK;
}

static Result_t pc_cfg_load_relay(void *self, RelayConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    return ERR_OK;
}
static Result_t pc_cfg_save_relay(void *s, const RelayConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}

static Result_t pc_cfg_load_gps(void *self, GPSConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    return ERR_OK;
}
static Result_t pc_cfg_save_gps(void *s, const GPSConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}

static Result_t pc_cfg_load_general(void *self, GeneralConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    return ERR_OK;
}
static Result_t pc_cfg_save_general(void *s, const GeneralConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}

static Result_t pc_cfg_load_timewindow(void *self, TimeWindowConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    return ERR_OK;
}
static Result_t pc_cfg_save_timewindow(void *s, const TimeWindowConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}

static Result_t pc_cfg_load_wifi(void *self, WifiConfig_t *out)
{
    (void)self;
    if (out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    memset(out, 0, sizeof(*out));
    return ERR_OK;
}
static Result_t pc_cfg_save_wifi(void *s, const WifiConfig_t *c)
{
    (void)s;
    (void)c;
    return ERR_OK;
}

static const IConfigStorage_Vtable s_pc_cfg_vtable = {
    .LoadSuperUserConfig = pc_cfg_load_superuser,
    .SaveSuperUserConfig = pc_cfg_save_superuser,
    .ResetToDefaults = pc_cfg_reset,
    .LoadRelayConfig = pc_cfg_load_relay,
    .SaveRelayConfig = pc_cfg_save_relay,
    .LoadGPSConfig = pc_cfg_load_gps,
    .SaveGPSConfig = pc_cfg_save_gps,
    .LoadGeneralConfig = pc_cfg_load_general,
    .SaveGeneralConfig = pc_cfg_save_general,
    .LoadTimeWindowConfig = pc_cfg_load_timewindow,
    .SaveTimeWindowConfig = pc_cfg_save_timewindow,
    .LoadWifiConfig = pc_cfg_load_wifi,
    .SaveWifiConfig = pc_cfg_save_wifi,
};

static IConfigStorage s_pc_config_storage = {
    .vtable = &s_pc_cfg_vtable,
    .impl = (void *)1,
};

IConfigStorage *MockConfigStorage_GetInstance(void)
{
    return &s_pc_config_storage;
}
