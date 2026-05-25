/**
 * @file i_rtc.h
 * @brief Interfaz abstracta para RTC (Real-Time Clock) independiente de plataforma
 * @version 1.0.0
 *
 * Abstrae las operaciones de RTC para mantener tiempo, fecha, alarmas y backup registers.
 * Permite portabilidad entre diferentes MCUs y testing sin hardware.
 */

#ifndef I_RTC_H
#define I_RTC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

/* ===== Tipos de Handle ===== */

/**
 * @brief Handle opaco al RTC subyacente (implementación específica).
 */
typedef void *RTC_Handle_t;

/* ===== Estructuras de Datos ===== */

/**
 * @brief Estructura de tiempo RTC.
 */
typedef struct
{
    uint8_t hours;       /**< Horas (0-23 en formato 24h). */
    uint8_t minutes;     /**< Minutos (0-59). */
    uint8_t seconds;     /**< Segundos (0-59). */
    uint16_t subSeconds; /**< Fracciones de segundo (dependiente de implementación). */
} RTC_Time_t;

/**
 * @brief Estructura de fecha RTC.
 */
typedef struct
{
    uint8_t weekDay; /**< Día de la semana: 1=Lunes ... 7=Domingo. */
    uint8_t day;     /**< Día del mes (1-31). */
    uint8_t month;   /**< Mes (1-12). */
    uint8_t year;    /**< Año (offset desde 2000, ej: 24 = 2024). */
} RTC_Date_t;

/**
 * @brief Identificador de alarma.
 */
typedef enum
{
    I_RTC_ALARM_A = 0, /**< Alarma A. */
    I_RTC_ALARM_B = 1  /**< Alarma B. */
} RTC_Alarm_t;

/**
 * @brief Máscara de campos de alarma (qué comparar).
 */
typedef enum
{
    I_RTC_ALARMMASK_NONE = 0x00,        /**< Compara todos los campos. */
    I_RTC_ALARMMASK_SECONDS = 0x01,     /**< Ignora segundos. */
    I_RTC_ALARMMASK_MINUTES = 0x02,     /**< Ignora minutos. */
    I_RTC_ALARMMASK_HOURS = 0x04,       /**< Ignora horas. */
    I_RTC_ALARMMASK_DATEWEEKDAY = 0x08, /**< Ignora fecha/día de semana. */
    I_RTC_ALARMMASK_ALL = 0x0F          /**< Ignora todos (alarma siempre activa). */
} RTC_AlarmMask_t;

/**
 * @brief Configuración de una alarma RTC.
 */
typedef struct
{
    RTC_Time_t alarmTime;      /**< Tiempo de disparo de la alarma. */
    uint8_t alarmDateWeekDay;  /**< Fecha o día de la semana (depende de useWeekDay). */
    RTC_AlarmMask_t alarmMask; /**< Máscara de campos a comparar. */
    bool useWeekDay;           /**< true = usar weekDay, false = usar fecha del mes. */
} RTC_AlarmConfig_t;

/* ===== Callbacks ===== */

/**
 * @brief Callback de alarma RTC.
 * @param context Contexto de usuario provisto en el registro.
 * @param alarm Identificador de la alarma que se disparó.
 */
typedef void (*RTC_AlarmCallback_t)(void *context, RTC_Alarm_t alarm);

/**
 * @brief Callback de wakeup timer.
 * @param context Contexto de usuario provisto en el registro.
 */
typedef void (*RTC_WakeupCallback_t)(void *context);

/* ===== V-Table de RTC ===== */

typedef struct I_RTC_Vtable
{
    /* Inicialización */
    Result_t (*Init)(void *self, RTC_Handle_t handle);
    Result_t (*DeInit)(void *self, RTC_Handle_t handle);

    /* Tiempo */
    Result_t (*SetTime)(void *self, RTC_Handle_t handle, const RTC_Time_t *time);
    Result_t (*GetTime)(void *self, RTC_Handle_t handle, RTC_Time_t *time);

    /* Fecha */
    Result_t (*SetDate)(void *self, RTC_Handle_t handle, const RTC_Date_t *date);
    Result_t (*GetDate)(void *self, RTC_Handle_t handle, RTC_Date_t *date);

    /* Alarmas */
    Result_t (*SetAlarm)(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config);
    Result_t (*SetAlarm_IT)(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config);
    Result_t (*DeactivateAlarm)(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm);

    /* Backup Registers (para guardar estado entre resets) */
    void (*WriteBackupRegister)(void *self, RTC_Handle_t handle, uint32_t regIndex, uint32_t data);
    uint32_t (*ReadBackupRegister)(void *self, RTC_Handle_t handle, uint32_t regIndex);
    void (*EraseBackupRegisters)(void *self, RTC_Handle_t handle);

    /* Callbacks */
    void (*RegisterAlarmCallback)(void *self, RTC_Handle_t handle, RTC_AlarmCallback_t cb, void *context);
    void (*RegisterWakeupCallback)(void *self, RTC_Handle_t handle, RTC_WakeupCallback_t cb, void *context);
} I_RTC_Vtable;

/**
 * @brief Instancia de interfaz RTC.
 */
typedef struct I_RTC
{
    const I_RTC_Vtable *vtable;
    void *impl;
} I_RTC;

/* ===== Helpers defensivos (inline) ===== */

static inline bool rtc_is_valid(const I_RTC *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t RTC_Init(I_RTC *iface, RTC_Handle_t handle)
{
    if (!rtc_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->Init(iface->impl, handle);
}

static inline Result_t RTC_DeInit(I_RTC *iface, RTC_Handle_t handle)
{
    if (!rtc_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeInit(iface->impl, handle);
}

static inline Result_t RTC_SetTime(I_RTC *iface, RTC_Handle_t handle, const RTC_Time_t *time)
{
    if (!rtc_is_valid(iface) || time == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->SetTime(iface->impl, handle, time);
}

static inline Result_t RTC_GetTime(I_RTC *iface, RTC_Handle_t handle, RTC_Time_t *time)
{
    if (!rtc_is_valid(iface) || time == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetTime(iface->impl, handle, time);
}

static inline Result_t RTC_SetDate(I_RTC *iface, RTC_Handle_t handle, const RTC_Date_t *date)
{
    if (!rtc_is_valid(iface) || date == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->SetDate(iface->impl, handle, date);
}

static inline Result_t RTC_GetDate(I_RTC *iface, RTC_Handle_t handle, RTC_Date_t *date)
{
    if (!rtc_is_valid(iface) || date == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetDate(iface->impl, handle, date);
}

static inline Result_t RTC_SetAlarm(I_RTC *iface, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config)
{
    if (!rtc_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->SetAlarm(iface->impl, handle, alarm, config);
}

static inline Result_t RTC_SetAlarm_IT(I_RTC *iface, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config)
{
    if (!rtc_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->SetAlarm_IT(iface->impl, handle, alarm, config);
}

static inline Result_t RTC_DeactivateAlarm(I_RTC *iface, RTC_Handle_t handle, RTC_Alarm_t alarm)
{
    if (!rtc_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->DeactivateAlarm(iface->impl, handle, alarm);
}

static inline void RTC_WriteBackupRegister(I_RTC *iface, RTC_Handle_t handle, uint32_t regIndex, uint32_t data)
{
    if (rtc_is_valid(iface))
    {
        iface->vtable->WriteBackupRegister(iface->impl, handle, regIndex, data);
    }
}

static inline uint32_t RTC_ReadBackupRegister(I_RTC *iface, RTC_Handle_t handle, uint32_t regIndex)
{
    if (!rtc_is_valid(iface))
        return 0;
    return iface->vtable->ReadBackupRegister(iface->impl, handle, regIndex);
}

static inline void RTC_EraseBackupRegisters(I_RTC *iface, RTC_Handle_t handle)
{
    if (rtc_is_valid(iface))
    {
        iface->vtable->EraseBackupRegisters(iface->impl, handle);
    }
}

static inline void RTC_RegisterAlarmCallback(I_RTC *iface, RTC_Handle_t handle, RTC_AlarmCallback_t cb, void *context)
{
    if (rtc_is_valid(iface))
    {
        iface->vtable->RegisterAlarmCallback(iface->impl, handle, cb, context);
    }
}

static inline void RTC_RegisterWakeupCallback(I_RTC *iface, RTC_Handle_t handle, RTC_WakeupCallback_t cb, void *context)
{
    if (rtc_is_valid(iface))
    {
        iface->vtable->RegisterWakeupCallback(iface->impl, handle, cb, context);
    }
}

#endif /* I_RTC_H */
