/**
 * @file superuser_config_handler.c
 * @brief Implementation of SuperUserConfigHandler (Phase 1).
 */

#include "infrastructure/adapters/storage/superuser_config_handler.h"
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

Result_t SuperUserConfigHandler_Init(SuperUserConfigHandler_t *self, const SuperUserConfigHandlerConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;
    if (config->eeprom == NULL || config->defaults == NULL)
        return ERR_NULL_POINTER;

    self->eeprom = config->eeprom;
    self->eeprom_handle = config->eeprom_handle;
    self->base_address = config->base_address;
    memcpy(&self->defaults, config->defaults, sizeof(SuperUserConfig_t));
    self->is_initialized = true;

    return ERR_OK;
}

Result_t SuperUserConfigHandler_Save(SuperUserConfigHandler_t *self, const SuperUserConfig_t *config)
{
    if (!self->is_initialized || config == NULL)
        return ERR_ERROR;

    /* Prepare storage structure */
    SuperUserConfigStorage_t storage;
    memset(&storage, 0xFF, sizeof(storage)); /* Fill with 0xFF (erased EEPROM) */

    /* Copy config and recalculate header fields */
    memcpy(&storage.config, config, sizeof(SuperUserConfig_t));
    storage.config.magic = SUPERUSER_CONFIG_MAGIC;
    storage.config.size = 32; /* Data size: 40 - 8 (header) = 32 bytes */

    /* Calculate CRC over data (skip 8-byte header) */
    uint8_t *data_ptr = (uint8_t *)&storage.config + 8;
    storage.config.checksum = calculate_crc16(data_ptr, 32);

    /* Write to EEPROM */
    Result_t res = EXT_EEPROM_WriteData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (const uint8_t *)&storage,
        sizeof(SuperUserConfigStorage_t));

    if (res != ERR_OK)
        return res;

    /* Read-back verification */
    SuperUserConfigStorage_t verify;
    res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (uint8_t *)&verify,
        sizeof(SuperUserConfigStorage_t));

    if (res != ERR_OK || memcmp(&storage, &verify, sizeof(storage)) != 0)
        return ERR_ERROR;

    return ERR_OK;
}

Result_t SuperUserConfigHandler_Load(SuperUserConfigHandler_t *self, SuperUserConfig_t *out_config)
{
    if (!self->is_initialized || out_config == NULL)
        return ERR_ERROR;

    SuperUserConfigStorage_t storage;

    /* Read from EEPROM */
    Result_t res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        self->base_address,
        (uint8_t *)&storage,
        sizeof(SuperUserConfigStorage_t));

    if (res != ERR_OK)
    {
        memcpy(out_config, &self->defaults, sizeof(SuperUserConfig_t));
        return ERR_ERROR;
    }

    /* Validate magic */
    if (storage.config.magic != SUPERUSER_CONFIG_MAGIC)
    {
        memcpy(out_config, &self->defaults, sizeof(SuperUserConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate size */
    if (storage.config.size != 32)
    {
        memcpy(out_config, &self->defaults, sizeof(SuperUserConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate CRC */
    uint8_t *data_ptr = (uint8_t *)&storage.config + 8;
    uint16_t computed_crc = calculate_crc16(data_ptr, 32);
    if (computed_crc != storage.config.checksum)
    {
        memcpy(out_config, &self->defaults, sizeof(SuperUserConfig_t));
        return ERR_CHECKSUM;
    }

    /* Success */
    memcpy(out_config, &storage.config, sizeof(SuperUserConfig_t));
    return ERR_OK;
}

Result_t SuperUserConfigHandler_Reset(SuperUserConfigHandler_t *self)
{
    if (!self->is_initialized)
        return ERR_ERROR;

    return SuperUserConfigHandler_Save(self, &self->defaults);
}
