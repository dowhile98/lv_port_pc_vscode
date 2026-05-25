/**
 * @file general_config_handler.h
 * @brief Handler for GeneralConfig persistence (Phase 1).
 *
 * Storage: 0x00A0, 32 bytes, Magic 0xC1C10403
 */

#ifndef GENERAL_CONFIG_HANDLER_H
#define GENERAL_CONFIG_HANDLER_H

#include "hal_types.h"
#include "common/general_types.h"
#include "interfaces/i_ext_eeprom.h"
#include <stdint.h>
#include <stdbool.h>

#include "infrastructure/storage/eeprom_layout.h"

#define GENERAL_CONFIG_MAGIC 0xC1C10403U
#define GENERAL_CONFIG_DATA_SIZE 9U /**< sizeof(GeneralConfig_t): buzzer_on_time_ms(4)+buzzer_high_temp_alarm(1)+screen_blacklight_timeout_ms(4) */

/* Compatibility aliases — offsets from eeprom_layout.h */
#define GENERAL_CONFIG_OFFSET EEPROM_GENERAL_CONFIG_OFFSET
#define GENERAL_CONFIG_TOTAL_SIZE EEPROM_GENERAL_CONFIG_SIZE

typedef struct __attribute__((packed)) GeneralConfigStorage
{
    uint32_t magic;
    uint16_t size;
    uint16_t checksum;
    GeneralConfig_t data;
    uint8_t reserved[15]; /**< Padding to 32 bytes. Shrink by 1 for each byte added to GeneralConfig_t. */
} GeneralConfigStorage_t;

_Static_assert(sizeof(GeneralConfigStorage_t) == 32, "GeneralConfigStorage_t must be 32 bytes");

typedef struct GeneralConfigHandler
{
    I_EXT_EEPROM *eeprom;
    I_EXT_EEPROM_Handle_t eeprom_handle;
    const GeneralConfig_t *default_config;
    bool is_initialized;
} GeneralConfigHandler_t;

typedef struct GeneralConfigHandlerConfig
{
    I_EXT_EEPROM *eeprom;
    I_EXT_EEPROM_Handle_t eeprom_handle;
    const GeneralConfig_t *default_config;
} GeneralConfigHandlerConfig_t;

Result_t GeneralConfigHandler_Init(GeneralConfigHandler_t *self, const GeneralConfigHandlerConfig_t *config);
Result_t GeneralConfigHandler_Save(GeneralConfigHandler_t *self, const GeneralConfig_t *cfg);
Result_t GeneralConfigHandler_Load(GeneralConfigHandler_t *self, GeneralConfig_t *out_cfg);
Result_t GeneralConfigHandler_Reset(GeneralConfigHandler_t *self);

#endif /* GENERAL_CONFIG_HANDLER_H */
