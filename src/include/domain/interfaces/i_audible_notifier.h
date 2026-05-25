/**
 * @file i_audible_notifier.h
 * @brief Audible Notifier Interface - Domain Abstraction for User Notifications
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @details
 * Interfaz de DOMINIO para notificaciones audibles al usuario. Esta interfaz
 * abstrae el mecanismo concreto de notificación (buzzer, speaker, etc.) y
 * proporciona una API semántica basada en prioridades.
 *
 * **Responsabilidades:**
 * - Definir contrato para notificaciones de usuario (async)
 * - Especificar niveles de prioridad semánticos
 * - Abstraer implementación concreta (DIP compliance)
 *
 * **Implementaciones:**
 * - BuzzerAO: Notificaciones vía buzzer hardware + Active Object
 * - SpeakerAdapter: Notificaciones vía I2S audio (future)
 * - MockAudibleNotifier: Testing
 *
 * **Principios:**
 * - Dependency Inversion: Domain define interfaz, Infrastructure implementa
 * - Interface Segregation: API mínima y enfocada
 * - Single Responsibility: Solo notificaciones audibles
 *
 * @note Esta interfaz pertenece al DOMAIN layer
 * @note Implementaciones residen en APPLICATION o INFRASTRUCTURE
 * @note Thread-safe: Implementaciones deben garantizar thread-safety
 *
 * @see BuzzerAO
 * @see BuzzerNotificationService
 */

#ifndef I_AUDIBLE_NOTIFIER_H
#define I_AUDIBLE_NOTIFIER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"

    /*============================================================================*
     * TYPES & ENUMS
     *============================================================================*/

    /**
     * @brief Niveles de prioridad de notificaciones (semánticos)
     *
     * @details
     * Mapeo semántico → técnico:
     * - LOW: UI feedback (botones) → 50ms, prioridad 5
     * - MEDIUM: Confirmaciones (config saved) → 200-500ms, prioridad 7
     * - HIGH: Alarmas (temperatura, GPS) → 1000ms, prioridad 9
     * - CRITICAL: Fallas críticas (power fail) → 2000ms, prioridad 10
     */
    typedef enum
    {
        NOTIFICATION_PRIORITY_LOW = 0, /**< UI feedback (botones, navegación) */
        NOTIFICATION_PRIORITY_MEDIUM,  /**< Confirmaciones de operaciones */
        NOTIFICATION_PRIORITY_HIGH,    /**< Alarmas no críticas (temperatura, GPS) */
        NOTIFICATION_PRIORITY_CRITICAL /**< Alarmas críticas (power fail, hardware fault) */
    } NotificationPriority_t;

    /**
     * @brief Forward declaration de implementación opaca
     */
    typedef struct IAudibleNotifier_Impl IAudibleNotifier_Impl;

    /**
     * @brief VTable de IAudibleNotifier interface
     */
    typedef struct
    {
        /**
         * @brief Notifica al usuario con sonido (asíncrono)
         *
         * @param self Puntero a implementación concreta
         * @param priority Prioridad de la notificación (LOW/MEDIUM/HIGH/CRITICAL)
         * @param duration_ms Duración del sonido en milisegundos (0 = default según prioridad)
         *
         * @return ERR_OK si notificación encolada exitosamente
         * @return ERR_NULL_POINTER si self es NULL
         * @return ERR_TIMEOUT si cola de notificaciones llena (después de timeout)
         * @return ERR_INVALID_STATE si notifier no está inicializado
         *
         * @note NO BLOQUEANTE: Retorna inmediatamente (<50μs)
         * @note Thread-safe: Puede llamarse desde múltiples contextos
         * @note ISR-safe: Puede llamarse desde ISR
         * @note Notificaciones críticas pueden interrumpir notificaciones de menor prioridad
         *
         * @code
         * IAudibleNotifier *notifier = GetSystemNotifier();
         * Result_t res = IAudibleNotifier_NotifyUser(
         *     notifier,
         *     NOTIFICATION_PRIORITY_HIGH,
         *     1000);  // 1 segundo
         * @endcode
         */
        Result_t (*NotifyUser)(IAudibleNotifier_Impl *self, NotificationPriority_t priority, uint16_t duration_ms);

        /**
         * @brief Silencia todas las notificaciones activas (inmediato)
         *
         * @param self Puntero a implementación concreta
         *
         * @return ERR_OK si silenciamiento exitoso
         * @return ERR_NULL_POINTER si self es NULL
         *
         * @note INMEDIATO: Detiene sonido actual y limpia cola de pendientes
         * @note Thread-safe
         * @note ISR-safe
         * @note Útil para botón de "silence alarm" o apagado de emergencia
         */
        Result_t (*SilenceAll)(IAudibleNotifier_Impl *self);

        /**
         * @brief Verifica si notifier está en modo silencioso (mute)
         *
         * @param self Puntero a implementación concreta
         *
         * @return true si modo silencioso activo (notificaciones bloqueadas)
         * @return false si notificaciones habilitadas
         *
         * @note Thread-safe
         * @note ISR-safe
         * @note Modo silencioso NO afecta notificaciones CRITICAL
         */
        bool (*IsMuted)(const IAudibleNotifier_Impl *self);

        /**
         * @brief Establece modo silencioso (opcional)
         *
         * @param self Puntero a implementación concreta
         * @param muted true para activar modo silencioso, false para desactivar
         *
         * @return ERR_OK si cambio exitoso
         * @return ERR_NULL_POINTER si self es NULL
         *
         * @note Thread-safe
         * @note Modo silencioso bloquea LOW/MEDIUM/HIGH, permite CRITICAL
         */
        Result_t (*SetMuted)(IAudibleNotifier_Impl *self, bool muted);

    } IAudibleNotifier_VTable;

    /**
     * @brief Estructura de interfaz IAudibleNotifier
     */
    typedef struct
    {
        const IAudibleNotifier_VTable *vtable; /**< VTable de funciones */
        IAudibleNotifier_Impl *impl;           /**< Puntero a implementación concreta */
    } IAudibleNotifier;

    /*============================================================================*
     * INLINE HELPER FUNCTIONS
     *============================================================================*/

    /**
     * @brief Helper: Notifica al usuario
     * @see IAudibleNotifier_VTable::NotifyUser
     */
    static inline Result_t IAudibleNotifier_NotifyUser(
        IAudibleNotifier *iface,
        NotificationPriority_t priority,
        uint16_t duration_ms)
    {
        if (!iface || !iface->vtable || !iface->vtable->NotifyUser)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->NotifyUser(iface->impl, priority, duration_ms);
    }

    /**
     * @brief Helper: Silencia todas las notificaciones
     * @see IAudibleNotifier_VTable::SilenceAll
     */
    static inline Result_t IAudibleNotifier_SilenceAll(IAudibleNotifier *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->SilenceAll)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SilenceAll(iface->impl);
    }

    /**
     * @brief Helper: Verifica si está en modo silencioso
     * @see IAudibleNotifier_VTable::IsMuted
     */
    static inline bool IAudibleNotifier_IsMuted(const IAudibleNotifier *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->IsMuted)
        {
            return false;
        }
        return iface->vtable->IsMuted(iface->impl);
    }

    /**
     * @brief Helper: Establece modo silencioso
     * @see IAudibleNotifier_VTable::SetMuted
     */
    static inline Result_t IAudibleNotifier_SetMuted(IAudibleNotifier *iface, bool muted)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetMuted)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetMuted(iface->impl, muted);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_AUDIBLE_NOTIFIER_H */
