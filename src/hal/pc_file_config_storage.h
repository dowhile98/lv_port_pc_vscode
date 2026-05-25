/**
 * @file pc_file_config_storage.h
 * @brief PC filesystem-backed IConfigStorage — replaces EEPROM for the simulator.
 *
 * Stores all configuration sections in a single binary file:
 *   @c simulator_config.bin  (in the process's current working directory)
 *
 * On first run (file absent or magic mismatch), the file is created with
 * sensible non-zero defaults so that all UI screens show meaningful values
 * instead of "0 / empty".
 *
 * File layout (@c PcConfigFile_t):
 *   ┌────────────┬──────────────────┐
 *   │ magic (4B) │ version (4B)     │
 *   ├────────────┴──────────────────┤
 *   │ SuperUserConfig_t  (40 B)     │
 *   │ RelayConfig_t      (198 B)    │
 *   │ GPSConfig_t        (4 B)      │
 *   │ GeneralConfig_t    (9 B)      │
 *   │ TimeWindowConfig_t (17 B)     │
 *   │ WifiConfig_t       (248 B)    │
 *   └───────────────────────────────┘
 *
 * Mirrors the real firmware pattern (ModularConfigStorageAdapter + M24M01E EEPROM)
 * without any hardware dependency.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef PC_FILE_CONFIG_STORAGE_H
#define PC_FILE_CONFIG_STORAGE_H

#include "interfaces/i_config_storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialise the PC file config storage.
     *
     * Opens (or creates) @c simulator_config.bin.  If the file does not exist,
     * or its magic does not match, sensible non-zero defaults are written and
     * then returned.
     *
     * Safe to call multiple times — subsequent calls are no-ops that return
     * the same instance.
     *
     * @return Pointer to the singleton @c IConfigStorage interface.
     */
    IConfigStorage *PCFileConfigStorage_GetInstance(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_FILE_CONFIG_STORAGE_H */
