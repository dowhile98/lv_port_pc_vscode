#include "domain/services/time_window_validator.h"
#include <stddef.h>

/**
 * @brief Convierte DateTime_t a segundos desde el inicio del día.
 */
static uint32_t time_to_seconds(const DateTime_t *time)
{
    return (uint32_t)time->hour * 3600U + (uint32_t)time->minute * 60U + (uint32_t)time->second;
}

/**
 * @brief Valida si el tiempo actual está dentro de la ventana de tiempo permitida.
 *
 * @param[in]  config          Configuración de ventana (no NULL).
 * @param[in]  now             Hora actual con weekDay (no NULL).
 * @param[out] out_is_active   true si está dentro de ventana, false en caso contrario.
 *
 * @return ERR_OK siempre.
 *
 * @note Mapeo de weekday_mask:
 *       - now->weekDay es 1=Monday ... 7=Sunday (ISO 8601)
 *       - Bit 0 de weekday_mask = Monday
 *       - Bit 1 = Tuesday
 *       - ...
 *       - Bit 6 = Sunday
 *
 * @note Comportamiento de ventana:
 *       - Si start < stop: ventana diurna normal (ej: 08:00-18:00)
 *       - Si start > stop: ventana nocturna que cruza medianoche (ej: 22:00-06:00)
 *       - Si start == stop: ventana inválida (retorna false)
 *
 * @note El segundo final de la ventana está INCLUIDO.
 *       Si stop=18:00:00, entonces 18:00:00 también está dentro.
 */
Result_t TimeWindowValidator_IsActive(const TimeWindowConfig_t *config,
                                      const DateTime_t *now,
                                      bool *out_is_active)
{
    if (config == NULL || now == NULL || out_is_active == NULL)
    {
        return ERR_NULL_POINTER;
    }

    *out_is_active = false;

    // 1. Validar día de la semana (weekday_mask)
    // weekDay: 1=Lun ... 7=Dom. Ajustamos a bit 0=Lun.
    uint8_t day_bit = (uint8_t)(1U << (now->weekDay - 1));
    if ((config->weekday_mask & day_bit) == 0)
    {
        return ERR_OK; // Día no permitido
    }


    uint32_t sec_now = time_to_seconds(now);
    DateTime_t d1 = config->start_time;
    DateTime_t d2 = config->stop_time;
    uint32_t sec_start = time_to_seconds(&d1);
    uint32_t sec_stop = time_to_seconds(&d2);

    // 2. Validar horas
    if (sec_start < sec_stop)
    {
        // Ventana normal (ej: 08:00 - 18:00)
        if (sec_now >= sec_start && sec_now <= sec_stop)
        {
            *out_is_active = true;
        }
    }
    else if (sec_start > sec_stop)
    {
        // Ventana nocturna (ej: 22:00 - 06:00)
        if (sec_now >= sec_start || sec_now < sec_stop)
        {
            *out_is_active = true;
        }
    }
    else
    {
        // sec_start == sec_stop: Ventana inválida o de 0 segundos
        *out_is_active = false;
    }

    return ERR_OK;
}

Result_t TimeWindowValidator_GetRelativeTime(const TimeWindowConfig_t *config,
                                             const DateTime_t *now,
                                             uint32_t *out_relative_seconds)
{
    bool is_active = false;
    Result_t res = TimeWindowValidator_IsActive(config, now, &is_active);

    if (res != ERR_OK)
        return res;
    if (!is_active)
        return ERR_INVALID_STATE;

    uint32_t sec_now = time_to_seconds(now);// + 1; // +1 para que el segundo final de la ventana también se considere dentro
    DateTime_t d1 = config->start_time;
    uint32_t sec_start = time_to_seconds(&d1);

    if (sec_now >= sec_start)
    {
        *out_relative_seconds = sec_now - sec_start;
    }
    else
    {
        // Caso ventana nocturna después de medianoche
        *out_relative_seconds = (86400U - sec_start) + sec_now;
    }

    return ERR_OK;
}

Result_t TimeWindowValidator_ValidateConfig(const TimeWindowConfig_t *config)
{
    if (config == NULL)
        return ERR_NULL_POINTER;

#if 0
    // Validar weekday_mask
    if (config->weekday_mask == 0)
        return ERR_INVALID_PARAM;
#endif
    // Validar start_time
    if (config->start_time.hour > 23 ||
        config->start_time.minute > 59 ||
        config->start_time.second > 59)
    {
        return ERR_INVALID_PARAM;
    }

    // Validar stop_time
    if (config->stop_time.hour > 23 ||
        config->stop_time.minute > 59 ||
        config->stop_time.second > 59)
    {
        return ERR_INVALID_PARAM;
    }

    // Note: start > stop es válido (ventana nocturna)
    // No validar orden aquí

    return ERR_OK;
}

Result_t TimeWindowValidator_GetNextEvent(const TimeWindowConfig_t *config,
                                          const DateTime_t *now,
                                          DateTime_t *out_next_event)
{
    if (config == NULL || now == NULL || out_next_event == NULL)
        return ERR_NULL_POINTER;

    bool is_active = false;
    TimeWindowValidator_IsActive(config, now, &is_active);

    // Si estamos activos, el próximo evento es stop_time
    // Si estamos inactivos, el próximo evento es start_time
    *out_next_event = is_active ? config->stop_time : config->start_time;

    return ERR_OK;
}
