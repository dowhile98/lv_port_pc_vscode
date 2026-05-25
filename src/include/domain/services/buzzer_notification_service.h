/**
 * @file buzzer_notification_service.h
 * @brief Buzzer Notification Service - Lógica de dominio para notificaciones sonoras
 * @version 1.0.0
 * @date 2026-02-03
 *
 * @details
 * Servicio de dominio que encapsula las reglas de negocio para notificaciones
 * sonoras del sistema. Traduce eventos del dominio (alarmas, confirmaciones, etc.)
 * en comandos específicos de buzzer según contexto y configuración.
 *
 * **Responsabilidades (SRP):**
 * - Determinar CUÁNDO y CÓMO sonar el buzzer según eventos del sistema
 * - Aplicar reglas de negocio (ej: no sonar si modo silencioso activo)
 * - Mapear eventos semánticos a patrones de beep concretos
 * - Consultar configuración de usuario (beep_duration, beep_enabled)
 *
 * **Eventos Manejados:**
 * - Presión de botón (feedback táctil)
 * - Alarma de temperatura alta
 * - Pérdida de señal GPS
 * - Fallo de sincronización PPS
 * - Confirmación de guardado de configuración
 * - Secuencias de boot/shutdown
 *
 * **Patrones:**
 * - Strategy Pattern: permite cambiar políticas de notificación
 * - Observer Pattern: puede suscribirse a eventos del sistema
 * - Dependency Injection: recibe IBuzzerControl en Init
 *
 * **Ejemplo de Regla de Negocio:**
 * ```c
 * // Si modo silencioso está activo, solo suenan alarmas críticas
 * if (config->silent_mode && !is_critical_alarm) {
 *     return ERR_OK;  // Ignorar beep
 * }
 * ```
 *
 * @note Este servicio NO maneja hardware directamente
 * @note Todas las operaciones son síncronas (no bloqueantes si se usa con BuzzerAO)
 * @note Thread-safe si IBuzzerControl subyacente es thread-safe
 *
 * @see BuzzerAO
 * @see IBuzzerControl
 */

#ifndef BUZZER_NOTIFICATION_SERVICE_H
#define BUZZER_NOTIFICATION_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "domain/interfaces/i_audible_notifier.h"
#include "hal_types.h" /* ✅ os_mutex_t ahora en hal_types.h (v2.0) */

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Tipos de eventos del sistema que generan notificaciones sonoras
     */
    typedef enum
    {
        BUZZER_EVENT_BUTTON_PRESS = 0,  /**< Feedback de presión de botón */
        BUZZER_EVENT_BUTTON_LONG_PRESS, /**< Feedback de long-press */
        BUZZER_EVENT_CONFIG_SAVED,      /**< Confirmación de guardado */
        BUZZER_EVENT_TEMP_ALARM_HIGH,   /**< Alarma: temperatura alta */
        BUZZER_EVENT_GPS_LOST,          /**< Alarma: pérdida de señal GPS */
        BUZZER_EVENT_PPS_SYNC_FAIL,     /**< Alarma: fallo sincronización PPS */
        BUZZER_EVENT_POWER_FAIL,        /**< Alarma crítica: fallo de alimentación */
        BUZZER_EVENT_BOOT_COMPLETE,     /**< Sistema iniciado correctamente */
        BUZZER_EVENT_SHUTDOWN_START,    /**< Inicio de apagado */
        BUZZER_EVENT_ERROR_GENERIC,     /**< Error genérico */
		BUZZER_EVENT_ACCESSDENIED,	    /**< Acceso denegado (ej: intento de configuración no autorizada) */
    } BuzzerEventType_t;

    /**
     * @brief Configuración del servicio de notificaciones
     */
    typedef struct
    {
        bool beep_enabled;                /**< Buzzer habilitado globalmente */
        uint16_t button_beep_duration_ms; /**< Duración beep de botones (ms) */
        uint16_t alarm_beep_duration_ms;  /**< Duración beep de alarmas (ms) */
        bool silent_mode;                 /**< Modo silencioso (solo alarmas críticas) */
        bool boot_beep_enabled;           /**< Beep de boot habilitado */
    } BuzzerNotificationConfig_t;

    /**
     * @brief Estructura del servicio de notificaciones (definición completa)
     * @version 2.1.0
     * @note Movida a header para permitir embedding en DependencyContainer
     *
     * **Thread-safety (sin mutex):**
     * Este servicio NO requiere mutex porque:
     * - Config fields (bool, uint16_t) son atómicos en ARM Cortex-M (single instruction)
     * - IAudibleNotifier_NotifyUser() es thread-safe (queue-based con ThreadX)
     * - Operaciones read-heavy: 99% lecturas, 1% escrituras (desde UI)
     * - No hay critical sections read-modify-write que requieran atomicidad
     * - volatile garantiza visibilidad entre cores/threads
     *
     * **Acceso concurrente seguro:**
     * - Múltiples threads leyendo config: ✅ Safe (atomic reads)
     * - Thread leyendo mientras UI escribe: ✅ Safe (peor caso: lee valor viejo 1 ciclo)
     * - Múltiples threads escribiendo: ✅ Safe (escriben campos diferentes)
     *
     * @note Si en futuro se agregan operaciones complejas, re-evaluar necesidad de mutex
     */
    typedef struct BuzzerNotificationService
    {
        /* Dependencias */
        IAudibleNotifier *audible_notifier;

        /* Configuración (atomic access, volatile para visibilidad entre threads) */
        volatile BuzzerNotificationConfig_t config;

        /* Estado */
        bool is_initialized;
    } BuzzerNotificationService_t;

    /*============================================================================*
     * PUBLIC API
     *============================================================================*/

    /**
     * @brief Inicializa el servicio de notificaciones de buzzer
     *
     * @param[in] service Puntero a estructura de servicio (pre-alocada)
     * @param[in] audible_notifier Interfaz de notificaciones audibles (inyección de dependencia)
     * @param[in] config Configuración inicial (puede ser NULL para defaults)
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si service o audible_notifier son NULL
     *
     * @note Si config es NULL, usa configuración por defecto
     * @note Thread-safe: puede llamarse una sola vez
     *
     * @code
     * BuzzerNotificationService_t beep_service;
     * BuzzerNotificationConfig_t config = {
     *     .beep_enabled = true,
     *     .button_beep_duration_ms = 50,
     *     .alarm_beep_duration_ms = 1000,
     *     .silent_mode = false,
     *     .boot_beep_enabled = true
     * };
     * Result_t res = BuzzerNotificationService_Init(
     *     &beep_service,
     *     audible_notifier,
     *     &config
     * );
     * @endcode
     */
    Result_t BuzzerNotificationService_Init(
        BuzzerNotificationService_t *service,
        IAudibleNotifier *audible_notifier,
        const BuzzerNotificationConfig_t *config);

    /**
     * @brief Notifica un evento del sistema que puede generar beep
     *
     * @param[in] service Puntero al servicio
     * @param[in] event_type Tipo de evento a notificar
     *
     * @return ERR_OK si beep procesado (incluso si fue silenciado)
     * @return ERR_NULL_POINTER si service es NULL
     * @return ERR_INVALID_PARAM si event_type es inválido
     *
     * @note Aplica lógica de negocio interna (silent_mode, beep_enabled, etc.)
     * @note NO bloqueante si se usa con BuzzerAO
     * @note Thread-safe
     *
     * @code
     * // Desde handler de botón
     * BuzzerNotificationService_NotifyEvent(&beep_service, BUZZER_EVENT_BUTTON_PRESS);
     *
     * // Desde monitor de temperatura
     * if (temp > TEMP_THRESHOLD) {
     *     BuzzerNotificationService_NotifyEvent(&beep_service, BUZZER_EVENT_TEMP_ALARM_HIGH);
     * }
     * @endcode
     */
    Result_t BuzzerNotificationService_NotifyEvent(
        BuzzerNotificationService_t *service,
        BuzzerEventType_t event_type);

    /**
     * @brief Actualiza configuración del servicio en tiempo de ejecución
     *
     * @param[in] service Puntero al servicio
     * @param[in] config Nueva configuración (no NULL)
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si service o config son NULL
     *
     * @note Permite cambiar parámetros sin reiniciar el servicio
     * @note Útil para aplicar cambios desde UI de configuración
     * @note Thread-safe: protegido por mutex interno
     */
    Result_t BuzzerNotificationService_UpdateConfig(
        BuzzerNotificationService_t *service,
        const BuzzerNotificationConfig_t *config);

    /**
     * @brief Obtiene configuración actual del servicio
     *
     * @param[in] service Puntero al servicio
     * @param[out] config Buffer para almacenar configuración (no NULL)
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si service o config son NULL
     *
     * @note Thread-safe: lectura protegida
     */
    Result_t BuzzerNotificationService_GetConfig(
        const BuzzerNotificationService_t *service,
        BuzzerNotificationConfig_t *config);

    /**
     * @brief Habilita/deshabilita el buzzer globalmente (mute)
     *
     * @param[in] service Puntero al servicio
     * @param[in] enabled true para habilitar, false para deshabilitar
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si service es NULL
     *
     * @note Shortcut para UpdateConfig cambiando solo beep_enabled
     * @note Útil para toggle rápido desde UI
     */
    Result_t BuzzerNotificationService_SetEnabled(
        BuzzerNotificationService_t *service,
        bool enabled);

    /**
     * @brief Activa/desactiva modo silencioso
     *
     * @param[in] service Puntero al servicio
     * @param[in] silent true para activar modo silencioso
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si service es NULL
     *
     * @note En modo silencioso, solo suenan alarmas críticas:
     *       - BUZZER_EVENT_POWER_FAIL
     *       - BUZZER_EVENT_TEMP_ALARM_HIGH
     */
    Result_t BuzzerNotificationService_SetSilentMode(
        BuzzerNotificationService_t *service,
        bool silent);

    /**
     * @brief Verifica si buzzer está habilitado
     *
     * @param[in] service Puntero al servicio
     * @return true si habilitado, false si disabled/mute
     */
    bool BuzzerNotificationService_IsEnabled(const BuzzerNotificationService_t *service);

    /**
     * @brief Verifica si modo silencioso está activo
     *
     * @param[in] service Puntero al servicio
     * @return true si modo silencioso activo
     */
    bool BuzzerNotificationService_IsSilentMode(const BuzzerNotificationService_t *service);

    /**
     * @brief De-inicializa el servicio
     *
     * @param[in] service Puntero al servicio
     * @return ERR_OK si exitoso
     *
     * @note Detiene cualquier beep en progreso
     */
    Result_t BuzzerNotificationService_Deinit(BuzzerNotificationService_t *service);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_NOTIFICATION_SERVICE_H */
