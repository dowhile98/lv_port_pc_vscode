/**
 * @file wifi_config_handler.c
 * @brief Implementation of WifiConfigHandler (Phase 1).
 */

#include "infrastructure/adapters/storage/wifi_config_handler.h"
#include <string.h>

/* ────────────────────────────────────────────────────────────────────────────
 * Private Functions
 * ──────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Calculate CRC16-CCITT over data (polynomial 0x1021, init 0xFFFF).
 */
static uint16_t calculate_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t WifiConfigHandler_Init(WifiConfigHandler_t *self, const WifiConfigHandlerConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;
    if (config->eeprom == NULL || config->defaults == NULL)
        return ERR_NULL_POINTER;

    self->eeprom = config->eeprom;
    self->eeprom_handle = config->eeprom_handle;
    self->base_address = config->base_address;
    memcpy(&self->defaults, config->defaults, sizeof(WifiConfig_t));
    self->is_initialized = true;

    return ERR_OK;
}

Result_t WifiConfigHandler_Save(WifiConfigHandler_t *self, const WifiConfig_t *config)
{
    if (!self->is_initialized || config == NULL)
        return ERR_ERROR;

    /* Prepare storage structure */
    WifiConfigStorage_t storage;
    memset(&storage, 0xFF, sizeof(storage)); /* Fill with 0xFF (erased EEPROM) */

    storage.magic = WIFI_CONFIG_MAGIC;
    storage.size = sizeof(WifiConfig_t);
    memcpy(&storage.data, config, sizeof(WifiConfig_t));
    storage.checksum = calculate_crc16((uint8_t *)&storage.data, sizeof(WifiConfig_t));

    /* Write to EEPROM */
    Result_t res = EXT_EEPROM_WriteData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (const uint8_t *)&storage,
        sizeof(WifiConfigStorage_t));

    if (res != ERR_OK)
        return res;

    /* Read-back verification */
    WifiConfigStorage_t verify;
    res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (uint8_t *)&verify,
        sizeof(WifiConfigStorage_t));

    if (res != ERR_OK || memcmp(&storage, &verify, sizeof(storage)) != 0)
        return ERR_ERROR;

    return ERR_OK;
}

Result_t WifiConfigHandler_Load(WifiConfigHandler_t *self, WifiConfig_t *out_config)
{
    if (!self->is_initialized || out_config == NULL)
        return ERR_ERROR;

    WifiConfigStorage_t storage;

    /* Read from EEPROM */
    Result_t res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (uint8_t *)&storage,
        sizeof(WifiConfigStorage_t));

    if (res != ERR_OK)
    {
        memcpy(out_config, &self->defaults, sizeof(WifiConfig_t));
        return ERR_ERROR;
    }

    /* Validate magic */
    if (storage.magic != WIFI_CONFIG_MAGIC)
    {
        memcpy(out_config, &self->defaults, sizeof(WifiConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate size */
    if (storage.size != sizeof(WifiConfig_t))
    {
        memcpy(out_config, &self->defaults, sizeof(WifiConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate CRC */
    uint16_t computed_crc = calculate_crc16((uint8_t *)&storage.data, sizeof(WifiConfig_t));
    if (computed_crc != storage.checksum)
    {
        memcpy(out_config, &self->defaults, sizeof(WifiConfig_t));
        return ERR_CHECKSUM;
    }

    /* Success */
    memcpy(out_config, &storage.data, sizeof(WifiConfig_t));
    return ERR_OK;
}

Result_t WifiConfigHandler_Reset(WifiConfigHandler_t *self)
{
    if (!self->is_initialized)
        return ERR_ERROR;

    return WifiConfigHandler_Save(self, &self->defaults);
}
