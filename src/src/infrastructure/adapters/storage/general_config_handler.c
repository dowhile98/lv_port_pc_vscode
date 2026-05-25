/**
 * @file general_config_handler.c
 * @brief Handler for GeneralConfig persistence (Phase 1).
 */

#include "infrastructure/adapters/storage/general_config_handler.h"
#include <string.h>

static uint16_t calculate_crc16(const uint8_t *data, uint16_t len);

Result_t GeneralConfigHandler_Init(GeneralConfigHandler_t *self, const GeneralConfigHandlerConfig_t *config)
{
    if (self == NULL || config == NULL || config->eeprom == NULL || config->default_config == NULL)
        return ERR_NULL_POINTER;

    self->eeprom = config->eeprom;
    self->eeprom_handle = config->eeprom_handle;
    self->default_config = config->default_config;
    self->is_initialized = true;
    return ERR_OK;
}

Result_t GeneralConfigHandler_Save(GeneralConfigHandler_t *self, const GeneralConfig_t *cfg)
{
    if (self == NULL || cfg == NULL || !self->is_initialized)
        return ERR_NULL_POINTER;

    GeneralConfigStorage_t storage;
    memset(&storage, 0xFF, sizeof(storage));
    storage.magic = GENERAL_CONFIG_MAGIC;
    storage.size = GENERAL_CONFIG_DATA_SIZE;
    memcpy(&storage.data, cfg, sizeof(GeneralConfig_t));
    storage.checksum = calculate_crc16((const uint8_t *)&storage.data, GENERAL_CONFIG_DATA_SIZE);

    Result_t res = EXT_EEPROM_WriteData(self->eeprom, self->eeprom_handle, GENERAL_CONFIG_OFFSET,
                                        (const uint8_t *)&storage, GENERAL_CONFIG_TOTAL_SIZE);
    if (res != ERR_OK)
        return res;

    GeneralConfigStorage_t verify_buf;
    res = EXT_EEPROM_ReadData(self->eeprom, self->eeprom_handle, GENERAL_CONFIG_OFFSET,
                              (uint8_t *)&verify_buf, GENERAL_CONFIG_TOTAL_SIZE);
    if (res != ERR_OK)
        return ERR_CHECKSUM;

    if (verify_buf.magic != GENERAL_CONFIG_MAGIC || verify_buf.checksum != storage.checksum)
        return ERR_CHECKSUM;

    return ERR_OK;
}

Result_t GeneralConfigHandler_Load(GeneralConfigHandler_t *self, GeneralConfig_t *out_cfg)
{
    if (self == NULL || out_cfg == NULL || !self->is_initialized)
        return ERR_NULL_POINTER;

    GeneralConfigStorage_t storage;
    Result_t res = EXT_EEPROM_ReadData(self->eeprom, self->eeprom_handle, GENERAL_CONFIG_OFFSET,
                                       (uint8_t *)&storage, GENERAL_CONFIG_TOTAL_SIZE);

    if (res != ERR_OK || storage.magic != GENERAL_CONFIG_MAGIC || storage.size != GENERAL_CONFIG_DATA_SIZE ||
        calculate_crc16((const uint8_t *)&storage.data, GENERAL_CONFIG_DATA_SIZE) != storage.checksum)
    {
        memcpy(out_cfg, self->default_config, sizeof(GeneralConfig_t));
        return ERR_CHECKSUM;
    }

    memcpy(out_cfg, &storage.data, sizeof(GeneralConfig_t));
    return ERR_OK;
}

Result_t GeneralConfigHandler_Reset(GeneralConfigHandler_t *self)
{
    if (self == NULL || !self->is_initialized)
        return ERR_NULL_POINTER;
    return GeneralConfigHandler_Save(self, self->default_config);
}

static uint16_t calculate_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}
