/**
 * @file i_network_service.h
 * @brief Interfaz V-Table para servicios de red arrancables/detenibles.
 *
 * @details
 * Define el contrato que cualquier servicio de red (HTTP server, MQTT client, etc.)
 * debe cumplir para ser orquestado por NetworkAO_t.
 *
 * Contrato LSP:
 *   - Start() con self=NULL       → ERR_NULL_POINTER
 *   - Stop()  con self=NULL       → ERR_NULL_POINTER
 *   - IsRunning() antes de Start  → false
 *   - IsRunning() tras Stop       → false
 *   - Stop() sin Start previo     → ERR_OK (idempotente)
 *
 * @note SRP: Esta interfaz NO define cómo se inicializa el servicio.
 *       La inicialización es responsabilidad del adapter (e.g. HttpServerAdapter_Init).
 *
 * @note ISP: Solo contiene las operaciones del ciclo de vida Prepare/Start/Stop/IsRunning.
 *       Prepare() es la fase de inicialización one-time (e.g. crear tarea OS).
 *       Start()/Stop() controlan si el servicio atiende peticiones.
 *       No contiene configuración, lectura de datos, ni callbacks.
 *
 * @version 1.0.0
 * @date    2026-02-20
 */

#ifndef I_NETWORK_SERVICE_H
#define I_NETWORK_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

    /* ===== V-Table ===== */

    typedef struct INetworkService_VTable
    {
        /**
         * @brief Inicialización one-time del servicio (opcional — NULL = no-op).
         *
         * @details Llamado UNA SOLA VEZ por NetworkAO durante handle_msg_transport_ready,
         *          después de que los puertos de red han sido configurados con
         *          NetworkPort_Configure().  Es el lugar correcto para operaciones
         *          costosas o de una sola vez:
         *            - HTTP server: httpServerInit() + httpServerStart() (crea tarea OS).
         *            - MQTT client: connect + subscribe (una vez levantada la red).
         *
         *          Contraste con Start(): Start()/Stop() se pueden llamar múltiples veces
         *          (e.g. tras un reset del módulo WiFi).  Prepare() solo se llama la
         *          primera vez que el stack TCP/IP queda listo.
         *
         * @param[in] self  Puntero a la implementación (no NULL).
         * @return ERR_OK           Preparación exitosa.
         * @return ERR_NULL_POINTER Si self es NULL.
         * @return ERR_ERROR        Si falló la inicialización interna.
         *
         * @note Slot opcional: si es NULL, NetworkService_Prepare() devuelve ERR_OK.
         * @note No thread-safe con Stop()/Start(). Llamar desde composition root.
         */
        Result_t (*Prepare)(void *self);

        /**
         * @brief Arranca el servicio de red.
         *
         * @param[in] self  Puntero a la implementación (nunca NULL).
         * @return ERR_OK           Si el servicio arrancó correctamente.
         * @return ERR_NULL_POINTER Si self es NULL.
         * @return ERR_ERROR        Si falló el inicio por error interno.
         * @return ERR_BUSY         Si el servicio ya estaba arrancado.
         */
        Result_t (*Start)(void *self);

        /**
         * @brief Detiene el servicio de red.
         *
         * @param[in] self  Puntero a la implementación (nunca NULL).
         * @return ERR_OK           Si el servicio se detuvo o ya estaba detenido.
         * @return ERR_NULL_POINTER Si self es NULL.
         * @return ERR_ERROR        Si falló la detención por error interno.
         *
         * @note Idempotente: llamar Stop cuando ya está detenido → ERR_OK.
         */
        Result_t (*Stop)(void *self);

        /**
         * @brief Indica si el servicio está activo.
         *
         * @param[in] self  Puntero a la implementación (nunca NULL).
         * @return true  Si el servicio está en ejecución.
         * @return false Si self es NULL o el servicio está detenido.
         */
        bool (*IsRunning)(const void *self);

    } INetworkService_VTable_t;

    /* ===== Interface Instance ===== */

    typedef struct INetworkService
    {
        const INetworkService_VTable_t *vtable;
        void *context; /**< Apunta a la implementación concreta. */
    } INetworkService_t;

    /* ===== Inline Helpers ===== */

    /**
     * @brief Inicializa one-time el servicio (e.g. crea tarea OS, httpServerInit).
     *        Si vtable->Prepare es NULL devuelve ERR_OK (implementación opcional).
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL → ERR_NULL_POINTER).
     * @return ERR_OK si la preparación fue exitosa o el slot es NULL (no-op).
     */
    static inline Result_t NetworkService_Prepare(INetworkService_t *iface)
    {
        if (iface == NULL || iface->vtable == NULL)
        {
            return ERR_NULL_POINTER;
        }
        if (iface->vtable->Prepare == NULL)
        {
            return ERR_OK; /* opcional — no-op por defecto */
        }
        return iface->vtable->Prepare(iface->context);
    }

    /**
     * @brief Arranca el servicio de red mediante su interfaz.
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL → ERR_NULL_POINTER).
     * @return Result_t según el contrato de INetworkService_VTable_t::Start.
     */
    static inline Result_t NetworkService_Start(INetworkService_t *iface)
    {
        if (iface == NULL || iface->vtable == NULL || iface->vtable->Start == NULL)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Start(iface->context);
    }

    /**
     * @brief Detiene el servicio de red mediante su interfaz.
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL → ERR_NULL_POINTER).
     * @return Result_t según el contrato de INetworkService_VTable_t::Stop.
     */
    static inline Result_t NetworkService_Stop(INetworkService_t *iface)
    {
        if (iface == NULL || iface->vtable == NULL || iface->vtable->Stop == NULL)
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Stop(iface->context);
    }

    /**
     * @brief Consulta si el servicio está activo mediante su interfaz.
     *
     * @param[in] iface  Puntero a la interfaz (puede ser NULL → false).
     * @return bool según el contrato de INetworkService_VTable_t::IsRunning.
     */
    static inline bool NetworkService_IsRunning(const INetworkService_t *iface)
    {
        if (iface == NULL || iface->vtable == NULL || iface->vtable->IsRunning == NULL)
        {
            return false;
        }
        return iface->vtable->IsRunning(iface->context);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_NETWORK_SERVICE_H */
