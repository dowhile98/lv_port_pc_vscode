/**
 * @file buzzer_ao.c
 * @brief Implementación del Buzzer Active Object
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Active Object que gestiona la cola de comandos de buzzer de forma asíncrona.
 * Implementa una máquina de estados simple (IDLE/PLAYING) con thread dedicado.
 * Provee interfaz IAudibleNotifier para domain layer (DIP compliance).
 *
 * **Máquina de Estados:**
 * - IDLE: Esperando comandos en cola (bloqueante)
 * - PLAYING: Reproduciendo beep, esperando timeout o nuevo comando
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "application/activeobjects/buzzer_ao.h"
#include "infrastructure/adapters/buzzer_adapter.h"
#include "infrastructure/osal/osal.h"
#include "domain/interfaces/i_audible_notifier.h"
#include "hal/hal_types.h"
#include <string.h>

/*============================================================================*
 * PRIVATE TYPES
 *============================================================================*/

/**
 * @brief Estructura completa del BuzzerAO (privada)
 */
/* struct BuzzerAO: Definición completa movida a buzzer_ao.h */

/*============================================================================*
 * FORWARD DECLARATIONS
 *============================================================================*/
static void BuzzerAO_ThreadEntry(void *param);
static void BuzzerAO_ProcessCommand(BuzzerAO_t *ao, const BuzzerCommand_t *cmd);

/*============================================================================*
 * PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Thread entry point del Active Object
 */
static void BuzzerAO_ThreadEntry(void *param)
{
    BuzzerAO_t *ao = (BuzzerAO_t *)param;

    while (!ao->terminate)
    {
        os_semaphore_get(ao->run_sem, OS_WAIT_FOREVER);
        if (ao->terminate)
        {
            break;
        }

        if (Logger_IsValid(ao->logger))
        {
            LOG_INFO(ao->logger, "BuzzerAO", "Thread started");
        }

        BuzzerCommand_t cmd;
        Result_t res;

        while (ao->is_running)
        {
            /* Actualizar buzzer control para auto-stop (polimórfico) */
            BuzzerControl_Update(ao->buzzer_control);

            switch (ao->state)
            {
            case BUZZER_AO_STATE_IDLE:
                res = os_queue_receive(ao->queue, &cmd, OS_WAIT_FOREVER);
                if (res == ERR_OK)
                {
                    BuzzerAO_ProcessCommand(ao, &cmd);
                }
                break;

            case BUZZER_AO_STATE_PLAYING:
            {
                uint32_t timeout_ms = 20;
                res = os_queue_receive(ao->queue, &cmd, timeout_ms);
                if (res == ERR_OK)
                {
                    if (cmd.tone == BUZZER_TONE_SILENT)
                    {
                        BuzzerControl_Stop(ao->buzzer_control);
                        ao->state = BUZZER_AO_STATE_IDLE;
                    }
                    else
                    {
                        BuzzerAO_ProcessCommand(ao, &cmd);
                    }
                }
                else if (res == ERR_TIMEOUT)
                {
                    if (!BuzzerControl_IsActive(ao->buzzer_control))
                    {
                        ao->state = BUZZER_AO_STATE_IDLE;
                    }
                }
                break;
            }
            default:
                ao->state = BUZZER_AO_STATE_IDLE;
                break;
            }
        }

        /* Asegurar buzzer apagado al salir del inner loop */
        BuzzerControl_Stop(ao->buzzer_control);
        if (Logger_IsValid(ao->logger))
        {
            LOG_INFO(ao->logger, "BuzzerAO", "Thread stopped");
        }
        (void)os_semaphore_put(ao->stopped_sem); /* ← WaitStopped() */
    }
    (void)os_semaphore_put(ao->stopped_sem); /* ← Deinit() */
}

/**
 * @brief Procesa un comando de beep
 */
static void BuzzerAO_ProcessCommand(BuzzerAO_t *ao, const BuzzerCommand_t *cmd)
{
    if (!ao || !cmd)
    {
        return;
    }

    /* Guardar comando actual */
    ao->current_command = *cmd;

    /* Ejecutar beep */
    Result_t res = BuzzerControl_Beep(ao->buzzer_control, cmd->tone, cmd->duration_ms);

    if (res == ERR_OK)
    {
        /* Si el beep es instantáneo o muy corto, no cambiar a PLAYING */
        if (cmd->duration_ms > 0)
        {
            ao->state = BUZZER_AO_STATE_PLAYING;
        }
        else
        {
            /* Beep continuo (duration=0) -> mantener en IDLE y controlar externamente */
            ao->state = BUZZER_AO_STATE_IDLE;
        }
    }
    else
    {
        /* Error al ejecutar beep, volver a IDLE */
        ao->state = BUZZER_AO_STATE_IDLE;
    }
}

/*============================================================================*
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

Result_t BuzzerAO_Init(BuzzerAO_t *ao, const BuzzerAO_Config_t *config)
{
    if (!ao || !config)
    {
        return ERR_NULL_POINTER;
    }

    if (!config->buzzer_control)
    {
        return ERR_INVALID_PARAM;
    }

    /* Limpiar estructura */
    memset(ao, 0, sizeof(BuzzerAO_t));

    ao->buzzer_control = config->buzzer_control;
    ao->thread_priority = config->thread_priority;
    ao->queue_size = config->queue_size;
    ao->state = BUZZER_AO_STATE_IDLE;
    ao->is_running = false;
    ao->terminate = false;
    ao->logger = config->logger;

    /* stopped_sem: binary, init=0 */
    if (os_semaphore_create(&ao->stopped_sem, "BUZZ_STOP_SEM", 0U) != ERR_OK)
        return ERR_ERROR;

    /* run_sem: binary, init=0 — gate para el outer loop */
    if (os_semaphore_create(&ao->run_sem, "BUZZ_RUN_SEM", 0U) != ERR_OK)
    {
        os_semaphore_delete(ao->stopped_sem);
        return ERR_ERROR;
    }

    /* Queue con buffer estático (NO os_alloc) */
    ao->queue = NULL;
    os_queue_config_t queue_cfg = {
        .name = "BuzzerQueue",
        .buffer = ao->queue_storage,
        .buffer_size = sizeof(ao->queue_storage),
        .item_size = sizeof(BuzzerCommand_t)};
    if (os_queue_create(&ao->queue, &queue_cfg) != ERR_OK)
    {
        os_semaphore_delete(ao->run_sem);
        os_semaphore_delete(ao->stopped_sem);
        return ERR_ERROR;
    }

    /* Thread con stack estático (NO os_alloc) — queda bloqueado en run_sem hasta Start() */
    ao->thread = NULL;
    os_thread_config_t thread_cfg = {
        .name = "BuzzerAO",
        .entry = BuzzerAO_ThreadEntry,
        .arg = ao,
        .stack_ptr = ao->stack,
        .stack_size = BUZZER_AO_THREAD_STACK_SIZE,
        .priority = ao->thread_priority,
        .auto_start = true};
    if (os_thread_create(&ao->thread, &thread_cfg) != ERR_OK)
    {
        os_queue_delete(ao->queue);
        os_semaphore_delete(ao->run_sem);
        os_semaphore_delete(ao->stopped_sem);
        return ERR_ERROR;
    }

    ao->is_initialized = true;

    return ERR_OK;
}

Result_t BuzzerAO_Start(BuzzerAO_t *ao)
{
    if (!ao || !ao->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    if (ao->is_running)
    {
        return ERR_OK; // Ya está corriendo
    }

    ao->is_running = true;
    (void)os_semaphore_put(ao->run_sem);

    return ERR_OK;
}

Result_t BuzzerAO_Stop(BuzzerAO_t *ao)
{
    if (!ao || !ao->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    if (!ao->is_running)
    {
        return ERR_OK; // Ya está detenido
    }

    ao->is_running = false;

    /* Postear SILENT para desbloquear el os_queue_receive del inner loop */
    BuzzerCommand_t stop_cmd = {
        .tone = BUZZER_TONE_SILENT,
        .duration_ms = 0,
        .priority = 255};
    (void)os_queue_send(ao->queue, &stop_cmd, OS_NO_WAIT);

    return ERR_OK;
}

Result_t BuzzerAO_WaitStopped(BuzzerAO_t *ao, uint32_t timeout_ms)
{
    if (!ao)
        return ERR_NULL_POINTER;
    return os_semaphore_get(ao->stopped_sem, timeout_ms);
}

Result_t BuzzerAO_PostCommand(BuzzerAO_t *ao, const BuzzerCommand_t *command)
{
    if (!ao || !command)
    {
        return ERR_NULL_POINTER;
    }

    if (!ao->is_initialized || !ao->is_running)
    {
        return ERR_INVALID_STATE;
    }

    /* Enviar comando a la cola con timeout */
    Result_t res = os_queue_send(ao->queue, command, BUZZER_AO_POST_TIMEOUT_MS);

    return res;
}

Result_t BuzzerAO_PostStop(BuzzerAO_t *ao)
{
    if (!ao)
    {
        return ERR_NULL_POINTER;
    }

    BuzzerCommand_t stop_cmd = {
        .tone = BUZZER_TONE_SILENT,
        .duration_ms = 0,
        .priority = 255 // Máxima prioridad
    };

    return BuzzerAO_PostCommand(ao, &stop_cmd);
}

BuzzerAO_State_t BuzzerAO_GetState(const BuzzerAO_t *ao)
{
    if (!ao || !ao->is_initialized)
    {
        return BUZZER_AO_STATE_IDLE;
    }

    return ao->state;
}

bool BuzzerAO_IsPlaying(const BuzzerAO_t *ao)
{
    return (BuzzerAO_GetState(ao) == BUZZER_AO_STATE_PLAYING);
}

uint32_t BuzzerAO_GetQueueDepth(const BuzzerAO_t *ao)
{
    if (!ao || !ao->is_initialized)
    {
        return 0;
    }

    /* OSAL no expone queue depth, retornar valor conservador */
    return 0;
}

Result_t BuzzerAO_Deinit(BuzzerAO_t *ao)
{
    if (!ao || !ao->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Detener thread si está corriendo */
    if (ao->is_running)
    {
        BuzzerAO_Stop(ao);
        (void)BuzzerAO_WaitStopped(ao, 1000U);
    }

    /* Despertar el outer loop con la señal de terminación */
    ao->terminate = true;
    (void)os_semaphore_put(ao->run_sem);
    (void)os_semaphore_get(ao->stopped_sem, OS_WAIT_FOREVER);

    /* Liberar recursos OSAL */
    os_queue_delete(ao->queue);
    os_semaphore_delete(ao->run_sem);
    os_semaphore_delete(ao->stopped_sem);

    ao->is_initialized = false;

    return ERR_OK;
}

/*============================================================================*
 * IAudibleNotifier IMPLEMENTATION
 *============================================================================*/

/**
 * @brief Mapea NotificationPriority_t → BuzzerCommand (duration + priority)
 *
 * @details
 * Tabla de mapeo semántico → técnico:
 * - LOW (UI): 50ms, prioridad 3, TONE_STANDARD
 * - MEDIUM (Confirm): 200ms, prioridad 5, TONE_STANDARD
 * - HIGH (Alarm): 1000ms, prioridad 8, TONE_WARNING
 * - CRITICAL (Fault): 2000ms, prioridad 10, TONE_ERROR
 */
static void MapNotificationPriorityToCommand(
    NotificationPriority_t priority,
    uint16_t user_duration_ms,
    BuzzerCommand_t *out_cmd)
{
    /* Defaults por prioridad */
    switch (priority)
    {
    case NOTIFICATION_PRIORITY_LOW:
        out_cmd->tone = BUZZER_TONE_STANDARD;
        out_cmd->duration_ms = (user_duration_ms > 0) ? user_duration_ms : 50;
        out_cmd->priority = 3;
        break;

    case NOTIFICATION_PRIORITY_MEDIUM:
        out_cmd->tone = BUZZER_TONE_STANDARD;
        out_cmd->duration_ms = (user_duration_ms > 0) ? user_duration_ms : 200;
        out_cmd->priority = 5;
        break;

    case NOTIFICATION_PRIORITY_HIGH:
        out_cmd->tone = BUZZER_TONE_WARNING;
        out_cmd->duration_ms = (user_duration_ms > 0) ? user_duration_ms : 1000;
        out_cmd->priority = 8;
        break;

    case NOTIFICATION_PRIORITY_CRITICAL:
        out_cmd->tone = BUZZER_TONE_ERROR;
        out_cmd->duration_ms = (user_duration_ms > 0) ? user_duration_ms : 2000;
        out_cmd->priority = 10;
        break;

    default:
        /* Fallback: low priority */
        out_cmd->tone = BUZZER_TONE_STANDARD;
        out_cmd->duration_ms = 50;
        out_cmd->priority = 3;
        break;
    }
}

/**
 * @brief Implementación de IAudibleNotifier::NotifyUser
 */
static Result_t BuzzerAO_NotifyUser_Impl(
    IAudibleNotifier_Impl *self,
    NotificationPriority_t priority,
    uint16_t duration_ms)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    BuzzerAO_t *ao = (BuzzerAO_t *)self;

    /* Mapear prioridad semántica → comando técnico */
    BuzzerCommand_t cmd;
    MapNotificationPriorityToCommand(priority, duration_ms, &cmd);

    /* Postear comando al Active Object (asíncrono) */
    return BuzzerAO_PostCommand(ao, &cmd);
}

/**
 * @brief Implementación de IAudibleNotifier::SilenceAll
 */
static Result_t BuzzerAO_SilenceAll_Impl(IAudibleNotifier_Impl *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    BuzzerAO_t *ao = (BuzzerAO_t *)self;
    return BuzzerAO_PostStop(ao);
}

/**
 * @brief Implementación de IAudibleNotifier::IsMuted
 */
static bool BuzzerAO_IsMuted_Impl(const IAudibleNotifier_Impl *self)
{
    if (!self)
    {
        return false;
    }

    BuzzerAO_t *ao = (BuzzerAO_t *)self;

    /* Delegar a IBuzzerControl (adapter) */
    if (ao->buzzer_control)
    {
        return !BuzzerControl_IsEnabled(ao->buzzer_control);
    }

    return false;
}

/**
 * @brief Implementación de IAudibleNotifier::SetMuted
 */
static Result_t BuzzerAO_SetMuted_Impl(IAudibleNotifier_Impl *self, bool muted)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    BuzzerAO_t *ao = (BuzzerAO_t *)self;

    /* Delegar a IBuzzerControl (adapter) */
    if (ao->buzzer_control)
    {
        return BuzzerControl_SetEnabled(ao->buzzer_control, !muted);
    }

    return ERR_ERROR;
}

/* VTable estática de IAudibleNotifier */
static const IAudibleNotifier_VTable s_audible_notifier_vtable = {
    .NotifyUser = BuzzerAO_NotifyUser_Impl,
    .SilenceAll = BuzzerAO_SilenceAll_Impl,
    .IsMuted = BuzzerAO_IsMuted_Impl,
    .SetMuted = BuzzerAO_SetMuted_Impl};

/* Instancia estática de interfaz (populated en GetInterface) */
static IAudibleNotifier s_audible_notifier_iface = {
    .vtable = &s_audible_notifier_vtable,
    .impl = NULL /* Se asigna en GetInterface */
};

/**
 * @brief Obtiene la interfaz IAudibleNotifier del BuzzerAO
 */
IAudibleNotifier *BuzzerAO_GetAudibleNotifierInterface(BuzzerAO_t *ao)
{
    if (!ao)
    {
        return NULL;
    }

    /* Asignar implementación concreta */
    s_audible_notifier_iface.impl = (IAudibleNotifier_Impl *)ao;

    return &s_audible_notifier_iface;
}
