/**
 * @file    i_network_stack.h
 * @brief   Interfaz de inicialización del stack de red TCP/IP (V-Table pattern, DIP).
 * @version 1.0.0
 * @date    2026-02-20
 * @author  Tecna Smart Lab
 *
 * @details
 * Abstrae la inicialización del stack TCP/IP (CycloneTCP en producción)
 * para mantener la capa Application (NetworkAO) desacoplada de CycloneTCP.
 *
 * Solo expone `Init()`: NetworkAO no necesita acceder a las interfaces
 * concretas (`NetInterface *`). El DI container obtiene los punteros a
 * `netInterface[0/1]` directamente desde el adapter (capa Infrastructure).
 *
 * @par Capas permitidas para incluir este header
 * - `src/application/activeobjects/` (NetworkAO)
 * - `src/infrastructure/di/`         (DI container)
 * - `src/infrastructure/adapters/`   (CycloneTcpNetworkStackAdapter)
 * - `tests/unit/` y `tests/fakes/`
 *
 * @note Cumple DIP: NetworkAO depende de esta abstracción, nunca de CycloneTCP.
 */

#ifndef I_NETWORK_STACK_H
#define I_NETWORK_STACK_H

#include "hal_types.h"
#include <stdint.h>
#include <stddef.h> /* NULL */

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * V-TABLE
     * ========================================================================= */

    /**
     * @brief V-Table de operaciones del stack de red.
     */
    typedef struct
    {
        /**
         * @brief Arranca el transporte físico subyacente (no bloqueante).
         *
         * @details En producción (CycloneTCP + ESP-Hosted):
         *            1. `EspWifiNicAdapter_Init()` — crea el OsEvent de transporte.
         *            2. `hosted_Init(handler)` — reset ESP32 + arranque SPI.
         *          Debe ejecutarse desde un contexto de TAREA (no desde main/DI init).
         *          La notificación de "transporte listo" llega de forma asíncrona
         *          vía `IWifiControl_OnTransportReady` registrado en el DI container.
         *
         * @details En UNIT_TEST: no-op (el NetworkAO simula TRANSPORT_READY inline).
         *
         * @param[in] ctx  Contexto del adapter (puntero al adapter concreto).
         *
         * @note ISR-safe: nunca bloquea. Solo inicia el proceso.
         */
        void (*LaunchTransport)(void *ctx);

        /**
         * @brief Inicializa el stack TCP/IP (`netInit()`).
         *
         * @details Se llama desde el NetworkAO thread tras recibir
         *          NET_AO_MSG_TRANSPORT_READY (transporte físico listo).
         *          En la versión N.8 solo ejecuta `netInit()`.
         *          La configuración de interfaces STA/AP es responsabilidad
         *          de CycloneNetworkPortAdapter (llamado desde NetworkAO).
         *
         * @param[in] ctx  Contexto del adapter (puntero al adapter concreto).
         *
         * @return ERR_OK      si stack inicializado correctamente.
         * @return ERR_ERROR   si falla `netInit()`.
         *
         * @note Thread-safe: se llama una sola vez desde NetworkAO thread.
         */
        Result_t (*Init)(void *ctx);

    } INetworkStack_VTable_t;

    /**
     * @brief Handle de la interfaz de stack de red (V-Table pattern).
     *
     * @note Los consumers llaman al helper inline `INetworkStack_Init()`.
     *       Nunca invocar `vtable->Init(context)` directamente.
     */
    typedef struct
    {
        const INetworkStack_VTable_t *vtable; /**< Puntero a la tabla de funciones. */
        void *context;                        /**< Contexto del adapter concreto.   */
    } INetworkStack_t;

    /* =========================================================================
     * INLINE HELPERS (safe dispatch)
     * ========================================================================= */

    /**
     * @brief Arranca el transporte físico (no bloqueante, desde task context).
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL — no-op).
     */
    static inline void INetworkStack_LaunchTransport(INetworkStack_t *iface)
    {
        if (iface != NULL && iface->vtable != NULL &&
            iface->vtable->LaunchTransport != NULL)
        {
            iface->vtable->LaunchTransport(iface->context);
        }
    }

    /**
     * @brief Inicializa el stack TCP/IP a través de la interfaz abstracta.
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL — no-op).
     *
     * @return ERR_OK           si exitoso o iface es NULL.
     * @return ERR_NULL_POINTER si iface o vtable son NULL.
     * @return ERR_ERROR        si Init() falla.
     */
    static inline Result_t INetworkStack_Init(INetworkStack_t *iface)
    {
        if (iface == NULL || iface->vtable == NULL)
        {
            return ERR_NULL_POINTER;
        }
        if (iface->vtable->Init == NULL)
        {
            return ERR_OK; /* No-op implementación vacía */
        }
        return iface->vtable->Init(iface->context);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_NETWORK_STACK_H */
