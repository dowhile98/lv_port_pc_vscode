/**
 * @file gps_config_handler.h
 * @brief Handler for GPSConfig persistence (Phase 1 - v4.0 modular storage).
 *
 * @note Handles GPSConfig_t as independent module with own CRC.
 *
 * Storage Layout:
 * - Offset: 0x0080
 * - Total size: 32 bytes
 * - Header: magic(4) + size(2) + checksum(2) = 8 bytes
 * - Data: GPSConfig_t = 4 bytes
 * - Padding: 20 bytes (0xFF)
 * - Magic: 0xC1C10402
 *
 * Phase: 1 (Config Handlers)
 * Author: Storage Redesign Team
 * Date: 2026-03-03
 */

#ifndef GPS_CONFIG_HANDLER_H
#define GPS_CONFIG_HANDLER_H

#include "hal_types.h"
#include "common/gps_types.h"
#include "interfaces/i_ext_eeprom.h"
#include "infrastructure/storage/eeprom_layout.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== Constants ===== */

#define GPS_CONFIG_MAGIC 0xC1C10402U /**< Magic number for GPSConfig v4.0 */
#define GPS_CONFIG_DATA_SIZE 4U      /**< sizeof(GPSConfig_t) */

/* Compatibility aliases — offsets from eeprom_layout.h */
#define GPS_CONFIG_OFFSET EEPROM_GPS_CONFIG_OFFSET
#define GPS_CONFIG_TOTAL_SIZE EEPROM_GPS_CONFIG_SIZE

/* ===== Types ===== */

/**
 * @brief Storage format for GPSConfig (on-disk representation).
 *
 * @note Layout:
 *   [0:3]    magic     (0xC1C10402)
 *   [4:5]    size      (4)
 *   [6:7]    checksum  (CRC16-CCITT over data)
 *   [8:11]   data      (GPSConfig_t, 4 bytes)
 *   [12:31]  reserved  (20 bytes, 0xFF)
 */
typedef struct __attribute__((packed)) GPSConfigStorage
{
    uint32_t magic;       /**< Magic number: 0xC1C10402 */
    uint16_t size;        /**< Data size: 4 bytes */
    uint16_t checksum;    /**< CRC16-CCITT over data (offset 8+) */
    GPSConfig_t data;     /**< GPSConfig_t (4 bytes) */
    uint8_t reserved[20]; /**< Padding to 32 bytes */
} GPSConfigStorage_t;

_Static_assert(sizeof(GPSConfigStorage_t) == 32, "GPSConfigStorage_t must be 32 bytes");

/**
 * @brief GPSConfigHandler context.
 */
typedef struct GPSConfigHandler
{
    I_EXT_EEPROM *eeprom;                /**< EEPROM interface (injected) */
    I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM handle */
    const GPSConfig_t *default_config;   /**< Default config (fallback) */
    bool is_initialized;                 /**< Initialization flag */
} GPSConfigHandler_t;

/**
 * @brief Initialization config for GPSConfigHandler.
 */
typedef struct GPSConfigHandlerConfig
{
    I_EXT_EEPROM *eeprom;                /**< EEPROM interface (required) */
    I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM handle */
    const GPSConfig_t *default_config;   /**< Default config (required) */
} GPSConfigHandlerConfig_t;

/* ===== Public API ===== */

/**
 * @brief Initialize GPSConfigHandler.
 *
 * @param[in] self         Handler context (must not be NULL).
 * @param[in] config       Initialization config (must not be NULL).
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if params invalid.
 */
Result_t GPSConfigHandler_Init(
    GPSConfigHandler_t *self,
    const GPSConfigHandlerConfig_t *config);

/**
 * @brief Save GPSConfig to EEPROM.
 *
 * @note Writes 32 bytes @ offset 0x0080.
 * @note Performs read-back verification after write.
 *
 * @param[in] self         Handler context (must be initialized).
 * @param[in] cfg          GPSConfig to save (must not be NULL).
 *
 * @return ERR_OK if saved and verified, ERR_CHECKSUM if verification failed.
 */
Result_t GPSConfigHandler_Save(
    GPSConfigHandler_t *self,
    const GPSConfig_t *cfg);

/**
 * @brief Load GPSConfig from EEPROM.
 *
 * @note Reads 32 bytes @ offset 0x0080.
 * @note Validates magic, size, and CRC.
 * @note On any validation failure, loads defaults and returns ERR_CHECKSUM.
 *
 * @param[in]  self        Handler context (must be initialized).
 * @param[out] out_cfg     Buffer for loaded config (must not be NULL).
 *
 * @return ERR_OK if valid, ERR_CHECKSUM if validation failed (defaults loaded).
 */
Result_t GPSConfigHandler_Load(
    GPSConfigHandler_t *self,
    GPSConfig_t *out_cfg);

/**
 * @brief Reset GPSConfig to defaults.
 *
 * @note Writes default config to EEPROM.
 *
 * @param[in] self         Handler context (must be initialized).
 *
 * @return ERR_OK on success, error code otherwise.
 */
Result_t GPSConfigHandler_Reset(GPSConfigHandler_t *self);

#endif /* GPS_CONFIG_HANDLER_H */
