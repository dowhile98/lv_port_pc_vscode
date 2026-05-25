/**
 * @file    i_wifi_control.h
 * @brief   Interfaz de control WiFi (V-Table pattern para DIP compliance).
 * @version 2.0.0
 * @date    2026-02-19
 * @author  Tecna Smart Lab
 *
 * @details
 * Define el contrato abstracto para control del plano de control WiFi:
 * modo, conexión a AP, SoftAP y eventos de conectividad.
 *
 * El único archivo que incluye `ctrl_api.h` es `wifi_control_adapter.c`.
 * Ningún código fuera de `Third_Party/esp_hosted/` accede a tipos internos
 * de ESP-Hosted (ctrl_cmd_t, wifi_mode_e, etc.) — todos están encapsulados
 * detrás de esta interfaz.
 *
 * @par Capas permitidas para incluir este header
 * - `src/infrastructure/adapters/wifi/` (implementación del adapter)
 * - `src/infrastructure/adapters/wifi/` (NicDriver de CycloneTCP)
 * - `src/infrastructure/di/` (DI container)
 * - `tests/unit/` y `tests/fakes/`
 *
 * @note NO incluir en `App/`, `Domain/`, ni `Third_Party/esp_hosted/`.
 * @note Cumple DIP: los módulos de alto nivel dependen de esta abstracción,
 *       no de la implementación concreta (ctrl_api.c).
 */

#ifndef I_WIFI_CONTROL_H
#define I_WIFI_CONTROL_H

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "hal_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> /* NULL */

#ifdef __cplusplus
extern "C"
{
#endif

    /*============================================================================*
     * TYPES — Basic
     *============================================================================*/

    /**
     * @brief Modo de operación WiFi.
     */
    typedef enum
    {
        WIFI_CTRL_MODE_NONE = 0,  /**< Sin modo WiFi activo */
        WIFI_CTRL_MODE_STA = 1,   /**< Station (cliente) */
        WIFI_CTRL_MODE_AP = 2,    /**< SoftAP (punto de acceso) */
        WIFI_CTRL_MODE_APSTA = 3, /**< STA + SoftAP simultáneos */
    } WifiCtrlMode_t;

    /**
     * @brief Modo de ahorro de energía WiFi.
     * Maps to wifi_ps_type_e in ctrl_api.h.
     */
    typedef enum
    {
        WIFI_CTRL_PS_NONE = 0,      /**< Sin ahorro de energía */
        WIFI_CTRL_PS_MIN_MODEM = 1, /**< Ahorro mínimo (DTIM period=1) */
        WIFI_CTRL_PS_MAX_MODEM = 2, /**< Ahorro máximo (DTIM period máximo) */
    } WifiPsMode_t;

    /**
     * @brief Feature de ESP-Hosted que se puede habilitar/deshabilitar.
     * Maps to hosted_features_t in ctrl_api.h.
     */
    typedef enum
    {
        WIFI_CTRL_FEATURE_WIFI = 0,          /**< WiFi subsystem */
        WIFI_CTRL_FEATURE_BT = 1,            /**< Bluetooth subsystem */
        WIFI_CTRL_FEATURE_NETWORK_SPLIT = 2, /**< Network split mode */
    } WifiFeature_t;

    /*============================================================================*
     * TYPES — Configuration structs
     *============================================================================*/

    /**
     * @brief Configuración para conexión a AP como estación (STA).
     */
    typedef struct
    {
        char ssid[33];           /**< SSID del AP (nul-terminado, máx 32 chars) */
        char pwd[65];            /**< Contraseña (nul-terminado, máx 64 chars) */
        bool is_wpa3;            /**< true si el AP usa WPA3 */
        uint8_t listen_interval; /**< Intervalo de escucha DTIM (0 = default) */
    } WifiStaConfig_t;

    /**
     * @brief Configuración para iniciar SoftAP.
     *
     * @note `encryption_mode` mapea internamente a `wifi_auth_mode_e`:
     *       0=Open,1=WEP,2=WPA-PSK,3=WPA2-PSK,4=WPA/WPA2,5=Enterprise,
     *       6=WPA3-PSK,7=WPA2/WPA3.
     * @note `bandwidth`: 0=HT20, 1=HT40.
     */
    typedef struct
    {
        char ssid[33];           /**< SSID del SoftAP (máx 32 chars + nul) */
        char pwd[65];            /**< Contraseña (máx 64 chars + nul) */
        uint8_t channel;         /**< Canal WiFi (1-13) */
        uint8_t encryption_mode; /**< Tipo de autenticación */
        uint8_t max_connections; /**< Máximo de clientes (1-10) */
        bool ssid_hidden;        /**< true = no emitir SSID en beacons */
        uint8_t bandwidth;       /**< 0=HT20, 1=HT40 */
    } WifiApConfig_t;

    /**
     * @brief Vendor-specific IE para SoftAP beacons/probes.
     *
     * @note `element_id` debe ser 0xDD.
     * @note `payload` debe permanecer válido durante la llamada a SetVendorIE().
     * @note `ie_type`: 0=Beacon,1=ProbeReq,2=ProbeResp,3=AssocReq,4=AssocResp.
     * @note `ie_id`: 0 o 1.
     */
    typedef struct
    {
        bool enable;             /**< true=activar IE, false=eliminar IE */
        uint8_t ie_type;         /**< Tipo de trama (wifi_vendor_ie_type_e) */
        uint8_t ie_id;           /**< ID del IE (0 o 1) */
        uint8_t element_id;      /**< Debe ser 0xDD */
        uint8_t vendor_oui[3];   /**< OUI del vendedor */
        uint8_t vendor_oui_type; /**< Tipo OUI del vendedor */
        const uint8_t *payload;  /**< Payload del IE (caller-owned) */
        uint16_t payload_len;    /**< Longitud del payload en bytes */
    } WifiVendorIE_t;

    /**
     * @brief Configuración del heartbeat de ESP-Hosted.
     */
    typedef struct
    {
        bool enable;           /**< true=habilitar, false=deshabilitar */
        uint32_t duration_sec; /**< Intervalo de heartbeat en segundos */
    } WifiHeartbeatConfig_t;

    /**
     * @brief Código de país 802.11d.
     */
    typedef struct
    {
        char country[4]; /**< ISO 3166-1 alpha-2 + nul (ej. "US\0") */
        bool ieee80211d_enabled;
    } WifiCountryCode_t;

    /**
     * @brief Estado DHCP/DNS de una interfaz.
     * @note `iface`: 0=STA, 1=AP.
     */
    typedef struct
    {
        int32_t iface; /**< Interfaz: 0=STA, 1=AP */
        bool net_link_up;
        bool dhcp_up;
        char dhcp_ip[64];
        char dhcp_nm[64];
        char dhcp_gw[64];
        bool dns_up;
        char dns_ip[64];
        int32_t dns_type;
    } WifiDhcpDnsStatus_t;

    /*============================================================================*
     * TYPES — Response/Status structs
     *============================================================================*/

    /**
     * @brief Entrada de resultado de escaneo de AP.
     * @note `bssid` es string "XX:XX:XX:XX:XX:XX" (17 chars + nul).
     * @note `auth_mode` mapea a wifi_auth_mode_e.
     */
    typedef struct
    {
        char ssid[33];
        char bssid[18]; /**< BSSID string "XX:XX:XX:XX:XX:XX" */
        int32_t rssi;
        uint8_t channel;
        uint8_t auth_mode;
    } WifiApScanEntry_t;

    /**
     * @brief Estado de la conexión actual del ESP32 como STA.
     */
    typedef struct
    {
        char ssid[33];
        char bssid[18];
        bool is_wpa3_supported;
        int32_t rssi;
        uint8_t channel;
        uint8_t auth_mode;
        uint16_t listen_interval;
        char status[14];  /**< "success", "failure", "not_connected" */
        char out_mac[18]; /**< MAC de la interfaz STA */
    } WifiApStatus_t;

    /**
     * @brief Configuración/estado actual del SoftAP del ESP32.
     */
    typedef struct
    {
        char ssid[33];
        char pwd[65];
        uint8_t channel;
        uint8_t encryption_mode;
        uint8_t max_connections;
        bool ssid_hidden;
        uint8_t bandwidth;
        char out_mac[18]; /**< MAC de la interfaz AP */
    } WifiSoftApStatus_t;

    /**
     * @brief Cliente conectado al SoftAP.
     * @note `mac` es string "XX:XX:XX:XX:XX:XX".
     */
    typedef struct
    {
        char mac[18];
        int32_t rssi;
    } WifiApClient_t;

    /**
     * @brief Versión del firmware ESP-Hosted.
     * @note `project_name` — 3 chars útiles; usar [4] con nul para strings.
     */
    typedef struct
    {
        char project_name[4];
        uint8_t major_1;
        uint8_t major_2;
        uint8_t minor;
        uint8_t patch_1;
        uint8_t patch_2;
    } WifiFwVersion_t;

    /*============================================================================*
     * TYPES — Callbacks
     *============================================================================*/

    /**
     * @brief Callback llamado una vez por AP durante un escaneo.
     * @param[in] entry Entrada de resultado (válido solo durante la llamada).
     * @param[in] ctx   Contexto opaco del caller.
     */
    typedef void (*WifiScanResultCallback_t)(const WifiApScanEntry_t *entry, void *ctx);

    /**
     * @brief Callback llamado una vez por cliente conectado al SoftAP.
     * @param[in] client Info del cliente (válido solo durante la llamada).
     * @param[in] ctx    Contexto opaco del caller.
     */
    typedef void (*WifiApClientCallback_t)(const WifiApClient_t *client, void *ctx);

    /**
     * @brief Callback para evento de heartbeat.
     * @param[in] hb_num Número de secuencia del heartbeat.
     * @param[in] ctx    Contexto opaco del caller.
     */
    typedef void (*WifiHeartbeatCallback_t)(uint32_t hb_num, void *ctx);

    /**
     * @brief Callback para evento de actualización DHCP/DNS.
     * @param[in] status Estado DHCP/DNS actualizado (válido solo durante la llamada).
     * @param[in] ctx    Contexto opaco del caller.
     */
    typedef void (*WifiDhcpUpdateCallback_t)(const WifiDhcpDnsStatus_t *status, void *ctx);

    /**
     * @brief Callback para mensajes RPC personalizados del slave.
     * @param[in] msg_id   ID del mensaje RPC.
     * @param[in] data     Datos del mensaje (válido solo durante la llamada).
     * @param[in] data_len Longitud de los datos en bytes.
     * @param[in] ctx      Contexto opaco del caller.
     */
    typedef void (*WifiCustomRpcCallback_t)(uint32_t msg_id,
                                            const uint8_t *data,
                                            uint16_t data_len,
                                            void *ctx);

    /*============================================================================*
     * V-TABLE
     *============================================================================*/

    /* Forward declaration */
    typedef struct IWifiControl_t IWifiControl_t;

    /**
     * @brief V-Table completa de la interfaz de control WiFi.
     *
     * Todos los punteros de función reciben `ctx` como primer argumento.
     * `ctx` es el campo `context` del `IWifiControl_t`.
     *
     * @note Thread-safety: Las llamadas de control son síncronas y bloquean
     *       hasta recibir respuesta del ESP32 (DEFAULT_CTRL_RESP_TIMEOUT=5s por defecto).
     *       Excepciones: ConnectAP (10s), ScanAPs (hasta 180s).
     */
    typedef struct
    {
        /* ------------------------------------------------------------------ */
        /*  Lifecycle                                                          */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Satisface contrato de interfaz (init real en WifiControlAdapter_Init).
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK siempre.
         */
        Result_t (*Init)(void *ctx);

        /**
         * @brief Satisface contrato de interfaz (deinit real en WifiControlAdapter_Deinit).
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK siempre.
         */
        Result_t (*Deinit)(void *ctx);

        /* ------------------------------------------------------------------ */
        /*  WiFi Mode                                                          */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Configura el modo WiFi del ESP32.
         * @param[in] ctx  Contexto del adapter.
         * @param[in] mode Modo deseado.
         * @return ERR_OK, ERR_INVALID_PARAM, ERR_ERROR.
         */
        Result_t (*SetMode)(void *ctx, WifiCtrlMode_t mode);

        /**
         * @brief Obtiene el modo WiFi actual del ESP32.
         * @param[in]  ctx      Contexto del adapter.
         * @param[out] out_mode Modo WiFi actual.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetMode)(void *ctx, WifiCtrlMode_t *out_mode);

        /* ------------------------------------------------------------------ */
        /*  MAC Address                                                        */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Obtiene la dirección MAC en formato binario (6 bytes, big-endian).
         * @param[in]  ctx     Contexto del adapter.
         * @param[in]  mode    WIFI_CTRL_MODE_STA o WIFI_CTRL_MODE_AP.
         * @param[out] out_mac Buffer de 6 bytes.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_INVALID_PARAM, ERR_ERROR.
         */
        Result_t (*GetMacAddr)(void *ctx, WifiCtrlMode_t mode, uint8_t out_mac[6]);

        /**
         * @brief Establece la dirección MAC de una interfaz del ESP32.
         * @param[in] ctx  Contexto del adapter.
         * @param[in] mode WIFI_CTRL_MODE_STA o WIFI_CTRL_MODE_AP.
         * @param[in] mac  6 bytes de la nueva MAC (big-endian).
         * @return ERR_OK, ERR_INVALID_PARAM, ERR_ERROR.
         * @note Requiere que el modo WiFi NO esté activo para esa interfaz.
         */
        Result_t (*SetMacAddr)(void *ctx, WifiCtrlMode_t mode, const uint8_t mac[6]);

        /* ------------------------------------------------------------------ */
        /*  STA — Connect / Disconnect / Status                                */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Conecta el ESP32 como STA a un AP.
         * @param[in] ctx Contexto del adapter.
         * @param[in] cfg Configuración (ssid no puede ser vacío).
         * @return ERR_OK, ERR_NULL_POINTER, ERR_INVALID_PARAM, ERR_ERROR.
         * @note Bloquea hasta DEFAULT_CTRL_RESP_CONNECT_AP_TIMEOUT (10s).
         */
        Result_t (*ConnectAP)(void *ctx, const WifiStaConfig_t *cfg);

        /**
         * @brief Desconecta el ESP32 del AP actual.
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*DisconnectAP)(void *ctx);

        /**
         * @brief Obtiene el estado de la conexión AP actual (STA mode).
         * @param[in]  ctx Contexto del adapter.
         * @param[out] out Estado de la conexión.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetApStatus)(void *ctx, WifiApStatus_t *out);

        /**
         * @brief Escanea APs vecinos y llama cb por cada resultado.
         * @param[in] ctx    Contexto del adapter.
         * @param[in] cb     Callback por AP (NO puede ser NULL).
         * @param[in] cb_ctx Contexto opaco para cb.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         * @note Bloquea hasta DEFAULT_CTRL_RESP_AP_SCAN_TIMEOUT (180s).
         * @note No expone memoria dinámica: cb se llama antes de liberar la lista.
         */
        Result_t (*ScanAPs)(void *ctx, WifiScanResultCallback_t cb, void *cb_ctx);

        /* ------------------------------------------------------------------ */
        /*  SoftAP                                                             */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Inicia el SoftAP del ESP32.
         * @param[in] ctx Contexto del adapter.
         * @param[in] cfg Configuración del SoftAP.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*StartSoftAP)(void *ctx, const WifiApConfig_t *cfg);

        /**
         * @brief Detiene el SoftAP del ESP32.
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*StopSoftAP)(void *ctx);

        /**
         * @brief Obtiene la configuración actual del SoftAP.
         * @param[in]  ctx Contexto del adapter.
         * @param[out] out Configuración y MAC del SoftAP.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetSoftApStatus)(void *ctx, WifiSoftApStatus_t *out);

        /**
         * @brief Obtiene clientes conectados al SoftAP (cb por cada cliente).
         * @param[in] ctx    Contexto del adapter.
         * @param[in] cb     Callback por cliente (puede ser NULL — solo verifica éxito).
         * @param[in] cb_ctx Contexto opaco para cb.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*GetSoftApClients)(void *ctx, WifiApClientCallback_t cb, void *cb_ctx);

        /**
         * @brief Configura un Vendor-Specific IE en el SoftAP.
         * @param[in] ctx Contexto del adapter.
         * @param[in] ie  Descriptor del IE (payload válido durante la llamada).
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         * @note Debe llamarse ANTES de StartSoftAP().
         */
        Result_t (*SetVendorIE)(void *ctx, const WifiVendorIE_t *ie);

        /* ------------------------------------------------------------------ */
        /*  Power Save                                                         */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Configura el modo de ahorro de energía.
         * @param[in] ctx  Contexto del adapter.
         * @param[in] mode Modo de PS.
         * @return ERR_OK, ERR_INVALID_PARAM, ERR_ERROR.
         */
        Result_t (*SetPowerSave)(void *ctx, WifiPsMode_t mode);

        /**
         * @brief Obtiene el modo de ahorro de energía actual.
         * @param[in]  ctx      Contexto del adapter.
         * @param[out] out_mode Modo de PS actual.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetPowerSave)(void *ctx, WifiPsMode_t *out_mode);

        /* ------------------------------------------------------------------ */
        /*  TX Power                                                           */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Configura la potencia máxima de TX.
         * @param[in] ctx              Contexto del adapter.
         * @param[in] power_quarter_dBm Potencia en unidades de 0.25 dBm (8…84 típico).
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*SetMaxTxPower)(void *ctx, int32_t power_quarter_dBm);

        /**
         * @brief Obtiene la potencia de TX actual.
         * @param[in]  ctx              Contexto del adapter.
         * @param[out] out_power_quarter_dBm Potencia en unidades de 0.25 dBm.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetTxPower)(void *ctx, int32_t *out_power_quarter_dBm);

        /* ------------------------------------------------------------------ */
        /*  Country Code                                                       */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Establece el código de país 802.11d.
         * @param[in] ctx Contexto del adapter.
         * @param[in] cc  Descriptor del código de país.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*SetCountryCode)(void *ctx, const WifiCountryCode_t *cc);

        /**
         * @brief Obtiene el código de país 802.11d actual.
         * @param[in]  ctx    Contexto del adapter.
         * @param[out] out_cc Código de país actual.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetCountryCode)(void *ctx, WifiCountryCode_t *out_cc);

        /* ------------------------------------------------------------------ */
        /*  Heartbeat                                                          */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Configura y habilita/deshabilita el evento de heartbeat.
         * @param[in] ctx Contexto del adapter.
         * @param[in] cfg Configuración de heartbeat.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         * @note Registrar también OnHeartbeat() para recibir los eventos.
         */
        Result_t (*ConfigHeartbeat)(void *ctx, const WifiHeartbeatConfig_t *cfg);

        /* ------------------------------------------------------------------ */
        /*  OTA                                                                */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Inicia el proceso de OTA (borra la partición flash OTA).
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*OtaBegin)(void *ctx);

        /**
         * @brief Escribe un chunk de datos OTA al ESP32.
         * @param[in] ctx  Contexto del adapter.
         * @param[in] data Puntero a los datos (caller-owned).
         * @param[in] len  Longitud del chunk en bytes.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         * @note Llamar repetidamente hasta escribir todo el binario.
         */
        Result_t (*OtaWrite)(void *ctx, const uint8_t *data, uint32_t len);

        /**
         * @brief Finaliza OTA, valida imagen y programa reinicio (~5s).
         * @param[in] ctx Contexto del adapter.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*OtaEnd)(void *ctx);

        /* ------------------------------------------------------------------ */
        /*  Feature Config                                                     */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Habilita o deshabilita un subsistema de ESP-Hosted.
         * @param[in] ctx     Contexto del adapter.
         * @param[in] feature Feature a configurar.
         * @param[in] enable  true=habilitar, false=deshabilitar.
         * @return ERR_OK, ERR_INVALID_PARAM, ERR_ERROR.
         */
        Result_t (*ConfigFeature)(void *ctx, WifiFeature_t feature, bool enable);

        /* ------------------------------------------------------------------ */
        /*  Firmware Version                                                   */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Obtiene la versión del firmware del ESP32.
         * @param[in]  ctx Contexto del adapter.
         * @param[out] out Versión del firmware.
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetFwVersion)(void *ctx, WifiFwVersion_t *out);

        /* ------------------------------------------------------------------ */
        /*  DHCP / DNS                                                         */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Obtiene el estado DHCP/DNS.
         * @param[in]  ctx Contexto del adapter.
         * @param[out] out Estado actual (iface viene en la respuesta).
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*GetDhcpDnsStatus)(void *ctx, WifiDhcpDnsStatus_t *out);

        /**
         * @brief Configura el estado DHCP/DNS.
         * @param[in] ctx Contexto del adapter.
         * @param[in] cfg Configuración (iface identifica la interfaz).
         * @return ERR_OK, ERR_NULL_POINTER, ERR_ERROR.
         */
        Result_t (*SetDhcpDnsStatus)(void *ctx, const WifiDhcpDnsStatus_t *cfg);

        /* ------------------------------------------------------------------ */
        /*  Custom RPC                                                         */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Envía un mensaje RPC personalizado no-serializado al ESP32.
         * @param[in] ctx      Contexto del adapter.
         * @param[in] msg_id   ID del mensaje RPC.
         * @param[in] data     Payload (caller-owned, puede ser NULL si data_len=0).
         * @param[in] data_len Longitud del payload en bytes.
         * @return ERR_OK, ERR_ERROR.
         */
        Result_t (*SendCustomRpc)(void *ctx,
                                  uint32_t msg_id,
                                  const uint8_t *data,
                                  uint16_t data_len);

        /* ------------------------------------------------------------------ */
        /*  Event registrations                                                */
        /* ------------------------------------------------------------------ */

        /**
         * @brief Registra callback para "transporte listo" (CTRL_EVENT_ESP_INIT).
         * @note cb=NULL desregistra. Un solo callback por evento.
         */
        void (*OnTransportReady)(void *ctx, void (*cb)(void *), void *cb_ctx);

        /**
         * @brief Registra callback para "STA conectada a AP"
         *        (CTRL_EVENT_STATION_CONNECTED_TO_AP).
         */
        void (*OnStaConnected)(void *ctx, void (*cb)(void *), void *cb_ctx);

        /**
         * @brief Registra callback para "STA desconectada de AP"
         *        (CTRL_EVENT_STATION_DISCONNECT_FROM_AP).
         */
        void (*OnStaDisconnected)(void *ctx, void (*cb)(void *), void *cb_ctx);

        /**
         * @brief Registra callback para "cliente conectado a nuestro SoftAP"
         *        (CTRL_EVENT_STATION_CONNECTED_TO_ESP_SOFTAP).
         */
        void (*OnApStarted)(void *ctx, void (*cb)(void *), void *cb_ctx);

        /**
         * @brief Registra callback para "cliente desconectado de nuestro SoftAP"
         *        (CTRL_EVENT_STATION_DISCONNECT_FROM_ESP_SOFTAP).
         */
        void (*OnApStopped)(void *ctx, void (*cb)(void *), void *cb_ctx);

        /**
         * @brief Registra callback para evento de heartbeat (CTRL_EVENT_HEARTBEAT).
         * @param[in] ctx    Contexto del adapter.
         * @param[in] cb     Callback con número de secuencia (NULL=desregistrar).
         * @param[in] cb_ctx Contexto opaco para cb.
         * @note Requiere también llamar a ConfigHeartbeat() para habilitar el evento.
         */
        void (*OnHeartbeat)(void *ctx, WifiHeartbeatCallback_t cb, void *cb_ctx);

        /**
         * @brief Registra callback para actualización DHCP/DNS
         *        (CTRL_EVENT_DHCP_DNS_STATUS).
         * @param[in] ctx    Contexto del adapter.
         * @param[in] cb     Callback con el nuevo estado (NULL=desregistrar).
         * @param[in] cb_ctx Contexto opaco para cb.
         */
        void (*OnDhcpUpdate)(void *ctx, WifiDhcpUpdateCallback_t cb, void *cb_ctx);

        /**
         * @brief Registra callback para mensajes RPC personalizados del slave
         *        (CTRL_EVENT_CUSTOM_RPC_UNSERIALISED_MSG).
         * @param[in] ctx    Contexto del adapter.
         * @param[in] cb     Callback con datos del RPC (NULL=desregistrar).
         * @param[in] cb_ctx Contexto opaco para cb.
         */
        void (*OnCustomRpcMessage)(void *ctx, WifiCustomRpcCallback_t cb, void *cb_ctx);

    } IWifiControl_VTable_t;

    /**
     * @brief Handle de interfaz de control WiFi (instancia de IWifiControl).
     *
     * Patrón V-Table. El caller solo necesita este struct.
     *
     * @code
     * IWifiControl_t *ctrl = WifiControlAdapter_GetInterface(&adapter);
     * IWifiControl_SetMode(ctrl, WIFI_CTRL_MODE_STA);
     * IWifiControl_ConnectAP(ctrl, &cfg);
     * @endcode
     */
    struct IWifiControl_t
    {
        const IWifiControl_VTable_t *vtable; /**< V-Table de operaciones */
        void *context;                       /**< Contexto opaco del adapter */
    };

    /*============================================================================*
     * INLINE HELPERS
     *============================================================================*/

    /**
     * @brief Valida que la interfaz tenga V-Table y funciones mínimas.
     */
    static inline bool WifiControl_IsValid(const IWifiControl_t *iface)
    {
        return (iface != NULL) &&
               (iface->vtable != NULL) &&
               (iface->vtable->Init != NULL) &&
               (iface->vtable->Deinit != NULL) &&
               (iface->vtable->SetMode != NULL) &&
               (iface->vtable->GetMacAddr != NULL) &&
               (iface->vtable->ConnectAP != NULL) &&
               (iface->vtable->DisconnectAP != NULL) &&
               (iface->vtable->StartSoftAP != NULL) &&
               (iface->vtable->StopSoftAP != NULL);
    }

    static inline Result_t IWifiControl_Init(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->Init)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Init(iface->context);
    }

    static inline Result_t IWifiControl_Deinit(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->Deinit)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Deinit(iface->context);
    }

    static inline Result_t IWifiControl_SetMode(IWifiControl_t *iface, WifiCtrlMode_t mode)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetMode)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetMode(iface->context, mode);
    }

    static inline Result_t IWifiControl_GetMode(IWifiControl_t *iface, WifiCtrlMode_t *out_mode)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetMode)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetMode(iface->context, out_mode);
    }

    static inline Result_t IWifiControl_GetMacAddr(IWifiControl_t *iface,
                                                   WifiCtrlMode_t mode,
                                                   uint8_t out_mac[6])
    {
        if (!iface || !iface->vtable || !iface->vtable->GetMacAddr)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetMacAddr(iface->context, mode, out_mac);
    }

    static inline Result_t IWifiControl_SetMacAddr(IWifiControl_t *iface,
                                                   WifiCtrlMode_t mode,
                                                   const uint8_t mac[6])
    {
        if (!iface || !iface->vtable || !iface->vtable->SetMacAddr)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetMacAddr(iface->context, mode, mac);
    }

    static inline Result_t IWifiControl_ConnectAP(IWifiControl_t *iface,
                                                  const WifiStaConfig_t *cfg)
    {
        if (!iface || !iface->vtable || !iface->vtable->ConnectAP)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->ConnectAP(iface->context, cfg);
    }

    static inline Result_t IWifiControl_DisconnectAP(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->DisconnectAP)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->DisconnectAP(iface->context);
    }

    static inline Result_t IWifiControl_GetApStatus(IWifiControl_t *iface,
                                                    WifiApStatus_t *out)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetApStatus)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetApStatus(iface->context, out);
    }

    static inline Result_t IWifiControl_ScanAPs(IWifiControl_t *iface,
                                                WifiScanResultCallback_t cb,
                                                void *cb_ctx)
    {
        if (!iface || !iface->vtable || !iface->vtable->ScanAPs)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->ScanAPs(iface->context, cb, cb_ctx);
    }

    static inline Result_t IWifiControl_StartSoftAP(IWifiControl_t *iface,
                                                    const WifiApConfig_t *cfg)
    {
        if (!iface || !iface->vtable || !iface->vtable->StartSoftAP)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->StartSoftAP(iface->context, cfg);
    }

    static inline Result_t IWifiControl_StopSoftAP(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->StopSoftAP)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->StopSoftAP(iface->context);
    }

    static inline Result_t IWifiControl_GetSoftApStatus(IWifiControl_t *iface,
                                                        WifiSoftApStatus_t *out)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetSoftApStatus)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetSoftApStatus(iface->context, out);
    }

    static inline Result_t IWifiControl_GetSoftApClients(IWifiControl_t *iface,
                                                         WifiApClientCallback_t cb,
                                                         void *cb_ctx)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetSoftApClients)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetSoftApClients(iface->context, cb, cb_ctx);
    }

    static inline Result_t IWifiControl_SetVendorIE(IWifiControl_t *iface,
                                                    const WifiVendorIE_t *ie)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetVendorIE)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetVendorIE(iface->context, ie);
    }

    static inline Result_t IWifiControl_SetPowerSave(IWifiControl_t *iface, WifiPsMode_t mode)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetPowerSave)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetPowerSave(iface->context, mode);
    }

    static inline Result_t IWifiControl_GetPowerSave(IWifiControl_t *iface,
                                                     WifiPsMode_t *out_mode)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetPowerSave)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetPowerSave(iface->context, out_mode);
    }

    static inline Result_t IWifiControl_SetMaxTxPower(IWifiControl_t *iface,
                                                      int32_t power_quarter_dBm)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetMaxTxPower)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetMaxTxPower(iface->context, power_quarter_dBm);
    }

    static inline Result_t IWifiControl_GetTxPower(IWifiControl_t *iface,
                                                   int32_t *out_power_quarter_dBm)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetTxPower)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetTxPower(iface->context, out_power_quarter_dBm);
    }

    static inline Result_t IWifiControl_SetCountryCode(IWifiControl_t *iface,
                                                       const WifiCountryCode_t *cc)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetCountryCode)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetCountryCode(iface->context, cc);
    }

    static inline Result_t IWifiControl_GetCountryCode(IWifiControl_t *iface,
                                                       WifiCountryCode_t *out_cc)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetCountryCode)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetCountryCode(iface->context, out_cc);
    }

    static inline Result_t IWifiControl_ConfigHeartbeat(IWifiControl_t *iface,
                                                        const WifiHeartbeatConfig_t *cfg)
    {
        if (!iface || !iface->vtable || !iface->vtable->ConfigHeartbeat)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->ConfigHeartbeat(iface->context, cfg);
    }

    static inline Result_t IWifiControl_OtaBegin(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->OtaBegin)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->OtaBegin(iface->context);
    }

    static inline Result_t IWifiControl_OtaWrite(IWifiControl_t *iface,
                                                 const uint8_t *data,
                                                 uint32_t len)
    {
        if (!iface || !iface->vtable || !iface->vtable->OtaWrite)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->OtaWrite(iface->context, data, len);
    }

    static inline Result_t IWifiControl_OtaEnd(IWifiControl_t *iface)
    {
        if (!iface || !iface->vtable || !iface->vtable->OtaEnd)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->OtaEnd(iface->context);
    }

    static inline Result_t IWifiControl_ConfigFeature(IWifiControl_t *iface,
                                                      WifiFeature_t feature,
                                                      bool enable)
    {
        if (!iface || !iface->vtable || !iface->vtable->ConfigFeature)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->ConfigFeature(iface->context, feature, enable);
    }

    static inline Result_t IWifiControl_GetFwVersion(IWifiControl_t *iface,
                                                     WifiFwVersion_t *out)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetFwVersion)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetFwVersion(iface->context, out);
    }

    static inline Result_t IWifiControl_GetDhcpDnsStatus(IWifiControl_t *iface,
                                                         WifiDhcpDnsStatus_t *out)
    {
        if (!iface || !iface->vtable || !iface->vtable->GetDhcpDnsStatus)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->GetDhcpDnsStatus(iface->context, out);
    }

    static inline Result_t IWifiControl_SetDhcpDnsStatus(IWifiControl_t *iface,
                                                         const WifiDhcpDnsStatus_t *cfg)
    {
        if (!iface || !iface->vtable || !iface->vtable->SetDhcpDnsStatus)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SetDhcpDnsStatus(iface->context, cfg);
    }

    static inline Result_t IWifiControl_SendCustomRpc(IWifiControl_t *iface,
                                                      uint32_t msg_id,
                                                      const uint8_t *data,
                                                      uint16_t data_len)
    {
        if (!iface || !iface->vtable || !iface->vtable->SendCustomRpc)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->SendCustomRpc(iface->context, msg_id, data, data_len);
    }

    static inline void IWifiControl_OnTransportReady(IWifiControl_t *iface,
                                                     void (*cb)(void *),
                                                     void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnTransportReady)
        {
            iface->vtable->OnTransportReady(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnStaConnected(IWifiControl_t *iface,
                                                   void (*cb)(void *),
                                                   void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnStaConnected)
        {
            iface->vtable->OnStaConnected(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnStaDisconnected(IWifiControl_t *iface,
                                                      void (*cb)(void *),
                                                      void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnStaDisconnected)
        {
            iface->vtable->OnStaDisconnected(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnApStarted(IWifiControl_t *iface,
                                                void (*cb)(void *),
                                                void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnApStarted)
        {
            iface->vtable->OnApStarted(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnApStopped(IWifiControl_t *iface,
                                                void (*cb)(void *),
                                                void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnApStopped)
        {
            iface->vtable->OnApStopped(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnHeartbeat(IWifiControl_t *iface,
                                                WifiHeartbeatCallback_t cb,
                                                void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnHeartbeat)
        {
            iface->vtable->OnHeartbeat(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnDhcpUpdate(IWifiControl_t *iface,
                                                 WifiDhcpUpdateCallback_t cb,
                                                 void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnDhcpUpdate)
        {
            iface->vtable->OnDhcpUpdate(iface->context, cb, cb_ctx);
        }
    }

    static inline void IWifiControl_OnCustomRpcMessage(IWifiControl_t *iface,
                                                       WifiCustomRpcCallback_t cb,
                                                       void *cb_ctx)
    {
        if (iface && iface->vtable && iface->vtable->OnCustomRpcMessage)
        {
            iface->vtable->OnCustomRpcMessage(iface->context, cb, cb_ctx);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_WIFI_CONTROL_H */
