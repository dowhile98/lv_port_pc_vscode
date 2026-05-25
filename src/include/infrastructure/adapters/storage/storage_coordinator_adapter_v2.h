/**
 * @file storage_coordinator_adapter_v2.h
 * @brief Adapter v2 que conecta IEventNotifier con EventLogAO.
 * @version 2.0.0
 *
 * @note Esta versión NO usa StorageCoordinatorAO v3.0.
 *       Escribe eventos directamente a EEPROM via IEventLogStorage.
 *       Más simple y eficiente para event logging (write-only, no cache needed).
 *
 * MIGRATION v3.0 -> v2:
 *   OLD: IEventNotifier -> StorageCoordinatorAO (queue) -> IEventLogStorage
 *   NEW: IEventNotifier -> IEventLogStorage (direct)
 *
 * @author GitHub Copilot (Claude Sonnet 4.5)
 * @date 3 marzo 2026
 */

#ifndef STORAGE_COORDINATOR_ADAPTER_V2_H
#define STORAGE_COORDINATOR_ADAPTER_V2_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "interfaces/i_event_notifier.h"
#include "interfaces/i_event_log_storage.h"
#include "hal_types.h"

    /**
     * @brief Contexto del StorageCoordinatorAdapter_v2.
     * @note Bypasses coordinator, writes directly to EEPROM log storage.
     */
    typedef struct
    {
        IEventNotifier interface;            /**< Interfaz pública IEventNotifier */
        IEventLogStorage *event_log_storage; /**< Referencia a IEventLogStorage (no owned) */
        bool is_initialized;                 /**< Flag de inicialización */
    } StorageCoordinatorAdapter_v2_t;

    /* ===== Public API ===== */

    /**
     * @brief Inicializa el StorageCoordinatorAdapter_v2.
     *
     * @param[in,out] self              Instancia del adapter (memoria estática/stack).
     * @param[in]     event_log_storage Puntero a IEventLogStorage (implementado por EventLogAO).
     *
     * @return ERR_OK en éxito, ERR_NULL_POINTER si algún parámetro es NULL.
     *
     * @note El adapter NO es dueño del event_log_storage; caller debe mantenerlo vivo.
     * @note event_log_storage debe estar inicializado antes de usar el adapter.
     */
    Result_t StorageCoordinatorAdapter_v2_Init(
        StorageCoordinatorAdapter_v2_t *self,
        IEventLogStorage *event_log_storage);

    /**
     * @brief Obtiene la interfaz IEventNotifier del adapter.
     *
     * @param[in] self Instancia del adapter.
     *
     * @return Puntero a IEventNotifier* (válido mientras self exista), NULL si no inicializado.
     *
     * @note El puntero retornado apunta a memoria dentro de 'self'.
     */
    IEventNotifier *StorageCoordinatorAdapter_v2_GetInterface(
        StorageCoordinatorAdapter_v2_t *self);

    /**
     * @brief Desinicializa el adapter (cleanup).
     *
     * @param[in] self Instancia del adapter.
     *
     * @return ERR_OK en éxito.
     *
     * @note NO desinicializa el event_log_storage (responsabilidad del caller).
     */
    Result_t StorageCoordinatorAdapter_v2_Deinit(StorageCoordinatorAdapter_v2_t *self);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_COORDINATOR_ADAPTER_V2_H */
