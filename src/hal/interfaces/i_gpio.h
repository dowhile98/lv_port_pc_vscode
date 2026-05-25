/**
 * @file i_gpio.h
 * @brief Interfaz abstracta para GPIO independiente de plataforma
 */

#ifndef I_GPIO_H
#define I_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "hal_types.h"

/* ===== Tipos Abstractos ===== */

/**
 * @brief Puerto GPIO opaco (definido por el BSP concreto).
 */
typedef void *GPIO_Port_t;

/**
 * @brief Tipo de número de pin.
 */
typedef uint16_t GPIO_Pin_t;

/**
 * @brief Modos de configuración soportados por la interfaz GPIO.
 */
typedef enum
{
    I_GPIO_MODE_INPUT,
    I_GPIO_MODE_OUTPUT_PP, /* Push-Pull */
    I_GPIO_MODE_OUTPUT_OD, /* Open-Drain */
    I_GPIO_MODE_AF_PP,     /* Alternate Function Push-Pull */
    I_GPIO_MODE_AF_OD,     /* Alternate Function Open-Drain */
    I_GPIO_MODE_ANALOG
} GPIO_Mode_t;

/**
 * @brief Resistencias internas del pin.
 */
typedef enum
{
    I_GPIO_PULL_NONE,
    I_GPIO_PULL_UP,
    I_GPIO_PULL_DOWN
} GPIO_Pull_t;

/**
 * @brief Velocidades de conmutación del pin.
 */
typedef enum
{
    I_GPIO_SPEED_LOW,
    I_GPIO_SPEED_MEDIUM,
    I_GPIO_SPEED_HIGH,
    I_GPIO_SPEED_VERY_HIGH
} GPIO_Speed_t;

/**
 * @brief Estado lógico del pin.
 */
typedef enum
{
    I_GPIO_STATE_LOW = 0,
    I_GPIO_STATE_HIGH = 1
} GPIO_State_t;

/**
 * @brief Configuración completa de un pin GPIO.
 */
typedef struct
{
    GPIO_Mode_t mode;
    GPIO_Pull_t pull;
    GPIO_Speed_t speed;
    uint8_t alternate; /* Número de función alternativa (0-15) */
} GPIO_Config_t;

/* ===== Vtable de GPIO ===== */

/**
 * @brief Vtable para operaciones GPIO portables.
 */
typedef struct I_GPIO_Vtable
{
    Result_t (*Init)(void *self, GPIO_Port_t port, GPIO_Pin_t pin, const GPIO_Config_t *config);
    Result_t (*DeInit)(void *self, GPIO_Port_t port, GPIO_Pin_t pin);

    Result_t (*WritePin)(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t state);
    Result_t (*ReadPin)(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t *state_out);
    Result_t (*TogglePin)(void *self, GPIO_Port_t port, GPIO_Pin_t pin);

    Result_t (*WritePort)(void *self, GPIO_Port_t port, uint32_t value);
    Result_t (*ReadPort)(void *self, GPIO_Port_t port, uint32_t *value_out);

    Result_t (*SetBits)(void *self, GPIO_Port_t port, uint32_t pins);   /* BSRR */
    Result_t (*ResetBits)(void *self, GPIO_Port_t port, uint32_t pins); /* BRR */

    Result_t (*LockPin)(void *self, GPIO_Port_t port, GPIO_Pin_t pin);
} I_GPIO_Vtable;

typedef struct I_GPIO
{
    const I_GPIO_Vtable *vtable;
    void *impl; /* Puntero a la implementación concreta (ej. BSP_STM32_GPIO) */
} I_GPIO;

/* ===== Inline helpers con defensas ===== */

static inline bool gpio_is_valid(const I_GPIO *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t GPIO_Init(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin, const GPIO_Config_t *cfg)
{
    if (!gpio_is_valid(iface) || cfg == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Init(iface->impl, port, pin, cfg);
}

static inline Result_t GPIO_DeInit(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->DeInit(iface->impl, port, pin);
}

static inline Result_t GPIO_WritePin(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t st)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->WritePin(iface->impl, port, pin, st);
}

static inline Result_t GPIO_ReadPin(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t *state_out)
{
    if (!gpio_is_valid(iface) || state_out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ReadPin(iface->impl, port, pin, state_out);
}

static inline Result_t GPIO_TogglePin(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->TogglePin(iface->impl, port, pin);
}

static inline Result_t GPIO_WritePort(I_GPIO *iface, GPIO_Port_t port, uint32_t value)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->WritePort(iface->impl, port, value);
}

static inline Result_t GPIO_ReadPort(I_GPIO *iface, GPIO_Port_t port, uint32_t *value_out)
{
    if (!gpio_is_valid(iface) || value_out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ReadPort(iface->impl, port, value_out);
}

static inline Result_t GPIO_SetBits(I_GPIO *iface, GPIO_Port_t port, uint32_t pins)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->SetBits(iface->impl, port, pins);
}

static inline Result_t GPIO_ResetBits(I_GPIO *iface, GPIO_Port_t port, uint32_t pins)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ResetBits(iface->impl, port, pins);
}

static inline Result_t GPIO_LockPin(I_GPIO *iface, GPIO_Port_t port, GPIO_Pin_t pin)
{
    if (!gpio_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->LockPin(iface->impl, port, pin);
}

#endif /* I_GPIO_H */
