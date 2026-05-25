/**
 * @file  update_manager.h
 * @brief Facade orquestador de actualizaciones OTA (firmware + recursos estáticos).
 *
 * `UpdateManager_t` implementa `IUpdateManager_t` y:
 *  - Delega a `FirmwareUpdateStrategy`  cuando type == UPDATE_TYPE_FIRMWARE.
 *  - Delega a `ExternalLoaderStrategy` cuando type == UPDATE_TYPE_EXTERNAL_LOADER.
 *  - Impide updates simultáneos: un segundo `Begin()` retorna `ERR_BUSY`.
 *  - Protege el flag de actividad con el mutex inyectado (ThreadX `TX_MUTEX`
 *    envuelto en `os_mutex_t`).
 *
 * @par Ciclo de vida típico — firmware OTA
 * @code
 *   IUpdateManager_Begin (&mgr_iface, UPDATE_TYPE_FIRMWARE);
 *   while (chunk)
 *       IUpdateManager_Write(&mgr_iface, buf, len);
 *   IUpdateManager_End   (&mgr_iface);
 *   // send HTTP 200 ...
 *   IUpdateManager_Reboot(&mgr_iface);   // no retorna
 * @endcode
 *
 * @par Ciclo de vida típico — recursos estáticos
 * @code
 *   IUpdateManager_Begin (&mgr_iface, UPDATE_TYPE_EXTERNAL_LOADER);
 *   while (chunk)
 *       IUpdateManager_Write(&mgr_iface, buf, len);
 *   IUpdateManager_End   (&mgr_iface);   // verifica ECDSA internamente
 *   // send HTTP 200 (no reboot necesario)
 * @endcode
 *
 * @par Thread-safety
 * `Begin()` adquiere el mutex con `OS_NO_WAIT`; si otra tarea tiene el mutex
 * (o si `is_busy` es true), retorna `ERR_BUSY` inmediatamente sin bloquear.
 * `End()` y `Abort()` liberan el mutex.
 *
 * @note El mutex debe ser creado externamente (DI Container) y pasado en
 *       `UpdateManagerConfig_t.mutex` **antes** de llamar a `UpdateManager_Init()`.
 *       El manager **no** crea ni destruye el mutex.
 */

#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "interfaces/i_update_manager.h"
#include "hal_types.h" /* os_mutex_t, Result_t */
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ═══════════════════════════════════════════════════════════════════════════
     * Configuration
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Parámetros de inicialización del UpdateManager.
     *
     * Todos los campos son obligatorios (no NULL / no cero).
     */
    typedef struct UpdateManagerConfig
    {
        IUpdateManager_t *fw_strategy;  /**< Estrategia de firmware (FirmwareUpdateStrategy). */
        IUpdateManager_t *ext_strategy; /**< Estrategia de recursos (ExternalLoaderStrategy). */
        os_mutex_t mutex;               /**< Mutex ya creado por el DI Container (no NULL). */
    } UpdateManagerConfig_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Concrete type
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Instancia concreta del gestor de actualizaciones.
     *
     * @note `iface` **debe** ser el primer campo — permite cast seguro desde
     *       `IUpdateManager_t *` a `UpdateManager_t *`.
     */
    typedef struct UpdateManager
    {
        IUpdateManager_t iface;         /**< V-Table + impl — MUST BE FIRST. */
        IUpdateManager_t *fw_strategy;  /**< Estrategia firmware inyectada. */
        IUpdateManager_t *ext_strategy; /**< Estrategia recursos inyectada. */
        IUpdateManager_t *active;       /**< Estrategia activa actualmente (o NULL). */
        os_mutex_t mutex;               /**< Mutex inyectado para serializar Begin(). */
        bool is_busy;                   /**< true entre Begin() y End()/Abort(). */
    } UpdateManager_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Public API
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Inicializa el UpdateManager con sus dependencias.
     *
     * @param[out] self    Instancia a inicializar (no NULL).
     * @param[in]  config  Configuración con las dos estrategias y el mutex (no NULL).
     *
     * @return ERR_OK           Inicialización correcta.
     * @return ERR_NULL_POINTER `self` o `config` son NULL.
     * @return ERR_NULL_POINTER `config->fw_strategy` o `config->ext_strategy` son NULL.
     * @return ERR_INVALID_PARAM `config->mutex` es NULL.
     */
    Result_t UpdateManager_Init(UpdateManager_t *self,
                                const UpdateManagerConfig_t *config);

    /**
     * @brief  Retorna el puntero a la interfaz `IUpdateManager_t` embebida.
     *
     * @param[in] self  Instancia inicializada (no NULL).
     *
     * @return Puntero a `IUpdateManager_t` válido, o NULL si `self` es NULL.
     */
    IUpdateManager_t *UpdateManager_GetInterface(UpdateManager_t *self);

#ifdef __cplusplus
}
#endif

#endif /* UPDATE_MANAGER_H */
