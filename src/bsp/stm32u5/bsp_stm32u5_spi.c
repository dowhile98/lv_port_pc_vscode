/**
 * @file bsp_stm32u5_spi.c
 * @brief Implementación STM32U5 de I_SPI (polling + DMA/IT, callbacks separados).
 */

#include "bsp_stm32u5_spi.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

#define MAX_SPI_INSTANCES 4

typedef struct
{
    I_SPI_Handle_t handle;
    I_SPI_Callback_t tx_cb;
    void *tx_ctx;
    I_SPI_Callback_t rx_cb;
    void *rx_ctx;
    I_SPI_Callback_t txrx_cb;
    void *txrx_ctx;
    I_SPI_Callback_t err_cb;
    void *err_ctx;
} SPI_Slot_t;

static SPI_Slot_t s_spi_slots[MAX_SPI_INSTANCES];

/* Prototipos V-Table */
static Result_t stm32u5_spi_init(void *self, I_SPI_Handle_t handle, const I_SPI_Config_t *config);
static Result_t stm32u5_spi_deinit(void *self, I_SPI_Handle_t handle);
static Result_t stm32u5_spi_transmit(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_spi_receive(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_spi_transmit_receive(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_spi_transmit_async(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size);
static Result_t stm32u5_spi_receive_async(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size);
static Result_t stm32u5_spi_transmit_receive_async(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
static Result_t stm32u5_spi_reg_tx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
static Result_t stm32u5_spi_reg_rx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
static Result_t stm32u5_spi_reg_txrx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);
static Result_t stm32u5_spi_reg_err_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context);

static const I_SPI_Vtable s_stm32u5_spi_vtable = {
    .Init = stm32u5_spi_init,
    .DeInit = stm32u5_spi_deinit,
    .Transmit = stm32u5_spi_transmit,
    .Receive = stm32u5_spi_receive,
    .TransmitReceive = stm32u5_spi_transmit_receive,
    .Transmit_Async = stm32u5_spi_transmit_async,
    .Receive_Async = stm32u5_spi_receive_async,
    .TransmitReceive_Async = stm32u5_spi_transmit_receive_async,
    .RegisterTxCallback = stm32u5_spi_reg_tx_cb,
    .RegisterRxCallback = stm32u5_spi_reg_rx_cb,
    .RegisterTxRxCallback = stm32u5_spi_reg_txrx_cb,
    .RegisterErrorCallback = stm32u5_spi_reg_err_cb};

static I_SPI s_stm32u5_spi_iface = {
    .vtable = &s_stm32u5_spi_vtable,
    .impl = (void *)&s_stm32u5_spi_vtable};

I_SPI *Bsp_Stm32U5_Spi_GetInterface(void) { return &s_stm32u5_spi_iface; }

static SPI_Slot_t *get_slot(I_SPI_Handle_t handle)
{
    for (int i = 0; i < MAX_SPI_INSTANCES; i++)
    {
        if (s_spi_slots[i].handle == handle)
            return &s_spi_slots[i];
    }
    for (int i = 0; i < MAX_SPI_INSTANCES; i++)
    {
        if (s_spi_slots[i].handle == NULL)
        {
            s_spi_slots[i].handle = handle;
            return &s_spi_slots[i];
        }
    }
    return NULL;
}

static Result_t stm32u5_spi_init(void *self, I_SPI_Handle_t handle, const I_SPI_Config_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
        return ERR_NULL_POINTER;

    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)handle;
    hspi->Init.Mode = (config->mode == I_SPI_MODE_MASTER) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi->Init.Direction = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize = (config->data_size == I_SPI_DATASIZE_8BIT) ? SPI_DATASIZE_8BIT : SPI_DATASIZE_16BIT;
    hspi->Init.CLKPolarity = (config->cpol == I_SPI_CPOL_LOW) ? SPI_POLARITY_LOW : SPI_POLARITY_HIGH;
    hspi->Init.CLKPhase = (config->cpha == I_SPI_CPHA_1EDGE) ? SPI_PHASE_1EDGE : SPI_PHASE_2EDGE;
    hspi->Init.NSS = SPI_NSS_SOFT;
    hspi->Init.BaudRatePrescaler = config->baudrate_prescaler;
    hspi->Init.FirstBit = (config->first_bit == I_SPI_FIRSTBIT_MSB) ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(hspi) != HAL_OK)
        return ERR_ERROR;
    get_slot(handle);
    return ERR_OK;
}

static Result_t stm32u5_spi_deinit(void *self, I_SPI_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    HAL_SPI_DeInit((SPI_HandleTypeDef *)handle);
    return ERR_OK;
}

static Result_t stm32u5_spi_transmit(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_Transmit((SPI_HandleTypeDef *)handle, (uint8_t *)data, size, timeout) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_spi_receive(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_Receive((SPI_HandleTypeDef *)handle, data, size, timeout) != HAL_OK)
        return ERR_TIMEOUT;
    return ERR_OK;
}

static Result_t stm32u5_spi_transmit_receive(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || tx_data == NULL || rx_data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_TransmitReceive((SPI_HandleTypeDef *)handle, (uint8_t *)tx_data, rx_data, size, timeout) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_spi_transmit_async(void *self, I_SPI_Handle_t handle, const uint8_t *data, uint16_t size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_Transmit_DMA((SPI_HandleTypeDef *)handle, (uint8_t *)data, size) != HAL_OK)
        return ERR_BUSY;
    return ERR_OK;
}

static Result_t stm32u5_spi_receive_async(void *self, I_SPI_Handle_t handle, uint8_t *data, uint16_t size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_Receive_DMA((SPI_HandleTypeDef *)handle, data, size) != HAL_OK)
        return ERR_BUSY;
    return ERR_OK;
}

static Result_t stm32u5_spi_transmit_receive_async(void *self, I_SPI_Handle_t handle, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    (void)self;
    if (handle == NULL || tx_data == NULL || rx_data == NULL)
        return ERR_NULL_POINTER;
    if (HAL_SPI_TransmitReceive_DMA((SPI_HandleTypeDef *)handle, (uint8_t *)tx_data, rx_data, size) != HAL_OK)
        return ERR_BUSY;
    return ERR_OK;
}

static Result_t stm32u5_spi_reg_tx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    (void)self;
    SPI_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;
    slot->tx_cb = cb;
    slot->tx_ctx = context;
    return ERR_OK;
}
static Result_t stm32u5_spi_reg_rx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    (void)self;
    SPI_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;
    slot->rx_cb = cb;
    slot->rx_ctx = context;
    return ERR_OK;
}
static Result_t stm32u5_spi_reg_txrx_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    (void)self;
    SPI_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;
    slot->txrx_cb = cb;
    slot->txrx_ctx = context;
    return ERR_OK;
}
static Result_t stm32u5_spi_reg_err_cb(void *self, I_SPI_Handle_t handle, I_SPI_Callback_t cb, void *context)
{
    (void)self;
    SPI_Slot_t *slot = get_slot(handle);
    if (slot == NULL)
        return ERR_ERROR;
    slot->err_cb = cb;
    slot->err_ctx = context;
    return ERR_OK;
}

/**
 * @brief Dispatchers para eventos de SPI del HAL
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Slot_t *slot = get_slot((I_SPI_Handle_t)hspi);
    if (slot && slot->txrx_cb)
    {
        slot->txrx_cb(slot->txrx_ctx, hspi->RxXferSize);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Slot_t *slot = get_slot((I_SPI_Handle_t)hspi);
    if (slot && slot->tx_cb)
    {
        slot->tx_cb(slot->tx_ctx, 0);
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Slot_t *slot = get_slot((I_SPI_Handle_t)hspi);
    if (slot && slot->rx_cb)
    {
        slot->rx_cb(slot->rx_ctx, hspi->RxXferSize);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    SPI_Slot_t *slot = get_slot((I_SPI_Handle_t)hspi);
    if (slot && slot->err_cb)
    {
        slot->err_cb(slot->err_ctx, 0);
    }
}
