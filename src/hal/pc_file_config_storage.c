/**
 * @file pc_file_config_storage.c
 * @brief Filesystem-backed IConfigStorage for the PC simulator.
 *
 * Replaces the EEPROM (M24M01E via ModularConfigStorageAdapter) with a plain
 * binary file.  The file layout mirrors the real firmware's SystemConfig_t so
 * struct changes cause an automatic magic mismatch and a clean re-initialisation
 * with defaults.
 *
 * Thread-safety: all public functions are called only from the UI-AO thread
 * (single-threaded LVGL context), so no additional locking is required.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "pc_file_config_storage.h"

#include <stdio.h>
#include <string.h>

#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include "common/gps_types.h"
#include "common/general_types.h"
#include "common/wifi_types.h"

/* ============================================================================
 * CONSTANTS
 * ========================================================================== */

/** Magic that identifies a valid PC config file.  Change when struct layout
 *  changes to force re-initialisation with defaults. */
#define PC_CFG_MAGIC 0xC1C1BEEFu
#define PC_CFG_VERSION 0x00000001u

#define PC_CFG_FILE_PATH "simulator_config.bin"

/* ============================================================================
 * FILE LAYOUT
 * ========================================================================== */

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint32_t version;
    SuperUserConfig_t super_user;
    RelayConfig_t relay;
    GPSConfig_t gps;
    GeneralConfig_t general;
    TimeWindowConfig_t time_window;
    WifiConfig_t wifi;
} PcConfigFile_t;

/* ============================================================================
 * DEFAULTS
 * ========================================================================== */

/**
 * @brief Build a PcConfigFile_t with sensible non-zero values.
 *
 * The goal is that every UI screen shows meaningful data on first launch
 * instead of all-zero placeholders.
 */
static PcConfigFile_t make_defaults(void)
{
    PcConfigFile_t f;
    memset(&f, 0, sizeof(f));

    f.magic = PC_CFG_MAGIC;
    f.version = PC_CFG_VERSION;

    /* ── SuperUser ─────────────────────────────────────────────────────── */
    f.super_user.magic = SUPERUSER_CONFIG_MAGIC;
    f.super_user.size = 32U;
    f.super_user.checksum = 0U;
    f.super_user.license_key = 0U;
    f.super_user.expiration_date = 0U;
    f.super_user.gps_pps_offset_us = 0;

    /* ── Relay ─────────────────────────────────────────────────────────── */
    f.relay.contact_type = RELAY_TYPE_NC; /* 1 = Normally Closed */
    f.relay.enabled = 0U;
    f.relay.simple_cycle.ton = 1000U;
    f.relay.simple_cycle.toff = 1000U;
    f.relay.ton_margin_ms = 0U;
    f.relay.toff_margin_ms = 0U;
    f.relay.start_with_on = 1U;
    /* time_window: start 00:00, stop 18:00, all weekdays */
    f.relay.time_window.stop_time.hour = 18U;
    f.relay.time_window.weekday_mask = 0x7FU;

    /* ── GPS ───────────────────────────────────────────────────────────── */
    f.gps.antenna_type = 0U; /* Internal */
    f.gps.time_offset = 0;
    f.gps.utc_offset_index = 8U;
    f.gps.antenna_switch_timeout_min = 10U;

    /* ── General ───────────────────────────────────────────────────────── */
    f.general.buzzer_on_time_ms = 50U;
    f.general.buzzer_high_temp_alarm = 1U;
    f.general.screen_blacklight_timeout_ms = 15000U;

    /* ── TimeWindow ────────────────────────────────────────────────────── */
    /* Embedded in RelayConfig_t — set above */

    /* ── WiFi ──────────────────────────────────────────────────────────── */
    f.wifi.mode = 2U; /* AP */
    f.wifi.wifi_enable = 1U;
    f.wifi.wifi_timeout_minutes = 10U;
    /* ap_ssid: firmware sets "TCS-<uid>" at first boot; use fixed name for simulator */
    (void)strncpy(f.wifi.ap_ssid, "CICX1-SIM", sizeof(f.wifi.ap_ssid) - 1U);
    (void)strncpy(f.wifi.ap_pwd, "12345678", sizeof(f.wifi.ap_pwd) - 1U);
    f.wifi.ap_channel = 6U;
    f.wifi.ap_max_conn = 4U;
    f.wifi.ap_use_dhcp_server = 1U;
    (void)strncpy(f.wifi.ap_ip, "192.168.8.1", sizeof(f.wifi.ap_ip) - 1U);
    (void)strncpy(f.wifi.ap_mask, "255.255.255.0", sizeof(f.wifi.ap_mask) - 1U);
    f.wifi.sta_use_dhcp = 1U;
    (void)strncpy(f.wifi.sta_ssid, "TSL-Interrupter", sizeof(f.wifi.sta_ssid) - 1U);
    (void)strncpy(f.wifi.sta_pwd, "Tecna2026", sizeof(f.wifi.sta_pwd) - 1U);
    (void)strncpy(f.wifi.sta_ip, "192.168.1.40", sizeof(f.wifi.sta_ip) - 1U);
    (void)strncpy(f.wifi.sta_mask, "255.255.255.0", sizeof(f.wifi.sta_mask) - 1U);
    (void)strncpy(f.wifi.sta_gateway, "192.168.1.1", sizeof(f.wifi.sta_gateway) - 1U);

    return f;
}

/* ============================================================================
 * INTERNAL STATE
 * ========================================================================== */

typedef struct
{
    PcConfigFile_t data;
    bool loaded;
} PCFileCfgState_t;

static PCFileCfgState_t s_state;

/* ============================================================================
 * FILE I/O HELPERS
 * ========================================================================== */

static bool file_load(PcConfigFile_t *out)
{
    FILE *fp = fopen(PC_CFG_FILE_PATH, "rb");
    if (fp == NULL)
    {
        return false;
    }

    bool ok = (fread(out, sizeof(*out), 1U, fp) == 1U);
    (void)fclose(fp);

    if (!ok || out->magic != PC_CFG_MAGIC || out->version != PC_CFG_VERSION)
    {
        return false;
    }
    return true;
}

static void file_save(const PcConfigFile_t *data)
{
    FILE *fp = fopen(PC_CFG_FILE_PATH, "wb");
    if (fp == NULL)
    {
        return; /* Silently skip — worst case: next boot starts from defaults again */
    }
    (void)fwrite(data, sizeof(*data), 1U, fp);
    (void)fclose(fp);
}

/**
 * @brief Load the file once on first access; create with defaults if needed.
 */
static PCFileCfgState_t *get_state(void)
{
    if (s_state.loaded)
    {
        return &s_state;
    }

    if (!file_load(&s_state.data))
    {
        s_state.data = make_defaults();
        file_save(&s_state.data);
    }

    s_state.loaded = true;
    return &s_state;
}

/* ============================================================================
 * VTABLE IMPLEMENTATIONS
 * ========================================================================== */

static Result_t pcf_load_superuser(void *self, SuperUserConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.super_user;
    return ERR_OK;
}

static Result_t pcf_save_superuser(void *self, const SuperUserConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.super_user = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

static Result_t pcf_reset(void *self)
{
    (void)self;
    s_state.data = make_defaults();
    s_state.loaded = true;
    file_save(&s_state.data);
    return ERR_OK;
}

static Result_t pcf_load_relay(void *self, RelayConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.relay;
    return ERR_OK;
}

static Result_t pcf_save_relay(void *self, const RelayConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.relay = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

static Result_t pcf_load_gps(void *self, GPSConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.gps;
    return ERR_OK;
}

static Result_t pcf_save_gps(void *self, const GPSConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.gps = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

static Result_t pcf_load_general(void *self, GeneralConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.general;
    return ERR_OK;
}

static Result_t pcf_save_general(void *self, const GeneralConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.general = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

static Result_t pcf_load_timewindow(void *self, TimeWindowConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.time_window;
    return ERR_OK;
}

static Result_t pcf_save_timewindow(void *self, const TimeWindowConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.time_window = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

static Result_t pcf_load_wifi(void *self, WifiConfig_t *out)
{
    (void)self;
    if (out == NULL)
        return ERR_NULL_POINTER;
    *out = get_state()->data.wifi;
    return ERR_OK;
}

static Result_t pcf_save_wifi(void *self, const WifiConfig_t *cfg)
{
    (void)self;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    get_state()->data.wifi = *cfg;
    file_save(&get_state()->data);
    return ERR_OK;
}

/* ============================================================================
 * SINGLETON
 * ========================================================================== */

static const IConfigStorage_Vtable s_vtable = {
    .LoadSuperUserConfig = pcf_load_superuser,
    .SaveSuperUserConfig = pcf_save_superuser,
    .ResetToDefaults = pcf_reset,
    .LoadRelayConfig = pcf_load_relay,
    .SaveRelayConfig = pcf_save_relay,
    .LoadGPSConfig = pcf_load_gps,
    .SaveGPSConfig = pcf_save_gps,
    .LoadGeneralConfig = pcf_load_general,
    .SaveGeneralConfig = pcf_save_general,
    .LoadTimeWindowConfig = pcf_load_timewindow,
    .SaveTimeWindowConfig = pcf_save_timewindow,
    .LoadWifiConfig = pcf_load_wifi,
    .SaveWifiConfig = pcf_save_wifi,
};

static IConfigStorage s_instance = {
    .vtable = &s_vtable,
    .impl = &s_state, /* non-NULL; not used by vtable functions */
};

IConfigStorage *PCFileConfigStorage_GetInstance(void)
{
    return &s_instance;
}
