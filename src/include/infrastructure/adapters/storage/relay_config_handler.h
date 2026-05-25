/**
 * @file relay_config_handler.h
 * @brief Handler for RelayConfig persistence (Phase 1 - v4.0 modular storage).
 *
 * @note Handles RelayConfig_t as independent module with own CRC.
 *
 * Storage Layout:
 * - Offset: 0x0000
 * - Total size: 128 bytes
 * - Header: magic(4) + size(2) + checksum(2) = 8 bytes
 * - Data: RelayConfig_t = 102 bytes
 * - Padding: 18 bytes (0xFF)
 * - Magic: 0xC1C10401
 *
 * Phase: 1 (Config Handlers)
 * Author: Storage Redesign Team
 * Date: 2026-03-03
 */

#ifndef RELAY_CONFIG_HANDLER_H
#define RELAY_CONFIG_HANDLER_H

#include "hal_types.h"
#include "common/relay_types.h"
#include "interfaces/i_ext_eeprom.h"
#include "infrastructure/storage/eeprom_layout.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== Constants ===== */

#define RELAY_CONFIG_MAGIC 0xC1C10404U /**< Magic number for RelayConfig v4.2 (RELAY_MAX_CYCLES=10) */
#define RELAY_CONFIG_DATA_SIZE 198U    /**< sizeof(RelayConfig_t) with RELAY_MAX_CYCLES=10 */

/* Compatibility aliases — offsets from eeprom_layout.h */
#define RELAY_CONFIG_OFFSET EEPROM_RELAY_CONFIG_OFFSET
#define RELAY_CONFIG_TOTAL_SIZE EEPROM_RELAY_CONFIG_SIZE

/* ===== Types ===== */

/**
 * @brief Storage format for RelayConfig (on-disk representation).
 *
 * @note Layout v4.2 (RELAY_MAX_CYCLES=10):
 *   [0:3]     magic     (0xC1C10404)
 *   [4:5]     size      (198)
 *   [6:7]     checksum  (CRC16-CCITT over data)
 *   [8:205]   data      (RelayConfig_t, 198 bytes)
 *   [206:215] reserved  (10 bytes, 0xFF)
 */
typedef struct __attribute__((packed)) RelayConfigStorage
{
    uint32_t magic;       /**< Magic number: 0xC1C10404 */
    uint16_t size;        /**< Data size: 198 bytes */
    uint16_t checksum;    /**< CRC16-CCITT over data (offset 8+) */
    RelayConfig_t data;   /**< RelayConfig_t (198 bytes) */
    uint8_t reserved[10]; /**< Padding to 216 bytes (EEPROM_RELAY_CONFIG_SIZE) */
} RelayConfigStorage_t;

_Static_assert(sizeof(RelayConfigStorage_t) == 216, "RelayConfigStorage_t must be 216 bytes (8+198+10)");

/**
 * @brief RelayConfigHandler context.
 */
typedef struct RelayConfigHandler
{
    I_EXT_EEPROM *eeprom;                /**< EEPROM interface (injected) */
    I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM handle */
    const RelayConfig_t *default_config; /**< Default config (fallback) */
    bool is_initialized;                 /**< Initialization flag */
} RelayConfigHandler_t;

/**
 * @brief Initialization config for RelayConfigHandler.
 */
typedef struct RelayConfigHandlerConfig
{
    I_EXT_EEPROM *eeprom;                /**< EEPROM interface (required) */
    I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM handle */
    const RelayConfig_t *default_config; /**< Default config (required) */
} RelayConfigHandlerConfig_t;

/* ===== Public API ===== */

/**
 * @brief Initialize RelayConfigHandler.
 *
 * @param[in] self         Handler context (must not be NULL).
 * @param[in] config       Initialization config (must not be NULL).
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if params invalid.
 */
Result_t RelayConfigHandler_Init(
    RelayConfigHandler_t *self,
    const RelayConfigHandlerConfig_t *config);

/**
 * @brief Save RelayConfig to EEPROM.
 *
 * @note Writes 128 bytes @ offset 0x0000.
 * @note Performs read-back verification after write.
 *
 * @param[in] self         Handler context (must be initialized).
 * @param[in] cfg          RelayConfig to save (must not be NULL).
 *
 * @return ERR_OK if saved and verified, ERR_CHECKSUM if verification failed.
 */
Result_t RelayConfigHandler_Save(
    RelayConfigHandler_t *self,
    const RelayConfig_t *cfg);

/**
 * @brief Load RelayConfig from EEPROM.
 *
 * @note Reads 128 bytes @ offset 0x0000.
 * @note Validates magic, size, and CRC.
 * @note On any validation failure, loads defaults and returns ERR_CHECKSUM.
 *
 * @param[in]  self        Handler context (must be initialized).
 * @param[out] out_cfg     Buffer for loaded config (must not be NULL).
 *
 * @return ERR_OK if valid, ERR_CHECKSUM if validation failed (defaults loaded).
 */
Result_t RelayConfigHandler_Load(
    RelayConfigHandler_t *self,
    RelayConfig_t *out_cfg);

/**
 * @brief Reset RelayConfig to defaults.
 *
 * @note Writes default config to EEPROM.
 *
 * @param[in] self         Handler context (must be initialized).
 *
 * @return ERR_OK on success, error code otherwise.
 */
Result_t RelayConfigHandler_Reset(RelayConfigHandler_t *self);

#endif /* RELAY_CONFIG_HANDLER_H */
