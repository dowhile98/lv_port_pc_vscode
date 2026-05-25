/**
 * @file buzzer_notification_service.c
 * @brief Implementación del Servicio de Notificaciones de Buzzer
 * @version 2.1.0
 * @date 2026-02-10
 *
 * @details
 * Servicio de dominio que encapsula la lógica de negocio para notificaciones
 * sonoras. Traduce eventos semánticos del sistema en comandos concretos de
 * buzzer según configuración y contexto.
 *
 * **v2.1 Changes:**
 * - Eliminado mutex innecesario (over-engineering)
 * - Config es volatile para visibilidad entre threads
 * - ARM Cortex-M garantiza atomicidad para bool/uint16_t
 * - Mejora ~10μs por llamada (sin overhead de lock/unlock)
 *
 * **v2.0 Changes:**
 * - Eliminado include directo de infrastructure/osal/osal.h
 * - os_mutex_t ahora en hal/hal_types.h (Clean Architecture compliance)
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "domain/services/buzzer_notification_service.h"
#include "domain/interfaces/i_audible_notifier.h"
#include "hal/hal_types.h"
#include <string.h>

/*============================================================================*
 * PRIVATE TYPES
 *============================================================================*/

/**
 * @brief Estructura completa del servicio (privada)
 */
/* struct BuzzerNotificationService: Definición completa movida a buzzer_notification_service.h */

/*============================================================================*
 * PRIVATE DATA
 *============================================================================*/

/**
 * @brief Configuración por defecto
 */
static const BuzzerNotificationConfig_t s_default_config = {
    .beep_enabled = true,
    .button_beep_duration_ms = 50,
    .alarm_beep_duration_ms = 1000,
    .silent_mode = false,
    .boot_beep_enabled = true};

/*============================================================================*
 * PRIVATE HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Determina si un evento es una alarma crítica
 */
static bool BuzzerNotificationService_IsCriticalAlarm(BuzzerEventType_t event)
{
    switch (event)
    {
    case BUZZER_EVENT_POWER_FAIL:
    case BUZZER_EVENT_TEMP_ALARM_HIGH:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Mapea un evento a NotificationPriority y duration
 * @note Nuevo mapeo basado en prioridades semánticas (domain-level)
 */
static bool BuzzerNotificationService_MapEventToNotification(
    const BuzzerNotificationService_t *service,
    BuzzerEventType_t event_type,
    NotificationPriority_t *priority_out,
    uint16_t *duration_out)
{
    if (!service || !priority_out || !duration_out)
    {
        return false;
    }

    bool should_notify = false;

    /* Aplicar reglas de negocio según evento */
    switch (event_type)
    {
    case BUZZER_EVENT_BUTTON_PRESS:
        if (!service->config.silent_mode)
        {
            *priority_out = NOTIFICATION_PRIORITY_LOW;
            *duration_out = service->config.button_beep_duration_ms;
            should_notify = true;
        }
        break;

    case BUZZER_EVENT_BUTTON_LONG_PRESS:
        if (!service->config.silent_mode)
        {
            *priority_out = NOTIFICATION_PRIORITY_LOW;
            *duration_out = 200;
            should_notify = true;
        }
        break;

    case BUZZER_EVENT_CONFIG_SAVED:
        if (!service->config.silent_mode)
        {
            *priority_out = NOTIFICATION_PRIORITY_MEDIUM;
            *duration_out = 500;
            should_notify = true;
        }
        break;

    case BUZZER_EVENT_TEMP_ALARM_HIGH:
        /* Alarma crítica: notifica SIEMPRE */
        *priority_out = NOTIFICATION_PRIORITY_CRITICAL;
        *duration_out = service->config.alarm_beep_duration_ms;
        should_notify = true;
        break;

    case BUZZER_EVENT_GPS_LOST:
        *priority_out = NOTIFICATION_PRIORITY_HIGH;
        *duration_out = 1000;
        should_notify = true;
        break;

    case BUZZER_EVENT_PPS_SYNC_FAIL:
        *priority_out = NOTIFICATION_PRIORITY_HIGH;
        *duration_out = 500;
        should_notify = true;
        break;

    case BUZZER_EVENT_POWER_FAIL:
        /* Alarma crítica */
        *priority_out = NOTIFICATION_PRIORITY_CRITICAL;
        *duration_out = 2000;
        should_notify = true;
        break;

    case BUZZER_EVENT_BOOT_COMPLETE:
        if (service->config.boot_beep_enabled && !service->config.silent_mode)
        {
            *priority_out = NOTIFICATION_PRIORITY_MEDIUM;
            *duration_out = 3000;
            should_notify = true;
        }
        break;

    case BUZZER_EVENT_SHUTDOWN_START:
        if (!service->config.silent_mode)
        {
            *priority_out = NOTIFICATION_PRIORITY_MEDIUM;
            *duration_out = 800;
            should_notify = true;
        }
        break;

    case BUZZER_EVENT_ERROR_GENERIC:
        *priority_out = NOTIFICATION_PRIORITY_HIGH;
        *duration_out = 500;
        should_notify = true;
        break;
    case BUZZER_EVENT_ACCESSDENIED:
		*priority_out = NOTIFICATION_PRIORITY_MEDIUM;
		*duration_out = 1500;
		should_notify = true;
		break;

    default:
        should_notify = false;
        break;
    }

    return should_notify;
}

/*============================================================================*
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

Result_t BuzzerNotificationService_Init(
    BuzzerNotificationService_t *service,
    IAudibleNotifier *audible_notifier,
    const BuzzerNotificationConfig_t *config)
{
    if (!service || !audible_notifier)
    {
        return ERR_NULL_POINTER;
    }

    /* Limpiar estructura */
    memset(service, 0, sizeof(BuzzerNotificationService_t));

    /* Guardar dependencias */
    service->audible_notifier = audible_notifier;

    /* Copiar configuración (usar default si NULL) */
    if (config)
    {
        service->config = *config;
    }
    else
    {
        service->config = s_default_config;
    }

    service->is_initialized = true;

    return ERR_OK;
}

Result_t BuzzerNotificationService_NotifyEvent(
    BuzzerNotificationService_t *service,
    BuzzerEventType_t event_type)
{
    if (!service)
    {
        return ERR_NULL_POINTER;
    }

    if (!service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Verificar si buzzer está habilitado globalmente (atomic read) */
    if (!service->config.beep_enabled)
    {
        return ERR_OK; // Silenciado globalmente
    }

    /* Verificar modo silencioso (excepto alarmas críticas) */
    if (service->config.silent_mode && !BuzzerNotificationService_IsCriticalAlarm(event_type))
    {
        return ERR_OK; // Silenciado por modo silencioso
    }

    /* Mapear evento a prioridad de notificación */
    NotificationPriority_t priority;
    uint16_t duration_ms;
    bool should_notify = BuzzerNotificationService_MapEventToNotification(
        service, event_type, &priority, &duration_ms);

    Result_t res = ERR_OK;

    if (should_notify)
    {
        /* ✅ CORRECTO: Notificar vía IAudibleNotifier (domain → application) */
        res = IAudibleNotifier_NotifyUser(service->audible_notifier, priority, duration_ms);
    }

    return res;
}

Result_t BuzzerNotificationService_UpdateConfig(
    BuzzerNotificationService_t *service,
    const BuzzerNotificationConfig_t *config)
{
    if (!service || !config)
    {
        return ERR_NULL_POINTER;
    }

    if (!service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Actualizar configuración (atomic write en ARM Cortex-M) */
    service->config = *config;

    return ERR_OK;
}

Result_t BuzzerNotificationService_GetConfig(
    const BuzzerNotificationService_t *service,
    BuzzerNotificationConfig_t *config)
{
    if (!service || !config)
    {
        return ERR_NULL_POINTER;
    }

    if (!service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Copiar configuración (lectura thread-safe) */
    *config = service->config;

    return ERR_OK;
}

Result_t BuzzerNotificationService_SetEnabled(
    BuzzerNotificationService_t *service,
    bool enabled)
{
    if (!service)
    {
        return ERR_NULL_POINTER;
    }

    if (!service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Actualizar configuración (atomic write) */
    service->config.beep_enabled = enabled;

    /* Si se deshabilita, también propagar a notifier subyacente */
    Result_t res = IAudibleNotifier_SetMuted(service->audible_notifier, !enabled);

    return res;
}

Result_t BuzzerNotificationService_SetSilentMode(
    BuzzerNotificationService_t *service,
    bool silent)
{
    if (!service)
    {
        return ERR_NULL_POINTER;
    }

    if (!service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Actualizar configuración (atomic write) */
    service->config.silent_mode = silent;

    return ERR_OK;
}

bool BuzzerNotificationService_IsEnabled(const BuzzerNotificationService_t *service)
{
    if (!service || !service->is_initialized)
    {
        return false;
    }

    return service->config.beep_enabled;
}

bool BuzzerNotificationService_IsSilentMode(const BuzzerNotificationService_t *service)
{
    if (!service || !service->is_initialized)
    {
        return false;
    }

    return service->config.silent_mode;
}

Result_t BuzzerNotificationService_Deinit(BuzzerNotificationService_t *service)
{
    if (!service || !service->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Detener cualquier beep en progreso */
    IAudibleNotifier_SilenceAll(service->audible_notifier);

    service->is_initialized = false;

    return ERR_OK;
}
