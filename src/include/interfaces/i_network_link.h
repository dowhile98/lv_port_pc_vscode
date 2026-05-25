/**
 * @file  i_network_link.h
 * @brief Interfaz V-Table para observación pasiva del estado de enlace de red.
 *
 * @details
 * Permite sondear si un puerto de red tiene enlace físico activo y dirección IP
 * asignada, sin acceder a los tipos concretos del stack TCP/IP subyacente.
 *
 * Diseño ISP: interfaz mínima, separada de INetworkPort_t.
 *   - INetworkPort_t  → plano de control (configurar, aplicar IP)
 *   - INetworkLink_t  → plano de observación (¿está up? ¿tiene IP?)
 *
 * Uso típico en NetworkAO (polling cada 500 ms):
 * @code
 *   if (NetworkLink_IsLinkUp(link) && NetworkLink_HasIpAddress(link))
 *   {
 *       // Transición → SERVICES_RUNNING
 *   }
 * @endcode
 *
 * Contrato LSP:
 *   - IsLinkUp(self=NULL)     → false (defensivo)
 *   - HasIpAddress(self=NULL) → false (defensivo)
 *   - HasIpAddress() → true implica IsLinkUp() → true
 *
 * @note DIP: Este archivo NO incluye CycloneTCP, lwIP, HAL ni RTOS.
 * @note Thread-safety: Ambas funciones son safe para lectura concurrente.
 *
 * @version 1.0.0
 * @date    2026-03-12
 */

#ifndef I_NETWORK_LINK_H
#define I_NETWORK_LINK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
    /* ===== V-Table ===== */

    typedef struct INetworkLink_VTable
    {
        /**
         * @brief Indica si el enlace físico/lógico está activo.
         *
         * @param[in] ctx  Contexto de la implementación.
         * @return true  si el enlace está up (carrier detect, asociado en WiFi, etc.).
         * @return false si ctx es NULL, el puerto no está configurado, o el enlace está caído.
         *
         * @note Thread-safe — solo lectura.
         */
        bool (*IsLinkUp)(const void *ctx);

        /**
         * @brief Indica si el puerto tiene una dirección IP asignada y válida.
         *
         * @param[in] ctx  Contexto de la implementación.
         * @return true  si tiene IP (DHCP lease obtenido o IP estática configurada).
         * @return false si ctx es NULL, sin enlace, o sin IP aún (DHCP pending).
         *
         * @note Thread-safe — solo lectura.
         * @note Postcondición: HasIpAddress() == true ⇒ IsLinkUp() == true.
         */
        bool (*HasIpAddress)(const void *ctx);

    } INetworkLink_VTable_t;

    /* ===== Interface Instance ===== */

    typedef struct INetworkLink
    {
        const INetworkLink_VTable_t *vtable;
        void *context;
    } INetworkLink_t;

    /* ===== Inline Wrappers ===== */

    /**
     * @brief Sondea el estado de enlace. Defensivo ante NULL.
     */
    static inline bool NetworkLink_IsLinkUp(const INetworkLink_t *link)
    {
        if ((link == NULL) || (link->vtable == NULL) || (link->vtable->IsLinkUp == NULL))
        {
            return false;
        }
        return link->vtable->IsLinkUp(link->context);
    }

    /**
     * @brief Sondea si tiene IP asignada. Defensivo ante NULL.
     */
    static inline bool NetworkLink_HasIpAddress(const INetworkLink_t *link)
    {
        if ((link == NULL) || (link->vtable == NULL) || (link->vtable->HasIpAddress == NULL))
        {
            return false;
        }
        return link->vtable->HasIpAddress(link->context);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_NETWORK_LINK_H */
