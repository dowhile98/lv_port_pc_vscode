/**
 * @file wifi_config_handler.h
 * @brief Handler for WifiConfig_t storage operations (Phase 1).
 *
 * Handles WifiConfig_t storage with:
 * - Storage: 256 bytes @ 0x0100
 * - Magic: 0xC1C10405
 * - Data: 248 bytes (WifiConfig_t)
 * - Header: 8 bytes (magic + size + checksum)
 * - Padding: 0 bytes (248 + 8 = 256)
 */

#ifndef WIFI_CONFIG_HANDLER_H
#define WIFI_CONFIG_HANDLER_H

#include "interfaces/i_ext_eeprom.h"
#include "common/wifi_types.h"
#include "hal/hal_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "infrastructure/storage/eeprom_layout.h"

/* Storage constants */
#define WIFI_CONFIG_MAGIC 0xC1C10405U

/* Compatibility aliases — offsets from eeprom_layout.h */
#define WIFI_CONFIG_ADDRESS      EEPROM_WIFI_CONFIG_OFFSET
#define WIFI_CONFIG_STORAGE_SIZE EEPROM_WIFI_CONFIG_SIZE

    /**
     * @brief Storage structure for WifiConfig_t (256 bytes).
     */
    typedef struct __attribute__((packed))
    {
        uint32_t magic;    /**< Magic number 0xC1C10405 */
        uint16_t size;     /**< Data size (248 bytes) */
        uint16_t checksum; /**< CRC16-CCITT over data */
        WifiConfig_t data; /**< 248 bytes */
    } WifiConfigStorage_t;

    _Static_assert(sizeof(WifiConfigStorage_t) == 256, "WifiConfigStorage_t must be 256 bytes");

    /**
     * @brief Configuration for WifiConfigHandler initialization.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;                /**< EEPROM interface */
        I_EXT_EEPROM_Handle_t eeprom_handle; /**< Handle opaco I2C */
        uint16_t base_address;               /**< Base address (0x0100) */
        const WifiConfig_t *defaults;        /**< Default config */
    } WifiConfigHandlerConfig_t;

    /**
     * @brief WifiConfig handler instance.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;
        I_EXT_EEPROM_Handle_t eeprom_handle;
        uint16_t base_address;
        WifiConfig_t defaults;
        bool is_initialized;
    } WifiConfigHandler_t;

    /**
     * @brief Initialize WifiConfigHandler.
     * @param self Handler instance (not NULL).
     * @param config Configuration (not NULL).
     * @return ERR_OK on success, ERR_NULL_POINTER if params invalid.
     */
    Result_t WifiConfigHandler_Init(WifiConfigHandler_t *self, const WifiConfigHandlerConfig_t *config);

    /**
     * @brief Save WifiConfig to EEPROM.
     * @param self Handler instance.
     * @param config Config to save.
     * @return ERR_OK on success, ERR_ERROR on verification failure.
     */
    Result_t WifiConfigHandler_Save(WifiConfigHandler_t *self, const WifiConfig_t *config);

    /**
     * @brief Load WifiConfig from EEPROM.
     * @param self Handler instance.
     * @param out_config Output buffer.
     * @return ERR_OK on success, ERR_CHECKSUM if validation fails (loads defaults).
     */
    Result_t WifiConfigHandler_Load(WifiConfigHandler_t *self, WifiConfig_t *out_config);

    /**
     * @brief Reset WifiConfig to defaults.
     * @param self Handler instance.
     * @return ERR_OK on success.
     */
    Result_t WifiConfigHandler_Reset(WifiConfigHandler_t *self);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CONFIG_HANDLER_H */
