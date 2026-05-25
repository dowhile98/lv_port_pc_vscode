/**
 * @file  network_port_types.h
 * @brief Tipos de datos para puertos de red — solo stdint/stdbool.
 *
 * @details
 * Tipos compartidos por INetworkPort_t e INetworkLink_t.
 * Este archivo NUNCA incluye cabeceras de vendor (CycloneTCP, lwIP, HAL, RTOS).
 * Puede compilarse en host para unit tests sin ninguna dependencia adicional.
 *
 * @note SRP: Solo define tipos de datos de configuración y estado.
 *       No contiene lógica ni funciones.
 *
 * @version 1.0.0
 * @date    2026-03-12
 */

#ifndef NETWORK_PORT_TYPES_H
#define NETWORK_PORT_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

    /* ===== IP Mode ===== */

    /**
     * @brief Modo de obtención de dirección IP para un puerto.
     */
    typedef enum NetworkIpMode
    {
        NETWORK_IP_MODE_DHCP = 0,   /**< Obtener IP por DHCP (cliente).    */
        NETWORK_IP_MODE_STATIC = 1, /**< IP estática configurada por Init. */
    } NetworkIpMode_t;

    /* ===== Port Type ===== */

    /**
     * @brief Tipo de puerto de red.
     */
    typedef enum NetworkPortType
    {
        NETWORK_PORT_TYPE_WIFI_STA = 0,  /**< WiFi Station (cliente).            */
        NETWORK_PORT_TYPE_WIFI_AP = 1,   /**< WiFi Access Point (punto de acceso).*/
        NETWORK_PORT_TYPE_ETH = 2,       /**< Ethernet cableado.                 */
        NETWORK_PORT_TYPE_USB_RNDIS = 3, /**< USB RNDIS (puerta de enlace USB).  */
    } NetworkPortType_t;

    /* ===== IPv4 Address ===== */

    /**
     * @brief Dirección IPv4 como cuatro octetos.
     * @note  Representación independiente de vendor (sin in_addr, Ipv4Addr, etc.).
     */
    typedef struct NetworkIpv4Addr
    {
        uint8_t octet[4]; /**< [0]=MSB … [3]=LSB  (ej. 192.168.1.1 → {192,168,1,1}) */
    } NetworkIpv4Addr_t;

    /* ===== Port IP Config ===== */

    /**
     * @brief Configuración de dirección IP para un puerto.
     * @note  Usada en INetworkPort_t.ApplyIpConfig().
     */
    typedef struct NetworkPortIpConfig
    {
        NetworkIpMode_t mode;          /**< DHCP o estática.          */
        NetworkIpv4Addr_t ip_addr;     /**< IP estática (si mode=STATIC). */
        NetworkIpv4Addr_t subnet_mask; /**< Máscara de subred.         */
        NetworkIpv4Addr_t gateway;     /**< Default gateway.           */
        NetworkIpv4Addr_t dns_server;  /**< Servidor DNS primario.     */
    } NetworkPortIpConfig_t;

    /* ===== Port Config ===== */

    /**
     * @brief Configuración completa de un puerto de red.
     * @note  Usada en INetworkPort_t.Configure().
     */
    typedef struct NetworkPortConfig
    {
        NetworkPortType_t port_type;     /**< STA / AP / ETH.           */
        char hostname[32];               /**< Hostname de la interfaz.  */
        uint8_t mac_addr[6];             /**< MAC address (0 = usar HW).*/
        bool mac_override;               /**< true → aplicar mac_addr.  */
        NetworkPortIpConfig_t ip_config; /**< Configuración IP.         */
        bool dhcp_server_en;             /**< true → habilitar DHCP srv (solo AP). */
        /* ── WiFi association parameters (STA / AP only — ignored for ETH) ── */
        uint8_t wifi_mode;      /**< WifiCtrlMode_t: mode to set on Configure() (0 = skip SetMode). */
        char wifi_ssid[33];     /**< SSID to connect/announce. Empty string → skip WiFi association. */
        char wifi_password[65]; /**< Password (nul-terminated). Empty = open network.               */
        uint8_t wifi_channel;   /**< AP channel (0 = use default 6).                                */
        uint8_t wifi_max_conn;  /**< AP max clients (0 = use default 4).                            */
        bool wifi_hidden_ssid;  /**< true → AP does not broadcast SSID in beacons.                  */
    } NetworkPortConfig_t;

    /* ===== Port Status ===== */

    /**
     * @brief Estado observable de un puerto de red (solo lectura).
     * @note  Retornado por INetworkPort_t.GetStatus().
     */
    typedef struct NetworkPortStatus
    {
        bool link_up;                  /**< true si el enlace físico está activo. */
        bool has_ip_address;           /**< true si tiene IP asignada (DHCP o estática). */
        NetworkIpv4Addr_t ip_addr;     /**< IP actualmente asignada.               */
        NetworkIpv4Addr_t subnet_mask; /**< Máscara actualmente activa.            */
        NetworkIpv4Addr_t gateway;     /**< Gateway actualmente activo.            */
        uint8_t mac_addr[6];           /**< MAC address efectiva del puerto.       */
    } NetworkPortStatus_t;

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_PORT_TYPES_H */
