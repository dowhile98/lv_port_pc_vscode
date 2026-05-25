/**
 * @file i_relay_controller.h
 * @brief Interfaz polimórfica para control de relés.
 *
 * Domain Layer Interface - Abstracción de hardware específico.
 * El RelayAdapter implementa esta interfaz.
 */

#ifndef I_RELAY_CONTROLLER_H
#define I_RELAY_CONTROLLER_H
#include <stddef.h>
#include "hal_types.h"
#include "common/relay_types.h"

/**
 * @brief Vtable para controladores de relés.
 */
typedef struct IRelayController_Vtable
{
    /**
     * @brief Cambia el estado del relé a OPEN o CLOSED.
     *
     * @param[in] self        Implementación (no NULL).
     * @param[in] new_state   Nuevo estado deseado.
     *
     * @return ERR_OK si éxito, ERR_NULL_POINTER si self es NULL,
     *         ERR_INVALID_STATE si transición no permitida.
     *
     * @note Esta función es rápida (non-blocking) pero puede disparar
     *       callbacks de estado que se ejecutan en el contexto del llamador.
     */
    Result_t (*SetState)(void *self, RelayContactState_t new_state);

    /**
     * @brief Obtiene el estado actual del relé.
     *
     * @param[in]  self         Implementación (no NULL).
     * @param[out] out_state    Estado actual (no NULL).
     *
     * @return ERR_OK si éxito, ERR_NULL_POINTER si parámetros son NULL.
     */
    Result_t (*GetState)(void *self, RelayContactState_t *out_state);

    /**
     * @brief Obtiene el estado completo observable del relé.
     *
     * @param[in]  self        Implementación (no NULL).
     * @param[out] out_status  Estado observable (no NULL).
     *
     * @return ERR_OK si éxito, ERR_NULL_POINTER si parámetros son NULL.
     */
    Result_t (*GetStatus)(void *self, RelayStatus_t *out_status);

    /**
     * @brief Actualiza la configuración del relé.
     *
     * @param[in] self   Implementación (no NULL).
     * @param[in] config Nueva configuración (no NULL).
     *
     * @return ERR_OK si éxito, ERR_NULL_POINTER si parámetros son NULL,
     *         ERR_INVALID_PARAM si configuración es inválida.
     *
     * @note Puede causar cambio inmediato de estado si ventana/ciclos cambian.
     */
    Result_t (*UpdateConfig)(void *self, const RelayConfig_t *config);

    /**
     * @brief Registra callback para notificación de cambios de estado.
     *
     * @param[in] self      Implementación (no NULL).
     * @param[in] callback  Función a llamar en cambios (no NULL).
     * @param[in] context   Contexto pasado al callback (puede ser NULL).
     *
     * @return ERR_OK si éxito, ERR_NULL_POINTER si self o callback son NULL,
     *         ERR_BUSY si ya hay callback registrado.
     *
     * @note El callback se ejecuta en el contexto de RelayAdapter.Update()
     *       (típicamente desde thread de aplicación, no ISR).
     */
    Result_t (*RegisterStateChangeCallback)(void *self,
                                            RelayStateChangeCallback_t callback,
                                            void *context);

    /**
     * @brief Desregistra el callback anterior.
     *
     * @param[in] self  Implementación (no NULL).
     *
     * @return ERR_OK si había callback registrado, ERR_IDLE si no había.
     */
    Result_t (*UnregisterStateChangeCallback)(void *self);

    /* ===== NEW: Active Object Integration Methods ===== */

    /**
     * @brief Handle PPS (1Hz GPS pulse) event.
     *
     * @note Called from Active Object thread context (NOT ISR).
     *       This triggers state machine re-evaluation based on GPS time sync.
     *
     * @param[in] self       Implementación (no NULL).
     * @param[in] gps_time   GPS time at PPS edge (can be NULL if unavailable).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER if self is NULL.
     *
     * @note Thread-safe: Can be called concurrently with Update().
     */
    Result_t (*OnPPS)(void *self, const DateTime_t *gps_time);

    /**
     * @brief Periodic state machine update (100ms tick).
     *
     * @note Called from Active Object thread context.
     *       Processes pending state transitions, window checks, cycle updates.
     *
     * @param[in] self  Implementación (no NULL).
     *
     * @return ERR_OK on success, ERR_NULL_POINTER if self is NULL.
     *
     * @note Must be called regularly (recommended: 100ms interval).
     */
    Result_t (*Update)(void *self);

    /**
     * @brief Actualiza estado de alarma del relay (safety-critical).
     *
     * Cuando alarma activa:
     * - Fuerza apertura de contacto inmediata (<100ms)
     * - Transiciona FSM a RELAY_FSM_FORCED_OPEN
     * - Sobrescribe operación normal de ciclo
     *
     * Cuando alarma liberada:
     * - Limpia flag de alarma
     * - Retorna FSM a operación normal (WAIT_TIME_SYNC)
     * - Update() gestiona reanudación de ciclo
     *
     * @param[in] self          Implementación (no NULL).
     * @param[in] alarm_active  true si alarma activa, false si liberada.
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si self es NULL
     * @return ERR_INVALID_STATE si no inicializado
     *
     * @note SAFETY-CRITICAL: Prioridad alta sobre eventos normales
     * @note Thread-safe: ejecuta en contexto RelayAO thread (NO ISR)
     * @note Implementación opcional: mocks pueden retornar ERR_NOT_SUPPORTED
     */
    Result_t (*SetAlarmState)(void *self, bool alarm_active);

} IRelayController_Vtable;

/**
 * @brief Interfaz polimórfica de controlador de relés.
 */
typedef struct IRelayController
{
    const IRelayController_Vtable *vtable;
    void *impl;
} IRelayController;

/* ===== Macros para llamadas polimórficas ===== */

/**
 * @brief Valida la interfaz de forma segura.
 */
static inline bool relay_controller_is_valid(const IRelayController *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

/**
 * @brief Cambia el estado del relé.
 */
static inline Result_t RelayController_SetState(IRelayController *iface,
                                                RelayContactState_t new_state)
{
    if (!relay_controller_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->SetState(iface->impl, new_state);
}

/**
 * @brief Obtiene el estado actual del relé.
 */
static inline Result_t RelayController_GetState(IRelayController *iface,
                                                RelayContactState_t *out_state)
{
    if (!relay_controller_is_valid(iface) || out_state == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetState(iface->impl, out_state);
}

/**
 * @brief Obtiene el estado observable completo.
 */
static inline Result_t RelayController_GetStatus(IRelayController *iface,
                                                 RelayStatus_t *out_status)
{
    if (!relay_controller_is_valid(iface) || out_status == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetStatus(iface->impl, out_status);
}

/**
 * @brief Actualiza la configuración.
 */
static inline Result_t RelayController_UpdateConfig(IRelayController *iface,
                                                    const RelayConfig_t *config)
{
    if (!relay_controller_is_valid(iface) || config == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->UpdateConfig(iface->impl, config);
}

/**
 * @brief Registra callback de cambio de estado.
 */
static inline Result_t RelayController_RegisterStateChangeCallback(IRelayController *iface,
                                                                   RelayStateChangeCallback_t callback,
                                                                   void *context)
{
    if (!relay_controller_is_valid(iface) || callback == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->RegisterStateChangeCallback(iface->impl, callback, context);
}

/**
 * @brief Desregistra callback de cambio de estado.
 */
static inline Result_t RelayController_UnregisterStateChangeCallback(IRelayController *iface)
{
    if (!relay_controller_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->UnregisterStateChangeCallback(iface->impl);
}

/* ===== NEW: Active Object Integration Helpers ===== */

/**
 * @brief Call OnPPS on the relay controller.
 *
 * @param[in] iface      Relay controller interface (not NULL).
 * @param[in] gps_time   GPS time at PPS edge (can be NULL).
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if iface is NULL.
 *
 * @note ISR-safe if implementation is ISR-safe (RelayAdapter queues event).
 */
static inline Result_t RelayController_OnPPS(IRelayController *iface,
                                             const DateTime_t *gps_time)
{
    if (!relay_controller_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->OnPPS(iface->impl, gps_time);
}

/**
 * @brief Call periodic Update on the relay controller.
 *
 * @param[in] iface  Relay controller interface (not NULL).
 *
 * @return ERR_OK on success, ERR_NULL_POINTER if iface is NULL.
 *
 * @note Thread-safe: Can be called from Active Object thread.
 */
static inline Result_t RelayController_Update(IRelayController *iface)
{
    if (!relay_controller_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->Update(iface->impl);
}

/**
 * @brief Actualiza estado de alarma (safety-critical).
 *
 * @param[in] iface         Relay controller interface (not NULL).
 * @param[in] alarm_active  true si alarma activa, false si liberada.
 *
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si iface es NULL
 * @return ERR_NOT_SUPPORTED si implementación no soporta alarmas (mocks)
 *
 * @note SAFETY-CRITICAL: Procesado con prioridad alta
 * @note Thread-safe: ejecuta en contexto RelayAO thread
 */
static inline Result_t RelayController_SetAlarmState(IRelayController *iface,
                                                     bool alarm_active)
{
    if (!relay_controller_is_valid(iface))
        return ERR_NULL_POINTER;
    if (iface->vtable->SetAlarmState == NULL)
        return ERR_NOT_SUPPORTED; /* Mock o implementación sin alarma */
    return iface->vtable->SetAlarmState(iface->impl, alarm_active);
}

#endif /* I_RELAY_CONTROLLER_H */
