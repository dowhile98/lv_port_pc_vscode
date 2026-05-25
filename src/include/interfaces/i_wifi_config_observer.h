/**
 * @file i_wifi_config_observer.h
 * @brief Interfaz ISP-especializada para observar cambios en la configuración WiFi.
 *
 * @note ISP: Esta interfaz notifica SOLO cuando cambia el campo `wifi` de
 *       SystemConfig_t. No se dispara por cambios en relay, gps, general, etc.
 *       Sigue el patrón Observer con un único suscriptor (single-subscriber design).
 *
 * Contrato LSP:
 *   - Subscribe(cb=NULL)          → ERR_NULL_POINTER
 *   - Subscribe cuando ya ocupado → ERR_BUSY
 *   - Callback invocado SOLO si memcmp(old.wifi, new.wifi) != 0
 *   - Callback ejecuta en thread StorageCoordinator: debe ser <1ms, sin I/O
 */

#ifndef I_WIFI_CONFIG_OBSERVER_H
#define I_WIFI_CONFIG_OBSERVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "common/wifi_types.h"

    /* ===== Callback Type ===== */

    /**
     * @brief Callback invocado cuando cambia la configuración WiFi.
     *
     * @param[in] context   Contexto registrado en Subscribe (puede ser NULL).
     * @param[in] new_wifi  Nueva configuración WiFi (nunca NULL, válida durante callback).
     *
     * @warning Ejecuta en thread StorageCoordinator. Debe ser rápido (<1ms).
     * @warning NO bloquear, NO llamar funciones de storage, NO dormir.
     * @warning Copiar `new_wifi` si se necesita fuera del callback.
     */
    typedef void (*WifiConfigObserver_Callback_t)(void *context,
                                                  const WifiConfig_t *new_wifi);

    /* ===== V-Table ===== */

    typedef struct IWifiConfigObserver_VTable
    {
        /**
         * @brief Registra un suscriptor.
         *
         * @param[in] self     Puntero a la implementación (nunca NULL).
         * @param[in] cb       Callback a invocar (nunca NULL).
         * @param[in] ctx      Contexto opaco pasado al callback (puede ser NULL).
         * @return ERR_OK            Si registrado exitosamente.
         * @return ERR_NULL_POINTER  Si self o cb son NULL.
         * @return ERR_BUSY          Si ya hay un suscriptor registrado.
         */
        Result_t (*Subscribe)(void *self, WifiConfigObserver_Callback_t cb, void *ctx);

        /**
         * @brief Elimina el suscriptor registrado.
         *
         * @param[in] self  Puntero a la implementación (nunca NULL).
         * @param[in] cb    Callback a eliminar (nunca NULL).
         * @return ERR_OK         Si eliminado exitosamente.
         * @return ERR_NULL_POINTER Si self o cb son NULL.
         * @return ERR_NOT_FOUND  Si el callback no estaba registrado.
         */
        Result_t (*Unsubscribe)(void *self, WifiConfigObserver_Callback_t cb);

    } IWifiConfigObserver_VTable_t;

    /* ===== Interface Instance ===== */

    typedef struct IWifiConfigObserver
    {
        const IWifiConfigObserver_VTable_t *vtable;
        void *context; /**< Apunta a la implementación. */
    } IWifiConfigObserver_t;

    /* ===== Inline Helpers (defensivos) ===== */

    static inline bool wifi_config_observer_is_valid(const IWifiConfigObserver_t *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->context != NULL);
    }

    /**
     * @brief Suscribe un observador de cambios de configuración WiFi.
     *
     * @see IWifiConfigObserver_VTable_t::Subscribe
     */
    static inline Result_t WifiConfigObserver_Subscribe(IWifiConfigObserver_t *iface,
                                                        WifiConfigObserver_Callback_t cb,
                                                        void *ctx)
    {
        if (!wifi_config_observer_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Subscribe(iface->context, cb, ctx);
    }

    /**
     * @brief Elimina el suscriptor registrado.
     *
     * @see IWifiConfigObserver_VTable_t::Unsubscribe
     */
    static inline Result_t WifiConfigObserver_Unsubscribe(IWifiConfigObserver_t *iface,
                                                          WifiConfigObserver_Callback_t cb)
    {
        if (!wifi_config_observer_is_valid(iface))
            return ERR_NULL_POINTER;
        return iface->vtable->Unsubscribe(iface->context, cb);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_WIFI_CONFIG_OBSERVER_H */
