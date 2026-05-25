/**
 * @file storage_coordinator_config_adapter_v2.c
 * @brief IConfigStorage adapter backed by StorageCoordinatorAO_v2 (modular configs).
 * @version 2.0.0
 * @date 3 marzo 2026
 */

#include "infrastructure/adapters/storage/storage_coordinator_config_adapter_v2.h"
#include <string.h>

/* ===== Private Prototypes ===== */
#if 0
/**
 * @brief Calculate CRC16-CCITT (poly 0x1021, init 0xFFFF).
 * @note Same algorithm used by config handlers.
 */
static uint16_t calculate_crc16(const uint8_t *data, uint16_t len);
#endif
/* ===== V-Table Implementations ===== */

/**
 * @brief Loads SuperUserConfig from v2 coordinator cache.
 */
static Result_t load_superuser_config_v2(void *impl, SuperUserConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;

    /* Directly delegate to v2 coordinator */
    return StorageCoordinatorAO_v2_GetSuperUserConfig(coordinator, out_cfg);
}

/**
 * @brief Saves SuperUserConfig via v2 coordinator.
 */
static Result_t save_superuser_config_v2(void *impl, const SuperUserConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;

    /* Directly delegate to v2 coordinator */
    return StorageCoordinatorAO_v2_SaveSuperUserConfig(coordinator, cfg);
}

/**
 * @brief Factory reset — not supported in v2 adapter.
 *
 * Factory reset is a privileged operation that should go through a dedicated
 * system command, not the UI config adapter. Return ERR_ERROR to force callers
 * to use the correct channel.
 *
 * @note Resets Relay, GPS, General and WiFi configs to factory defaults.
 *       SuperUser (AdminConfig) is intentionally preserved.
 */
static Result_t reset_to_defaults_v2(void *impl)
{
    if (impl == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;

    Result_t res = ERR_OK;
    Result_t r;

    /* Reset each config type individually, skip SuperUser */
    r = StorageCoordinatorAO_v2_ResetConfig(coordinator, CONFIG_TYPE_RELAY);
    if (r != ERR_OK)
    {
        res = r;
    }

    r = StorageCoordinatorAO_v2_ResetConfig(coordinator, CONFIG_TYPE_GPS);
    if (r != ERR_OK)
    {
        res = r;
    }

    r = StorageCoordinatorAO_v2_ResetConfig(coordinator, CONFIG_TYPE_GENERAL);
    if (r != ERR_OK)
    {
        res = r;
    }

    r = StorageCoordinatorAO_v2_ResetConfig(coordinator, CONFIG_TYPE_WIFI);
    if (r != ERR_OK)
    {
        res = r;
    }

    /* CONFIG_TYPE_SUPERUSER intentionally excluded */
    return res;
}

/* ===== Modular Config Functions (v2.0 - efficient partial access) ===== */

/**
 * @brief Load only RelayConfig_t from v2 coordinator cache.
 * @note Direct cache read, no I2C, <1μs latency.
 */
static Result_t load_relay_config_v2(void *impl, RelayConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_GetRelayConfig(coordinator, out_cfg);
}

static Result_t save_relay_config_v2(void *impl, const RelayConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_SaveRelayConfig(coordinator, cfg);
}

static Result_t load_gps_config_v2(void *impl, GPSConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_GetGPSConfig(coordinator, out_cfg);
}

static Result_t save_gps_config_v2(void *impl, const GPSConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_SaveGPSConfig(coordinator, cfg);
}

static Result_t load_general_config_v2(void *impl, GeneralConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_GetGeneralConfig(coordinator, out_cfg);
}

static Result_t save_general_config_v2(void *impl, const GeneralConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_SaveGeneralConfig(coordinator, cfg);
}

/**
 * @brief Load TimeWindowConfig from the relay cache (time_window is embedded in RelayConfig_t).
 */
static Result_t load_time_window_config_v2(void *impl, TimeWindowConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    RelayConfig_t relay;
    Result_t res = StorageCoordinatorAO_v2_GetRelayConfig(coordinator, &relay);
    if (res == ERR_OK)
    {
        *out_cfg = relay.time_window;
    }
    return res;
}

/**
 * @brief Save TimeWindowConfig by updating relay.time_window and re-saving the full RelayConfig.
 */
static Result_t save_time_window_config_v2(void *impl, const TimeWindowConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    RelayConfig_t relay;
    Result_t res = StorageCoordinatorAO_v2_GetRelayConfig(coordinator, &relay);
    if (res != ERR_OK)
    {
        return res;
    }
    relay.time_window = *cfg;
    return StorageCoordinatorAO_v2_SaveRelayConfig(coordinator, &relay);
}

static Result_t load_wifi_config_v2(void *impl, WifiConfig_t *out_cfg)
{
    if (impl == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_GetWifiConfig(coordinator, out_cfg);
}

static Result_t save_wifi_config_v2(void *impl, const WifiConfig_t *cfg)
{
    if (impl == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    StorageCoordinatorAO_v2_t *coordinator = (StorageCoordinatorAO_v2_t *)impl;
    return StorageCoordinatorAO_v2_SaveWifiConfig(coordinator, cfg);
}

/* ===== V-Table ===== */

static const IConfigStorage_Vtable s_vtable_v2 = {
    /* Monolithic (legacy v1 compatibility) */
    .LoadSuperUserConfig = load_superuser_config_v2,
    .SaveSuperUserConfig = save_superuser_config_v2,
    .ResetToDefaults = reset_to_defaults_v2,

    /* Modular (v2.0 - efficient partial access) */
    .LoadRelayConfig = load_relay_config_v2,
    .SaveRelayConfig = save_relay_config_v2,
    .LoadGPSConfig = load_gps_config_v2,
    .SaveGPSConfig = save_gps_config_v2,
    .LoadGeneralConfig = load_general_config_v2,
    .SaveGeneralConfig = save_general_config_v2,
    .LoadTimeWindowConfig = load_time_window_config_v2,
    .SaveTimeWindowConfig = save_time_window_config_v2,
    .LoadWifiConfig = load_wifi_config_v2,
    .SaveWifiConfig = save_wifi_config_v2,
};

/* ===== Public API ===== */

Result_t StorageCoordinatorConfigAdapter_v2_Init(
    StorageCoordinatorConfigAdapter_v2_t *self,
    StorageCoordinatorAO_v2_t *coordinator)
{
    if (self == NULL || coordinator == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(StorageCoordinatorConfigAdapter_v2_t));

    /* Wire IConfigStorage vtable to v2 backend */
    self->iface.vtable = &s_vtable_v2;
    self->iface.impl = coordinator;

    return ERR_OK;
}

IConfigStorage *StorageCoordinatorConfigAdapter_v2_GetInterface(
    StorageCoordinatorConfigAdapter_v2_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}

/* ===== Private Helpers ===== */
#if 0
/**
 * @brief Calculate CRC16-CCITT (poly 0x1021, init 0xFFFF).
 *
 * Standard CRC16-CCITT algorithm used by all config handlers.
 *
 * @param[in] data  Data buffer.
 * @param[in] len   Length in bytes.
 *
 * @return CRC16 value.
 */
static uint16_t calculate_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF; /* CRC16-CCITT init value */

    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021; /* Polynomial 0x1021 */
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}
#endif
