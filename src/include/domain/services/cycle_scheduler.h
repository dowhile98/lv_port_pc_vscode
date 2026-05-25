/**
 * @file cycle_scheduler.h
 * @brief Servicio de programación de ciclos ON/OFF.
 *
 * Domain Service - Implementa la lógica de ciclos simples y multicycle
 * (con variación según rangos de fecha).
 *
 * Responsabilidades:
 * · Determinar si relé debe estar ON o OFF en momento dado
 * · Calcular siguiente punto de transición (ON→OFF o OFF→ON)
 * · Manejar multicycle con rangos de fecha
 * · 100% portátil, sin conocimiento de hardware
 *
 * No Responsable de:
 * · Acceso a hardware (GPIO, Timer)
 * · Sincronización de tiempo
 * · Cambios efectivos de pins (eso lo hace RelayAdapter)
 */

#ifndef CYCLE_SCHEDULER_H
#define CYCLE_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "common/date_time.h"
#include "common/relay_types.h"

/**
 * @brief Estructura privada del servicio de ciclos.
 *
 * Los clientes NO deben acceder a estos miembros directamente.
 * Usar las funciones públicas.
 */
typedef struct
{

    RelayContactType_t contact_type; /**< Tipo de contacto (NO o NC). */
    bool enabled;                    /**< Si los ciclos están habilitados. */

    /* Duraciones de transición (margen de seguridad). */
    uint32_t ton_margin_ms;  /**< Margen antes de activar (ms). */
    uint32_t toff_margin_ms; /**< Margen antes de desactivar (ms). */

    /* Flags de control. */
    bool start_with_on; /**< Si inicia con relé cerrado (ON) o abierto (OFF). */
    /* Ciclos ON/OFF */
    SimpleCycle_t simple_cycle;
    MultiCycleConfig_t multicycle;

    /* Estado de seguimiento */
    uint8_t multicycle_current_index; /**< 0-3: índice del rango de fecha actual. */
    uint32_t last_period_start_time;  /**< Timestamp (ms) del último inicio de período. */
} CycleSchedulerService_t;

/**
 * @brief Inicializa el servicio de ciclos.
 *
 * @param[out] self       Instancia del servicio (no NULL).
 * @param[in]  config     Configuración del relé (no NULL).
 *
 * @return ERR_OK si éxito,
 *         ERR_NULL_POINTER si parámetros NULL,
 *         ERR_INVALID_PARAM si configuración de ciclos es inválida.
 */
Result_t CycleScheduler_Init(CycleSchedulerService_t *self,
                             const RelayConfig_t *config);

/**
 * @brief Actualiza la configuración de ciclos en tiempo de ejecución.
 *
 * @param[in] self      Instancia (no NULL).
 * @param[in] config    Nueva configuración (no NULL).
 *
 * @return ERR_OK si éxito,
 *         ERR_NULL_POINTER si parámetros NULL,
 *         ERR_INVALID_PARAM si nueva config es inválida.
 *
 * @note Si está en medio de un ciclo, continúa hasta completar el período
 *       actual antes de aplicar la nueva configuración.
 */
Result_t CycleScheduler_UpdateConfig(CycleSchedulerService_t *self,
                                     const RelayConfig_t *config);

/**
 * @brief De-inicializa el servicio de ciclos y libera recursos.
 *
 * @param[in] self Instancia (no NULL).
 * @return ERR_OK si exitoso, ERR_NULL_POINTER si self es NULL.
 *
 * @note Idempotente: puede llamarse múltiples veces sin efectos adversos.
 */
Result_t CycleScheduler_Deinit(CycleSchedulerService_t *self);

/**
 * @brief Determina si el relé debe estar ON o OFF en el momento dado.
 *
 * Considera:
 * · Tiempo relativo dentro del período actual
 * · Ton y Toff (simple o según rango de fecha si multicycle)
 * · Último inicio de período conocido
 *
 * @param[in]  self         Instancia (no NULL).
 * @param[in]  relative_sec Segundos transcurridos desde inicio de ventana (ej: de TimeWindowValidator).
 * @param[in]  now          Hora actual para cálculo de rango de fecha multicycle (no NULL).
 * @param[out] out_should_be_on Verdadero si debe estar ON, falso si OFF (no NULL).
 *
 * @return ERR_OK si se determina el estado,
 *         ERR_NULL_POINTER si parámetros NULL.
 *
 * Ejemplo:
 *   CycleSchedulerService_t scheduler = {0};
 *   CycleScheduler_Init(&scheduler, &config);  // Ton=2s, Toff=3s
 *
 *   DateTime_t now = {.hour=10};
 *   uint32_t rel_sec = 0;   // Inicio de ventana
 *   bool should_be_on = false;
 *   CycleScheduler_GetStateAtTime(&scheduler, rel_sec, &now, &should_be_on);
 *   // should_be_on = true (0-2 segundos = ON)
 *
 *   rel_sec = 2;   // 2 segundos desde inicio de ventana
 *   CycleScheduler_GetStateAtTime(&scheduler, rel_sec, &now, &should_be_on);
 *   // should_be_on = false (2-5 segundos = OFF)
 *
 *   rel_sec = 5;   // 5 segundos = inicio de nuevo período
 *   CycleScheduler_GetStateAtTime(&scheduler, rel_sec, &now, &should_be_on);
 *   // should_be_on = true (nuevamente ON)
 */
Result_t CycleScheduler_GetStateAtTime(CycleSchedulerService_t *self,
                                       uint32_t relative_sec,
                                       const DateTime_t *now,
                                       bool *out_should_be_on);

/**
 * @brief Calcula el tiempo (en segundos) hasta el próximo cambio de estado.
 *
 * @param[in]  self              Instancia (no NULL).
 * @param[in]  relative_sec      Segundos transcurridos desde inicio de ventana.
 * @param[in]  now               Hora actual (no NULL).
 * @param[out] out_seconds_until Segundos hasta próxima transición (no NULL).
 *
 * @return ERR_OK si se calcula la transición,
 *         ERR_NULL_POINTER si parámetros NULL,
 *         ERR_INVALID_STATE si no hay configuración válida.
 *
 * Ejemplo:
 *   Ton=2s, Toff=3s, rel_sec=1 (dentro de ON)
 *   CycleScheduler_GetTimeToNextTransition(...) retorna out_seconds_until=1
 *   (falta 1 segundo para OFF)
 */
Result_t CycleScheduler_GetTimeToNextTransition(CycleSchedulerService_t *self,
                                                uint32_t relative_sec,
                                                const DateTime_t *now,
                                                uint32_t *out_seconds_until);

/**
 * @brief Actualiza el índice de rango de fecha para multicycle.
 *
 * Calcula en qué rango de fecha se encuentra hoy y actualiza el índice interno.
 * Se llama automáticamente en GetStateAtTime(), pero puede ser llamado
 * explícitamente para debugging.
 *
 * @param[in] self  Instancia (no NULL).
 * @param[in] now   Hora actual (solo day/month relevante) (no NULL).
 *
 * @return ERR_OK si se actualiza,
 *         ERR_NULL_POINTER si parámetros NULL.
 */
Result_t CycleScheduler_UpdateMulticycleIndex(CycleSchedulerService_t *self,
                                              const DateTime_t *now);

/**
 * @brief Obtiene el índice de rango de fecha actual (para debugging/UI).
 *
 * @param[in]  self          Instancia (no NULL).
 * @param[out] out_index     Índice actual 0-4 (no NULL).
 *
 * @return ERR_OK si se obtiene,
 *         ERR_NULL_POINTER si parámetros NULL.
 *
 * @note Rango 4 indica "fuera de todos los rangos" (después del 4to rango).
 */
Result_t CycleScheduler_GetMulticycleIndex(CycleSchedulerService_t *self,
                                           uint8_t *out_index);

/**
 * @brief Obtiene los valores Ton y Toff actuales (según multicycle o simple).
 *
 * @param[in]  self      Instancia (no NULL).
 * @param[in]  now       Hora actual (para determinar rango multicycle) (no NULL).
 * @param[out] out_ton   Duración ON (segundos) (no NULL).
 * @param[out] out_toff  Duración OFF (segundos) (no NULL).
 *
 * @return ERR_OK si se obtienen,
 *         ERR_NULL_POINTER si parámetros NULL.
 */
Result_t CycleScheduler_GetCurrentTonToff(CycleSchedulerService_t *self,
                                          const DateTime_t *now,
                                          uint32_t *out_ton,
                                          uint32_t *out_toff);

/**
 * @brief Obtiene la configuración del ciclo activo actual.
 *
 * Si multicycle está habilitado, retorna el ciclo correspondiente
 * al índice actual. Si no, retorna el ciclo simple.
 *
 * @param[in]  self        Instancia (no NULL).
 * @param[out] out_config  Configuración del ciclo activo (no NULL).
 *
 * @return ERR_OK si éxito,
 *         ERR_NULL_POINTER si parámetros NULL.
 */
Result_t CycleScheduler_GetCurrentConfig(CycleSchedulerService_t *self,
                                         RelayConfig_t *out_config);

#endif /* CYCLE_SCHEDULER_H */
