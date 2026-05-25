/**
 * @file i_ext_flash.h
 * @brief Interfaz HAL para memoria flash externa (QSPI/OSPI/SPI).
 *
 * @note Esta interfaz abstrae cualquier tipo de memoria flash externa:
 *       - OCTOSPI (STM32U5) en modo memory-mapped o indirecto
 *       - QSPI (STM32F7/H7) en modo memory-mapped o indirecto
 *       - SPI estándar (bitbanging o periférico dedicado)
 */

#ifndef I_EXT_FLASH_H
#define I_EXT_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

/**
 * @brief Modos de operación de la memoria flash externa.
 */
typedef enum
{
    I_EXT_FLASH_MODE_INDIRECT,     /**< Polling/DMA - Comandos manuales */
    I_EXT_FLASH_MODE_MEMORY_MAPPED /**< Memory-mapped (XIP) - Acceso directo */
} I_EXT_FLASH_Mode_t;

/**
 * @brief Configuración de memoria externa.
 */
typedef struct
{
    uint32_t size_bytes;     /**< Tamaño total en bytes (ej: 16MB = 0x1000000) */
    uint32_t sector_size;    /**< Tamaño del sector para erase (ej: 4KB = 0x1000) */
    uint32_t page_size;      /**< Tamaño de página para write (ej: 256B = 0x100) */
    I_EXT_FLASH_Mode_t mode; /**< Modo de operación deseado */
} I_EXT_FLASH_Config_t;

/**
 * @brief Información del chip de memoria.
 */
typedef struct
{
    uint8_t manufacturer_id; /**< ID del fabricante (ej: 0xEF para Winbond) */
    uint8_t device_id;       /**< ID del dispositivo */
    uint32_t capacity_bytes; /**< Capacidad total en bytes */
} I_EXT_FLASH_Info_t;

/* Puntero opaco al hardware */
typedef void *I_EXT_FLASH_Handle_t;

/**
 * @brief V-Table para interfaz de flash externo.
 */
typedef struct I_EXT_FLASH_Vtable
{
    Result_t (*Init)(void *self, I_EXT_FLASH_Handle_t handle, const I_EXT_FLASH_Config_t *config);
    Result_t (*DeInit)(void *self, I_EXT_FLASH_Handle_t handle);

    /* Operaciones básicas - Modo Indirecto */
    Result_t (*Read)(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, uint8_t *data, uint32_t size);
    Result_t (*Write)(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, const uint8_t *data, uint32_t size);

    /* Erase (sector o chip completo) */
    Result_t (*EraseSector)(void *self, I_EXT_FLASH_Handle_t handle, uint32_t sector_address);
    Result_t (*EraseChip)(void *self, I_EXT_FLASH_Handle_t handle);

    /* Control de modo memory-mapped (opcional, no todos los BSPs lo soportan) */
    Result_t (*EnableMemoryMapped)(void *self, I_EXT_FLASH_Handle_t handle);
    Result_t (*DisableMemoryMapped)(void *self, I_EXT_FLASH_Handle_t handle);

    /* Utilidades */
    Result_t (*GetInfo)(void *self, I_EXT_FLASH_Handle_t handle, I_EXT_FLASH_Info_t *info);
    bool (*IsBusy)(void *self, I_EXT_FLASH_Handle_t handle);
} I_EXT_FLASH_Vtable;

/**
 * @brief Instancia de interfaz flash externo.
 */
typedef struct I_EXT_FLASH
{
    const I_EXT_FLASH_Vtable *vtable;
    void *impl;
} I_EXT_FLASH;

/* ===== Helpers defensivos ===== */

static inline bool ext_flash_is_valid(const I_EXT_FLASH *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t EXT_FLASH_Init(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle, const I_EXT_FLASH_Config_t *config)
{
    if (!ext_flash_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t EXT_FLASH_DeInit(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle)
{
    if (!ext_flash_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t EXT_FLASH_Read(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle, uint32_t addr, uint8_t *data, uint32_t size)
{
    if (!ext_flash_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Read(iface->impl, handle, addr, data, size);
}

static inline Result_t EXT_FLASH_Write(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle, uint32_t addr, const uint8_t *data, uint32_t size)
{
    if (!ext_flash_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Write(iface->impl, handle, addr, data, size);
}

static inline Result_t EXT_FLASH_EraseSector(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle, uint32_t sector_addr)
{
    if (!ext_flash_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->EraseSector(iface->impl, handle, sector_addr);
}

static inline Result_t EXT_FLASH_EraseChip(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle)
{
    if (!ext_flash_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->EraseChip(iface->impl, handle);
}

static inline Result_t EXT_FLASH_EnableMemoryMapped(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle)
{
    if (!ext_flash_is_valid(iface))
        return ERR_NULL_POINTER;
    if (iface->vtable->EnableMemoryMapped == NULL)
        return ERR_ERROR; /* No soportado */
    return iface->vtable->EnableMemoryMapped(iface->impl, handle);
}

static inline Result_t EXT_FLASH_DisableMemoryMapped(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle)
{
    if (!ext_flash_is_valid(iface))
        return ERR_NULL_POINTER;
    if (iface->vtable->DisableMemoryMapped == NULL)
        return ERR_ERROR; /* No soportado */
    return iface->vtable->DisableMemoryMapped(iface->impl, handle);
}

static inline Result_t EXT_FLASH_GetInfo(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle, I_EXT_FLASH_Info_t *info)
{
    if (!ext_flash_is_valid(iface) || info == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetInfo(iface->impl, handle, info);
}

static inline bool EXT_FLASH_IsBusy(I_EXT_FLASH *iface, I_EXT_FLASH_Handle_t handle)
{
    if (!ext_flash_is_valid(iface))
        return false;
    return iface->vtable->IsBusy(iface->impl, handle);
}

#endif /* I_EXT_FLASH_H */
