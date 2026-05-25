/**
 * @file bsp_stm32u5_i2c.c
 * @brief Implementación del BSP I2C para STM32U5 mapeando al HAL de ST.
 *
 * @note Thread-safe: Todas las operaciones I2C están protegidas con mutex
 *       para prevenir race conditions entre múltiples threads.
 */

#include "bsp_stm32u5_i2c.h"
#include "infrastructure/osal/osal.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

/* ===== Mutex para protección I2C (thread-safe) ===== */

#define BSP_I2C_MAX_BUSES 3 /**< STM32U5 tiene hasta 3 buses I2C */

static os_mutex_t s_i2c_mutex[BSP_I2C_MAX_BUSES];
static bool s_i2c_mutex_initialized = false;

/**
 * @brief Obtiene índice del bus I2C desde el handle.
 * @param handle Handle del I2C (I2C_HandleTypeDef*).
 * @return Índice (0-2) o 0xFF si inválido.
 */
static uint8_t get_i2c_bus_index(I2C_HandleTypeDef *handle)
{
    if (handle == NULL)
        return 0xFF;

    if (handle->Instance == I2C1)
        return 0;
    else if (handle->Instance == I2C2)
        return 1;
    else if (handle->Instance == I2C3)
        return 2;

    return 0xFF; /* Bus desconocido */
}

/**
 * @brief Inicializa mutexes I2C (llamar una vez al boot).
 */
static void init_i2c_mutexes(void)
{
    if (s_i2c_mutex_initialized)
        return;

    for (uint8_t i = 0; i < BSP_I2C_MAX_BUSES; i++)
    {

        os_mutex_create(&s_i2c_mutex[i], "I2C_Bus_Mtx");
    }

    s_i2c_mutex_initialized = true;
}

/* Prototipos V-Table */
static Result_t stm32u5_i2c_init(void *self, I_I2C_Handle_t handle, const I_I2C_Config_t *config);
static Result_t stm32u5_i2c_deinit(void *self, I_I2C_Handle_t handle);
static Result_t stm32u5_i2c_master_tx(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, const uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_i2c_master_rx(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_i2c_mem_write(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, const uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_i2c_mem_read(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, uint8_t *data, uint16_t size, uint32_t timeout);
static Result_t stm32u5_i2c_is_ready(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint32_t trials, uint32_t timeout);

static const I_I2C_Vtable s_stm32u5_i2c_vtable = {
    .Init = stm32u5_i2c_init,
    .DeInit = stm32u5_i2c_deinit,
    .Master_Transmit = stm32u5_i2c_master_tx,
    .Master_Receive = stm32u5_i2c_master_rx,
    .Mem_Write = stm32u5_i2c_mem_write,
    .Mem_Read = stm32u5_i2c_mem_read,
    .IsDeviceReady = stm32u5_i2c_is_ready};

static I_I2C s_stm32u5_i2c_iface = {
    .vtable = &s_stm32u5_i2c_vtable,
    .impl = (void *)&s_stm32u5_i2c_vtable};

I_I2C *Bsp_Stm32U5_I2c_GetInterface(void)
{
    return &s_stm32u5_i2c_iface;
}

static Result_t stm32u5_i2c_init(void *self, I_I2C_Handle_t handle, const I_I2C_Config_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
        return ERR_NULL_POINTER;

    /* Inicializar mutexes (solo primera vez) */
    init_i2c_mutexes();

    /* Nota: En STM32U5 el Timing del I2C suele configurarse vía CubeMX y pasarse al HAL_I2C_Init */
    if (HAL_I2C_Init((I2C_HandleTypeDef *)handle) != HAL_OK)
        return ERR_ERROR;
    return ERR_OK;
}

static Result_t stm32u5_i2c_deinit(void *self, I_I2C_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    HAL_I2C_DeInit((I2C_HandleTypeDef *)handle);
    return ERR_OK;
}

static Result_t stm32u5_i2c_master_tx(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    uint8_t bus_index = get_i2c_bus_index((I2C_HandleTypeDef *)handle);
    if (bus_index == 0xFF)
        return ERR_INVALID_PARAM;

    /* Proteger acceso al bus I2C (thread-safe) */

    os_mutex_acquire(&s_i2c_mutex[bus_index], OS_WAIT_FOREVER);

    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)handle, dev_addr, (uint8_t *)data, size, timeout);

    os_mutex_release(&s_i2c_mutex[bus_index]);

    return (hal_status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_i2c_master_rx(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    uint8_t bus_index = get_i2c_bus_index((I2C_HandleTypeDef *)handle);
    if (bus_index == 0xFF)
        return ERR_INVALID_PARAM;

    /* Proteger acceso al bus I2C (thread-safe) */
    os_mutex_acquire(&s_i2c_mutex[bus_index], OS_WAIT_FOREVER);

    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)handle, dev_addr, data, size, timeout);

    os_mutex_release(&s_i2c_mutex[bus_index]);

    return (hal_status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_i2c_mem_write(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    uint8_t bus_index = get_i2c_bus_index((I2C_HandleTypeDef *)handle);
    if (bus_index == 0xFF)
        return ERR_INVALID_PARAM;

    uint16_t st_addr_size = (addr_size == I_I2C_MEMADDR_SIZE_8BIT) ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;

    /* Proteger acceso al bus I2C (thread-safe) */
    os_mutex_acquire(&s_i2c_mutex[bus_index], OS_WAIT_FOREVER);

    HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Write((I2C_HandleTypeDef *)handle, dev_addr, mem_addr, st_addr_size, (uint8_t *)data, size, timeout);

    os_mutex_release(&s_i2c_mutex[bus_index]);

    return (hal_status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_i2c_mem_read(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint16_t mem_addr, I_I2C_MemAddrSize_t addr_size, uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    uint8_t bus_index = get_i2c_bus_index((I2C_HandleTypeDef *)handle);
    if (bus_index == 0xFF)
        return ERR_INVALID_PARAM;

    uint16_t st_addr_size = (addr_size == I_I2C_MEMADDR_SIZE_8BIT) ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;

    /* Proteger acceso al bus I2C (thread-safe) */
    os_mutex_acquire(&s_i2c_mutex[bus_index], OS_WAIT_FOREVER);

    HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)handle, dev_addr, mem_addr, st_addr_size, data, size, timeout);

    os_mutex_release(&s_i2c_mutex[bus_index]);

    return (hal_status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_i2c_is_ready(void *self, I_I2C_Handle_t handle, uint16_t dev_addr, uint32_t trials, uint32_t timeout)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;

    uint8_t bus_index = get_i2c_bus_index((I2C_HandleTypeDef *)handle);
    if (bus_index == 0xFF)
        return ERR_INVALID_PARAM;

    /* Proteger acceso al bus I2C (thread-safe) */
    os_mutex_acquire(&s_i2c_mutex[bus_index], OS_WAIT_FOREVER);

    HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady((I2C_HandleTypeDef *)handle, dev_addr, trials, timeout);

    os_mutex_release(&s_i2c_mutex[bus_index]);

    return (hal_status == HAL_OK) ? ERR_OK : ERR_ERROR;
}
