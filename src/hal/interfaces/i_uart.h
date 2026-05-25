/**
 * @file i_uart.h
 * @brief Interfaz abstracta para comunicación serie (UART).
 */

#ifndef I_UART_H
#define I_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include <stddef.h>

/**
 * @brief Configuraciones de Baudrate soportadas.
 */
typedef enum
{
    I_UART_BAUD_9600 = 9600,
    I_UART_BAUD_19200 = 19200,
    I_UART_BAUD_38400 = 38400,
    I_UART_BAUD_57600 = 57600,
    I_UART_BAUD_115200 = 115200,
    I_UART_BAUD_230400 = 230400,
    I_UART_BAUD_460800 = 460800,
    I_UART_BAUD_921600 = 921600
} I_UART_Baudrate_t;

/**
 * @brief Paridad de la comunicación.
 */
typedef enum
{
    I_UART_PARITY_NONE,
    I_UART_PARITY_EVEN,
    I_UART_PARITY_ODD
} I_UART_Parity_t;

/**
 * @brief Bits de parada.
 */
typedef enum
{
    I_UART_STOP_1,
    I_UART_STOP_2
} I_UART_StopBits_t;

/**
 * @brief Estructura de configuración UART.
 */
typedef struct
{
    I_UART_Baudrate_t baudrate;
    I_UART_Parity_t parity;
    I_UART_StopBits_t stop_bits;
} I_UART_Config_t;

/**
 * @brief Callback para eventos UART (TX completo, RX completo, error).
 * @param context Puntero al contexto de usuario.
 * @param size Cantidad de bytes transmitidos o recibidos.
 */
typedef void (*I_UART_Callback_t)(void *context, uint16_t size);

/**
 * @brief Puntero opaco al recurso hardware UART.
 */
typedef void *I_UART_Handle_t;

/**
 * @brief V-Table para operaciones portables de UART.
 */
typedef struct I_UART_Vtable
{
    Result_t (*Init)(void *self, I_UART_Handle_t handle, const I_UART_Config_t *config);
    Result_t (*DeInit)(void *self, I_UART_Handle_t handle);

    /* Operaciones bloqueantes (polling) */
    Result_t (*Transmit)(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout);
    Result_t (*Receive)(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout);

    /* Operaciones asíncronas (DMA/IT según BSP) */
    Result_t (*Transmit_Async)(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size);
    Result_t (*ReceiveUntilIdle_Async)(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t max_size);

    /* Registro de callbacks */
    Result_t (*RegisterTxCompleteCallback)(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);
    Result_t (*RegisterRxEventCallback)(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);
    Result_t (*RegisterErrorCallback)(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);
} I_UART_Vtable;

/**
 * @brief Instancia de la interfaz UART.
 */
typedef struct I_UART
{
    const I_UART_Vtable *vtable;
    void *impl;
} I_UART;

/* ===== Helpers defensivos ===== */

/**
 * @brief Valida que la interfaz UART sea válida.
 */
static inline bool uart_is_valid(const I_UART *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t UART_Init(I_UART *iface, I_UART_Handle_t handle, const I_UART_Config_t *config)
{
    if (!uart_is_valid(iface) || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t UART_DeInit(I_UART *iface, I_UART_Handle_t handle)
{
    if (!uart_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t UART_Transmit(I_UART *iface, I_UART_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!uart_is_valid(iface) || data == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Transmit(iface->impl, handle, data, size, timeout);
}

static inline Result_t UART_Receive(I_UART *iface, I_UART_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout)
{
    if (!uart_is_valid(iface) || data == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Receive(iface->impl, handle, data, size, timeout);
}

static inline Result_t UART_Transmit_Async(I_UART *iface, I_UART_Handle_t handle, const uint8_t *data, uint16_t size)
{
    if (!uart_is_valid(iface) || data == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Transmit_Async(iface->impl, handle, data, size);
}

static inline Result_t UART_ReceiveUntilIdle_Async(I_UART *iface, I_UART_Handle_t handle, uint8_t *data, uint16_t max_size)
{
    if (!uart_is_valid(iface) || data == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ReceiveUntilIdle_Async(iface->impl, handle, data, max_size);
}

static inline Result_t UART_RegisterTxCompleteCallback(I_UART *iface, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    if (!uart_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterTxCompleteCallback(iface->impl, handle, cb, context);
}

static inline Result_t UART_RegisterRxEventCallback(I_UART *iface, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    if (!uart_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterRxEventCallback(iface->impl, handle, cb, context);
}

static inline Result_t UART_RegisterErrorCallback(I_UART *iface, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    if (!uart_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterErrorCallback(iface->impl, handle, cb, context);
}

#endif /* I_UART_H */
