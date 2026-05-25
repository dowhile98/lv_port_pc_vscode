/**
 * @file i_spi.h
 * @brief Interfaz abstracta para comunicación por bus SPI (portable).
 */

#ifndef I_SPI_H
#define I_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

/**
 * @brief Modos de operación SPI.
 */
typedef enum
{
    I_SPI_MODE_MASTER,
    I_SPI_MODE_SLAVE
} I_SPI_Mode_t;

/**
 * @brief Polaridad del reloj (CPOL).
 */
typedef enum
{
    I_SPI_CPOL_LOW,
    I_SPI_CPOL_HIGH
} I_SPI_Cpol_t;

/**
 * @brief Fase del reloj (CPHA).
 */
typedef enum
{
    I_SPI_CPHA_1EDGE,
    I_SPI_CPHA_2EDGE
} I_SPI_Cpha_t;

/**
 * @brief Tamaño de los datos.
 */
typedef enum
{
    I_SPI_DATASIZE_8BIT,
    I_SPI_DATASIZE_16BIT
} I_SPI_DataSize_t;

/**
 * @brief Orden de los bits.
 */
typedef enum
{
    I_SPI_FIRSTBIT_MSB,
    I_SPI_FIRSTBIT_LSB
} I_SPI_FirstBit_t;

/**
 * @brief Configuración del bus SPI.
 */
typedef struct
{
    I_SPI_Mode_t mode;
    I_SPI_Cpol_t cpol;
    I_SPI_Cpha_t cpha;
    I_SPI_DataSize_t data_size;
    I_SPI_FirstBit_t first_bit;
    uint32_t baudrate_prescaler;
} I_SPI_Config_t;

/**
 * @brief Callback para eventos asíncronos de SPI.
 * @param context Puntero al contexto de usuario.
 * @param size Cantidad de bytes procesados.
 */
typedef void (*I_SPI_Callback_t)(void *context, uint16_t size);

/* Puntero opaco al hardware */
typedef void *I_SPI_Handle_t;

/**
 * @brief V-Table para la interfaz SPI.
 */
typedef struct I_SPI_Vtable
{
    Result_t (*Init)(void *self, I_SPI_Handle_t handle, const I_SPI_Config_t *config);
    Result_t (*DeInit)(void *self, I_SPI_Handle_t handle);

    /* Operaciones bloqueantes (polling) */
    Result_t (*Transmit)(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout);
    Result_t (*Receive)(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout);
    Result_t (*TransmitReceive)(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout);

    /* Operaciones asíncronas (DMA/IT) */
    Result_t (*Transmit_Async)(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size);
    Result_t (*Receive_Async)(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size);
    Result_t (*TransmitReceive_Async)(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

    /* Registro de callbacks separados */
    Result_t (*RegisterTxCallback)(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
    Result_t (*RegisterRxCallback)(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
    Result_t (*RegisterTxRxCallback)(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
    Result_t (*RegisterErrorCallback)(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
} I_SPI_Vtable;

/**
 * @brief Instancia de la interfaz SPI.
 */
typedef struct I_SPI
{
    const I_SPI_Vtable *vtable;
    void *impl;
} I_SPI;

/* ===== Helpers defensivos ===== */

static inline bool spi_is_valid(const I_SPI *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t SPI_Init(I_SPI *iface, I_SPI_Handle_t handle, const I_SPI_Config_t *config)
{
    if (!spi_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t SPI_DeInit(I_SPI *iface, I_SPI_Handle_t handle)
{
    if (!spi_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t SPI_Transmit(I_SPI *iface, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!spi_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Transmit(iface->impl, handle, data, size, timeout);
}

static inline Result_t SPI_Receive(I_SPI *iface, I_SPI_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!spi_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Receive(iface->impl, handle, data, size, timeout);
}

static inline Result_t SPI_TransmitReceive(I_SPI *iface, I_SPI_Handle_t handle, const uint8_t *tx, uint8_t *rx, uint16_t size, uint32_t timeout)
{
    if (!spi_is_valid(iface) || tx == NULL || rx == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->TransmitReceive(iface->impl, handle, tx, rx, size, timeout);
}

static inline Result_t SPI_Transmit_Async(I_SPI *iface, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size)
{
    if (!spi_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Transmit_Async(iface->impl, handle, data, size);
}

static inline Result_t SPI_Receive_Async(I_SPI *iface, I_SPI_Handle_t handle, uint8_t *data, uint16_t size)
{
    if (!spi_is_valid(iface) || data == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Receive_Async(iface->impl, handle, data, size);
}

static inline Result_t SPI_TransmitReceive_Async(I_SPI *iface, I_SPI_Handle_t handle, const uint8_t *tx, uint8_t *rx, uint16_t size)
{
    if (!spi_is_valid(iface) || tx == NULL || rx == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->TransmitReceive_Async(iface->impl, handle, tx, rx, size);
}

static inline Result_t SPI_RegisterTxCallback(I_SPI *iface, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    if (!spi_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->RegisterTxCallback(iface->impl, handle, cb, context);
}

static inline Result_t SPI_RegisterRxCallback(I_SPI *iface, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    if (!spi_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->RegisterRxCallback(iface->impl, handle, cb, context);
}

static inline Result_t SPI_RegisterTxRxCallback(I_SPI *iface, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    if (!spi_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->RegisterTxRxCallback(iface->impl, handle, cb, context);
}

static inline Result_t SPI_RegisterErrorCallback(I_SPI *iface, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    if (!spi_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->RegisterErrorCallback(iface->impl, handle, cb, context);
}

#endif /* I_SPI_H */
