/**
 * @file bsp_stm32u5_timer.c
 * @brief Implementación STM32U5 de la interfaz I_TIMER (base tiempo + OC por interrupción).
 */

#include "bsp_stm32u5_timer.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

#define MAX_TIMER_INSTANCES 4

typedef struct
{
    I_TIMER_Handle_t handle;
    I_TIMER_Callback_t update_cb;
    void *update_ctx;
    I_TIMER_OC_Callback_t oc_cb;
    void *oc_ctx;
} Timer_Slot_t;

static Timer_Slot_t s_timer_slots[MAX_TIMER_INSTANCES];

/* Prototipos V-Table */
static Result_t stm32u5_timer_init(void *self, I_TIMER_Handle_t handle, const I_TIMER_Config_t *config);
static Result_t stm32u5_timer_deinit(void *self, I_TIMER_Handle_t handle);
static Result_t stm32u5_timer_start_update_it(void *self, I_TIMER_Handle_t handle);
static Result_t stm32u5_timer_stop_update_it(void *self, I_TIMER_Handle_t handle);
static Result_t stm32u5_timer_oc_config(void *self, I_TIMER_Handle_t handle, const I_TIMER_OC_Config_t *oc_config);
static Result_t stm32u5_timer_oc_start_it(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel);
static Result_t stm32u5_timer_oc_stop_it(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel);
static Result_t stm32u5_timer_set_compare(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel, uint32_t compare_value);
static Result_t stm32u5_timer_set_arr(void *self, I_TIMER_Handle_t handle, uint32_t period);
static Result_t stm32u5_timer_reg_update_cb(void *self, I_TIMER_Handle_t handle, I_TIMER_Callback_t cb, void *context);
static Result_t stm32u5_timer_reg_oc_cb(void *self, I_TIMER_Handle_t handle, I_TIMER_OC_Callback_t cb, void *context);


void BSP_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

static const I_TIMER_Vtable s_stm32u5_timer_vtable = {
    .Init = stm32u5_timer_init,
    .DeInit = stm32u5_timer_deinit,
    .StartUpdate_IT = stm32u5_timer_start_update_it,
    .StopUpdate_IT = stm32u5_timer_stop_update_it,
    .OC_ConfigChannel = stm32u5_timer_oc_config,
    .OC_Start_IT = stm32u5_timer_oc_start_it,
    .OC_Stop_IT = stm32u5_timer_oc_stop_it,
    .SetCompare = stm32u5_timer_set_compare,
    .SetAutoReload = stm32u5_timer_set_arr,
    .RegisterUpdateCallback = stm32u5_timer_reg_update_cb,
    .RegisterOCCallback = stm32u5_timer_reg_oc_cb};

static I_TIMER s_stm32u5_timer_iface = {
    .vtable = &s_stm32u5_timer_vtable,
    .impl = (void *)&s_stm32u5_timer_vtable};

I_TIMER *Bsp_Stm32U5_Timer_GetInterface(void)
{
    return &s_stm32u5_timer_iface;
}

static uint32_t map_channel(I_TIMER_Channel_t channel)
{
    switch (channel)
    {
    case I_TIMER_CHANNEL_1:
        return TIM_CHANNEL_1;
    case I_TIMER_CHANNEL_2:
        return TIM_CHANNEL_2;
    case I_TIMER_CHANNEL_3:
        return TIM_CHANNEL_3;
    case I_TIMER_CHANNEL_4:
        return TIM_CHANNEL_4;
    default:
        return 0;
    }
}

static Timer_Slot_t *get_slot(I_TIMER_Handle_t handle)
{
    for (int i = 0; i < MAX_TIMER_INSTANCES; i++)
    {
        if (s_timer_slots[i].handle == handle)
            return &s_timer_slots[i];
    }
    for (int i = 0; i < MAX_TIMER_INSTANCES; i++)
    {
        if (s_timer_slots[i].handle == NULL)
        {
            s_timer_slots[i].handle = handle;
            return &s_timer_slots[i];
        }
    }
    return NULL;
}

static Result_t stm32u5_timer_init(void *self, I_TIMER_Handle_t handle, const I_TIMER_Config_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
        return ERR_NULL_POINTER;

    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)handle;
    htim->Init.Prescaler = config->prescaler;
    htim->Init.Period = config->period;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = config->auto_reload_preload ? TIM_AUTORELOAD_PRELOAD_ENABLE : TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(htim) != HAL_OK)
        return ERR_ERROR;

    (void)get_slot(handle);

    HAL_TIM_RegisterCallback(htim, HAL_TIM_PERIOD_ELAPSED_CB_ID, BSP_TIM_PeriodElapsedCallback);
    return ERR_OK;
}

static Result_t stm32u5_timer_deinit(void *self, I_TIMER_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    HAL_TIM_Base_DeInit((TIM_HandleTypeDef *)handle);
    return ERR_OK;
}

static Result_t stm32u5_timer_start_update_it(void *self, I_TIMER_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    if (HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)handle) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_timer_stop_update_it(void *self, I_TIMER_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *)handle);
    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)handle, 0); /* Reset counter on stop */
    return ERR_OK;
}

static Result_t stm32u5_timer_oc_config(void *self, I_TIMER_Handle_t handle, const I_TIMER_OC_Config_t *oc_config)
{
    (void)self;
    if (handle == NULL || oc_config == NULL)
        return ERR_NULL_POINTER;

    uint32_t ch = map_channel(oc_config->channel);
//    if (ch == 0)
//        return ERR_INVALID_PARAM;

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_TIMING; /* No salida física, solo interrupción */
    sConfigOC.Pulse = oc_config->compare_value;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = oc_config->preload_enable ? TIM_OCFAST_ENABLE : TIM_OCFAST_DISABLE;

    if (HAL_TIM_OC_ConfigChannel((TIM_HandleTypeDef *)handle, &sConfigOC, ch) != HAL_OK)
    {
        return ERR_ERROR;
    }

    return ERR_OK;
}

static Result_t stm32u5_timer_oc_start_it(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    uint32_t ch = map_channel(channel);
//    if (ch == 0)
//        return ERR_INVALID_PARAM;

    if (HAL_TIM_OC_Start_IT((TIM_HandleTypeDef *)handle, ch) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_timer_oc_stop_it(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    uint32_t ch = map_channel(channel);
//    if (ch == 0)
//        return ERR_INVALID_PARAM;

    HAL_TIM_OC_Stop_IT((TIM_HandleTypeDef *)handle, ch);
    return ERR_OK;
}

static Result_t stm32u5_timer_set_compare(void *self, I_TIMER_Handle_t handle, I_TIMER_Channel_t channel, uint32_t compare_value)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    uint32_t ch = map_channel(channel);
//    if (ch == 0)
//        return ERR_INVALID_PARAM;
    __HAL_TIM_SET_COMPARE((TIM_HandleTypeDef *)handle, ch, compare_value);
    return ERR_OK;
}

static Result_t stm32u5_timer_set_arr(void *self, I_TIMER_Handle_t handle, uint32_t period)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)handle, period);
    return ERR_OK;
}

static Result_t stm32u5_timer_reg_update_cb(void *self, I_TIMER_Handle_t handle, I_TIMER_Callback_t cb, void *context)
{
    (void)self;
    Timer_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;
    slot->update_cb = cb;
    slot->update_ctx = context;
    return ERR_OK;
}

static Result_t stm32u5_timer_reg_oc_cb(void *self, I_TIMER_Handle_t handle, I_TIMER_OC_Callback_t cb, void *context)
{
    (void)self;
    Timer_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;

    slot->oc_cb = cb;
    slot->oc_ctx = context;
    return ERR_OK;
}

/**
 * @brief Dispatcher HAL: evento de comparación (OC)
 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    Timer_Slot_t *slot = get_slot((I_TIMER_Handle_t)htim);
    if (slot != NULL && slot->oc_cb != NULL)
    {
        I_TIMER_Channel_t channel;
        switch (htim->Channel)
        {
        case HAL_TIM_ACTIVE_CHANNEL_1:
            channel = I_TIMER_CHANNEL_1;
            break;
        case HAL_TIM_ACTIVE_CHANNEL_2:
            channel = I_TIMER_CHANNEL_2;
            break;
        case HAL_TIM_ACTIVE_CHANNEL_3:
            channel = I_TIMER_CHANNEL_3;
            break;
        case HAL_TIM_ACTIVE_CHANNEL_4:
            channel = I_TIMER_CHANNEL_4;
            break;
        default:
            return;
        }
        slot->oc_cb(slot->oc_ctx, channel);
    }
}

/**
 * @brief Dispatcher HAL: evento de actualización (overflow / update)
 */
void BSP_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    Timer_Slot_t *slot = get_slot((I_TIMER_Handle_t)htim);
    if (slot != NULL && slot->update_cb != NULL)
    {
        slot->update_cb(slot->update_ctx);
    }
}
