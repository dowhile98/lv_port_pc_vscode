/**
 * @file relay_config_handler.c
 * @brief Handler for RelayConfig persistence (Phase 1 - v4.0 modular storage).
 *
 * @note TDD Implementation: Red → Green → Refactor
 *
 * Phase: 1 (Config Handlers)
 * Author: Storage Redesign Team
 * Date: 2026-03-03
 */

#include "infrastructure/adapters/storage/relay_config_handler.h"
#include <string.h>

/* ===== Private Prototypes ===== */

/**
 * @brief Calculate CRC16-CCITT over data.
 *
 * @param[in] data   Data buffer.
 * @param[in] len    Length in bytes.
 *
 * @return CRC16 value.
 */
static uint16_t calculate_crc16(const uint8_t *data, uint16_t len);

/* ===== Public API Implementation ===== */

Result_t RelayConfigHandler_Init(
    RelayConfigHandler_t *self,
    const RelayConfigHandlerConfig_t *config)
{
    /* Validate parameters */
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->eeprom == NULL || config->default_config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Initialize context */
    self->eeprom = config->eeprom;
    self->eeprom_handle = config->eeprom_handle;
    self->default_config = config->default_config;
    self->is_initialized = true;

    return ERR_OK;
}

Result_t RelayConfigHandler_Save(
    RelayConfigHandler_t *self,
    const RelayConfig_t *cfg)
{
    /* Validate parameters */
    if (self == NULL || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->is_initialized)
    {
        return ERR_ERROR;
    }

    /* Prepare storage structure */
    RelayConfigStorage_t storage;
    memset(&storage, 0xFF, sizeof(storage)); // Pre-fill with 0xFF (EEPROM erased state)

    storage.magic = RELAY_CONFIG_MAGIC;
    storage.size = RELAY_CONFIG_DATA_SIZE;
    memcpy(&storage.data, cfg, sizeof(RelayConfig_t));

    /* Calculate CRC over data only (offset 8+) */
    storage.checksum = calculate_crc16((const uint8_t *)&storage.data, RELAY_CONFIG_DATA_SIZE);

    /* Write to EEPROM */
    Result_t res = EXT_EEPROM_WriteData(
        self->eeprom,
        self->eeprom_handle,
        RELAY_CONFIG_OFFSET,
        (const uint8_t *)&storage,
        RELAY_CONFIG_TOTAL_SIZE);

    if (res != ERR_OK)
    {
        return res;
    }

    /* Read-back verification */
    RelayConfigStorage_t verify_buf;
    res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        RELAY_CONFIG_OFFSET,
        (uint8_t *)&verify_buf,
        RELAY_CONFIG_TOTAL_SIZE);

    if (res != ERR_OK)
    {
        return ERR_CHECKSUM;
    }

    /* Verify critical fields */
    if (verify_buf.magic != RELAY_CONFIG_MAGIC ||
        verify_buf.size != RELAY_CONFIG_DATA_SIZE ||
        verify_buf.checksum != storage.checksum)
    {
        return ERR_CHECKSUM;
    }

    return ERR_OK;
}

Result_t RelayConfigHandler_Load(
    RelayConfigHandler_t *self,
    RelayConfig_t *out_cfg)
{
    /* Validate parameters */
    if (self == NULL || out_cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->is_initialized)
    {
        return ERR_ERROR;
    }

    /* Read from EEPROM */
    RelayConfigStorage_t storage;
    Result_t res = EXT_EEPROM_ReadData(
        self->eeprom,
        self->eeprom_handle,
        RELAY_CONFIG_OFFSET,
        (uint8_t *)&storage,
        RELAY_CONFIG_TOTAL_SIZE);

    if (res != ERR_OK)
    {
        /* Load defaults on read error */
        memcpy(out_cfg, self->default_config, sizeof(RelayConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate magic */
    if (storage.magic != RELAY_CONFIG_MAGIC)
    {
        memcpy(out_cfg, self->default_config, sizeof(RelayConfig_t));
        return ERR_CHECKSUM;
    }

    /* Validate size */
    if (storage.size != RELAY_CONFIG_DATA_SIZE)
    {
        memcpy(out_cfg, self->default_config, sizeof(RelayConfig_t));
        return ERR_CHECKSUM;
    }

    /* Calculate and verify CRC */
    uint16_t calculated_crc = calculate_crc16((const uint8_t *)&storage.data, RELAY_CONFIG_DATA_SIZE);
    if (calculated_crc != storage.checksum)
    {
        memcpy(out_cfg, self->default_config, sizeof(RelayConfig_t));
        return ERR_CHECKSUM;
    }

    /* Copy validated data */
    memcpy(out_cfg, &storage.data, sizeof(RelayConfig_t));

    return ERR_OK;
}

Result_t RelayConfigHandler_Reset(RelayConfigHandler_t *self)
{
    /* Validate parameters */
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->is_initialized)
    {
        return ERR_ERROR;
    }

    /* Write default config */
    return RelayConfigHandler_Save(self, self->default_config);
}

/* ===== Private Implementation ===== */

/**
 * @brief Calculate CRC16-CCITT (poly 0x1021, init 0xFFFF).
 *
 * @note Same algorithm as v3.0 (compatibility).
 *
 * @param[in] data   Data buffer.
 * @param[in] len    Length in bytes.
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
                crc = (crc << 1) ^ 0x1021; /* Polynomial */
            }
            else
            {
                crc = crc << 1;
            }
        }
    }

    return crc;
}
