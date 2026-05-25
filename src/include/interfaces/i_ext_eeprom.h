/**
 * @file i_ext_eeprom.h
 * @brief Interfaz HAL para EEPROM externa I2C (portable).
 *
 * @note Abstrae cualquier EEPROM I2C (AT24CXX, M24M01E, etc.) con operaciones
 *       comunes de lectura/escritura por byte, página y bloque.
 */

#ifndef I_EXT_EEPROM_H
#define I_EXT_EEPROM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

/**
 * @brief Configuración de EEPROM externa.
 */
typedef struct
{
    uint32_t total_size;     /**< Tamaño total en bytes (ej: 128KB = 131072). */
    uint16_t page_size;      /**< Tamaño de página para escritura (ej: 256 bytes). */
    uint16_t device_address; /**< Dirección I2C del dispositivo (7-bit, ej: 0xA0). */
    uint32_t write_delay_ms; /**< Delay después de escritura (típico: 5-10ms). */
} I_EXT_EEPROM_Config_t;

/**
 * @brief Información del dispositivo EEPROM.
 */
typedef struct
{
    const char *manufacturer; /**< Nombre del fabricante (ej: "STMicroelectronics"). */
    const char *part_number;  /**< Número de parte (ej: "M24M01E"). */
    uint32_t capacity_bytes;  /**< Capacidad en bytes. */
} I_EXT_EEPROM_Info_t;

/* Puntero opaco al objeto I2C subyacente */
typedef void *I_EXT_EEPROM_Handle_t;

/**
 * @brief V-Table para interfaz de EEPROM externa.
 */
typedef struct I_EXT_EEPROM_Vtable
{
    Result_t (*Init)(void *self, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config);
    Result_t (*DeInit)(void *self, I_EXT_EEPROM_Handle_t handle);

    /* Operaciones de lectura */
    Result_t (*ReadByte)(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data);
    Result_t (*ReadData)(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data, uint16_t size);

    /* Operaciones de escritura */
    Result_t (*WriteByte)(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t data);
    Result_t (*WriteData)(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, const uint8_t *data, uint16_t size);

    /* Utilidades */
    Result_t (*IsDeviceReady)(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms);
    Result_t (*GetInfo)(void *self, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info);
} I_EXT_EEPROM_Vtable;

/**
 * @brief Instancia de interfaz EEPROM externa.
 */
typedef struct I_EXT_EEPROM
{
    const I_EXT_EEPROM_Vtable *vtable;
    void *impl;
} I_EXT_EEPROM;

/* ===== Helpers defensivos ===== */

/**
 * @brief Validate I_EXT_EEPROM interface instance.
 * @note Per Interface Designer Skill: validates vtable, impl, AND critical function pointers.
 * @return true if interface is fully initialized and safe to use.
 */
static inline bool ext_eeprom_is_valid(const I_EXT_EEPROM *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->vtable->Init != NULL) && (iface->vtable->ReadData != NULL) && (iface->vtable->WriteData != NULL) && (iface->impl != NULL);
}

static inline Result_t EXT_EEPROM_Init(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config)
{
    if (!ext_eeprom_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t EXT_EEPROM_DeInit(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle)
{
    if (!ext_eeprom_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t EXT_EEPROM_ReadByte(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, uint32_t addr, uint8_t *data)
{
    if (!ext_eeprom_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->ReadByte(iface->impl, handle, addr, data);
}

static inline Result_t EXT_EEPROM_ReadData(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, uint32_t addr, uint8_t *data, uint16_t size)
{
    if (!ext_eeprom_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->ReadData(iface->impl, handle, addr, data, size);
}

static inline Result_t EXT_EEPROM_WriteByte(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, uint32_t addr, uint8_t data)
{
    if (!ext_eeprom_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->WriteByte(iface->impl, handle, addr, data);
}

static inline Result_t EXT_EEPROM_WriteData(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, uint32_t addr, const uint8_t *data, uint16_t size)
{
    if (!ext_eeprom_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->WriteData(iface->impl, handle, addr, data, size);
}

static inline Result_t EXT_EEPROM_IsDeviceReady(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms)
{
    if (!ext_eeprom_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->IsDeviceReady(iface->impl, handle, timeout_ms);
}

static inline Result_t EXT_EEPROM_GetInfo(I_EXT_EEPROM *iface, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info)
{
    if (!ext_eeprom_is_valid(iface) || info == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetInfo(iface->impl, handle, info);
}

#endif /* I_EXT_EEPROM_H */
