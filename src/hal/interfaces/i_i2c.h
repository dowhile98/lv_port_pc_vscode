/**
 * @file i_i2c.h
 * @brief Interfaz abstracta para comunicación por bus I2C.
 */

#ifndef I_I2C_H
#define I_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include <stddef.h>
/**
 * @brief Tamaño de la dirección de memoria o registro interno.
 */
typedef enum
{
    I_I2C_MEMADDR_SIZE_8BIT = 1,
    I_I2C_MEMADDR_SIZE_16BIT = 2
} I_I2C_MemAddrSize_t;

/**
 * @brief Configuración del bus I2C.
 */
typedef struct
{
    uint32_t clock_speed;
    bool fast_mode;
} I_I2C_Config_t;

/* Puntero opaco al hardware */
typedef void *I_I2C_Handle_t;

/**
 * @brief V-Table para la interfaz I2C.
 */
typedef struct I_I2C_Vtable
{
    Result_t (*Init)(void *self, I_I2C_Handle_t handle, const I_I2C_Config_t *config);
    Result_t (*DeInit)(void *self, I_I2C_Handle_t handle);

    /* Operaciones Master Estándar (Polling) */
    Result_t (*Master_Transmit)(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, const uint8_t *data, uint16_t size, uint32_t timeout);
    Result_t (*Master_Receive)(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint8_t *data, uint16_t size, uint32_t timeout);

    /* Operaciones de Memoria/Registros (Polling) */
    Result_t (*Mem_Write)(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, const uint8_t *data, uint16_t size, uint32_t timeout);
    Result_t (*Mem_Read)(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, uint8_t *data, uint16_t size, uint32_t timeout);

    /* Utilidades */
    Result_t (*IsDeviceReady)(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint32_t trials, uint32_t timeout);
} I_I2C_Vtable;

/**
 * @brief Instancia de la interfaz I2C.
 */
typedef struct I_I2C
{
    const I_I2C_Vtable *vtable;
    void *impl;
} I_I2C;

/* ===== Helpers de conveniencia ===== */

static inline bool i2c_is_valid(const I_I2C *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t I2C_Init(I_I2C *iface, I_I2C_Handle_t handle, const I_I2C_Config_t *config)
{
    if (!i2c_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t I2C_DeInit(I_I2C *iface, I_I2C_Handle_t handle)
{
    if (!i2c_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t I2C_Master_Transmit(I_I2C *iface, I_I2C_Handle_t handle, uint16_t dev_addr, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!i2c_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Master_Transmit(iface->impl, handle, dev_addr, data, size, timeout);
}

static inline Result_t I2C_Master_Receive(I_I2C *iface, I_I2C_Handle_t handle, uint16_t dev_addr, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!i2c_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Master_Receive(iface->impl, handle, dev_addr, data, size, timeout);
}

static inline Result_t I2C_Mem_Write(I_I2C *iface, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!i2c_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Mem_Write(iface->impl, handle, dev_addr, mem_addr, addr_size, data, size, timeout);
}

static inline Result_t I2C_Mem_Read(I_I2C *iface, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!i2c_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Mem_Read(iface->impl, handle, dev_addr, mem_addr, addr_size, data, size, timeout);
}

static inline Result_t I2C_IsDeviceReady(I_I2C *iface, I_I2C_Handle_t handle, uint16_t dev_addr, uint32_t trials, uint32_t timeout)
{
    if (!i2c_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->IsDeviceReady(iface->impl, handle, dev_addr, trials, timeout);
}

#endif /* I_I2C_H */
