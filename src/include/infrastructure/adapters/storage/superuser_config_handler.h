/**
 * @file superuser_config_handler.h
 * @brief Handler for SuperUserConfig_t storage operations (Phase 1).
 *
 * Handles SuperUserConfig_t storage with:
 * - Storage: 64 bytes @ 0x0200
 * - Magic: 0xC1C15EE0
 * - Data: 32 bytes (SuperUserConfig_t without header)
 * - Header: 8 bytes (magic + size + checksum)
 * - Padding: 24 bytes
 * - Total: 40 bytes (struct) + 24 padding = 64 bytes storage
 */

#ifndef SUPERUSER_CONFIG_HANDLER_H
#define SUPERUSER_CONFIG_HANDLER_H

#include "interfaces/i_ext_eeprom.h"
#include "interfaces/i_config_storage.h"
#include "hal/hal_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Storage constants */
#include "infrastructure/storage/eeprom_layout.h"

#define SUPERUSER_CONFIG_MAGIC 0xC1C15EE0U

/* Compatibility aliases — offsets from eeprom_layout.h */
#define SUPERUSER_CONFIG_ADDRESS      EEPROM_SUPERUSER_CONFIG_OFFSET
#define SUPERUSER_CONFIG_STORAGE_SIZE EEPROM_SUPERUSER_CONFIG_SIZE

    /**
     * @brief Storage structure for SuperUserConfig_t (64 bytes).
     *
     * Note: SuperUserConfig_t is 40 bytes (8 header + 32 data).
     * We add 24 bytes padding to reach 64 bytes storage size.
     */
    typedef struct __attribute__((packed))
    {
        SuperUserConfig_t config; /**< 40 bytes (includes magic/size/checksum header) */
        uint8_t reserved[24];     /**< 24 bytes padding */
    } SuperUserConfigStorage_t;

    _Static_assert(sizeof(SuperUserConfigStorage_t) == 64, "SuperUserConfigStorage_t must be 64 bytes");

    /**
     * @brief Configuration for SuperUserConfigHandler initialization.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;                /**< EEPROM interface */
        I_EXT_EEPROM_Handle_t eeprom_handle; /**< Handle opaco I2C */
        uint16_t base_address;               /**< Base address (0x0200) */
        const SuperUserConfig_t *defaults;   /**< Default config */
    } SuperUserConfigHandlerConfig_t;

    /**
     * @brief SuperUserConfig handler instance.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;
        I_EXT_EEPROM_Handle_t eeprom_handle;
        uint16_t base_address;
        SuperUserConfig_t defaults;
        bool is_initialized;
    } SuperUserConfigHandler_t;

    /**
     * @brief Initialize SuperUserConfigHandler.
     * @param self Handler instance (not NULL).
     * @param config Configuration (not NULL).
     * @return ERR_OK on success, ERR_NULL_POINTER if params invalid.
     */
    Result_t SuperUserConfigHandler_Init(SuperUserConfigHandler_t *self, const SuperUserConfigHandlerConfig_t *config);

    /**
     * @brief Save SuperUserConfig to EEPROM.
     * @param self Handler instance.
     * @param config Config to save.
     * @return ERR_OK on success, ERR_ERROR on verification failure.
     */
    Result_t SuperUserConfigHandler_Save(SuperUserConfigHandler_t *self, const SuperUserConfig_t *config);

    /**
     * @brief Load SuperUserConfig from EEPROM.
     * @param self Handler instance.
     * @param out_config Output buffer.
     * @return ERR_OK on success, ERR_CHECKSUM if validation fails (loads defaults).
     */
    Result_t SuperUserConfigHandler_Load(SuperUserConfigHandler_t *self, SuperUserConfig_t *out_config);

    /**
     * @brief Reset SuperUserConfig to defaults.
     * @param self Handler instance.
     * @return ERR_OK on success.
     */
    Result_t SuperUserConfigHandler_Reset(SuperUserConfigHandler_t *self);

#ifdef __cplusplus
}
#endif

#endif /* SUPERUSER_CONFIG_HANDLER_H */
