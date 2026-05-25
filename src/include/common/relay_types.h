/**
 * @file relay_types.h
 * @brief Tipos compartidos para el subsistema de relés (Domain-facing).
 *
 * Contiene enumeraciones y estructuras que definen la configuración
 * y el estado de los relés, independientes del hardware específico.
 */

#ifndef RELAY_TYPES_H
#define RELAY_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "common/date_time.h"

#ifndef RELAY_MAX_CYCLES
#define RELAY_MAX_CYCLES 10 /**< Número máximo de ciclos en configuración multicycle. */
#endif
/**
 * @brief Estado físico del contacto del relé.
 */
typedef enum
{
    RELAY_CONTACT_OPEN = 1,  /**< Relé abierto (NO contact abierío, sin corriente). */
    RELAY_CONTACT_CLOSED = 0 /**< Relé cerrado (corriente fluyendo). */
} RelayContactState_t;

/**
 * @brief Tipo de contacto físico disponible.
 */
typedef enum
{
    RELAY_TYPE_NO = 0, /**< Normally Open. Corriente cuando CLOSED. */
    RELAY_TYPE_NC = 1  /**< Normally Closed. Corriente cuando OPEN. */
} RelayContactType_t;

/**
 * @brief Estado lógico interno del relé (para máquina de estados).
 */
typedef enum
{
    RELAY_STATE_IDLE = 0,    /**< Inactivo, esperando configuración/sincronización. */
    RELAY_STATE_WAITING = 1, /**< Sincronizado, esperando entrar en ventana de tiempo. */
    RELAY_STATE_ACTIVE = 2,  /**< Activo, ejecutando ciclos ON/OFF. */
    RELAY_STATE_ERROR = 3    /**< Error crítico. */
} RelayInternalState_t;

/**
 * @brief Callback disparado cuando cambia el estado del relé.
 *
 * @param[in] context   Contexto registrado al callback.
 * @param[in] old_state Estado anterior.
 * @param[in] new_state Nuevo estado.
 * @param[in] timestamp Timestamp del cambio (en ms).
 */
typedef void (*RelayStateChangeCallback_t)(void *context,
                                           RelayContactState_t old_state,
                                           RelayContactState_t new_state,
                                           uint32_t timestamp);

/**
 * @brief Configuración de un ciclo ON/OFF simple.
 */
typedef struct __attribute__((packed)) SimpleCycle
{
    uint32_t ton;  /**< Duración del período ON (en milisegundos). */
    uint32_t toff; /**< Duración del período OFF (en milisegundos). */
} SimpleCycle_t;

//_Static_assert(sizeof(SimpleCycle_t) == 8, "SimpleCycle_t must be 8 bytes");

/**
 * @brief Configuración de un ciclo multicycle con rangos de fecha.
 *
 * Permite variar Ton y Toff según rangos de fechas.
 * Ejemplo:
 *   - 01 Jan - 28 Feb: Ton[0], Toff[0]
 *   - 01 Mar - 31 May: Ton[1], Toff[1]
 *   - 01 Jun - 31 Aug: Ton[2], Toff[2]
 *   - 01 Sep - 31 Dec: Ton[3], Toff[3]
 */
typedef struct __attribute__((packed)) MultiCycleConfig
{
    uint8_t enabled;                             /**< Si multicycle está habilitado. */
    uint8_t stop_at_end;                         /**< Si parar ciclos después del 4to rango. */
    uint32_t ton[RELAY_MAX_CYCLES];              /**< Ton para cada rango. */
    uint32_t toff[RELAY_MAX_CYCLES];             /**< Toff para cada rango. */
    DateTime_t boundary_dates[RELAY_MAX_CYCLES]; /**< Fechas límite de cada rango (day/month). */
} MultiCycleConfig_t;

//_Static_assert(sizeof(MultiCycleConfig_t) == 162, "MultiCycleConfig_t must be 162 bytes (2 + 40 + 40 + 80)");

/**
 * @brief Configuración de la ventana temporal de interrupción.
 */
typedef struct __attribute__((packed)) TimeWindowConfig
{
    DateTime_t start_time; /**< Hora de inicio (solo hh:mm:ss relevante). */
    DateTime_t stop_time;  /**< Hora de parada (solo hh:mm:ss relevante). */
    uint8_t weekday_mask;  /**< Máscara de días válidos (bit 0=Lun, ..., bit 6=Dom). */
} TimeWindowConfig_t;

//_Static_assert(sizeof(TimeWindowConfig_t) == 17, "TimeWindowConfig_t must be 17 bytes (2*8 + 1)");

/**
 * @brief Configuración completa del relé.
 *
 * Encapsula todos los parámetros necesarios para operar el relé:
 * - Tipo de contacto (NO/NC)
 * - Ciclos (simple o multicycle)
 * - Ventana temporal
 * - Duraciones de transición
 */
typedef struct __attribute__((packed)) RelayConfig
{
    uint8_t contact_type; /**< Tipo de contacto (NO o NC). */
    uint8_t enabled;      /**< Si los ciclos están habilitados. */

    TimeWindowConfig_t time_window; /**< Ventana de tiempo permitida. */

    /* Ciclos ON/OFF */
    SimpleCycle_t simple_cycle;    /**< Ciclo simple (si no es multicycle). */
    MultiCycleConfig_t multicycle; /**< Ciclo múltiple según fechas. */

    /* Duraciones de transición (margen de seguridad). */
    uint32_t ton_margin_ms;  /**< Margen antes de activar (ms). */
    uint32_t toff_margin_ms; /**< Margen antes de desactivar (ms). */

    /* Flags de control. */
    uint8_t start_with_on; /**< Si inicia con relé cerrado (ON) o abierto (OFF). */

} RelayConfig_t;

_Static_assert(sizeof(RelayConfig_t) == 198, "RelayConfig_t must be 198 bytes (updated for RELAY_MAX_CYCLES=10)");

/**
 * @brief Estado observable del relé (para UI/logging).
 */
typedef struct
{
    RelayContactState_t contact_state;   /**< Estado actual del contacto. */
    RelayInternalState_t internal_state; /**< Estado interno de la máquina. */

    uint8_t is_synchronized; /**< Si el tiempo está sincronizado. */
    uint8_t is_in_window;    /**< Si está dentro de ventana de tiempo. */
    uint8_t alarm_active;    /**< Si alarma de alta temperatura (overtemp) está activa.
                               @note Cuando true: relay forzado abierto (FORCED_OPEN state).
                               @note Safety-critical: contacto abierto en <100ms desde detección. */

    uint32_t next_transition_time; /**< Timestamp del próximo cambio (ms). */
    uint32_t uptime_ms;            /**< Tiempo acumulado con relé cerrado (ms). */

    DateTime_t last_state_change; /**< Hora del último cambio de estado. */
} RelayStatus_t;

#endif /* RELAY_TYPES_H */
