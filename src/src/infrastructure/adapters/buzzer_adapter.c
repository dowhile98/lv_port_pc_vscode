/**
 * @file buzzer_adapter.c
 * @brief Implementación del BuzzerAdapter sobre GPIO
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Implementación concreta de IBuzzerControl usando:
 * - I_GPIO para control on/off del buzzer
 * - OSAL mutex para thread-safety
 * - Polling para auto-stop (sin timer RTOS)
 *
 * **Thread-Safety:**
 * Todas las operaciones están protegidas por un mutex OSAL.
 *
 * **Auto-Stop por Polling:**
 * Cuando se llama Beep() con duration_ms > 0, se calcula un tick objetivo.
 * BuzzerAdapter_Update() debe llamarse periódicamente (cada ~20ms) para
 * verificar si se alcanzó el tick y detener el buzzer automáticamente.
 *
 * **Nota:** BuzzerAO llama Update() automáticamente en su loop interno.
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "infrastructure/adapters/buzzer_adapter.h"
#include "infrastructure/osal/osal.h"
#include "hal_types.h"
#include <string.h>

/*============================================================================*
 * PRIVATE TYPES
 *============================================================================*/

/**
 * @brief Estructura completa del BuzzerAdapter (privada)
 */
/* struct BuzzerAdapter: Definición completa movida a buzzer_adapter.h */

/*============================================================================*
 * FORWARD DECLARATIONS
 *============================================================================*/
static Result_t BuzzerAdapter_Beep_Impl(IBuzzerControl_Impl *self, BuzzerTone_t tone, uint16_t duration_ms);
static Result_t BuzzerAdapter_Stop_Impl(IBuzzerControl_Impl *self);
static bool BuzzerAdapter_IsActive_Impl(const IBuzzerControl_Impl *self);
static Result_t BuzzerAdapter_SetEnabled_Impl(IBuzzerControl_Impl *self, bool enabled);
static bool BuzzerAdapter_IsEnabled_Impl(const IBuzzerControl_Impl *self);
static Result_t BuzzerAdapter_Update_Impl(IBuzzerControl_Impl *self);

/*============================================================================*
 * PRIVATE DATA
 *============================================================================*/

/**
 * @brief VTable del BuzzerAdapter (única instancia estática)
 */
static const IBuzzerControl_VTable s_buzzer_adapter_vtable = {
    .Beep = BuzzerAdapter_Beep_Impl,
    .Stop = BuzzerAdapter_Stop_Impl,
    .IsActive = BuzzerAdapter_IsActive_Impl,
    .SetEnabled = BuzzerAdapter_SetEnabled_Impl,
    .IsEnabled = BuzzerAdapter_IsEnabled_Impl,
    .Update = BuzzerAdapter_Update_Impl};

/*============================================================================*
 * PRIVATE HELPER FUNCTIONS
 *============================================================================*/

/**
 * @brief Activa el GPIO del buzzer según polaridad configurada
 */
static Result_t BuzzerAdapter_ActivateGPIO(BuzzerAdapter_t *adapter)
{
    GPIO_State_t active_state = adapter->active_high ? I_GPIO_STATE_HIGH : I_GPIO_STATE_LOW;
    return GPIO_WritePin((I_GPIO *)adapter->gpio_iface, adapter->gpio_port, adapter->gpio_pin, active_state);
}

/**
 * @brief Desactiva el GPIO del buzzer según polaridad configurada
 */
static Result_t BuzzerAdapter_DeactivateGPIO(BuzzerAdapter_t *adapter)
{
    GPIO_State_t inactive_state = adapter->active_high ? I_GPIO_STATE_LOW : I_GPIO_STATE_HIGH;
    return GPIO_WritePin((I_GPIO *)adapter->gpio_iface, adapter->gpio_port, adapter->gpio_pin, inactive_state);
}

/*============================================================================*
 * VTABLE IMPLEMENTATIONS
 *============================================================================*/

static Result_t BuzzerAdapter_Beep_Impl(IBuzzerControl_Impl *self, BuzzerTone_t tone, uint16_t duration_ms)
{
    BuzzerAdapter_t *adapter = (BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Validar parámetros */
    if (tone > BUZZER_TONE_SUCCESS)
    {
        return ERR_INVALID_PARAM;
    }

    /* Si está deshabilitado globalmente, retornar OK sin sonar */
    if (!adapter->is_enabled)
    {
        return ERR_OK;
    }

    /* Validar duración */
    if (duration_ms > 0 && duration_ms < BUZZER_ADAPTER_MIN_BEEP_MS)
    {
        return ERR_INVALID_PARAM; // Beep demasiado corto (anti-ruido)
    }

    if (duration_ms > BUZZER_ADAPTER_MAX_BEEP_MS)
    {
        return ERR_INVALID_PARAM; // Beep demasiado largo (anti-hang)
    }

    /* Adquirir mutex */
    Result_t mutex_res = os_mutex_acquire(adapter->mutex, OS_WAIT_FOREVER);
    if (mutex_res != ERR_OK)
    {
        return ERR_BUSY;
    }

    /* Activar GPIO */
    Result_t res = BuzzerAdapter_ActivateGPIO(adapter);
    if (res != ERR_OK)
    {
        os_mutex_release(adapter->mutex);
        return res;
    }

    adapter->is_active = true;

    /* Si duration_ms > 0, programar auto-stop por polling */
    if (duration_ms > 0)
    {
        uint32_t current_tick = os_ticks_get();
        adapter->stop_tick = current_tick + duration_ms;
    }
    else
    {
        adapter->stop_tick = 0; // Sin auto-stop
    }

    /* Liberar mutex */
    os_mutex_release(adapter->mutex);

    return ERR_OK;
}

static Result_t BuzzerAdapter_Stop_Impl(IBuzzerControl_Impl *self)
{
    BuzzerAdapter_t *adapter = (BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Adquirir mutex */
    Result_t mutex_res = os_mutex_acquire(adapter->mutex, OS_WAIT_FOREVER);
    if (mutex_res != ERR_OK)
    {
        return ERR_BUSY;
    }

    /* Cancelar auto-stop */
    adapter->stop_tick = 0;

    /* Desactivar GPIO */
    Result_t res = BuzzerAdapter_DeactivateGPIO(adapter);
    adapter->is_active = false;

    /* Liberar mutex */
    os_mutex_release(adapter->mutex);

    return res;
}

static bool BuzzerAdapter_IsActive_Impl(const IBuzzerControl_Impl *self)
{
    const BuzzerAdapter_t *adapter = (const BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return false;
    }

    return adapter->is_active;
}

static Result_t BuzzerAdapter_SetEnabled_Impl(IBuzzerControl_Impl *self, bool enabled)
{
    BuzzerAdapter_t *adapter = (BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Adquirir mutex */
    Result_t mutex_res = os_mutex_acquire(adapter->mutex, OS_WAIT_FOREVER);
    if (mutex_res != ERR_OK)
    {
        return ERR_BUSY;
    }

    adapter->is_enabled = enabled;

    /* Si se deshabilita mientras está sonando, detener */
    if (!enabled && adapter->is_active)
    {
        adapter->stop_tick = 0;
        BuzzerAdapter_DeactivateGPIO(adapter);
        adapter->is_active = false;
    }

    /* Liberar mutex */
    os_mutex_release(adapter->mutex);

    return ERR_OK;
}

static bool BuzzerAdapter_IsEnabled_Impl(const IBuzzerControl_Impl *self)
{
    const BuzzerAdapter_t *adapter = (const BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return false;
    }

    return adapter->is_enabled;
}

/*============================================================================*
 * VTABLE IMPLEMENTATION: Update
 *============================================================================*/

/**
 * @brief Implementación VTable de Update (polling auto-stop)
 * @note Delega a BuzzerAdapter_Update() pública para mantener compatibilidad
 */
static Result_t BuzzerAdapter_Update_Impl(IBuzzerControl_Impl *self)
{
    BuzzerAdapter_t *adapter = (BuzzerAdapter_t *)self;

    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Si no hay auto-stop programado, salir rápido */
    if (adapter->stop_tick == 0)
    {
        return ERR_OK;
    }

    /* Verificar si alcanzamos el tick de auto-stop */
    uint32_t current_tick = os_ticks_get();

    /* Manejo de overflow (current_tick puede ser < stop_tick si hubo wrap-around) */
    bool should_stop = false;
    if (current_tick >= adapter->stop_tick)
    {
        should_stop = true;
    }
    else if (adapter->stop_tick > (UINT32_MAX - 1000)) /* Si stop_tick está cerca del overflow */
    {
        /* Asumimos que hubo overflow y current_tick es el nuevo valor */
        should_stop = true;
    }

    if (should_stop)
    {
        /* Llamar Stop() para apagar el buzzer */
        BuzzerAdapter_Stop_Impl((IBuzzerControl_Impl *)adapter);
    }

    return ERR_OK;
}

/*============================================================================*
 * PUBLIC API IMPLEMENTATION
 *============================================================================*/

Result_t BuzzerAdapter_Init(BuzzerAdapter_t *adapter, const BuzzerAdapterConfig_t *config)
{
    if (!adapter || !config)
    {
        return ERR_NULL_POINTER;
    }

    if (!config->gpio_iface)
    {
        return ERR_INVALID_PARAM;
    }

    /* Limpiar estructura */
    memset(adapter, 0, sizeof(BuzzerAdapter_t));

    /* Copiar configuración */
    adapter->gpio_iface = config->gpio_iface;
    adapter->gpio_port = config->gpio_port;
    adapter->gpio_pin = config->gpio_pin;
    adapter->active_high = config->active_high;
    adapter->is_enabled = config->default_enabled;
    adapter->is_active = false;

    /* Crear mutex */
    adapter->mutex = NULL;
    Result_t res = os_mutex_create(&adapter->mutex, "BuzzerMutex");
    if (res != ERR_OK)
    {
        return ERR_ERROR;
    }

    /* Inicializar estado de polling */
    adapter->stop_tick = 0;

    /* Configurar GPIO como salida */
    GPIO_Config_t gpio_cfg = {
        .mode = I_GPIO_MODE_OUTPUT_PP,
        .pull = I_GPIO_PULL_NONE,
        .speed = I_GPIO_SPEED_LOW,
        .alternate = 0};

    res = GPIO_Init((I_GPIO *)adapter->gpio_iface, adapter->gpio_port, adapter->gpio_pin, &gpio_cfg);
    if (res != ERR_OK)
    {
        os_mutex_delete(adapter->mutex);
        return res;
    }

    /* Asegurar que empieza en estado inactivo */
    BuzzerAdapter_DeactivateGPIO(adapter);

    /* Setup interfaz pública */
    adapter->interface.vtable = &s_buzzer_adapter_vtable;
    adapter->interface.impl = (IBuzzerControl_Impl *)adapter;

    adapter->is_initialized = true;

    return ERR_OK;
}

IBuzzerControl *BuzzerAdapter_GetInterface(BuzzerAdapter_t *adapter)
{
    if (!adapter || !adapter->is_initialized)
    {
        return NULL;
    }

    return &adapter->interface;
}

Result_t BuzzerAdapter_Deinit(BuzzerAdapter_t *adapter)
{
    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Detener cualquier beep en progreso */
    BuzzerAdapter_Stop_Impl((IBuzzerControl_Impl *)adapter);

    /* Eliminar mutex */
    os_mutex_delete(adapter->mutex);

    /* Deinicializar GPIO */
    GPIO_DeInit((I_GPIO *)adapter->gpio_iface, adapter->gpio_port, adapter->gpio_pin);

    adapter->is_initialized = false;

    return ERR_OK;
}

Result_t BuzzerAdapter_Update(BuzzerAdapter_t *adapter)
{
    if (!adapter || !adapter->is_initialized)
    {
        return ERR_INVALID_STATE;
    }

    /* Delegar a implementación VTable (DRY: Don't Repeat Yourself) */
    return BuzzerAdapter_Update_Impl((IBuzzerControl_Impl *)adapter);
}
