/**
 * @file modular_config_storage_adapter.c
 * @brief Coordinator adapter dispatching to config handlers.
 */

#include "infrastructure/adapters/storage/modular_config_storage_adapter.h"
#include <string.h>

/* ────────────────────────────────────────────────────────────────────────────
 * Default Configurations (fallback values)
 * ──────────────────────────────────────────────────────────────────────────── */

static const RelayConfig_t s_default_relay_config = {
    .contact_type = RELAY_TYPE_NC,
    .enabled = 0,
    .simple_cycle = {.ton = 1000, .toff = 1000},
    .ton_margin_ms = 0,
    .toff_margin_ms = 0,
    .start_with_on = 1,
    .time_window = {
        .start_time = {
            0,
            0,
            0,
            6,
            0,
        },                             /* 00:00 */
        .stop_time = {0, 0, 0, 18, 0}, /* 23:59 */
        .weekday_mask = 0x7F           /* All days enabled (Mon-Sun) */
    }};

static const GPSConfig_t s_default_gps_config = {
    .antenna_type = 0, /* 0=Internal */
    .time_offset = 0,
    .utc_offset_index = 8,
	.antenna_switch_timeout_min = 10,
};

static const GeneralConfig_t s_default_general_config = {
    .buzzer_on_time_ms = 50,             /* 100ms buzzer duration */
    .buzzer_high_temp_alarm = 1,          /* Enable high temp alarm */
    .screen_blacklight_timeout_ms = 15000 /* 30 seconds */
};

/* s_default_time_window_config removed — TimeWindow is now embedded in RelayConfig_t */

static const WifiConfig_t s_default_wifi_config = {
    .mode = 2, /* AP */
	.wifi_timeout_minutes = 10,
    .ap_ssid = "", /* Initialized at first boot by DI container with "TCS-<uid>" */
    .ap_pwd = "12345678",
    .ap_channel = 6,
    .ap_max_conn = 4,
    .sta_use_dhcp = 1,
    .sta_ssid = "TSL-Interrupter",
    .sta_pwd = "Tecna2026",
    .ap_use_dhcp_server = 1,
    .sta_ip = "192.168.1.40",
    .sta_mask = "255.255.255.0",
    .sta_gateway = "192.168.1.1",
    .ap_ip = "192.168.8.1",
    .ap_mask = "255.255.255.0",
	.wifi_enable = 1,
	.wifi_timeout_minutes = 10,
};

static const SuperUserConfig_t s_default_superuser_config = {
    .magic = SUPERUSER_CONFIG_MAGIC,
    .size = 32,
    .checksum = 0,
    .license_key = 0,
    .expiration_date = 0,
    .gps_pps_offset_us = 0,
    .reserved = {0}};

/* ────────────────────────────────────────────────────────────────────────────
 * Private Implementation Functions (V-Table)
 * ──────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Load config by dispatching to appropriate handler.
 */
static Result_t load_impl(
    void *self,
    ConfigType_t type,
    void *out_cfg,
    size_t cfg_size)
{
    ModularConfigStorageAdapter_t *adapter = (ModularConfigStorageAdapter_t *)self;

    if (adapter == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    switch (type)
    {
    case CONFIG_TYPE_RELAY:
        if (cfg_size != sizeof(RelayConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return RelayConfigHandler_Load(&adapter->relay_handler, (RelayConfig_t *)out_cfg);

    case CONFIG_TYPE_GPS:
        if (cfg_size != sizeof(GPSConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return GPSConfigHandler_Load(&adapter->gps_handler, (GPSConfig_t *)out_cfg);

    case CONFIG_TYPE_GENERAL:
        if (cfg_size != sizeof(GeneralConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return GeneralConfigHandler_Load(&adapter->general_handler, (GeneralConfig_t *)out_cfg);

    case CONFIG_TYPE_WIFI:
        if (cfg_size != sizeof(WifiConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return WifiConfigHandler_Load(&adapter->wifi_handler, (WifiConfig_t *)out_cfg);

    case CONFIG_TYPE_SUPERUSER:
        if (cfg_size != sizeof(SuperUserConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return SuperUserConfigHandler_Load(&adapter->superuser_handler, (SuperUserConfig_t *)out_cfg);

    default:
        return ERR_INVALID_PARAM;
    }
}

/**
 * @brief Save config by dispatching to appropriate handler.
 */
static Result_t save_impl(
    void *self,
    ConfigType_t type,
    const void *cfg,
    size_t cfg_size)
{
    ModularConfigStorageAdapter_t *adapter = (ModularConfigStorageAdapter_t *)self;

    if (adapter == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    switch (type)
    {
    case CONFIG_TYPE_RELAY:
        if (cfg_size != sizeof(RelayConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return RelayConfigHandler_Save(&adapter->relay_handler, (const RelayConfig_t *)cfg);

    case CONFIG_TYPE_GPS:
        if (cfg_size != sizeof(GPSConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return GPSConfigHandler_Save(&adapter->gps_handler, (const GPSConfig_t *)cfg);

    case CONFIG_TYPE_GENERAL:
        if (cfg_size != sizeof(GeneralConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return GeneralConfigHandler_Save(&adapter->general_handler, (const GeneralConfig_t *)cfg);

    case CONFIG_TYPE_WIFI:
        if (cfg_size != sizeof(WifiConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return WifiConfigHandler_Save(&adapter->wifi_handler, (const WifiConfig_t *)cfg);

    case CONFIG_TYPE_SUPERUSER:
        if (cfg_size != sizeof(SuperUserConfig_t))
        {
            return ERR_INVALID_PARAM;
        }
        return SuperUserConfigHandler_Save(&adapter->superuser_handler, (const SuperUserConfig_t *)cfg);

    default:
        return ERR_INVALID_PARAM;
    }
}

/**
 * @brief Reset single config by dispatching to appropriate handler.
 */
static Result_t reset_impl(void *self, ConfigType_t type)
{
    ModularConfigStorageAdapter_t *adapter = (ModularConfigStorageAdapter_t *)self;

    if (adapter == NULL)
    {
        return ERR_NULL_POINTER;
    }

    switch (type)
    {
    case CONFIG_TYPE_RELAY:
        return RelayConfigHandler_Reset(&adapter->relay_handler);

    case CONFIG_TYPE_GPS:
        return GPSConfigHandler_Reset(&adapter->gps_handler);

    case CONFIG_TYPE_GENERAL:
        return GeneralConfigHandler_Reset(&adapter->general_handler);

    case CONFIG_TYPE_WIFI:
        return WifiConfigHandler_Reset(&adapter->wifi_handler);

    case CONFIG_TYPE_SUPERUSER:
        return SuperUserConfigHandler_Reset(&adapter->superuser_handler);

    default:
        return ERR_INVALID_PARAM;
    }
}

/**
 * @brief Reset all configs (factory reset).
 */
static Result_t reset_all_impl(void *self)
{
    ModularConfigStorageAdapter_t *adapter = (ModularConfigStorageAdapter_t *)self;

    if (adapter == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Reset all handlers (continue even if one fails) */
    Result_t res = ERR_OK;
    Result_t temp;

    temp = RelayConfigHandler_Reset(&adapter->relay_handler);
    if (temp != ERR_OK)
    {
        res = temp;
    }

    temp = GPSConfigHandler_Reset(&adapter->gps_handler);
    if (temp != ERR_OK)
    {
        res = temp;
    }

    temp = GeneralConfigHandler_Reset(&adapter->general_handler);
    if (temp != ERR_OK)
    {
        res = temp;
    }

    temp = WifiConfigHandler_Reset(&adapter->wifi_handler);
    if (temp != ERR_OK)
    {
        res = temp;
    }

    temp = SuperUserConfigHandler_Reset(&adapter->superuser_handler);
    if (temp != ERR_OK)
    {
        res = temp;
    }

    return res;
}

/* ────────────────────────────────────────────────────────────────────────────
 * V-Table
 * ──────────────────────────────────────────────────────────────────────────── */

static const IModularConfigStorage_Vtable s_vtable = {
    .Load = load_impl,
    .Save = save_impl,
    .Reset = reset_impl,
    .ResetAll = reset_all_impl};

/* ────────────────────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t ModularConfigStorageAdapter_Init(
    ModularConfigStorageAdapter_t *self,
    const ModularConfigStorageAdapterConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->eeprom == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));

    /* Initialize all handlers */
    Result_t res;

    RelayConfigHandlerConfig_t relay_cfg = {
        .eeprom = config->eeprom,
        .eeprom_handle = config->eeprom_handle,
        .default_config = &s_default_relay_config};
    res = RelayConfigHandler_Init(&self->relay_handler, &relay_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    GPSConfigHandlerConfig_t gps_cfg = {
        .eeprom = config->eeprom,
        .eeprom_handle = config->eeprom_handle,
        .default_config = &s_default_gps_config};
    res = GPSConfigHandler_Init(&self->gps_handler, &gps_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    GeneralConfigHandlerConfig_t general_cfg = {
        .eeprom = config->eeprom,
        .eeprom_handle = config->eeprom_handle,
        .default_config = &s_default_general_config};
    res = GeneralConfigHandler_Init(&self->general_handler, &general_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    /* TimeWindowConfigHandler_Init removed — TimeWindow embedded in RelayConfig_t */

    WifiConfigHandlerConfig_t wifi_cfg = {
        .eeprom = config->eeprom,
        .eeprom_handle = config->eeprom_handle,
        .base_address = WIFI_CONFIG_ADDRESS,
        .defaults = &s_default_wifi_config};
    res = WifiConfigHandler_Init(&self->wifi_handler, &wifi_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    SuperUserConfigHandlerConfig_t superuser_cfg = {
        .eeprom = config->eeprom,
        .eeprom_handle = config->eeprom_handle,
        .base_address = SUPERUSER_CONFIG_ADDRESS,
        .defaults = &s_default_superuser_config};
    res = SuperUserConfigHandler_Init(&self->superuser_handler, &superuser_cfg);
    if (res != ERR_OK)
    {
        return res;
    }

    /* Setup interface */
    self->interface.vtable = &s_vtable;
    self->interface.impl = self;

    return ERR_OK;
}

IModularConfigStorage *ModularConfigStorageAdapter_GetInterface(
    ModularConfigStorageAdapter_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return &self->interface;
}
