/**
 * @file digital_input_adapter.h
 * @brief Adaptador de entradas digitales - Implementa IDigitalInputSource.
 *
 * Integra lwbtn (lightweight button library) con I_GPIO interface.
 * Proporciona debouncing, detección de long-press y Observer Pattern.
 *
 * @note Arquitectura:
 *       - lwbtn maneja: debouncing, long-press detection, state machine
 *       - Adapter maneja: GPIO abstraction, callback dispatch, config mapping
 *
 * @note Thread-safety:
 *       - Process() NO es reentrant → llamar desde un solo thread (DigitalInputAO).
 *       - Callbacks ejecutan en contexto de Process() (polling thread, ~20ms).
 *
 * @note Extensibilidad:
 *       - Añadir entrada: solo modificar DigitalInputAdapterConfig_t array.
 *       - Cambiar debouncing: modificar lwbtn config (lwbtn_opts.h).
 */

#ifndef DIGITAL_INPUT_ADAPTER_H
#define DIGITAL_INPUT_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "interfaces/i_gpio.h"
#include "interfaces/i_digital_input_source.h"
#include "lwbtn/lwbtn.h"
#include "osal/osal.h"
#include "application/services/display_backlight_service.h"
/* ===== Configuración por Entrada ===== */

/**
 * @brief Configuración de una entrada digital individual.
 */
typedef struct
{
    DigitalInputID_t id;     /**< Identificador único */
    DigitalInputType_t type; /**< Tipo funcional (button/alarm) */
    GPIO_Port_t port;        /**< Puerto GPIO (opaco, definido por BSP) */
    GPIO_Pin_t pin;          /**< Pin GPIO (ej: GPIO_PIN_0) */
    bool active_low;         /**< true = activo en LOW, false = activo en HIGH */
    uint16_t long_press_ms;  /**< Umbral long-press en ms (0 = deshabilitado) */
} DigitalInputConfig_t;

/* ===== Configuración del Adapter ===== */

/**
 * @brief Configuración de inicialización del DigitalInputAdapter.
 */
typedef struct
{
    I_GPIO *gpio;                             /**< Interfaz GPIO (inyectada, no NULL) */
    const DigitalInputConfig_t *inputs;       /**< Array de configs de entrada (no NULL) */
    uint8_t input_count;                      /**< Número de entradas (típico: 5) */
    uint16_t debounce_ms;                     /**< Tiempo de debounce global (típico: 20ms) */
    DisplayBacklightService_t *backlight_svc; /**< Servicio de backlight (NULL = sin gestión) */
} DigitalInputAdapterConfig_t;

/* ===== Observer Pattern: Subscriber Storage ===== */

/**
 * @brief Máximo de callbacks por input (antes era por evento, ahora por input completo).
 * @note Con bitmask, todos los callbacks de un input están en un array 1D.
 * @note Si se excede, RegisterCallback() retorna ERR_BUSY.
 */
#ifndef DI_MAX_CALLBACKS_PER_INPUT
#define DI_MAX_CALLBACKS_PER_INPUT 20
#endif

/* Legacy define para compatibilidad durante migración */
#define DI_MAX_CALLBACKS_PER_EVENT DI_MAX_CALLBACKS_PER_INPUT

/**
 * @brief Subscriber slot (Observer Pattern) con bitmask de eventos.
 */
typedef struct
{
    DigitalInputCallback_t callback; /**< Función callback */
    void *context;                   /**< Contexto del observador */
    bool active;                     /**< true = slot ocupado, false = libre */
    uint32_t event_mask;             /**< Máscara de eventos suscritos (bitmask: DI_EVENT_PRESS | DI_EVENT_RELEASE, etc.) */
} DigitalInputSubscriber_t;

/* ===== Estructura Principal del Adapter ===== */

/**
 * @brief Contexto por botón para callbacks lwbtn.
 * @note Se pasa como btn->arg, permitiendo acceso a adapter + config desde callbacks.
 */
typedef struct
{
    struct DigitalInputAdapter *adapter; /**< Puntero al adapter (para acceder a GPIO) */
    const DigitalInputConfig_t *config;  /**< Puntero a config de este input (para port/pin) */
} DI_ButtonContext_t;

/**
 * @brief Instancia del DigitalInputAdapter.
 *
 * @note Tamaño aproximado:
 *       - lwbtn_btn_t[5] = ~100 bytes
 *       - DI_ButtonContext_t[5] = ~80 bytes
 *       - Subscribers[5][4][4] = ~320 bytes (5 inputs * 4 events * 4 slots)
 *       Total: ~600 bytes (stack-allocatable)
 */
typedef struct DigitalInputAdapter
{
    /* Interfaz pública */
    IDigitalInputSource iface;

    /* Dependencias inyectadas */
    I_GPIO *gpio;
    const DigitalInputConfig_t *inputs;
    uint8_t input_count;
    DisplayBacklightService_t *backlight_svc; /**< Servicio de backlight (NULL = sin gestión) */

    /* Estado interno lwbtn */
    lwbtn_btn_t buttons[DI_ID_COUNT];                /**< Instancias lwbtn por entrada */
    DI_ButtonContext_t button_contexts[DI_ID_COUNT]; /**< Contextos para callbacks (adapter+config) */

    /* Observer Pattern: Callbacks[input_id][slot] con event_mask por subscriber (2D) */
    DigitalInputSubscriber_t subscribers[DI_ID_COUNT][DI_MAX_CALLBACKS_PER_INPUT]; /**< 2D: [id][slot] + event_mask bitmask */

} DigitalInputAdapter_t;

/* ===== API Pública ===== */

/**
 * @brief Inicializa el DigitalInputAdapter.
 *
 * @pre config != NULL, config->gpio != NULL, config->inputs != NULL, config->input_count > 0
 * @pre Todos los pines GPIO ya configurados como INPUT con pull apropiado (Init_GPIO previo).
 *
 * @param[in,out] self   Instancia a inicializar (no NULL).
 * @param[in]     config Configuración (no NULL).
 *
 * @return ERR_OK si inicialización exitosa.
 * @return ERR_NULL_POINTER si algún parámetro es NULL.
 * @return ERR_INVALID_PARAM si input_count == 0 o > DI_ID_COUNT.
 *
 * @note Inicializa lwbtn internamente con:
 *       - Debounce time = config->debounce_ms
 *       - Read callback = di_adapter_get_state (usa I_GPIO)
 *       - Event callback = di_adapter_on_event (dispatch a observers)
 *
 * @note Después de Init(), llamar RegisterCallback() para observadores antes de Start().
 */
Result_t DigitalInputAdapter_Init(
    DigitalInputAdapter_t *self,
    const DigitalInputAdapterConfig_t *config);

/**
 * @brief De-inicializa DigitalInputAdapter y libera recursos
 * @param[in] self Puntero al adapter
 * @return ERR_OK si exitoso
 */
Result_t DigitalInputAdapter_Deinit(DigitalInputAdapter_t *self);

/**
 * @brief Obtiene interfaz IDigitalInputSource asociada.
 * @return Puntero a interfaz o NULL si self es NULL.
 */
IDigitalInputSource *DigitalInputAdapter_GetInterface(DigitalInputAdapter_t *self);

#endif /* DIGITAL_INPUT_ADAPTER_H */
