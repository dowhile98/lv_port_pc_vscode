/**
 * @file i_wifi_transport.h
 * @brief Interfaz de transporte WiFi (V-Table pattern para DIP compliance)
 * @version 1.0.0
 * @date 2026-02-17
 * @author Tecna Smart Lab
 *
 * @details
 * Interfaz abstracta para transporte SPI/SDIO con ESP32 (ESP-Hosted).
 * Desacopla el dominio/aplicación de la implementación concreta del transporte.
 *
 * @note Sigue el patrón V-Table (C-style polymorphism).
 * @note Permite testear Domain/Application sin hardware real.
 * @note Cumple con DIP: High-level modules depend on abstraction, not concrete.
 */

#ifndef I_WIFI_TRANSPORT_H
#define I_WIFI_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "hal_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Callback para evento DATA_READY (EXTI).
     * @note Ejecuta en contexto ISR, debe ser <50μs.
     * @param context Contexto de usuario (registrado en RegisterDataReadyCallback)
     */
    typedef void (*WifiTransport_DataReadyCallback_t)(void *context);

    /**
     * @brief Interfaz abstracta de transporte WiFi (V-Table).
     *
     * @details
     * Define operaciones de transporte SPI/SDIO para ESP-Hosted:
     * - Transmit/Receive: Transferencia de datos
     * - Reset: Control del pin RESET de ESP32
     * - Handshake: Consulta de pin HANDSHAKE
     * - Data Ready: Registro de callback para EXTI
     *
     * @note Implementations: ESP_SpiTransportAdapter, ESP_SdioTransportAdapter (future)
     * @note Context pointer apunta a la instancia concreta del adapter.
     */
    typedef struct IWifiTransport_VTable IWifiTransport_VTable;

    typedef struct
    {
        /**
         * @brief V-Table de funciones del transporte.
         */
        const IWifiTransport_VTable *vtable;

        /**
         * @brief Contexto opaco (apunta a instancia concreta del adapter).
         * @note No acceder directamente, usar solo a través de vtable.
         */
        void *context;

    } IWifiTransport;

    /**
     * @brief V-Table de funciones de transporte WiFi.
     */
    struct IWifiTransport_VTable
    {
        /**
         * @brief Transmite datos por SPI/SDIO (TX only).
         *
         * @param[in] self Instancia de la interfaz (context interno).
         * @param[in] data Buffer de datos a transmitir (no NULL).
         * @param[in] len Longitud de datos en bytes (>0).
         *
         * @return ERR_OK si éxito,
         *         ERR_NULL_POINTER si data es NULL,
         *         ERR_INVALID_PARAM si len == 0,
         *         ERR_BUSY si transporte ocupado,
         *         ERR_ERROR si falla hardware.
         *
         * @note Modo bloqueante (timeout interno ~500ms).
         * @note Thread-safe si el adapter usa mutex.
         */
        Result_t (*Transmit)(void *self,
                             const uint8_t *data,
                             uint32_t len);

        /**
         * @brief Recibe datos por SPI/SDIO (RX only).
         *
         * @param[in] self Instancia de la interfaz.
         * @param[out] buffer Buffer de recepción (no NULL).
         * @param[in] buffer_size Capacidad del buffer en bytes.
         * @param[out] received Bytes recibidos realmente (puede ser NULL).
         *
         * @return ERR_OK si éxito,
         *         ERR_NULL_POINTER si buffer es NULL,
         *         ERR_INVALID_PARAM si buffer_size == 0,
         *         ERR_TIMEOUT si no hay datos disponibles,
         *         ERR_ERROR si falla hardware.
         *
         * @note Modo bloqueante (timeout interno ~500ms).
         */
        Result_t (*Receive)(void *self,
                            uint8_t *buffer,
                            uint32_t buffer_size,
                            uint32_t *received);

        /**
         * @brief Transacción full-duplex TX+RX (SPI típico).
         *
         * @param[in] self Instancia de la interfaz.
         * @param[in] tx_data Buffer de transmisión (puede ser NULL si tx_len == 0).
         * @param[in] tx_len Longitud de TX en bytes.
         * @param[out] rx_buffer Buffer de recepción (puede ser NULL si rx_size == 0).
         * @param[in] rx_size Capacidad del buffer RX.
         * @param[out] rx_received Bytes RX recibidos (puede ser NULL).
         *
         * @return ERR_OK si éxito,
         *         ERR_NULL_POINTER si parámetros inválidos,
         *         ERR_BUSY si transporte ocupado,
         *         ERR_ERROR si falla hardware.
         *
         * @note Modo bloqueante (timeout ~500ms).
         * @note Útil para SPI donde TX y RX son simultáneos.
         */
        Result_t (*TransmitReceive)(void *self,
                                    const uint8_t *tx_data,
                                    uint32_t tx_len,
                                    uint8_t *rx_buffer,
                                    uint32_t rx_size,
                                    uint32_t *rx_received);

        /**
         * @brief Controla el pin RESET de ESP32.
         *
         * @param[in] self Instancia de la interfaz.
         * @param[in] assert_reset true = RESET activo (ESP32 en reset),
         *                         false = RESET inactivo (ESP32 running).
         *
         * @return ERR_OK si éxito,
         *         ERR_ERROR si falla GPIO.
         *
         * @note Secuencia típica: Assert(true) → delay(10ms) → Assert(false).
         */
        Result_t (*ResetESP32)(void *self, bool assert_reset);

        /**
         * @brief Lee el estado del pin HANDSHAKE de ESP32.
         *
         * @param[in] self Instancia de la interfaz.
         * @param[out] handshake_active true si HANDSHAKE activo, false si inactivo.
         *
         * @return ERR_OK si éxito,
         *         ERR_NULL_POINTER si handshake_active es NULL,
         *         ERR_ERROR si falla GPIO.
         *
         * @note Handshake activo = ESP32 listo para recibir datos.
         */
        Result_t (*ReadHandshake)(void *self, bool *handshake_active);

        /**
         * @brief Lee el estado del pin DATA_READY de ESP32.
         *
         * @param[in] self Instancia de la interfaz.
         * @param[out] data_ready true si DATA_READY activo, false si inactivo.
         *
         * @return ERR_OK si éxito,
         *         ERR_NULL_POINTER si data_ready es NULL,
         *         ERR_ERROR si falla GPIO.
         *
         * @note Data ready activo = ESP32 tiene datos para enviar.
         */
        Result_t (*ReadDataReady)(void *self, bool *data_ready);

        /**
         * @brief Registra callback para interrupción DATA_READY (EXTI).
         *
         * @param[in] self Instancia de la interfaz.
         * @param[in] callback Función callback (puede ser NULL para deshabilitar).
         * @param[in] context Contexto de usuario pasado al callback.
         *
         * @return ERR_OK si éxito,
         *         ERR_ERROR si falla configuración de EXTI.
         *
         * @note Callback ejecuta en contexto ISR, debe ser <50μs.
         * @note Llamar con callback=NULL desregistra la interrupción.
         */
        Result_t (*RegisterDataReadyCallback)(void *self,
                                              WifiTransport_DataReadyCallback_t callback,
                                              void *context);
    };

    /*============================================================================*
     * INLINE VALIDATION HELPERS
     *============================================================================*/

    /**
     * @brief Valida que la interfaz es válida (no NULL, vtable completa).
     *
     * @param[in] iface Interfaz a validar.
     *
     * @return true si válida, false si NULL o vtable incompleta.
     */
    static inline bool WifiTransport_IsValid(const IWifiTransport *iface)
    {
        return (iface != NULL &&
                iface->vtable != NULL &&
                iface->context != NULL &&
                iface->vtable->Transmit != NULL &&
                iface->vtable->Receive != NULL &&
                iface->vtable->TransmitReceive != NULL &&
                iface->vtable->ResetESP32 != NULL &&
                iface->vtable->ReadHandshake != NULL &&
                iface->vtable->ReadDataReady != NULL &&
                iface->vtable->RegisterDataReadyCallback != NULL);
    }

    /*============================================================================*
     * INTERFACE METHODS (Call vtable functions)
     *============================================================================*/

    /**
     * @brief Transmite datos (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_Transmit(IWifiTransport *iface,
                                                  const uint8_t *data,
                                                  uint32_t len)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Transmit(iface->context, data, len);
    }

    /**
     * @brief Recibe datos (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_Receive(IWifiTransport *iface,
                                                 uint8_t *buffer,
                                                 uint32_t buffer_size,
                                                 uint32_t *received)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Receive(iface->context, buffer, buffer_size, received);
    }

    /**
     * @brief Transacción TX+RX (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_TransmitReceive(IWifiTransport *iface,
                                                         const uint8_t *tx_data,
                                                         uint32_t tx_len,
                                                         uint8_t *rx_buffer,
                                                         uint32_t rx_size,
                                                         uint32_t *rx_received)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->TransmitReceive(iface->context, tx_data, tx_len,
                                              rx_buffer, rx_size, rx_received);
    }

    /**
     * @brief Controla RESET de ESP32 (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_ResetESP32(IWifiTransport *iface, bool assert_reset)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->ResetESP32(iface->context, assert_reset);
    }

    /**
     * @brief Lee estado HANDSHAKE (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_ReadHandshake(IWifiTransport *iface, bool *handshake_active)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->ReadHandshake(iface->context, handshake_active);
    }

    /**
     * @brief Lee estado DATA_READY (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_ReadDataReady(IWifiTransport *iface, bool *data_ready)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->ReadDataReady(iface->context, data_ready);
    }

    /**
     * @brief Registra callback DATA_READY (wrapper sobre vtable).
     */
    static inline Result_t WifiTransport_RegisterDataReadyCallback(IWifiTransport *iface,
                                                                   WifiTransport_DataReadyCallback_t callback,
                                                                   void *context)
    {
        if (!WifiTransport_IsValid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->RegisterDataReadyCallback(iface->context, callback, context);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_WIFI_TRANSPORT_H */
