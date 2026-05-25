/**
 * @file i_timer.h
 * @brief Interfaz abstracta de temporizador (TIM) con soporte para Output Compare (OC).
 */

#ifndef I_TIMER_H
#define I_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include <stddef.h>
/**
 * @brief Canales soportados por el temporizador.
 */
typedef enum
{
    I_TIMER_CHANNEL_1 = 0x01,
    I_TIMER_CHANNEL_2 = 0x02,
    I_TIMER_CHANNEL_3 = 0x04,
    I_TIMER_CHANNEL_4 = 0x08
} I_TIMER_Channel_t;

/**
 * @brief Callback para evento de actualización (overflow / update).
 */
typedef void (*I_TIMER_Callback_t)(void *context);

/**
 * @brief Callback para evento de Output Compare por canal.
 */
typedef void (*I_TIMER_OC_Callback_t)(void *context, I_TIMER_Channel_t channel);

/**
 * @brief Configuración base del temporizador.
 */
typedef struct
{
    uint32_t prescaler;       /**< Valor PSC */
    uint32_t period;          /**< Valor ARR (auto-reload) */
    bool auto_reload_preload; /**< Habilita preload del ARR */
} I_TIMER_Config_t;

/**
 * @brief Configuración específica de Output Compare por canal.
 */
typedef struct
{
    I_TIMER_Channel_t channel; /**< Canal a configurar */
    uint32_t compare_value;    /**< Valor de comparación CCRx */
    bool preload_enable;       /**< Habilita preload del CCRx */
} I_TIMER_OC_Config_t;

/**
 * @brief Puntero opaco al recurso hardware del timer.
 */
typedef void *I_TIMER_Handle_t;

/**
 * @brief V-Table para operaciones portables de temporizador.
 */
typedef struct I_TIMER_Vtable
{
    Result_t (*Init)(void *self, I_TIMER_Handle_t handle, const I_TIMER_Config_t *config);
    Result_t (*DeInit)(void *self, I_TIMER_Handle_t handle);

    /* Control de base de tiempo con interrupción de actualización */
    Result_t (*StartUpdate_IT)(void *self, I_TIMER_Handle_t handle);
    Result_t (*StopUpdate_IT)(void *self, I_TIMER_Handle_t handle);

    /* Control de Output Compare por canal (solo interrupción, sin salida física obligatoria) */
    Result_t (*OC_ConfigChannel)(void *self, I_TIMER_Handle_t handle, const I_TIMER_OC_Config_t *oc_config);
    Result_t (*OC_Start_IT)(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel);
    Result_t (*OC_Stop_IT)(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel);
    Result_t (*SetCompare)(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel, uint32_t compare_value);

    /* Reconfiguración en vuelo */
    Result_t (*SetAutoReload)(void *self, I_TIMER_Handle_t handle, uint32_t period);

    /* Registro de callbacks */
    Result_t (*RegisterUpdateCallback)(void *self, I_TIMER_Handle_t handle, I_TIMER_Callback_t cb, void *context);
    Result_t (*RegisterOCCallback)(void *self, I_TIMER_Handle_t handle, I_TIMER_OC_Callback_t cb, void *context);
} I_TIMER_Vtable;

/**
 * @brief Instancia de la interfaz de temporizador.
 */
typedef struct I_TIMER
{
    const I_TIMER_Vtable *vtable;
    void *impl;
} I_TIMER;

/* ===== Helpers defensivos ===== */

static inline bool timer_is_valid(const I_TIMER *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t Timer_Init(I_TIMER *iface, I_TIMER_Handle_t handle, const I_TIMER_Config_t *config)
{
    if (!timer_is_valid(iface) || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Init(iface->impl, handle, config);
}

static inline Result_t Timer_DeInit(I_TIMER *iface, I_TIMER_Handle_t handle)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t Timer_StartUpdate_IT(I_TIMER *iface, I_TIMER_Handle_t handle)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->StartUpdate_IT(iface->impl, handle);
}

static inline Result_t Timer_StopUpdate_IT(I_TIMER *iface, I_TIMER_Handle_t handle)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->StopUpdate_IT(iface->impl, handle);
}

static inline Result_t Timer_OC_ConfigChannel(I_TIMER *iface, I_TIMER_Handle_t handle, const I_TIMER_OC_Config_t *oc_config)
{
    if (!timer_is_valid(iface) || oc_config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->OC_ConfigChannel(iface->impl, handle, oc_config);
}

static inline Result_t Timer_OC_Start_IT(I_TIMER *iface, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->OC_Start_IT(iface->impl, handle, channel);
}

static inline Result_t Timer_OC_Stop_IT(I_TIMER *iface, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->OC_Stop_IT(iface->impl, handle, channel);
}

static inline Result_t Timer_SetCompare(I_TIMER *iface, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel, uint32_t value)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->SetCompare(iface->impl, handle, channel, value);
}

static inline Result_t Timer_SetAutoReload(I_TIMER *iface, I_TIMER_Handle_t handle, uint32_t period)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->SetAutoReload(iface->impl, handle, period);
}

static inline Result_t Timer_RegisterUpdateCallback(I_TIMER *iface, I_TIMER_Handle_t handle, I_TIMER_Callback_t cb, void *context)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterUpdateCallback(iface->impl, handle, cb, context);
}

static inline Result_t Timer_RegisterOCCallback(I_TIMER *iface, I_TIMER_Handle_t handle, I_TIMER_OC_Callback_t cb, void *context)
{
    if (!timer_is_valid(iface))
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterOCCallback(iface->impl, handle, cb, context);
}

#endif /* I_TIMER_H */
