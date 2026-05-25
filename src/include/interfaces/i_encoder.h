/**
 * @file i_encoder.h
 * @brief Interfaz abstracta para dispositivo tipo encoder rotativo.
 *
 * Permite abstraer cualquier tipo de entrada de encoder:
 * - Encoder físico con cuadratura
 * - Tres botones UP/DOWN/ENTER (implementación del proyecto)
 * - Joystick analógico
 *
 * @note Esta interfaz es el contrato entre UI_Port_Indev y el BSP.
 *       UI_Port no sabe si el hardware es un encoder real o botones.
 *
 * @author Tecna Smart Lab
 * @date   23 de Febrero 2026
 */
#ifndef INCLUDE_INTERFACES_I_ENCODER_H_
#define INCLUDE_INTERFACES_I_ENCODER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IEncoder IEncoder_t;

    /**
     * @brief V-Table para el encoder / dispositivo de entrada rotativo.
     */
    typedef struct
    {
        /**
         * @brief Lee el estado actual del encoder.
         *
         * Debe ser llamado desde el @c read_cb de LVGL (contexto de tarea, no ISR).
         * La implementación debe ser no-bloqueante.
         *
         * @param[in]  self     Puntero a la instancia del encoder.
         * @param[out] delta    Cambio de posición desde la última lectura.
         *                      +1 = giro derecha / botón UP
         *                      -1 = giro izquierda / botón DOWN
         *                       0 = sin movimiento
         * @param[out] pressed  true si el botón central (ENTER/click) está presionado.
         *
         * @return ERR_OK siempre (lectura no bloqueante).
         */
        Result_t (*Poll)(IEncoder_t *self, int32_t *delta, bool *pressed);

    } IEncoder_Vtable_t;

    /**
     * @brief Instancia del encoder. Puntero de contexto + V-Table.
     */
    struct IEncoder
    {
        const IEncoder_Vtable_t *vtable;
        void *impl;
    };

    /* ===== Helper defensivo inline ===== */

    /**
     * @brief Verifica que una instancia de IEncoder_t es válida y no tiene NULLs.
     *
     * @param[in] enc Puntero a la instancia a validar.
     * @return true si todos los punteros son válidos.
     */
    static inline bool encoder_is_valid(const IEncoder_t *enc)
    {
        return (enc != NULL) && (enc->vtable != NULL) && (enc->impl != NULL) && (enc->vtable->Poll != NULL);
    }

/**
 * @brief Macro de despacho para Poll.
 *
 * Asume que @c enc ya fue validado con encoder_is_valid().
 */
#define Encoder_Poll(enc, delta, pressed) \
    ((enc)->vtable->Poll((enc), (delta), (pressed)))

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_INTERFACES_I_ENCODER_H_ */
