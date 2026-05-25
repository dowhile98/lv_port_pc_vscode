/**
 * @file bsp_stm32u5_exti.c
 * @brief Implementación completa de I_EXTI para STM32U5.
 * @version 1.0.0
 * @author Tecna Smart Lab
 *
 * Mapea operaciones EXTI abstractas a funciones concretas del HAL de STM32U5:
 * - Inicialización de líneas EXTI con configuración de trigger y prioridad.
 * - Registro de callbacks por pin en tabla interna.
 * - Enable/Disable de IRQs en el controlador NVIC.
 * - Despacho de eventos EXTI Rising/Falling a callbacks registrados.
 *
 * Usa tabla estática s_exti_slots para almacenar callbacks, indexada por pin.
 * Máximo 16 líneas EXTI soportadas (GPIO_PIN_0 a GPIO_PIN_15).
 */

#include "bsp_stm32u5_exti.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

#define MAX_EXTI_LINES 16

/**
 * @brief Slot para almacenar callback y contexto asociado a una línea EXTI.
 */
typedef struct
{
    I_EXTI_Callback_t cb; /**< Función callback, NULL si sin registrar */
    void *context;        /**< Contexto opaco para callback */
} EXTI_Slot_t;

/** @brief Tabla de callbacks indexada por pin (0-15) */
static EXTI_Slot_t s_exti_slots[MAX_EXTI_LINES];

/* ===== Prototipos de funciones privadas (implementan V-Table) ===== */

/**
 * @brief Inicializa una línea EXTI con configuración de trigger y prioridad.
 * @param self Puntero opaco (sin usar).
 * @param config Configuración EXTI (puerto, pin, trigger, prioridad).
 * @return ERR_OK si éxito, ERR_NULL_POINTER si config es NULL.
 *
 * Internamente:
 *   1. Valida que config y config->port no sean NULL.
 *   2. Mapea I_EXTI_Trigger_t a GPIO_MODE_IT_* del HAL.
 *   3. Inicializa pin vía HAL_GPIO_Init() con modo interrupción.
 *   4. Configura prioridad en NVIC (SetPriority) e habilita IRQ (EnableIRQ).
 */
static Result_t stm32u5_exti_init(void *self, const I_EXTI_Config_t *config);

/**
 * @brief Desinicializa una línea EXTI deshabilitando su IRQ en NVIC.
 * @param self Puntero opaco (sin usar).
 * @param pin Pin GPIO a desiniicializar.
 * @return ERR_OK siempre.
 *
 * @note La desconexión del pin del puerto GPIO no se realiza aquí;
 *       se reserva para GPIO_DeInit cuando sea necesario.
 */
static Result_t stm32u5_exti_deinit(void *self, GPIO_Pin_t pin);

/**
 * @brief Habilita la IRQ de una línea EXTI en el NVIC.
 * @param self Puntero opaco (sin usar).
 * @param pin Pin GPIO cuya IRQ se va a habilitar.
 * @return ERR_OK siempre.
 *
 * Internamente: Llama a HAL_NVIC_EnableIRQ() con el IRQn mapeado del pin.
 */
static Result_t stm32u5_exti_enable(void *self, GPIO_Pin_t pin);

/**
 * @brief Deshabilita la IRQ de una línea EXTI en el NVIC.
 * @param self Puntero opaco (sin usar).
 * @param pin Pin GPIO cuya IRQ se va a deshabilitar.
 * @return ERR_OK siempre.
 *
 * Internamente: Llama a HAL_NVIC_DisableIRQ() con el IRQn mapeado del pin.
 */
static Result_t stm32u5_exti_disable(void *self, GPIO_Pin_t pin);

/**
 * @brief Registra callback para una línea EXTI.
 * @param self Puntero opaco (sin usar).
 * @param pin Pin GPIO (debe ser potencia de 2: GPIO_PIN_0..GPIO_PIN_15).
 * @param cb Función callback a registrar (puede ser NULL para desregistrar).
 * @param context Puntero opaco a contexto de usuario (pasado a callback).
 * @return ERR_OK si idx < MAX_EXTI_LINES, ERR_INVALID_PARAM en caso contrario.
 *
 * Internamente:
 *   1. Calcula idx = get_pin_index(pin).
 *   2. Si idx >= MAX_EXTI_LINES, retorna ERR_INVALID_PARAM.
 *   3. Almacena cb y context en s_exti_slots[idx].
 *   4. El callback será invocado en ISR context cuando se dispare la interrupción.
 */
static Result_t stm32u5_exti_register_callback(void *self, GPIO_Pin_t pin, I_EXTI_Callback_t cb, void *context);

/**
 * @brief Limpia el flag de pending de una línea EXTI.
 * @param self Puntero opaco (sin usar).
 * @param pin Pin GPIO cuyo pending se va a limpiar.
 * @return ERR_OK siempre.
 *
 * Internamente: Llama a __HAL_GPIO_EXTI_CLEAR_IT(pin) para borrar la pendencia.
 */
static Result_t stm32u5_exti_clear_pending(void *self, GPIO_Pin_t pin);

/* ===== V-Table estática ===== */

static const I_EXTI_Vtable s_stm32u5_exti_vtable = {
    .Init = stm32u5_exti_init,
    .DeInit = stm32u5_exti_deinit,
    .Enable = stm32u5_exti_enable,
    .Disable = stm32u5_exti_disable,
    .RegisterCallback = stm32u5_exti_register_callback,
    .ClearPending = stm32u5_exti_clear_pending};

/* ===== Instancia Singleton ===== */

static I_EXTI s_stm32u5_exti_iface = {
    .vtable = &s_stm32u5_exti_vtable,
    .impl =(void *) &s_stm32u5_exti_vtable /* puntero válido para validación */
};

/**
 * @brief Retorna la instancia singleton de I_EXTI para STM32U5.
 * @return I_EXTI* Instancia singleton no-NULL.
 */
I_EXTI *Bsp_Stm32U5_Exti_GetInterface(void)
{
    return &s_stm32u5_exti_iface;
}

/* ===== Funciones auxiliares privadas ===== */

/**
 * @brief Mapea un GPIO_Pin_t a su correspondiente IRQn_Type.
 * @param pin Pin GPIO (GPIO_PIN_0 a GPIO_PIN_15).
 * @return IRQn_Type IRQ number para NVIC.
 *
 * Pines 0-4 tienen IRQn dedicadas, 5-9 comparten EXTI9_5_IRQn, 10-15 comparten EXTI15_10_IRQn.
 */
static IRQn_Type get_irqn_from_pin(GPIO_Pin_t pin)
{
    if (pin == GPIO_PIN_0)
        return EXTI0_IRQn;
    if (pin == GPIO_PIN_1)
        return EXTI1_IRQn;
    if (pin == GPIO_PIN_2)
        return EXTI2_IRQn;
    if (pin == GPIO_PIN_3)
        return EXTI3_IRQn;
    if (pin == GPIO_PIN_4)
        return EXTI4_IRQn;
    if (pin == GPIO_PIN_5)
        return EXTI5_IRQn;
    if (pin == GPIO_PIN_6)
        return EXTI6_IRQn;
    if (pin == GPIO_PIN_7)
        return EXTI7_IRQn;
    if (pin == GPIO_PIN_8)
        return EXTI8_IRQn;
    if (pin == GPIO_PIN_9)
        return EXTI9_IRQn;
    if (pin == GPIO_PIN_10)
        return EXTI10_IRQn;
    if (pin == GPIO_PIN_11)
        return EXTI11_IRQn;
    if (pin == GPIO_PIN_12)
        return EXTI12_IRQn;
    if (pin == GPIO_PIN_13)
        return EXTI13_IRQn;
    if (pin == GPIO_PIN_14)
        return EXTI14_IRQn;
    if (pin == GPIO_PIN_15)
        return EXTI15_IRQn;

    /* Valor por defecto si pin inválido */
    return EXTI15_IRQn;
}

/**
 * @brief Calcula índice en tabla s_exti_slots desde GPIO_Pin_t.
 * @param pin Pin GPIO (debe ser potencia de 2: 0x0001, 0x0002, 0x0004, ..., 0x8000).
 * @return uint32_t Índice (0-15).
 *
 * Realiza lógica de bit-scan para convertir pin a índice.
 */
static uint32_t get_pin_index(GPIO_Pin_t pin)
{
    uint32_t idx = 0;
    while (pin >>= 1)
        idx++;
    return idx;
}

/* ===== Implementaciones privadas ===== */

/**
 * @brief Implementación privada de EXTI_Init en V-Table.
 *
 * Mapea la configuración abstracta I_EXTI_Config_t a estructuras STM32 HAL:
 *   - I_EXTI_TRIGGER_RISING → GPIO_MODE_IT_RISING
 *   - I_EXTI_TRIGGER_FALLING → GPIO_MODE_IT_FALLING
 *   - I_EXTI_TRIGGER_RISING_FALLING → GPIO_MODE_IT_RISING_FALLING
 *
 * Secuencia:
 *   1. Valida punteros (config, config->port).
 *   2. Construye GPIO_InitTypeDef con pin, trigger mapeado, y NOPULL.
 *   3. Llama HAL_GPIO_Init() para configurar el pin con modo interrupción.
 *   4. Obtiene IRQn mapeado del pin (get_irqn_from_pin).
 *   5. Configura prioridad en NVIC y habilita IRQ.
 */
static Result_t stm32u5_exti_init(void *self, const I_EXTI_Config_t *config)
{
    (void)self;
    if (config == NULL || config->port == NULL)
        return ERR_NULL_POINTER;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = config->pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    /* Mapear modo de trigger a modo GPIO HAL */
    switch (config->trigger)
    {
    case I_EXTI_TRIGGER_RISING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        break;
    case I_EXTI_TRIGGER_FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        break;
    case I_EXTI_TRIGGER_RISING_FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
    default:
        return ERR_INVALID_PARAM;
    }

    HAL_GPIO_Init((GPIO_TypeDef *)config->port, &GPIO_InitStruct);

    /* Configurar y habilitar en NVIC */
    IRQn_Type irq = get_irqn_from_pin(config->pin);
    HAL_NVIC_SetPriority(irq, config->priority, config->subPriority);
    HAL_NVIC_EnableIRQ(irq);

    return ERR_OK;
}

static Result_t stm32u5_exti_deinit(void *self, GPIO_Pin_t pin)
{
    (void)self;
    (void)pin;
    /* HAL_GPIO_DeInit no solo limpia EXTI sino el pin completo; reservado para GPIO */
    return ERR_OK;
}

static Result_t stm32u5_exti_enable(void *self, GPIO_Pin_t pin)
{
    (void)self;
    /* Habilita la IRQ en NVIC para que se procesen interrupciones del pin */
    HAL_NVIC_EnableIRQ(get_irqn_from_pin(pin));
    return ERR_OK;
}

static Result_t stm32u5_exti_disable(void *self, GPIO_Pin_t pin)
{
    (void)self;
    /* Deshabilita la IRQ en NVIC; las interrupciones del pin serán ignoradas */
    HAL_NVIC_DisableIRQ(get_irqn_from_pin(pin));
    return ERR_OK;
}

static Result_t stm32u5_exti_register_callback(void *self, GPIO_Pin_t pin, I_EXTI_Callback_t cb, void *context)
{
    (void)self;
    uint32_t idx = get_pin_index(pin);
    if (idx >= MAX_EXTI_LINES)
        return ERR_INVALID_PARAM;

    /* Almacena callback y contexto en tabla; será invocado en ISR Rising/Falling */
    s_exti_slots[idx].cb = cb;
    s_exti_slots[idx].context = context;
    return ERR_OK;
}

static Result_t stm32u5_exti_clear_pending(void *self, GPIO_Pin_t pin)
{
    (void)self;
    /* Borra el flag de pending en EXTI para ese pin */
    __HAL_GPIO_EXTI_CLEAR_IT(pin);
    return ERR_OK;
}

/**
 * @brief Dispatcher de interrupciones EXTI Rising (llamado por HAL en ISR).
 * @param GPIO_Pin Bits correspondientes a pins que causaron interrupción rising.
 *
 * Busca el índice del pin (get_pin_index), verifica si hay callback registrado
 * en s_exti_slots[idx], e invoca el callback con su contexto asociado.
 *
 * @note Ejecuta en contexto de ISR; mantener callback breve y sin bloqueos.
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    uint32_t idx = get_pin_index(GPIO_Pin);
    if (idx < MAX_EXTI_LINES && s_exti_slots[idx].cb != NULL)
    {
        s_exti_slots[idx].cb(s_exti_slots[idx].context, (GPIO_Pin_t)GPIO_Pin);
    }
}

/**
 * @brief Dispatcher de interrupciones EXTI Falling (llamado por HAL en ISR).
 * @param GPIO_Pin Bits correspondientes a pins que causaron interrupción falling.
 *
 * Delega al mismo manejador que rising; el callback es agnóstico al tipo de trigger.
 * El tipo (rising/falling) se especifica en la configuración (I_EXTI_Config_t)
 * pero no se propaga al callback, que es más simple así.
 *
 * @note Ejecuta en contexto de ISR; mantener callback breve y sin bloqueos.
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    HAL_GPIO_EXTI_Rising_Callback(GPIO_Pin);
}
