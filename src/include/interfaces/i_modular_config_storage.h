/**
 * @file i_modular_config_storage.h
 * @brief Generic interface for modular configuration storage (Phase 2).
 *
 * @note Replaces IConfigStorage (monolithic).
 *       Each config type is stored/loaded independently with own CRC.
 *       Dispatches calls to specialized handlers.
 *
 * Design principles:
 * - Type-safe: Each config has specific Load/Save helpers
 * - Extensible: Add new config types without breaking existing code
 * - Testable: Interface allows mock implementations
 * - Independent: Each config validated/stored separately
 */

#ifndef I_MODULAR_CONFIG_STORAGE_H
#define I_MODULAR_CONFIG_STORAGE_H

#include "hal/hal_types.h"
#include "common/relay_types.h"
#include "common/gps_types.h"
#include "common/general_types.h"
#include "common/wifi_types.h"
#include "interfaces/i_config_storage.h" /* For SuperUserConfig_t */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Config type identifiers (extensible).
     * @note TimeWindow removed: now embedded in RelayConfig_t only.
     */
    typedef enum
    {
        CONFIG_TYPE_RELAY = 1,
        CONFIG_TYPE_GPS = 2,
        CONFIG_TYPE_GENERAL = 3,
        CONFIG_TYPE_WIFI = 4,      /* Renumbered: was 5 */
        CONFIG_TYPE_SUPERUSER = 5, /* Renumbered: was 6 */
        /* Future types: CONFIG_TYPE_NETWORK, CONFIG_TYPE_DISPLAY... */
    } ConfigType_t;

    /**
     * @brief V-Table for modular config storage.
     */
    typedef struct IModularConfigStorage_Vtable
    {
        /**
         * @brief Load config by type.
         * @param self Implementation context.
         * @param type Config type identifier.
         * @param out_cfg Buffer to store loaded config (must be sized correctly).
         * @param cfg_size Expected size (for validation).
         * @return ERR_OK if valid, ERR_CHECKSUM if CRC fails (loads defaults).
         * @note Thread-safe if underlying handlers are thread-safe.
         */
        Result_t (*Load)(void *self, ConfigType_t type, void *out_cfg, size_t cfg_size);

        /**
         * @brief Save config by type.
         * @param self Implementation context.
         * @param type Config type identifier.
         * @param cfg Config data to save.
         * @param cfg_size Size of config data (excluding CRC header).
         * @return ERR_OK if written and verified, ERR_ERROR otherwise.
         * @note Performs read-back verification.
         */
        Result_t (*Save)(void *self, ConfigType_t type, const void *cfg, size_t cfg_size);

        /**
         * @brief Reset config to defaults.
         * @param self Implementation context.
         * @param type Config type identifier.
         * @return ERR_OK if reset successful.
         */
        Result_t (*Reset)(void *self, ConfigType_t type);

        /**
         * @brief Reset ALL configs to defaults.
         * @param self Implementation context.
         * @return ERR_OK if all resets successful.
         * @note Useful for factory reset scenarios.
         */
        Result_t (*ResetAll)(void *self);

    } IModularConfigStorage_Vtable;

    /**
     * @brief Interface instance.
     */
    typedef struct IModularConfigStorage
    {
        const IModularConfigStorage_Vtable *vtable;
        void *impl;
    } IModularConfigStorage;

    /* =============================================================================
     * Type-safe helpers (inline wrappers)
     * ============================================================================= */

    /**
     * @brief Validate IModularConfigStorage interface instance.
     * @note Per Interface Designer Skill: validates vtable, impl, AND critical function pointers.
     * @return true if interface is fully initialized and safe to use.
     */
    static inline bool modular_config_storage_is_valid(const IModularConfigStorage *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->vtable->Load != NULL) && (iface->vtable->Save != NULL) && (iface->vtable->Reset != NULL) && (iface->vtable->ResetAll != NULL) && (iface->impl != NULL);
    }

    /* Relay Config */
    static inline Result_t ModularConfigStorage_LoadRelay(
        IModularConfigStorage *iface, RelayConfig_t *out_cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !out_cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Load(iface->impl, CONFIG_TYPE_RELAY, out_cfg, sizeof(RelayConfig_t));
    }

    static inline Result_t ModularConfigStorage_SaveRelay(
        IModularConfigStorage *iface, const RelayConfig_t *cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Save(iface->impl, CONFIG_TYPE_RELAY, cfg, sizeof(RelayConfig_t));
    }

    static inline Result_t ModularConfigStorage_ResetRelay(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Reset(iface->impl, CONFIG_TYPE_RELAY);
    }

    /* GPS Config */
    static inline Result_t ModularConfigStorage_LoadGPS(
        IModularConfigStorage *iface, GPSConfig_t *out_cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !out_cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Load(iface->impl, CONFIG_TYPE_GPS, out_cfg, sizeof(GPSConfig_t));
    }

    static inline Result_t ModularConfigStorage_SaveGPS(
        IModularConfigStorage *iface, const GPSConfig_t *cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Save(iface->impl, CONFIG_TYPE_GPS, cfg, sizeof(GPSConfig_t));
    }

    static inline Result_t ModularConfigStorage_ResetGPS(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Reset(iface->impl, CONFIG_TYPE_GPS);
    }

    /* General Config */
    static inline Result_t ModularConfigStorage_LoadGeneral(
        IModularConfigStorage *iface, GeneralConfig_t *out_cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !out_cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Load(iface->impl, CONFIG_TYPE_GENERAL, out_cfg, sizeof(GeneralConfig_t));
    }

    static inline Result_t ModularConfigStorage_SaveGeneral(
        IModularConfigStorage *iface, const GeneralConfig_t *cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Save(iface->impl, CONFIG_TYPE_GENERAL, cfg, sizeof(GeneralConfig_t));
    }

    static inline Result_t ModularConfigStorage_ResetGeneral(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Reset(iface->impl, CONFIG_TYPE_GENERAL);
    }

    /* Wifi Config */
    /* Note: TimeWindow removed — now embedded in RelayConfig_t */
    static inline Result_t ModularConfigStorage_LoadWifi(
        IModularConfigStorage *iface, WifiConfig_t *out_cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !out_cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Load(iface->impl, CONFIG_TYPE_WIFI, out_cfg, sizeof(WifiConfig_t));
    }

    static inline Result_t ModularConfigStorage_SaveWifi(
        IModularConfigStorage *iface, const WifiConfig_t *cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Save(iface->impl, CONFIG_TYPE_WIFI, cfg, sizeof(WifiConfig_t));
    }

    static inline Result_t ModularConfigStorage_ResetWifi(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Reset(iface->impl, CONFIG_TYPE_WIFI);
    }

    /* SuperUser Config */
    static inline Result_t ModularConfigStorage_LoadSuperUser(
        IModularConfigStorage *iface, SuperUserConfig_t *out_cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !out_cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Load(iface->impl, CONFIG_TYPE_SUPERUSER, out_cfg, sizeof(SuperUserConfig_t));
    }

    static inline Result_t ModularConfigStorage_SaveSuperUser(
        IModularConfigStorage *iface, const SuperUserConfig_t *cfg)
    {
        if (!modular_config_storage_is_valid(iface) || !cfg)
            return ERR_NULL_POINTER;
        return iface->vtable->Save(iface->impl, CONFIG_TYPE_SUPERUSER, cfg, sizeof(SuperUserConfig_t));
    }

    static inline Result_t ModularConfigStorage_ResetSuperUser(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Reset(iface->impl, CONFIG_TYPE_SUPERUSER);
    }

    /* ResetAll helper */
    static inline Result_t ModularConfigStorage_ResetAll(IModularConfigStorage *iface)
    {
        if (!modular_config_storage_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->ResetAll(iface->impl);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_MODULAR_CONFIG_STORAGE_H */
