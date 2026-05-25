/**
 * @file i_event_notifier.h
 * @brief Interfaz abstracta para notificación de eventos del sistema (persistentes).
 * @version 1.0.0
 *
 * @note Esta interfaz permite logging de eventos estructurados que persisten
 *       en EEPROM para auditoría. NO usar para debug volátil (usar ILogger).
 *       Cumple con DIP: Domain/Application no dependen de StorageCoordinatorAO.
 */

#ifndef I_EVENT_NOTIFIER_H
#define I_EVENT_NOTIFIER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "hal_types.h"
#include "common/date_time.h"               /* DateTime_t */
#include "interfaces/i_event_log_storage.h" /* EventType_t, EventSeverity_t */

    /**
     * @brief V-Table para IEventNotifier.
     */
    typedef struct IEventNotifier_Vtable
    {
        /**
         * @brief Notifica un evento del sistema (asíncrono).
         * @note NON-BLOCKING: retorna inmediatamente, evento procesado en background.
         * @note Thread-safe: puede llamarse desde cualquier contexto (ISR-safe).
         *
         * @param[in] impl      Instancia de implementación concreta.
         * @param[in] timestamp Timestamp UTC del evento (DateTime_t).
         * @param[in] type      Tipo de evento (EventType_t).
         * @param[in] severity  Severidad del evento (EventSeverity_t).
         * @param[in] info      Información adicional (uint16_t).
         * @param[in] params    Parámetros específicos del evento (uint16_t).
         *
         * @return ERR_OK si aceptado, ERR_BUSY si cola llena, ERR_NULL_POINTER si impl nulo.
         */
        Result_t (*NotifyEvent)(void *impl,
                                const DateTime_t *timestamp,
                                EventType_t type,
                                EventSeverity_t severity,
                                uint16_t info,
                                uint16_t params);
    } IEventNotifier_Vtable;

    /**
     * @brief Instancia de interfaz IEventNotifier.
     */
    typedef struct IEventNotifier
    {
        const IEventNotifier_Vtable *vtable;
        void *impl;
    } IEventNotifier;

    /* ===== Helper Functions ===== */

    /**
     * @brief Valida si la instancia de notifier es válida.
     * @return true si válida, false si null o corrupta.
     */
    static inline bool EventNotifier_IsValid(const IEventNotifier *notifier)
    {
        return (notifier != NULL) &&
               (notifier->vtable != NULL) &&
               (notifier->vtable->NotifyEvent != NULL) &&
               (notifier->impl != NULL);
    }

    /**
     * @brief Helper para invocar NotifyEvent de forma segura.
     *
     * @param[in] notifier  Instancia de IEventNotifier.
     * @param[in] timestamp Timestamp UTC del evento (pointer a DateTime_t).
     * @param[in] type      Tipo de evento.
     * @param[in] severity  Severidad del evento.
     * @param[in] info      Información adicional.
     * @param[in] params    Parámetros del evento.
     *
     * @return ERR_OK si notificado, código de error si falla.
     *
     * @note ISR-safe si la implementación usa queue (StorageCoordinatorAO lo hace).
     */
    static inline Result_t EventNotifier_NotifyEvent(
        IEventNotifier *notifier,
        const DateTime_t *timestamp,
        EventType_t type,
        EventSeverity_t severity,
        uint16_t info,
        uint16_t params)
    {

        if (!EventNotifier_IsValid(notifier))
        {
            return ERR_NULL_POINTER;
        }

        return notifier->vtable->NotifyEvent(notifier->impl,
                                             timestamp,
                                             type,
                                             severity,
                                             info,
                                             params);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_EVENT_NOTIFIER_H */
