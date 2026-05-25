/**
 * @file bsp_stm32u5_uart.c
 * @brief Implementación del BSP UART para STM32U5 con DMA e Idle Line detection.
 * @version 1.0.0
 * @author Tecna Smart Lab
 */

#include "bsp_stm32u5_uart.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

#define MAX_UART_INSTANCES 4

typedef struct
{
    I_UART_Handle_t handle;
    I_UART_Callback_t tx_cb;
    void *tx_ctx;
    I_UART_Callback_t rx_cb;
    void *rx_ctx;
    I_UART_Callback_t err_cb;
    void *err_ctx;
} UART_Slot_t;

static UART_Slot_t s_uart_slots[MAX_UART_INSTANCES];

/* Prototipos V-Table */
static Result_t stm32u5_uart_init(void *self, I_UART_Handle_t handle, const I_UART_Config_t *config);
static Result_t stm32u5_uart_deinit(void *self, I_UART_Handle_t handle);
static Result_t stm32u5_uart_transmit(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_uart_receive(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_uart_transmit_async(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size);
static Result_t stm32u5_uart_receive_idle_async(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t max_size);
static Result_t stm32u5_uart_reg_tx_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);
static Result_t stm32u5_uart_reg_rx_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);
static Result_t stm32u5_uart_reg_err_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context);

static const I_UART_Vtable s_stm32u5_uart_vtable = {
    .Init = stm32u5_uart_init,
    .DeInit = stm32u5_uart_deinit,
    .Transmit = stm32u5_uart_transmit,
    .Receive = stm32u5_uart_receive,
    .Transmit_Async = stm32u5_uart_transmit_async,
    .ReceiveUntilIdle_Async = stm32u5_uart_receive_idle_async,
    .RegisterTxCompleteCallback = stm32u5_uart_reg_tx_cb,
    .RegisterRxEventCallback = stm32u5_uart_reg_rx_cb,
    .RegisterErrorCallback = stm32u5_uart_reg_err_cb};

static I_UART s_stm32u5_uart_iface = {
    .vtable = &s_stm32u5_uart_vtable,
    .impl = (void *)&s_stm32u5_uart_vtable};

I_UART *Bsp_Stm32U5_Uart_GetInterface(void)
{
    return &s_stm32u5_uart_iface;
}

static UART_Slot_t *get_slot(I_UART_Handle_t handle)
{
    for (int i = 0; i < MAX_UART_INSTANCES; i++)
    {
        if (s_uart_slots[i].handle == handle)
            return &s_uart_slots[i];
    }
    for (int i = 0; i < MAX_UART_INSTANCES; i++)
    {
        if (s_uart_slots[i].handle == NULL)
        {
            s_uart_slots[i].handle = handle;
            return &s_uart_slots[i];
        }
    }
    return NULL;
}

static Result_t stm32u5_uart_init(void *self, I_UART_Handle_t handle, const I_UART_Config_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
        return ERR_NULL_POINTER;

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)handle;
    huart->Init.BaudRate = (uint32_t)config->baudrate;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = (config->stop_bits == I_UART_STOP_1) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    huart->Init.Parity = (config->parity == I_UART_PARITY_NONE) ? UART_PARITY_NONE : (config->parity == I_UART_PARITY_EVEN) ? UART_PARITY_EVEN
                                                                                                                            : UART_PARITY_ODD;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(huart) != HAL_OK)
        return ERR_ERROR;

    (void)get_slot(handle);
    return ERR_OK;
}

static Result_t stm32u5_uart_deinit(void *self, I_UART_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    HAL_UART_DeInit((UART_HandleTypeDef *)handle);
    return ERR_OK;
}

static Result_t stm32u5_uart_transmit(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_UART_Transmit((UART_HandleTypeDef *)handle, data, size, timeout) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_uart_receive(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_UART_Receive((UART_HandleTypeDef *)handle, data, size, timeout) != HAL_OK)
        return ERR_TIMEOUT;
    return ERR_OK;
}

static Result_t stm32u5_uart_transmit_async(void *self, I_UART_Handle_t handle, const uint8_t *data, uint16_t size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_UART_Transmit_DMA((UART_HandleTypeDef *)handle, data, size) != HAL_OK)
        return ERR_BUSY;
    return ERR_OK;
}

static Result_t stm32u5_uart_receive_idle_async(void *self, I_UART_Handle_t handle, uint8_t *data, uint16_t max_size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    /* HAL_UARTEx_ReceiveToIdle_DMA utiliza DMA + interrupción IDLE Line para detectar fin de trama */
    if (HAL_UARTEx_ReceiveToIdle_DMA((UART_HandleTypeDef *)handle, data, max_size) != HAL_OK)
        return ERR_BUSY;
    return ERR_OK;
}

static Result_t stm32u5_uart_reg_tx_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    (void)self;
    UART_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;

    slot->tx_cb = cb;
    slot->tx_ctx = context;
    return ERR_OK;
}

static Result_t stm32u5_uart_reg_rx_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    (void)self;
    UART_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;

    slot->rx_cb = cb;
    slot->rx_ctx = context;
    return ERR_OK;
}

static Result_t stm32u5_uart_reg_err_cb(void *self, I_UART_Handle_t handle, I_UART_Callback_t cb, void *context)
{
    (void)self;
    UART_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;

    slot->err_cb = cb;
    slot->err_ctx = context;
    return ERR_OK;
}

/**
 * @brief Dispatcher HAL: evento de recepción (Normal e IDLE).
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    UART_Slot_t *slot = get_slot((I_UART_Handle_t)huart);
    if (slot != NULL && slot->rx_cb != NULL)
    {
        slot->rx_cb(slot->rx_ctx, Size);
    }
}

/**
 * @brief Dispatcher HAL: fin de transmisión DMA.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_Slot_t *slot = get_slot((I_UART_Handle_t)huart);
    if (slot != NULL && slot->tx_cb != NULL)
    {
        slot->tx_cb(slot->tx_ctx, 0);
    }
}

/**
 * @brief Dispatcher HAL: error de UART.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    UART_Slot_t *slot = get_slot((I_UART_Handle_t)huart);
    if (slot != NULL && slot->err_cb != NULL)
    {
        slot->err_cb(slot->err_ctx, 0);
    }
}
