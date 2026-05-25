/**
 * @file bsp_logging_output.h
 * @brief Output callbacks para logging system (UART/ITM/SWO).
 * @version 1.0.0
 *
 * @note Provee callbacks de output para el logging adapter.
 *       Soporta UART (terminal serie) y ITM (SWO debug).
 */

#ifndef BSP_LOGGING_OUTPUT_H
#define BSP_LOGGING_OUTPUT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief Output callback para UART (terminal serie).
     * @param[in] ch Carácter a escribir.
     * @param[in] arg Argumento de usuario (puede ser UART_HandleTypeDef*).
     * @return ch en éxito, 0 para terminar.
     *
     * @note Usa HAL_UART_Transmit en modo bloqueante (para logging simple).
     *       Para producción, considerar DMA o circular buffer.
     */
    int BSP_Logging_OutputUART(int ch, void *arg);

    /**
     * @brief Output callback para ITM (SWO - debug trace).
     * @param[in] ch Carácter a escribir.
     * @param[in] arg Argumento de usuario (no usado).
     * @return ch en éxito, 0 si ITM no disponible.
     *
     * @note Solo funciona si debugger está conectado y SWO configurado.
     *       Zero overhead si debugger desconectado.
     */
    int BSP_Logging_OutputITM(int ch, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LOGGING_OUTPUT_H */
