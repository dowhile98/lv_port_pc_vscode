/**
 * @file i_buzzer_control.h
 * @brief Buzzer Control Interface - High-Level Hardware Abstraction
 * @version 1.0.0
 * @date 2026-02-03
 *
 * @details
 * Interfaz de alto nivel para control de buzzer/beeper. Abstrae el hardware
 * específico (PWM/GPIO) y proporciona control de patrones de sonido.
 *
 * Esta interfaz es consumida por:
 * - BuzzerAO (Active Object para gestión asíncrona)
 * - BuzzerNotificationService (lógica de eventos del dominio)
 *
 * Implementada por:
 * - BuzzerAdapter (producción: usa I_GPIO + Timer)
 * - MockBuzzerAdapter (testing)
 *
 * @note Thread-safe: Las implementaciones deben garantizar atomicidad.
 * @note Calls desde ISR: Solo permitidos si doc indica "ISR-safe".
 *
 * @see BuzzerAdapter
 * @see BuzzerAO
 */

#ifndef I_BUZZER_CONTROL_H
#define I_BUZZER_CONTROL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"

    /*============================================================================*
     * TYPES & ENUMS
     *============================================================================*/

    /**
     * @brief Buzzer tone patterns (future expansion for melodies)
     */
    typedef enum
    {
        BUZZER_TONE_SILENT = 0,            /**< No sound */
        BUZZER_PATTERN_WIFI_ENABLED = 10,  /**< WiFi enabled (2 beeps) */
        BUZZER_PATTERN_WIFI_DISABLED = 11, /**< WiFi disabled (1 long beep) */
        BUZZER_PATTERN_WIFI_TIMEOUT = 12,  /**< WiFi auto-disabled by timeout (3 beeps) */
        BUZZER_TONE_STANDARD,              /**< Standard continuous beep */
        BUZZER_TONE_PULSE,                 /**< Pulsed beep (fast on/off) */
        BUZZER_TONE_WARNING,               /**< Warning tone (slow pulse) */
        BUZZER_TONE_ERROR,                 /**< Error tone (double beep) */
        BUZZER_TONE_SUCCESS                /**< Success tone (ascending) */
    } BuzzerTone_t;

    /**
     * @brief Buzzer event/command for queue-based control
     */
    typedef struct
    {
        BuzzerTone_t tone;    /**< Tone pattern to play */
        uint16_t duration_ms; /**< Duration in milliseconds (0 = continuous) */
        uint8_t priority;     /**< Priority (0=lowest, 255=highest) */
    } BuzzerCommand_t;

    /**
     * @brief Forward declaration of opaque implementation pointer
     */
    typedef struct IBuzzerControl_Impl IBuzzerControl_Impl;

    /**
     * @brief VTable for IBuzzerControl interface
     */
    typedef struct
    {
        /**
         * @brief Activa el buzzer con tono y duración especificados
         * @param self Puntero a implementación concreta
         * @param tone Patrón de tono a reproducir
         * @param duration_ms Duración en milisegundos (0 = indefinido)
         * @return ERR_OK si exitoso, ERR_BUSY si buzzer ocupado, ERR_INVALID_PARAM si tone inválido
         * @note Thread-safe. ISR-safe si duration_ms > 0.
         */
        Result_t (*Beep)(IBuzzerControl_Impl *self, BuzzerTone_t tone, uint16_t duration_ms);

        /**
         * @brief Detiene inmediatamente el buzzer
         * @param self Puntero a implementación concreta
         * @return ERR_OK siempre exitoso
         * @note Thread-safe. ISR-safe.
         */
        Result_t (*Stop)(IBuzzerControl_Impl *self);

        /**
         * @brief Verifica si el buzzer está actualmente activo
         * @param self Puntero a implementación concreta
         * @return true si buzzer activo, false si silencio
         * @note Thread-safe. ISR-safe.
         */
        bool (*IsActive)(const IBuzzerControl_Impl *self);

        /**
         * @brief Habilita/deshabilita el buzzer globalmente (mute)
         * @param self Puntero a implementación concreta
         * @param enabled true para habilitar, false para deshabilitar
         * @return ERR_OK
         * @note Thread-safe. Cuando disabled, Beep() retorna ERR_OK pero no suena.
         */
        Result_t (*SetEnabled)(IBuzzerControl_Impl *self, bool enabled);

        /**
         * @brief Obtiene estado de habilitación del buzzer
         * @param self Puntero a implementación concreta
         * @return true si habilitado, false si mute
         */
        bool (*IsEnabled)(const IBuzzerControl_Impl *self);

        /**
         * @brief Actualiza estado del buzzer (polling para auto-stop)
         * @param self Puntero a implementación concreta
         * @return ERR_OK si exitoso
         * @note Thread-safe. Debe llamarse periódicamente (~20ms) para auto-stop.
         * @note Implementaciones sin auto-stop pueden retornar ERR_OK sin hacer nada.
         */
        Result_t (*Update)(IBuzzerControl_Impl *self);

    } IBuzzerControl_VTable;

    /**
     * @brief Interface handle (combina vtable + implementación)
     */
    typedef struct
    {
        const IBuzzerControl_VTable *vtable; /**< Virtual table */
        IBuzzerControl_Impl *impl;           /**< Opaque implementation pointer */
    } IBuzzerControl;

    /*============================================================================*
     * INLINE FUNCTIONS (Polimorfismo en C)
     *============================================================================*/

    /**
     * @brief Macro para llamar Beep polimórficamente
     */
    static inline Result_t BuzzerControl_Beep(IBuzzerControl *iface, BuzzerTone_t tone, uint16_t duration_ms)
    {
        if (!iface || !iface->vtable || !iface->vtable->Beep)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Beep(iface->impl, tone, duration_ms);
    }

    /**
     * @brief Macro para llamar Stop polimórficamente
     */
    static inline Result_t BuzzerControl_Stop(IBuzzerControl *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->Stop)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Stop(iface->impl);
    }

    /**
     * @brief Macro para llamar IsActive polimórficamente
     */
    static inline bool BuzzerControl_IsActive(const IBuzzerControl *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->IsActive)
        {
            return false;
        }
        return iface->vtable->IsActive(iface->impl);
    }

    /**
     * @brief Macro para llamar SetEnabled polimórficamente
     */
    static inline Result_t BuzzerControl_SetEnabled(IBuzzerControl *iface, bool enabled)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetEnabled)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetEnabled(iface->impl, enabled);
    }

    /**
     * @brief Macro para llamar IsEnabled polimórficamente
     */
    static inline bool BuzzerControl_IsEnabled(const IBuzzerControl *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->IsEnabled)
        {
            return false;
        }
        return iface->vtable->IsEnabled(iface->impl);
    }

    /**
     * @brief Macro para llamar Update polimórficamente (auto-stop polling)
     */
    static inline Result_t BuzzerControl_Update(IBuzzerControl *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->Update)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Update(iface->impl);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_BUZZER_CONTROL_H */
