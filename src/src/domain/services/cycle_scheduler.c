#include "domain/services/cycle_scheduler.h"
#include <stddef.h>
#include <string.h>
/**
 * @brief Compara si date1 es estrictamente posterior a date2.
 *
 * Comparación: año → mes → día (orden de significancia)
 *
 * @return true si d1 > d2, false en caso contrario (incluyendo igualdad)
 */
static bool is_date_after(const DateTime_t *d1, const DateTime_t *d2)
{
    // Comparar año primero
    if (d1->year > d2->year)
        return true;
    if (d1->year < d2->year)
        return false;

    // Mismo año: comparar mes
    if (d1->month > d2->month)
        return true;
    if (d1->month < d2->month)
        return false;

    // Mismo año/mes: comparar día
    return d1->day > d2->day;
}

Result_t CycleScheduler_Init(CycleSchedulerService_t *self, const RelayConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;
    /*cycle*/
    self->simple_cycle = config->simple_cycle;
    self->multicycle = config->multicycle;
    /*start with on*/
    self->start_with_on = config->start_with_on;
    /*margins*/
    self->ton_margin_ms = config->ton_margin_ms;
    self->toff_margin_ms = config->toff_margin_ms;
    /*enabled*/
    self->enabled = config->enabled;
    /*contact type*/
    self->contact_type = config->contact_type;

    self->multicycle_current_index = 0;
    self->last_period_start_time = 0;

    return ERR_OK;
}

Result_t CycleScheduler_UpdateConfig(CycleSchedulerService_t *self, const RelayConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;

    /*cycle*/
    self->simple_cycle = config->simple_cycle;
    self->multicycle = config->multicycle;
    /*start with on*/
    self->start_with_on = config->start_with_on;
    /*margins*/
    self->ton_margin_ms = config->ton_margin_ms;
    self->toff_margin_ms = config->toff_margin_ms;
    /*enabled*/
    self->enabled = config->enabled;
    /*contact type*/
    self->contact_type = config->contact_type;
    return ERR_OK;
}

Result_t CycleScheduler_UpdateMulticycleIndex(CycleSchedulerService_t *self, const DateTime_t *now)
{
    if (self == NULL || now == NULL)
        return ERR_NULL_POINTER;
    if (!self->multicycle.enabled)
        return ERR_OK;

    DateTime_t d2 = {0};

    uint8_t index = RELAY_MAX_CYCLES; // Default: fuera de rangos
    for (uint8_t i = 0; i < RELAY_MAX_CYCLES; i++)
    {
    	d2 = self->multicycle.boundary_dates[i];
        // Si 'now' es antes o igual a la fecha límite, este es el rango
        if (!is_date_after(now, &d2))
        {
            index = i;
            break;
        }
    }

    self->multicycle_current_index = index;
    return ERR_OK;
}

Result_t CycleScheduler_GetCurrentTonToff(CycleSchedulerService_t *self, const DateTime_t *now,
                                          uint32_t *out_ton, uint32_t *out_toff)
{
    if (self == NULL || out_ton == NULL || out_toff == NULL)
        return ERR_NULL_POINTER;

    if (self->multicycle.enabled)
    {
        CycleScheduler_UpdateMulticycleIndex(self, now);
        uint8_t idx = self->multicycle_current_index;

        if (idx < RELAY_MAX_CYCLES)
        {
            *out_ton = self->multicycle.ton[idx];
            *out_toff = self->multicycle.toff[idx];
        }
        else
        {
            // Fuera de rango, usar el último
            *out_ton = self->multicycle.ton[RELAY_MAX_CYCLES - 1];
            *out_toff = self->multicycle.toff[RELAY_MAX_CYCLES - 1];
        }
    }
    else
    {
        *out_ton = self->simple_cycle.ton;
        *out_toff = self->simple_cycle.toff;
    }

    // Seguridad contra división por cero
    if ((*out_ton + *out_toff) == 0)
    {
        *out_ton = 50;  // Fallback mínimo (50ms)
        *out_toff = 50; // Fallback mínimo (50ms)
    }

    return ERR_OK;
}

Result_t CycleScheduler_GetStateAtTime(CycleSchedulerService_t *self, uint32_t relative_sec,
                                       const DateTime_t *now, bool *out_should_be_on)
{
    if (self == NULL || now == NULL || out_should_be_on == NULL)
        return ERR_NULL_POINTER;

    uint32_t ton, toff;

    CycleScheduler_GetCurrentTonToff(self, now, &ton, &toff);

    uint32_t period = ton + toff; // period in miliseconds
    if (period == 0)
        return ERR_INVALID_STATE;

    /*se verifica si se cumple Tr es divisble de forma exacta por T*/
    uint32_t time_in_period = (relative_sec * 1000) % period;

    if (time_in_period < ton)
    {
        *out_should_be_on = true;
    }
    else
    {
        *out_should_be_on = false;
    }

    return ERR_OK;
}

Result_t CycleScheduler_GetTimeToNextTransition(CycleSchedulerService_t *self, uint32_t relative_sec,
                                                const DateTime_t *now, uint32_t *out_seconds_until)
{
    if (self == NULL || now == NULL || out_seconds_until == NULL)
        return ERR_NULL_POINTER;

    uint32_t ton, toff;
    CycleScheduler_GetCurrentTonToff(self, now, &ton, &toff);

    uint32_t period = ton + toff; // period in miliseconds
    if (period == 0)
        return ERR_INVALID_STATE;

    uint32_t time_in_period = (relative_sec * 1000) % period;

    if (time_in_period < ton)
    {
        // Estamos en ON, falta para OFF
        *out_seconds_until = ton - time_in_period;
    }
    else
    {
        // Estamos en OFF, falta para ON
        *out_seconds_until = period - time_in_period;
    }

    return ERR_OK;
}

Result_t CycleScheduler_GetMulticycleIndex(CycleSchedulerService_t *self, uint8_t *out_index)
{
    if (self == NULL || out_index == NULL)
        return ERR_NULL_POINTER;
    *out_index = self->multicycle_current_index;
    return ERR_OK;
}

Result_t CycleScheduler_GetCurrentConfig(CycleSchedulerService_t *self, RelayConfig_t *out_config)
{
    if (self == NULL || out_config == NULL)
        return ERR_NULL_POINTER;

    /* Construir configuración basada en estado actual */
    if (self->multicycle.enabled)
    {
        uint8_t idx = self->multicycle_current_index;
        if (idx >= RELAY_MAX_CYCLES)
            idx = RELAY_MAX_CYCLES - 1; /* Usar último rango si fuera de límites */

        /* Copiar ciclo del multicycle actual */
        out_config->simple_cycle.ton = self->multicycle.ton[idx];
        out_config->simple_cycle.toff = self->multicycle.toff[idx];

        /* Multicycle tiene configuración compartida */
        out_config->multicycle = self->multicycle;
    }
    else
    {
        out_config->simple_cycle = self->simple_cycle;
    }

    /*enabled*/
    out_config->enabled = self->enabled;

    /*contact type*/
    out_config->contact_type = self->contact_type;
    /*start with on*/
    out_config->start_with_on = self->start_with_on;
    /*margins*/
    out_config->ton_margin_ms = self->ton_margin_ms;
    out_config->toff_margin_ms = self->toff_margin_ms;

    return ERR_OK;
}

Result_t CycleScheduler_Deinit(CycleSchedulerService_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Reset state (no dynamic resources) */
    memset(self, 0, sizeof(CycleSchedulerService_t));
    return ERR_OK;
}
