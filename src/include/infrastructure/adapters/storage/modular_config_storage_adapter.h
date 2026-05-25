/**
 * @file modular_config_storage_adapter.h
 * @brief Adapter that coordinates all config handlers via IModularConfigStorage.
 *
 * Dispatches Load/Save/Reset/ResetAll operations to specialized handlers
 * based on ConfigType_t.
 *
 * Uses (Layout v4.2):
 * - RelayConfigHandler  (216B @ 0x0000)  [includes TimeWindow, RELAY_MAX_CYCLES=10]
 * - GPSConfigHandler    (21B  @ 0x00D8)
 * - GeneralConfigHandler(27B  @ 0x00ED)
 * - WifiConfigHandler   (256B @ auto-aligned page)
 * - SuperUserConfigHandler (64B @ auto-aligned page)
 */

#ifndef MODULAR_CONFIG_STORAGE_ADAPTER_H
#define MODULAR_CONFIG_STORAGE_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal/hal_types.h"
#include "interfaces/i_modular_config_storage.h"
#include "interfaces/i_ext_eeprom.h"
#include "infrastructure/adapters/storage/relay_config_handler.h"
#include "infrastructure/adapters/storage/gps_config_handler.h"
#include "infrastructure/adapters/storage/general_config_handler.h"
#include "infrastructure/adapters/storage/wifi_config_handler.h"
#include "infrastructure/adapters/storage/superuser_config_handler.h"

    /**
     * @brief Configuration for ModularConfigStorageAdapter initialization.
     */
    typedef struct
    {
        I_EXT_EEPROM *eeprom;                /**< EEPROM interface (non-const for handlers) */
        I_EXT_EEPROM_Handle_t eeprom_handle; /**< EEPROM handle */
    } ModularConfigStorageAdapterConfig_t;

    /**
     * @brief Adapter holding all config handlers.
     *
     * Dispatches operations to appropriate handler based on ConfigType_t.
     */
    typedef struct
    {
        RelayConfigHandler_t relay_handler;         /**< Handles relay config (216B @ 0x0000) */
        GPSConfigHandler_t gps_handler;             /**< Handles GPS config (21B @ 0x00D8) */
        GeneralConfigHandler_t general_handler;     /**< Handles general config (27B @ 0x00ED) */
        WifiConfigHandler_t wifi_handler;           /**< Handles WiFi config (256B @ auto-aligned) */
        SuperUserConfigHandler_t superuser_handler; /**< Handles superuser config (64B @ auto-aligned) */
        /* TimeWindowConfigHandler removed — TimeWindow is now embedded in RelayConfig_t */

        IModularConfigStorage interface; /**< Public interface */
    } ModularConfigStorageAdapter_t;

    /**
     * @brief Initialize the ModularConfigStorageAdapter.
     *
     * Initializes all internal handlers (relay, GPS, general, time window, WiFi, superuser).
     * Each handler is initialized with its respective EEPROM address offset.
     *
     * @param[in,out] self   Adapter instance.
     * @param[in]     config Configuration containing EEPROM interface.
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self or config is NULL.
     * @return Other error codes from handler Init methods.
     */
    Result_t ModularConfigStorageAdapter_Init(
        ModularConfigStorageAdapter_t *self,
        const ModularConfigStorageAdapterConfig_t *config);

    /**
     * @brief Get IModularConfigStorage interface for this adapter.
     *
     * @param[in] self Adapter instance.
     *
     * @return Pointer to IModularConfigStorage interface.
     * @return NULL if self is NULL.
     */
    IModularConfigStorage *ModularConfigStorageAdapter_GetInterface(
        ModularConfigStorageAdapter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* MODULAR_CONFIG_STORAGE_ADAPTER_H */
