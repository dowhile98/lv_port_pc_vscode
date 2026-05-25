/**
 * @file time_window_validator.h
 * @brief Servicio de validación de ventanas temporales.
 *
 * Domain Service - Determina si la hora actual cae dentro de la ventana
 * de tiempo permitida para operar el relé, considerando hora del día y
 * día de la semana.
 *
 * Responsabilidades:
 * · Validar ventana temporal (start_time, stop_time)
 * · Validar día de semana (weekday_mask)
 * · Calcular tiempo relativo dentro de ventana
 * · 100% portátil, sin conocimiento de hardware
 *
 * No Responsable de:
 * · Acceso a hardware (GPIO, RTC, Timer)
 * · Sincronización de tiempo (eso lo hace TimeAdapter)
 * · Cálculo de ciclos (eso lo hace CycleSchedulerService)
 */

#ifndef TIME_WINDOW_VALIDATOR_H
#define TIME_WINDOW_VALIDATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "common/date_time.h"
#include "common/relay_types.h"

/**
 * @brief Valida si la hora actual está dentro de la ventana temporal.
 *
 * Verifica que:
 * 1. La hora (hh:mm:ss) esté entre start_time y stop_time
 * 2. El día actual sea uno de los permitidos en weekday_mask
 *
 * @param[in] config        Configuración de ventana (no NULL).
 * @param[in] now           Hora actual (no NULL).
 *
 * @return true si está dentro de la ventana, false en caso contrario.
 *
 * @note Casos especiales:
 *   - Si start_time == stop_time, la ventana se considera inactiva (retorna false).
 *   - Si start_time > stop_time, se interpreta como ventana nocturna (23:00 - 06:00).
 *   - weekday_mask = 0x7F (0b1111111) = todos los días.
 *   - weekday_mask = 0x1F (0b0011111) = Lun-Vie (bit 0=Lun, bit 4=Vie, bits 5-6=Sab-Dom).
 *
 * Ejemplos:
 *   TimeWindowConfig_t cfg = {
 *       .start_time = {.hour=08, .minute=00, .second=00},
 *       .stop_time  = {.hour=18, .minute=00, .second=00},
 *       .weekday_mask = 0x1F  // Lun-Vie
 *   };
 *   DateTime_t now = {.hour=10, .minute=30, .second=0, .weekDay=2}; // Martes
 *   bool in_window = TimeWindowValidator_IsActive(&cfg, &now);
 *   // Retorna: true (dentro de hora y día)
 */
Result_t TimeWindowValidator_IsActive(const TimeWindowConfig_t *config,
                                      const DateTime_t *now,
                                      bool *out_is_active);

/**
 * @brief Calcula el tiempo transcurrido desde el inicio de la ventana actual.
 *
 * Si la hora actual está dentro de la ventana, retorna cuántos segundos
 * han transcurrido desde start_time.
 *
 * @param[in]  config               Configuración de ventana (no NULL).
 * @param[in]  now                  Hora actual (no NULL).
 * @param[out] out_relative_seconds  Segundos desde start_time (no NULL).
 *
 * @return ERR_OK si está dentro de la ventana y se calcula el tiempo relativo,
 *         ERR_INVALID_STATE si no está dentro de la ventana.
 *
 * @note Casos especiales:
 *   - Para ventanas nocturnas, calcula tiempo transcurrido considerando el cruce de medianoche.
 *   - Rango de retorno: 0 a (period de ventana).
 *
 * Ejemplo:
 *   TimeWindowConfig_t cfg = {.start_time = 08:00, .stop_time = 18:00};
 *   DateTime_t now = {.hour=10, .minute=30};
 *   uint32_t rel_sec = 0;
 *   TimeWindowValidator_GetRelativeTime(&cfg, &now, &rel_sec);
 *   // rel_sec = 9000 (2.5 horas = 9000 segundos)
 */
Result_t TimeWindowValidator_GetRelativeTime(const TimeWindowConfig_t *config,
                                             const DateTime_t *now,
                                             uint32_t *out_relative_seconds);

/**
 * @brief Valida la configuración de ventana (detección de errores de configuración).
 *
 * @param[in] config Configuración a validar (no NULL).
 *
 * @return ERR_OK si es válida,
 *         ERR_INVALID_PARAM si weekday_mask es 0 (ningún día permitido),
 *         ERR_INVALID_PARAM si hay otros problemas de validación.
 */
Result_t TimeWindowValidator_ValidateConfig(const TimeWindowConfig_t *config);

/**
 * @brief Obtiene el tiempo (hh:mm:ss) del siguiente cambio de ventana.
 *
 * Determina cuándo cambiaremos de "dentro de ventana" a "fuera de ventana"
 * o viceversa.
 *
 * @param[in]  config         Configuración de ventana (no NULL).
 * @param[in]  now            Hora actual (no NULL).
 * @param[out] out_next_event Hora del próximo evento (solo hh:mm:ss) (no NULL).
 *
 * @return ERR_OK si se puede calcular (siempre sucede),
 *         ERR_NULL_POINTER si parámetros NULL.
 *
 * Ejemplo:
 *   cfg = {.start_time = 08:00, .stop_time = 18:00};
 *   now = {.hour = 10:00};
 *   DateTime_t next = {0};
 *   TimeWindowValidator_GetNextEvent(&cfg, &now, &next);
 *   // next.hour = 18, next.minute = 0 (próxima salida de ventana)
 *
 *   now = {.hour = 20:00};  // Fuera de ventana
 *   TimeWindowValidator_GetNextEvent(&cfg, &now, &next);
 *   // next.hour = 8, next.minute = 0 (próxima entrada a ventana)
 */
Result_t TimeWindowValidator_GetNextEvent(const TimeWindowConfig_t *config,
                                          const DateTime_t *now,
                                          DateTime_t *out_next_event);

#endif /* TIME_WINDOW_VALIDATOR_H */
