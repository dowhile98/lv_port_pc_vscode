/**
 * @file i_config_storage.h
 * @brief Interfaz para persistencia de configuración del sistema.
 *
 * @note Esta interfaz abstrae el almacenamiento de configuración,
 *       permitiendo múltiples implementaciones (EEPROM, Flash, Mock).
 */

#ifndef I_CONFIG_STORAGE_H
#define I_CONFIG_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // NULL
#include "hal_types.h"
#include "common/relay_types.h"   /* RelayConfig_t, TimeWindowConfig_t */
#include "common/gps_types.h"     /* GPSConfig_t */
#include "common/general_types.h" /* GeneralConfig_t */
#include "common/wifi_types.h"    /* WifiConfig_t — v2.0 layout */

/**
 * @brief Versión del layout de SystemConfig_t almacenado en EEPROM.
 *
 * v1.0 (0x0100): relay, gps, general, window, reserved[32]
 * v2.0 (0x0200): igual + WifiConfig_t wifi (168 bytes) en lugar de reserved.
 *                WifiConfig_t: ssid/pwd/mode/channel/max_conn/dhcp_enabled(1 flag).
 * v3.0 (0x0300): WifiConfig_t ampliado a 248 bytes — IP estática por interfaz,
 *                flags DHCP separados (sta_use_dhcp / ap_use_dhcp_server).
 */
/**
 * @brief Magic numbers para detección de configuración válida.
 * @note C1C1 = CICX1 (Current Interrupter CICX1), 0x03xx = v3.x layout.
 */
#define CONFIG_MAGIC 0xC1C10300U           /**< Magic number para SystemConfig_t v3.0 */
#define SUPERUSER_CONFIG_MAGIC 0xC1C15EE0U /**< Magic number para SuperUserConfig_t (5EE = "SEE") */

//_Static_assert(sizeof(SystemConfig_t) == 387, "SystemConfig_t must be 387 bytes (8 header + 379 data)");

/**
 * @brief Configuración de super usuario (licencias, calibración).
 * @note Total: 40 bytes (8 header + 32 data)
 */
typedef struct __attribute__((packed)) SuperUserConfig
{
    uint32_t magic;    /**< Magic number: 0xC1C15EE0 */
    uint16_t size;     /**< Tamaño de datos (sizeof() - 8) */
    uint16_t checksum; /**< CRC16 solo sobre datos */

    uint32_t license_key;     /**< Llave de activación */
    uint32_t expiration_date; /**< Fecha de expiración (epoch) */

    /* Calibración */
    int16_t gps_pps_offset_us; /**< Compensación PPS en microsegundos */

    uint8_t reserved[22]; /**< Reservado para expansión futura */
} SuperUserConfig_t;

_Static_assert(sizeof(SuperUserConfig_t) == 40, "SuperUserConfig_t must be 40 bytes (8 header + 32 data)");

/**
 * @brief Modo de operación del equipo, derivado de SuperUserConfig_t.license_key.
 *
 * @note No ocupa campo propio en la struct — se infiere del valor de license_key:
 *       0 = OPERATION_MODE_FREE, distinto de 0 = OPERATION_MODE_RENT.
 */
typedef enum OperationMode
{
    OPERATION_MODE_FREE = 0, /**< Modo libre: sin licencia (license_key == 0).      */
    OPERATION_MODE_RENT = 1, /**< Modo arriendo: licencia activa (license_key != 0). */
} OperationMode_t;

/**
 * @brief V-Table para IConfigStorage.
 *
 * @note v2.0: Agregadas funciones modulares para evitar carga/guardado innecesario
 *       de SystemConfig_t completa (387 bytes) cuando solo se necesita un subconfig.
 */
typedef struct IConfigStorage_Vtable
{
    /* ===== Monolithic (legacy v1 compatibility) ===== */

    Result_t (*LoadSuperUserConfig)(void *self, SuperUserConfig_t *out_cfg);
    Result_t (*SaveSuperUserConfig)(void *self, const SuperUserConfig_t *cfg);

    /**
     * @brief Resetea a configuración de fábrica.
     */
    Result_t (*ResetToDefaults)(void *self);

    /* ===== Modular (v2.0 - efficient partial access) ===== */

    /**
     * @brief Carga solo RelayConfig_t desde cache (sin I2C, <1μs).
     * @note Evita cargar SystemConfig_t completa (387 bytes).
     */
    Result_t (*LoadRelayConfig)(void *self, RelayConfig_t *out_cfg);
    Result_t (*SaveRelayConfig)(void *self, const RelayConfig_t *cfg);

    Result_t (*LoadGPSConfig)(void *self, GPSConfig_t *out_cfg);
    Result_t (*SaveGPSConfig)(void *self, const GPSConfig_t *cfg);

    Result_t (*LoadGeneralConfig)(void *self, GeneralConfig_t *out_cfg);
    Result_t (*SaveGeneralConfig)(void *self, const GeneralConfig_t *cfg);

    Result_t (*LoadTimeWindowConfig)(void *self, TimeWindowConfig_t *out_cfg);
    Result_t (*SaveTimeWindowConfig)(void *self, const TimeWindowConfig_t *cfg);

    Result_t (*LoadWifiConfig)(void *self, WifiConfig_t *out_cfg);
    Result_t (*SaveWifiConfig)(void *self, const WifiConfig_t *cfg);

} IConfigStorage_Vtable;

/**
 * @brief Instancia de interfaz de almacenamiento de configuración.
 */
typedef struct IConfigStorage
{
    const IConfigStorage_Vtable *vtable;
    void *impl;
} IConfigStorage;

/* ===== Helpers defensivos ===== */

static inline bool config_storage_is_valid(const IConfigStorage *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t ConfigStorage_LoadSuperUserConfig(IConfigStorage *iface, SuperUserConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->LoadSuperUserConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveSuperUserConfig(IConfigStorage *iface, const SuperUserConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->SaveSuperUserConfig(iface->impl, cfg);
}

static inline Result_t ConfigStorage_ResetToDefaults(IConfigStorage *iface)
{
    if (!config_storage_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->ResetToDefaults(iface->impl);
}

/* ===== Modular Config Wrappers (v2.0) ===== */

static inline Result_t ConfigStorage_LoadRelayConfig(IConfigStorage *iface, RelayConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->LoadRelayConfig == NULL)
        return ERR_NOT_SUPPORTED; /* v1 adapter */
    return iface->vtable->LoadRelayConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveRelayConfig(IConfigStorage *iface, const RelayConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->SaveRelayConfig == NULL)
        return ERR_NOT_SUPPORTED; /* v1 adapter */
    return iface->vtable->SaveRelayConfig(iface->impl, cfg);
}

static inline Result_t ConfigStorage_LoadGPSConfig(IConfigStorage *iface, GPSConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->LoadGPSConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->LoadGPSConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveGPSConfig(IConfigStorage *iface, const GPSConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->SaveGPSConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->SaveGPSConfig(iface->impl, cfg);
}

static inline Result_t ConfigStorage_LoadGeneralConfig(IConfigStorage *iface, GeneralConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->LoadGeneralConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->LoadGeneralConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveGeneralConfig(IConfigStorage *iface, const GeneralConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->SaveGeneralConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->SaveGeneralConfig(iface->impl, cfg);
}

static inline Result_t ConfigStorage_LoadTimeWindowConfig(IConfigStorage *iface, TimeWindowConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->LoadTimeWindowConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->LoadTimeWindowConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveTimeWindowConfig(IConfigStorage *iface, const TimeWindowConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->SaveTimeWindowConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->SaveTimeWindowConfig(iface->impl, cfg);
}

static inline Result_t ConfigStorage_LoadWifiConfig(IConfigStorage *iface, WifiConfig_t *out_cfg)
{
    if (!config_storage_is_valid(iface) || out_cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->LoadWifiConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->LoadWifiConfig(iface->impl, out_cfg);
}

static inline Result_t ConfigStorage_SaveWifiConfig(IConfigStorage *iface, const WifiConfig_t *cfg)
{
    if (!config_storage_is_valid(iface) || cfg == NULL)
        return ERR_NULL_POINTER;
    if (iface->vtable->SaveWifiConfig == NULL)
        return ERR_NOT_SUPPORTED;
    return iface->vtable->SaveWifiConfig(iface->impl, cfg);
}

#endif /* I_CONFIG_STORAGE_H */
