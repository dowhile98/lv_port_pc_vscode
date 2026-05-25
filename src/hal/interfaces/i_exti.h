/**
 * @file i_exti.h
 * @brief Interfaz abstracta para interrupciones externas (EXTI).
 */

#ifndef I_EXTI_H
#define I_EXTI_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "i_gpio.h"

/**
 * @brief Modos de disparo de la interrupción externa.
 */
typedef enum
{
    I_EXTI_TRIGGER_RISING,
    I_EXTI_TRIGGER_FALLING,
    I_EXTI_TRIGGER_RISING_FALLING
} I_EXTI_Trigger_t;

/**
 * @brief Callback para eventos EXTI.
 */
typedef void (*I_EXTI_Callback_t)(void *context, GPIO_Pin_t pin);

/**
 * @brief Configuración para una línea EXTI.
 */
typedef struct
{
    GPIO_Port_t port;
    GPIO_Pin_t pin;
    I_EXTI_Trigger_t trigger;
    uint32_t priority;
    uint32_t subPriority;
} I_EXTI_Config_t;

/**
 * @brief V-Table para operaciones EXTI portables.
 */
typedef struct I_EXTI_Vtable
{
    Result_t (*Init)(void *self, const I_EXTI_Config_t *config);
    Result_t (*DeInit)(void *self, GPIO_Pin_t pin);
    Result_t (*Enable)(void *self, GPIO_Pin_t pin);
    Result_t (*Disable)(void *self, GPIO_Pin_t pin);
    Result_t (*RegisterCallback)(void *self, GPIO_Pin_t pin, I_EXTI_Callback_t cb, void *context);
    Result_t (*ClearPending)(void *self, GPIO_Pin_t pin);
} I_EXTI_Vtable;

/**
 * @brief Instancia de la interfaz EXTI.
 */
typedef struct I_EXTI
{
    const I_EXTI_Vtable *vtable;
    void *impl;
} I_EXTI;

/* ===== Inline helpers con defensas ===== */

static inline bool exti_is_valid(const I_EXTI *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t EXTI_Init(I_EXTI *iface, const I_EXTI_Config_t *config)
{
    if (!exti_is_valid(iface) || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Init(iface->impl, config);
}

static inline Result_t EXTI_DeInit(I_EXTI *iface, GPIO_Pin_t pin)
{
    if (!exti_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->DeInit(iface->impl, pin);
}

static inline Result_t EXTI_Enable(I_EXTI *iface, GPIO_Pin_t pin)
{
    if (!exti_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Enable(iface->impl, pin);
}

static inline Result_t EXTI_Disable(I_EXTI *iface, GPIO_Pin_t pin)
{
    if (!exti_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Disable(iface->impl, pin);
}

static inline Result_t EXTI_RegisterCallback(I_EXTI *iface, GPIO_Pin_t pin, I_EXTI_Callback_t cb, void *context)
{
    if (!exti_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterCallback(iface->impl, pin, cb, context);
}

static inline Result_t EXTI_ClearPending(I_EXTI *iface, GPIO_Pin_t pin)
{
    if (!exti_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ClearPending(iface->impl, pin);
}

#endif /* I_EXTI_H */
