/**
 * @file i_http_request_handler.h
 * @brief Interfaz V-Table para el despacho de peticiones HTTP entrantes.
 *
 * @details
 * Define el contrato que los handlers de rutas HTTP deben cumplir para
 * ser registrados en HttpRouteTable_t / HttpServerAdapter_t.
 * Complementa a INetworkService_t (ciclo de vida) con la lógica de routing.
 *
 * La petición se modela como HttpRequestContext_t — un struct portable que
 * no expone tipos CycloneTCP. El campo conn_opaque es un token opaco que
 * solo los helpers de Infrastructure (http_response_writer) castean a
 * HttpConnection*. Los handlers de Application/Domain NUNCA hacen ese cast.
 *
 * Contrato LSP:
 *   - HandleRequest con self=NULL o req=NULL → ERR_NULL_POINTER.
 *   - HandleRequest retorna ERR_NOT_FOUND si no hay ruta para esa URI.
 *     El adapter responde 404 automáticamente ante ERR_NOT_FOUND.
 *
 * @note v2: HandleNotFound eliminado — la tabla de rutas y el adapter
 *       manejan 404 de forma centralizada vía httpSendErrorResponse.
 * @note ISP: Esta interfaz solo contiene routing/dispatch.
 *       El ciclo de vida del servidor (Start/Stop) es INetworkService_t.
 * @note DIP: Application/Domain handlers dependen SOLO de esta interfaz.
 *       NUNCA incluyen http/http_server.h ni tipos CycloneTCP.
 *
 * @version 1.0.0
 * @date    2026-02-20
 */

#ifndef I_HTTP_REQUEST_HANDLER_H
#define I_HTTP_REQUEST_HANDLER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "hal_types.h"
#include "infrastructure/adapters/network/http_request_context.h"

    /* ===== V-Table ===== */

    typedef struct IHttpRequestHandler_VTable
    {
        /**
         * @brief Despacha una petición HTTP recibida.
         *
         * @param[in] self  Puntero al handler concreto (nunca NULL).
         * @param[in] req   Contexto de la petición (nunca NULL). req->uri nunca NULL.
         *                  conn_opaque es opaco — solo Infrastructure lo castea.
         * @return ERR_OK        Si la petición fue procesada y respuesta enviada.
         * @return ERR_NOT_FOUND Si no existe ruta para la URI → adapter responde 404.
         * @return ERR_NULL_POINTER Si self o req son NULL.
         * @return ERR_ERROR     Si hubo error interno procesando la petición.
         */
        Result_t (*HandleRequest)(void *self, const HttpRequestContext_t *req);

    } IHttpRequestHandler_VTable_t;

    /* ===== Interface Instance ===== */

    typedef struct IHttpRequestHandler
    {
        const IHttpRequestHandler_VTable_t *vtable;
        void *context; /**< Apunta al handler concreto. */
    } IHttpRequestHandler_t;

    /* ===== Inline Helpers ===== */

    /**
     * @brief Despacha una petición HTTP a través de la interfaz.
     *
     * @param[in] iface  Interfaz del handler. NULL → retorna ERR_NOT_FOUND.
     * @param[in] req    Contexto de la petición (ver HttpRequestContext_t).
     * @return ERR_NOT_FOUND si iface es NULL o no hay handler registrado.
     */
    static inline Result_t HttpRequestHandler_Handle(IHttpRequestHandler_t *iface,
                                                     const HttpRequestContext_t *req)
    {
        if (iface == NULL || iface->vtable == NULL || iface->vtable->HandleRequest == NULL)
        {
            return ERR_NOT_FOUND;
        }
        return iface->vtable->HandleRequest(iface->context, req);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_HTTP_REQUEST_HANDLER_H */
