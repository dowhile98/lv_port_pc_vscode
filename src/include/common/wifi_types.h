/**
 * @file wifi_types.h
 * @brief Tipos portables para configuración y estado WiFi.
 *
 * @note ISP: Este header contiene SOLO tipos relacionados con WiFi.
 *       No depende de HAL, CycloneTCP ni ningún driver específico.
 *       Es seguro incluirlo en Domain y Application layers.
 */

#ifndef WIFI_TYPES_H
#define WIFI_TYPES_H

/* C/C++ portable static assertion: _Static_assert is C11, static_assert is C++ */
#if defined(__cplusplus) && !defined(_Static_assert)
#define _Static_assert static_assert
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

/* ===== Constants ===== */

/** @brief Máxima longitud de SSID (IEEE 802.11: 32 octetos). */
#define WIFI_SSID_MAX_LEN 32U

/** @brief Máxima longitud de password WPA2 (63 + null). */
#define WIFI_PWD_MAX_LEN 64U

/** @brief Máxima longitud de password AP (32 + null). */
#define WIFI_AP_PWD_MAX_LEN 32U

/** @brief Canal AP por defecto. */
#define WIFI_DEFAULT_AP_CHANNEL 6U

/** @brief Máximo de clientes AP por defecto. */
#define WIFI_DEFAULT_AP_MAX_CONN 4U

/** @brief IP de AP por defecto. */
#define WIFI_DEFAULT_AP_IP "192.168.8.1"

/** @brief Máscara AP por defecto. */
#define WIFI_DEFAULT_AP_MASK "255.255.255.0"

/** @brief IP estática STA por defecto (no configurada: todo ceros). */
#define WIFI_DEFAULT_STA_IP "192.168.1.40"

/** @brief Máscara STA por defecto. */
#define WIFI_DEFAULT_STA_MASK "255.255.255.0"

/** @brief Gateway STA por defecto. */
#define WIFI_DEFAULT_STA_GATEWAY "192.168.1.1"

    /* ===== Enumerations ===== */

    /**
     * @brief Modos de operación WiFi.
     */
    typedef enum
    {
        WIFI_MODE_DISABLED = 0, /**< WiFi apagado. */
        WIFI_MODE_STA = 1,      /**< Solo Station (cliente). */
        WIFI_MODE_AP = 2,       /**< Solo Access Point. */
        WIFI_MODE_APSTA = 3,    /**< Station + AP simultáneo. */
    } WifiMode_t;

    /* ===== Structs ===== */

    /**
     * @brief Configuración WiFi persistible en EEPROM.
     *
     * @note Tamaño: 248 bytes (packed).
     *       v3.0: añade campos de IP estática por interfaz y flag DHCP por interfaz.
     *
     * Layout (packed):
     *   sta_ssid        [  0..31 ]  32 bytes
     *   sta_pwd         [ 32..95 ]  64 bytes
     *   ap_ssid         [ 96..127]  32 bytes
     *   ap_pwd          [128..159]  32 bytes
     *   mode            [160]        1 byte   WifiMode_t
     *   ap_channel      [161]        1 byte
     *   ap_max_conn     [162]        1 byte
     *   sta_use_dhcp    [163]        1 byte   1=DHCP, 0=static (STA)
     *   ap_use_dhcp_srv [164]        1 byte   1=run DHCP server on AP
     *   reserved        [165..167]   3 bytes
     *   sta_ip          [168..183]  16 bytes  "a.b.c.d\0"
     *   sta_mask        [184..199]  16 bytes  "a.b.c.d\0"
     *   sta_gateway     [200..215]  16 bytes  "a.b.c.d\0"
     *   ap_ip           [216..231]  16 bytes  "a.b.c.d\0"
     *   ap_mask         [232..247]  16 bytes  "a.b.c.d\0"
     *                             = 248 bytes total
     */
    typedef struct __attribute__((packed)) WifiConfig
    {
        char sta_ssid[WIFI_SSID_MAX_LEN]; /**< SSID de la red STA (null-terminated). */
        char sta_pwd[WIFI_PWD_MAX_LEN];   /**< Password STA (null-terminated). */
        char ap_ssid[WIFI_SSID_MAX_LEN];  /**< SSID del AP propio (null-terminated). */
        char ap_pwd[WIFI_AP_PWD_MAX_LEN]; /**< Password del AP (null-terminated). */
        uint8_t mode;                     /**< WifiMode_t: DISABLED/STA/AP/APSTA. */
        uint8_t ap_channel;               /**< Canal del AP: 1–13. */
        uint8_t ap_max_conn;              /**< Máx. clientes conectados al AP (1–4). */
        uint8_t sta_use_dhcp;             /**< 1 = DHCP en STA, 0 = IP estática. */
        uint8_t ap_use_dhcp_server;       /**< 1 = habilitar servidor DHCP en AP. */
        uint8_t wifi_enable;              /**< 1 = WiFi habilitado, 0 = deshabilitado. */
        uint8_t wifi_timeout_minutes;     /**< Auto-disable timeout (0=always on, 1-60 minutes). */
        uint8_t reserved[1];              /**< Reservado para expansión futura. */
        char sta_ip[16];                  /**< IP estática STA  e.g. "192.168.0.40\0". */
        char sta_mask[16];                /**< Máscara STA       e.g. "255.255.255.0\0". */
        char sta_gateway[16];             /**< Gateway STA       e.g. "192.168.0.1\0". */
        char ap_ip[16];                   /**< IP del AP         e.g. "192.168.8.1\0". */
        char ap_mask[16];                 /**< Máscara AP        e.g. "255.255.255.0\0". */
    } WifiConfig_t;

    /* Compile-time size assertion */
    _Static_assert(sizeof(WifiConfig_t) == 248U, "WifiConfig_t debe ser exactamente 248 bytes");

#ifdef __cplusplus
}
#endif

#endif /* WIFI_TYPES_H */
