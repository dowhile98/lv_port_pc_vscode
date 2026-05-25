/**
 * @file bsp_stm32u5_gpio.c
 * @brief Implementación de la interfaz I_GPIO para el hardware STM32U5.
 */

#include "bsp_stm32u5_gpio.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

/* Prototipos de funciones privadas (mapeo a la interfaz) */
static Result_t stm32u5_gpio_init(void *self, GPIO_Port_t port, GPIO_Pin_t pin, const GPIO_Config_t *config);
static Result_t stm32u5_gpio_deinit(void *self, GPIO_Port_t port, GPIO_Pin_t pin);
static Result_t stm32u5_gpio_write_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t state);
static Result_t stm32u5_gpio_read_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t *state_out);
static Result_t stm32u5_gpio_toggle_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin);
static Result_t stm32u5_gpio_set_bits(void *self, GPIO_Port_t port, uint32_t pins);
static Result_t stm32u5_gpio_reset_bits(void *self, GPIO_Port_t port, uint32_t pins);
static Result_t stm32u5_gpio_write_port(void *self, GPIO_Port_t port, uint32_t value);
static Result_t stm32u5_gpio_read_port(void *self, GPIO_Port_t port, uint32_t *value_out);
static Result_t stm32u5_gpio_lock_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin);

/* Definición de la V-Table para STM32U5 */
static const I_GPIO_Vtable s_stm32u5_gpio_vtable = {
    .Init = stm32u5_gpio_init,
    .DeInit = stm32u5_gpio_deinit,
    .WritePin = stm32u5_gpio_write_pin,
    .ReadPin = stm32u5_gpio_read_pin,
    .TogglePin = stm32u5_gpio_toggle_pin,
    .WritePort = stm32u5_gpio_write_port,
    .ReadPort = stm32u5_gpio_read_port,
    .SetBits = stm32u5_gpio_set_bits,
    .ResetBits = stm32u5_gpio_reset_bits,
    .LockPin = stm32u5_gpio_lock_pin};

/* Instancia del objeto I_GPIO */
static I_GPIO s_stm32u5_gpio_iface = {
    .vtable = &s_stm32u5_gpio_vtable,
    .impl = (void *)&s_stm32u5_gpio_vtable /* puntero válido para validación */
};

I_GPIO *Bsp_Stm32U5_Gpio_GetInterface(void)
{
    return &s_stm32u5_gpio_iface;
}

/* Implementaciones Privadas */

static Result_t stm32u5_gpio_init(void *self, GPIO_Port_t port, GPIO_Pin_t pin, const GPIO_Config_t *config)
{
    (void)self;
    if (port == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin;

    /* Mapeo de modo */
    switch (config->mode)
    {
    case I_GPIO_MODE_INPUT:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    case I_GPIO_MODE_OUTPUT_PP:
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case I_GPIO_MODE_OUTPUT_OD:
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        break;
    case I_GPIO_MODE_AF_PP:
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        break;
    case I_GPIO_MODE_AF_OD:
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        break;
    case I_GPIO_MODE_ANALOG:
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        break;
    default:
        return ERR_INVALID_PARAM;
    }

    /* Mapeo de pull */
    switch (config->pull)
    {
    case I_GPIO_PULL_NONE:
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        break;
    case I_GPIO_PULL_UP:
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
    case I_GPIO_PULL_DOWN:
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
    default:
        return ERR_INVALID_PARAM;
    }

    /* Mapeo de velocidad */
    switch (config->speed)
    {
    case I_GPIO_SPEED_LOW:
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        break;
    case I_GPIO_SPEED_MEDIUM:
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
        break;
    case I_GPIO_SPEED_HIGH:
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        break;
    case I_GPIO_SPEED_VERY_HIGH:
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        break;
    default:
        return ERR_INVALID_PARAM;
    }

    GPIO_InitStruct.Alternate = config->alternate;

    HAL_GPIO_Init((GPIO_TypeDef *)port, &GPIO_InitStruct);
    return ERR_OK;
}

static Result_t stm32u5_gpio_deinit(void *self, GPIO_Port_t port, GPIO_Pin_t pin)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    HAL_GPIO_DeInit((GPIO_TypeDef *)port, pin);
    return ERR_OK;
}

static Result_t stm32u5_gpio_write_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t state)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    HAL_GPIO_WritePin((GPIO_TypeDef *)port, pin, (state == I_GPIO_STATE_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return ERR_OK;
}

static Result_t stm32u5_gpio_read_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin, GPIO_State_t *state_out)
{
    (void)self;
    if (port == NULL || state_out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    GPIO_PinState pin_state = HAL_GPIO_ReadPin((GPIO_TypeDef *)port, pin);
    *state_out = (pin_state == GPIO_PIN_SET) ? I_GPIO_STATE_HIGH : I_GPIO_STATE_LOW;
    return ERR_OK;
}

static Result_t stm32u5_gpio_toggle_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, pin);
    return ERR_OK;
}

static Result_t stm32u5_gpio_set_bits(void *self, GPIO_Port_t port, uint32_t pins)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    ((GPIO_TypeDef *)port)->BSRR = pins;
    return ERR_OK;
}

static Result_t stm32u5_gpio_reset_bits(void *self, GPIO_Port_t port, uint32_t pins)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    ((GPIO_TypeDef *)port)->BRR = pins;
    return ERR_OK;
}

static Result_t stm32u5_gpio_write_port(void *self, GPIO_Port_t port, uint32_t value)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    ((GPIO_TypeDef *)port)->ODR = value;
    return ERR_OK;
}

static Result_t stm32u5_gpio_read_port(void *self, GPIO_Port_t port, uint32_t *value_out)
{
    (void)self;
    if (port == NULL || value_out == NULL)
    {
        return ERR_NULL_POINTER;
    }
    *value_out = ((GPIO_TypeDef *)port)->IDR;
    return ERR_OK;
}

static Result_t stm32u5_gpio_lock_pin(void *self, GPIO_Port_t port, GPIO_Pin_t pin)
{
    (void)self;
    if (port == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (HAL_GPIO_LockPin((GPIO_TypeDef *)port, pin) != HAL_OK)
    {
        return ERR_ERROR;
    }
    return ERR_OK;
}
