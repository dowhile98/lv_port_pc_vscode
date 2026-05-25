/**
 * @file  i_network_port.h
 * @brief Interfaz V-Table para un puerto de red (STA / AP / ETH).
 *
 * @details
 * Abstrae la configuración y estado de un puerto físico o lógico de red.
 * Permite sustituir CycloneTCP, lwIP, u otras pilas TCP/IP sin modificar
 * las capas de aplicación o dominio.
 *
 * Contrato LSP:
 *   - Configure(self=NULL, ...)       → ERR_NULL_POINTER
 *   - Configure(self, cfg=NULL)       → ERR_NULL_POINTER
 *   - GetStatus(self=NULL, ...)       → ERR_NULL_POINTER
 *   - GetStatus(self, out=NULL)       → ERR_NULL_POINTER
 *   - ApplyIpConfig(self=NULL, ...)   → ERR_NULL_POINTER
 *   - ApplyIpConfig(self, ip=NULL)    → ERR_NULL_POINTER
 *   - Configure() antes de Init()     → ERR_ERROR (no inicializado)
 *   - Reassociate(self=NULL, ...)      → ERR_NULL_POINTER
 *   - Reassociate(self, cfg=NULL)      → ERR_NULL_POINTER
 *
 * @note DIP: Este archivo NO incluye ningún tipo de CycloneTCP, lwIP, HAL ni RTOS.
 *       Las implementaciones concretas viven en src/infrastructure/adapters/network/.
 *
 * @note ISP: Interfaz separada de INetworkLink_t (observación pasiva del enlace).
 *       INetworkPort_t es el plano de control; INetworkLink_t es el plano de datos.
 *
 * @version 1.0.0
 * @date    2026-03-12
 */

#ifndef I_NETWORK_PORT_H
#define I_NETWORK_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stddef.h>
#include "hal_types.h"
#include "interfaces/network_port_types.h"

    /* ===== V-Table ===== */

    typedef struct INetworkPort_VTable
    {
        /**
         * @brief Configura el puerto con los parámetros indicados.
         *
         * @details Aplica tipo de puerto, MAC, IP mode, hostname y DHCP server.
         *          Debe llamarse una vez tras Init() del stack TCP/IP.
         *
         * @param[in] self  Contexto de la implementación (no NULL).
         * @param[in] cfg   Configuración del puerto (no NULL).
         *
         * @return ERR_OK           Configuración aplicada correctamente.
         * @return ERR_NULL_POINTER Si self o cfg son NULL.
         * @return ERR_ERROR        Si el stack subyacente rechazó la config.
         * @return ERR_BUSY         Si el puerto ya está configurado y activo.
         *
         * @note No thread-safe. Llamar desde el composition root antes de Start().
         */
        Result_t (*Configure)(void *self, const NetworkPortConfig_t *cfg);

        /**
         * @brief Lee el estado actual del puerto.
         *
         * @param[in]  self  Contexto de la implementación (no NULL).
         * @param[out] out   Estado a rellenar (no NULL, se sobreescribe completamente).
         *
         * @return ERR_OK           Estado leído correctamente.
         * @return ERR_NULL_POINTER Si self o out son NULL.
         * @return ERR_NOT_INIT     Si el puerto no ha sido configurado.
         *
         * @note Thread-safe respecto a lecturas concurrentes.
         */
        Result_t (*GetStatus)(const void *self, NetworkPortStatus_t *out);

        /**
         * @brief Aplica (o reconfigura) la configuración IP del puerto.
         *
         * @details Permite cambiar entre DHCP/estática en caliente.
         *          Útil para reconexión con nueva IP tras desapilado de DHCP.
         *
         * @param[in] self    Contexto de la implementación (no NULL).
         * @param[in] ip_cfg  Nueva configuración IP (no NULL).
         *
         * @return ERR_OK           IP reconfigured correctamente.
         * @return ERR_NULL_POINTER Si self o ip_cfg son NULL.
         * @return ERR_NOT_INIT     Si el puerto no ha sido configurado.
         * @return ERR_ERROR        Si el stack rechazó la nueva configuración.
         *
         * @note No se interrumpe el tráfico existente inmediatamente;
         *       el stack aplica la nueva IP en el siguiente ciclo de red.
         */
        Result_t (*ApplyIpConfig)(void *self, const NetworkPortIpConfig_t *ip_cfg);

        /**
         * @brief Vuelve a lanzar la asociación WiFi sin reinicializar el driver.
         *
         * @details Llamado por NetworkAO en estado LINK_DOWN para reconectar
         *          sin pasar por netConfigInterface() de nuevo.
         *
         *          - STA port: llama ConnectAP() con las credenciales de la
         *            última Configure().
         *          - AP  port: llama StartSoftAP() si el AP no está activo.
         *          - ETH port: no-op (ERR_OK).
         *
         * @param[in] self  Contexto de la implementación (no NULL).
         * @param[in] cfg   Configuración original del puerto (no NULL).
         *
         * @return ERR_OK           Reconexión lanzada.
         * @return ERR_NULL_POINTER Si self o cfg son NULL.
         * @return ERR_NOT_INIT     Si el puerto no ha sido configurado previamente.
         * @return ERR_ERROR        Si el stack/driver rechazó la petición.
         *
         * @note Non-blocking: ConnectAP dispatch regresa cuando el ESP32 ack'd
         *       el comando.  El evento CONNECTED llega de forma asíncrona.
         * @note Debe llamarse desde el hilo del NetworkAO (no ISR).
         */
        Result_t (*Reconnect)(void *self, const NetworkPortConfig_t *cfg);

        /**
         * @brief Re-aplica modo WiFi y credenciales tras un reinicio del módulo.
         *
         * @details Diferencia clave con Reconnect():
         *   - Reconnect() reintenta la asociación cuando el enlace cae (STA lost AP).
         *   - Reassociate() se usa cuando el módulo WiFi (ESP32) se reinició y ha
         *     perdido toda su configuración: llama primero SetMode(), luego
         *     ConnectAP() o StartSoftAP().  No pasa por netConfigInterface().
         *
         *   - STA port: SetMode(cfg->wifi_mode) → ConnectAP(ssid, pwd).
         *   - AP  port: SetMode(cfg->wifi_mode) → StartSoftAP(ssid, pwd, ch...).
         *   - ETH port: no-op (ERR_OK).
         *
         * @param[in] self  Contexto de la implementación (no NULL).
         * @param[in] cfg   Configuración original del puerto (no NULL).
         *
         * @return ERR_OK           Reconfiguración lanzada.
         * @return ERR_NULL_POINTER Si self o cfg son NULL.
         * @return ERR_NOT_INIT     Si el puerto no ha sido configurado previamente.
         * @return ERR_ERROR        Si el driver rechazó la petición.
         *
         * @note Non-blocking: el resultado de la asociación llegará como evento async.
         * @note Debe llamarse desde el hilo del NetworkAO (no ISR).
         */
        Result_t (*Reassociate)(void *self, const NetworkPortConfig_t *cfg);

    } INetworkPort_VTable_t;

    /* ===== Interface Instance ===== */

    typedef struct INetworkPort
    {
        const INetworkPort_VTable_t *vtable;
        void *context;
    } INetworkPort_t;

    /* ===== Inline Wrappers ===== */

    /**
     * @brief Configura el puerto. Guarda NULL-safe.
     */
    static inline Result_t NetworkPort_Configure(INetworkPort_t *port,
                                                 const NetworkPortConfig_t *cfg)
    {
        if ((port == NULL) || (port->vtable == NULL) || (port->vtable->Configure == NULL))
        {
            return ERR_NULL_POINTER;
        }
        return port->vtable->Configure(port->context, cfg);
    }

    /**
     * @brief Lee el estado del puerto. Guarda NULL-safe.
     */
    static inline Result_t NetworkPort_GetStatus(const INetworkPort_t *port,
                                                 NetworkPortStatus_t *out)
    {
        if ((port == NULL) || (port->vtable == NULL) || (port->vtable->GetStatus == NULL))
        {
            return ERR_NULL_POINTER;
        }
        return port->vtable->GetStatus(port->context, out);
    }

    /**
     * @brief Aplica configuración IP. Guarda NULL-safe.
     */
    static inline Result_t NetworkPort_ApplyIpConfig(INetworkPort_t *port,
                                                     const NetworkPortIpConfig_t *ip_cfg)
    {
        if ((port == NULL) || (port->vtable == NULL) || (port->vtable->ApplyIpConfig == NULL))
        {
            return ERR_NULL_POINTER;
        }
        return port->vtable->ApplyIpConfig(port->context, ip_cfg);
    }

    /**
     * @brief Lanza reconexión WiFi. Guarda NULL-safe.
     *        Si vtable->Reconnect es NULL devuelve ERR_OK (p.ej. ETH o fake simple).
     */
    static inline Result_t NetworkPort_Reconnect(INetworkPort_t *port,
                                                 const NetworkPortConfig_t *cfg)
    {
        if ((port == NULL) || (port->vtable == NULL))
        {
            return ERR_NULL_POINTER;
        }
        if (port->vtable->Reconnect == NULL)
        {
            return ERR_OK; /* implementación opcional — no-op por defecto */
        }
        return port->vtable->Reconnect(port->context, cfg);
    }

    /**
     * @brief Re-aplica modo WiFi y credenciales tras reinicio del módulo. Guarda NULL-safe.
     *        Si vtable->Reassociate es NULL devuelve ERR_OK (p.ej. ETH o fake sin WiFi).
     */
    static inline Result_t NetworkPort_Reassociate(INetworkPort_t *port,
                                                   const NetworkPortConfig_t *cfg)
    {
        if ((port == NULL) || (port->vtable == NULL))
        {
            return ERR_NULL_POINTER;
        }
        if (port->vtable->Reassociate == NULL)
        {
            return ERR_OK; /* implementación opcional — no-op por defecto */
        }
        return port->vtable->Reassociate(port->context, cfg);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_NETWORK_PORT_H */
