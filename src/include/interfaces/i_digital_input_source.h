/**
 * @file i_digital_input_source.h
 * @brief Domain interface para lectura de entradas digitales.
 *
 * Define contrato polimórfico para fuentes de entrada digital (botones, alarmas).
 * Implementa Observer Pattern para notificación de eventos.
 *
 * @note Compatible con polling (Process() llamado periódicamente)
 *       o interrupt-driven (EXTI callbacks internos).
 *
 * @note Thread-safety: ReadInput() puede llamarse desde cualquier thread.
 *                      Callbacks ejecutan en contexto de Process() o ISR.
 */

#ifndef I_DIGITAL_INPUT_SOURCE_H
#define I_DIGITAL_INPUT_SOURCE_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include <stddef.h>
/* ===== Tipos de Entrada Digital ===== */

/**
 * @brief Clasificación funcional de entradas.
 */
typedef enum
{
    DI_TYPE_BUTTON,      /**< Botón UI (navegación, interacción usuario) */
    DI_TYPE_ALARM_INPUT, /**< Entrada de alarma crítica (safety) */
    DI_TYPE_GENERIC      /**< Entrada de propósito general */
} DigitalInputType_t;

/* ===== Identificadores de Entrada ===== */

/**
 * @brief IDs únicos por entrada física.
 * @note Mapeo a pines GPIO definido en DigitalInputAdapterConfig_t.
 */
typedef enum
{
    DI_ID_BUTTON_ONOFF = 0,   /**< Botón ON/OFF (PBOUT) */
    DI_ID_BUTTON_ENTER = 1,   /**< Botón Enter (DB1) */
    DI_ID_BUTTON_UP = 2,      /**< Botón Up (DB0) */
    DI_ID_BUTTON_DOWN = 3,    /**< Botón Down (DB2) */
    DI_ID_SIDE_BUTTON = 4,    /**< Botón lateral (SIDE_BTN) */
    DI_ID_ALARM_OVERTEMP = 5, /**< Alarma temperatura unidad potencia (SYS_RXD) */
    DI_ID_COUNT = 6
} DigitalInputID_t;

/* ===== Eventos de Entrada ===== */

/**
 * @brief Tipos de eventos detectados en entradas.
 * @note Equivalencias lwbtn:
 *       - DI_EVENT_PRESS      = LWBTN_EVT_ONPRESS
 *       - DI_EVENT_LONG_PRESS = Legacy (no usado)
 *       - DI_EVENT_RELEASE    = LWBTN_EVT_ONRELEASE
 *       - DI_EVENT_CLICK      = LWBTN_EVT_ONCLICK
 *       - DI_EVENT_KEEPALIVE  = LWBTN_EVT_KEEPALIVE
 */
#if 0
typedef enum
{
    DI_EVENT_PRESS = 0,      /**< Presión inicial (post-debounce) */
    DI_EVENT_LONG_PRESS = 1, /**< Presión mantenida (legacy, deprecado) */
    DI_EVENT_RELEASE = 2,    /**< Liberación del input */
    DI_EVENT_CLICK = 3,      /**< Click completo (press + release rápido) */
    DI_EVENT_KEEPALIVE = 4,  /**< Auto-repeat periódico (ACTION-012) */
    DI_EVENT_COUNT = 5
} DigitalInputEvent_t;
#endif
typedef enum
{
    DI_EVENT_NONE = 0,
    DI_EVENT_PRESS = (1 << 0),      // 0x01
    DI_EVENT_LONG_PRESS = (1 << 1), // 0x02
    DI_EVENT_RELEASE = (1 << 2),    // 0x04
    DI_EVENT_CLICK = (1 << 3),      // 0x08
    DI_EVENT_KEEPALIVE = (1 << 4),  // 0x10
    DI_EVENT_ALL = 0x1F,            // Todos los eventos
    DI_EVENT_MAX_BIT = 5
} DigitalInputEvent_t;

/* ===== Metadata de Eventos ===== */

/**
 * @brief Metadata de evento de entrada digital
 * @note Se pasa a callbacks para proveer contexto adicional.
 */
typedef struct
{
    uint32_t timestamp_ms;      /**< Timestamp del evento (desde OSAL, ms) */
    uint16_t click_count;       /**< Número de clicks consecutivos (0 si no aplica) */
    uint16_t keepalive_count;   /**< Número de keep-alive desde press (0 si no aplica) */
    uint32_t press_duration_ms; /**< Duración de presión en ms (solo RELEASE/CLICK, 0 si no aplica) */
} DigitalInputEventData_t;

/* ===== Observer Pattern: Callback Signature ===== */

/**
 * @brief Callback notificado cuando ocurre evento en entrada (ACTION-012: Updated signature).
 *
 * @warning DEBE ser rápido (<50μs). NO bloquear con mutex/semaphore.
 * @warning Puede ejecutar en ISR context (si impl usa EXTI) o polling thread.
 *
 * @param context     Contexto del observador (void* para flexibilidad).
 * @param input_id    ID de la entrada que generó el evento.
 * @param event       Tipo de evento detectado.
 * @param data        Metadata del evento (no NULL, contiene timestamp + counters).
 *
 * @note Responsabilidad del callback:
 *       - Botones UI: Postear evento a UI queue (non-blocking).
 *       - Alarmas: Postear evento a safety controller (RelayAO).
 *
 * @note BREAKING CHANGE (ACTION-012): timestamp ahora dentro de data->timestamp_ms.
 *
 * @example
 * void on_button_press(void *ctx, DigitalInputID_t id,
 *                      DigitalInputEvent_t evt,
 *                      const DigitalInputEventData_t *data) {
 *     if (id == DI_ID_BUTTON_UP && evt == DI_EVENT_KEEPALIVE) {
 *         if (data->keepalive_count > 5) {
 *             ui_increment_fast();  // Aceleración después de 500ms
 *         }
 *     }
 * }
 */
typedef void (*DigitalInputCallback_t)(
    void *context,
    DigitalInputID_t input_id,
    DigitalInputEvent_t event,
    const DigitalInputEventData_t *data);

/* ===== Vtable de IDigitalInputSource ===== */

/**
 * @brief Vtable para operaciones de lectura de entradas digitales.
 */
typedef struct IDigitalInputSource_Vtable
{
    /**
     * @brief Lee estado instantáneo de una entrada.
     * @note Thread-safe. NO bloqueante.
     *
     * @param[in]  self      Puntero a implementación concreta.
     * @param[in]  id        ID de entrada a leer.
     * @param[out] out_state true = activo (post-debounce), false = inactivo.
     *
     * @return ERR_OK si éxito.
     * @return ERR_NULL_POINTER si self o out_state es NULL.
     * @return ERR_INVALID_PARAM si id >= DI_ID_COUNT.
     */
    Result_t (*ReadInput)(void *self, DigitalInputID_t id, bool *out_state);

    /**
     * @brief Obtiene tipo funcional de entrada.
     * @note Útil para filtrado (ej: solo procesar botones, ignorar alarmas).
     */
    Result_t (*GetInputType)(void *self, DigitalInputID_t id, DigitalInputType_t *out_type);

    /**
     * @brief Registra callback para uno o más eventos en entrada (bitmask).
     * @note Observer Pattern: Múltiples callbacks pueden registrarse.
     * @note Callbacks se invocan en orden de registro.
     * @note Si el mismo callback+context ya existe, hace merge de máscaras (OR).
     *
     * @param[in] self       Puntero a implementación.
     * @param[in] id         ID de entrada.
     * @param[in] event_mask Máscara de eventos a observar (ej: DI_EVENT_PRESS | DI_EVENT_RELEASE).
     * @param[in] callback   Función a llamar cuando ocurra evento.
     * @param[in] context    Contexto pasado a callback (puede ser NULL).
     *
     * @return ERR_OK si callback registrado o máscara mergeada.
     * @return ERR_NULL_POINTER si self o callback es NULL.
     * @return ERR_INVALID_PARAM si event_mask == DI_EVENT_NONE.
     * @return ERR_BUSY si no hay slots disponibles (MAX_CALLBACKS alcanzado).
     *
     * @example RegisterCallback(src, DI_ID_BUTTON_UP, DI_EVENT_PRESS | DI_EVENT_KEEPALIVE, cb, ctx);
     */
    Result_t (*RegisterCallback)(
        void *self,
        DigitalInputID_t id,
        DigitalInputEvent_t event_mask,
        DigitalInputCallback_t callback,
        void *context);

    /**
     * @brief Desregistra eventos específicos de un callback (partial removal).
     * @note Remueve eventos indicados en event_mask de la suscripción.
     * @note Si la máscara queda vacía (DI_EVENT_NONE), libera el slot completamente.
     * @note Solo procesa la primera coincidencia de callback.
     *
     * @param[in] self       Puntero a implementación.
     * @param[in] id         ID de entrada.
     * @param[in] callback   Callback a desuscribir.
     * @param[in] event_mask Máscara de eventos a remover (ej: DI_EVENT_PRESS).
     *
     * @return ERR_OK si eventos removidos o slot liberado.
     * @return ERR_NULL_POINTER si self o callback es NULL.
     * @return ERR_ERROR si callback no encontrado.
     *
     * @example UnregisterCallback(src, DI_ID_BUTTON_UP, cb, DI_EVENT_PRESS); // Remueve PRESS, mantiene otros
     */
    Result_t (*UnregisterCallback)(
        void *self,
        DigitalInputID_t id,
        DigitalInputCallback_t callback,
        DigitalInputEvent_t event_mask);

    /**
     * @brief Procesa polling de entradas (debouncing + detección eventos).
     * @note DEBE llamarse periódicamente (típico: cada 20ms) si impl usa polling.
     * @note Si impl usa EXTI, puede ser no-op.
     *
     * @warning NO es reentrant. Llamar desde un solo thread (DigitalInputAO).
     *
     * @return ERR_OK si procesamiento exitoso.
     */
    Result_t (*Process)(void *self);

} IDigitalInputSource_Vtable;

/**
 * @brief Interfaz pública IDigitalInputSource.
 */
typedef struct IDigitalInputSource
{
    const IDigitalInputSource_Vtable *vtable;
    void *impl; /**< Puntero a implementación concreta (ej: DigitalInputAdapter_t) */
} IDigitalInputSource;

/* ===== Inline Helpers (Type-Safe Wrappers) ===== */

static inline Result_t DigitalInputSource_ReadInput(
    IDigitalInputSource *iface,
    DigitalInputID_t id,
    bool *out_state)
{
    if (iface == NULL || iface->vtable == NULL || out_state == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->ReadInput(iface->impl, id, out_state);
}

static inline Result_t DigitalInputSource_GetInputType(
    IDigitalInputSource *iface,
    DigitalInputID_t id,
    DigitalInputType_t *out_type)
{
    if (iface == NULL || iface->vtable == NULL || out_type == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->GetInputType(iface->impl, id, out_type);
}

static inline Result_t DigitalInputSource_RegisterCallback(
    IDigitalInputSource *iface,
    DigitalInputID_t id,
    DigitalInputEvent_t event_mask,
    DigitalInputCallback_t callback,
    void *context)
{
    if (iface == NULL || iface->vtable == NULL || callback == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->RegisterCallback(iface->impl, id, event_mask, callback, context);
}

static inline Result_t DigitalInputSource_UnregisterCallback(
    IDigitalInputSource *iface,
    DigitalInputID_t id,
    DigitalInputCallback_t callback,
    DigitalInputEvent_t event_mask)
{
    if (iface == NULL || iface->vtable == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->UnregisterCallback(iface->impl, id, callback, event_mask);
}

static inline Result_t DigitalInputSource_Process(IDigitalInputSource *iface)
{
    if (iface == NULL || iface->vtable == NULL)
    {
        return ERR_NULL_POINTER;
    }
    return iface->vtable->Process(iface->impl);
}

#endif /* I_DIGITAL_INPUT_SOURCE_H */
