/**
 * @file storage_coordinator_ao_v2.h
 * @brief Simplified Storage Coordinator Active Object v2 (Phase 3, write-through cache).
 *
 * Features:
 * - Async command queue (Save/Reset)
 * - Write-through cache (updated ONLY after EEPROM success)
 * - Rate limiting (min 100ms between writes per config type)
 * - Thread-safe cache access (mutex-protected)
 * - No debounce (immediate writes)
 *
 * Phase: 3 (Storage Coordinator AO)
 * Author: Storage Redesign Team
 * Date: 2026-03-03
 */

#ifndef STORAGE_COORDINATOR_AO_V2_H
#define STORAGE_COORDINATOR_AO_V2_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal/hal_types.h"
#include "interfaces/i_modular_config_storage.h"
#include "common/relay_types.h"
#include "common/gps_types.h"
#include "common/general_types.h"
#include "common/wifi_types.h"
#include "interfaces/i_config_storage.h" /* SuperUserConfig_t */
#include "interfaces/i_logger.h"
#include "interfaces/i_event_notifier.h"
#include <stdint.h>
#include <stdbool.h>

    /* ────────────────────────────────────────────────────────────────────────────
     * Constants
     * ──────────────────────────────────────────────────────────────────────────── */

#define STORAGE_COMMAND_QUEUE_SIZE 16 /**< Max commands in queue */
#define STORAGE_RATE_LIMIT_MS 100     /**< Min 100ms between writes per type */
#define STORAGE_MAX_CONFIG_SIZE 256   /**< Max config size (Wifi = 248B) */
#define CONFIG_SUBSCRIBERS_MAX 8      /**< Max 8 config change subscribers (Phase 4.9) */

    /* ────────────────────────────────────────────────────────────────────────────
     * Command Types
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Storage command types.
     */
    typedef enum
    {
        STORAGE_CMD_SAVE_CONFIG,  /**< Save config to EEPROM */
        STORAGE_CMD_RESET_CONFIG, /**< Reset single config to defaults */
        STORAGE_CMD_RESET_ALL,    /**< Factory reset all configs */
    } StorageCmdType_t;

    /**
     * @brief Storage command structure.
     * @note Uses heap allocation (os_alloc/os_free) to keep size < 64 bytes (ThreadX queue limit).
     * @note config_data is allocated by caller, freed by worker thread after processing.
     */
    typedef struct
    {
        StorageCmdType_t type;    /**< Command type */
        ConfigType_t config_type; /**< Which config (relay, gps, wifi, etc.) */
        void *config_data;        /**< Allocated pointer to config data (freed after processing) */
        size_t config_size;       /**< Size of config_data in bytes */
    } StorageCommand_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Cache Structure
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Write-through cache for all configs.
     * @note Updated ONLY after successful EEPROM write.
     */
    typedef struct
    {
        RelayConfig_t relay;         /**< Cached relay config */
        GPSConfig_t gps;             /**< Cached GPS config */
        GeneralConfig_t general;     /**< Cached general config */
        WifiConfig_t wifi;           /**< Cached WiFi config */
        SuperUserConfig_t superuser; /**< Cached superuser config */
        void *mutex;                 /**< Mutex protecting cache (OS-specific) */
        /* TimeWindow removed: embedded in relay.time_window */
    } StorageCache_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Rate Limiter Structure
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Rate limiter to prevent EEPROM wear.
     */
    typedef struct
    {
        uint32_t last_write_tick[CONFIG_TYPE_SUPERUSER + 1]; /**< Last write tick per type */
        uint32_t min_interval_ms;                            /**< Min interval (default 100ms) */
    } RateLimiter_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Observer Pattern: Config Change Notification (Phase 4.9)
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Callback invoked cuando config cambia (después de EEPROM write exitoso).
     * @warning Ejecuta en thread StorageCoordinator. Debe ser <1ms, no bloquear.
     * @param context User context (típicamente un adapter instance).
     * @param config_type Tipo de config que cambió.
     * @param config_data Puntero a config en cache (válido solo durante callback).
     * @param config_size Tamaño del config.
     */
    typedef void (*ConfigChangeCallback_t)(
        void *context,
        ConfigType_t config_type,
        const void *config_data,
        size_t config_size);

    /**
     * @brief Subscriber entry (callback + context).
     */
    typedef struct
    {
        ConfigChangeCallback_t callback; /**< Callback function (NULL = slot libre) */
        void *context;                   /**< User context */
    } ConfigSubscriber_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Storage Coordinator AO Structure
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Storage Coordinator Active Object v2 (simplified).
     */
    typedef struct StorageCoordinatorAO_v2
    {
        /* Thread management */
        void *thread;               /**< Thread handle (OS-specific) */
        void *command_queue;        /**< Command queue (OS-specific) */
        os_semaphore_t stopped_sem; /**< Señalizado al salir del inner loop (WaitStopped) y outer loop (Deinit) */
        os_semaphore_t run_sem;     /**< Gate del outer loop: dado por Start(), tomado por el hilo */
        uint8_t thread_stack[1024*4]; /**< Stack estático del thread worker (NO malloc) */
        volatile bool is_running;   /**< Flag de ejecución del inner loop */
        volatile bool terminate;    /**< Flag de salida del outer loop (Deinit) */

        /* Dependencies (Dependency Injection) */
        IModularConfigStorage *config_storage; /**< Modular config storage interface */

        /* Write-through cache */
        StorageCache_t cache; /**< Fast read cache */

        /* Rate limiting */
        RateLimiter_t rate_limiter; /**< EEPROM wear protection */

        /*logger*/
        ILogger *logger;                /**< Logger interface for debug/info/error messages */
        IEventNotifier *event_notifier; /**< Event notifier para ERROR_EEPROM (opcional). */
        /* Observer Pattern (Phase 4.9) */
        ConfigSubscriber_t subscribers[CONFIG_SUBSCRIBERS_MAX]; /**< Config change subscribers */
    } StorageCoordinatorAO_v2_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Configuration Structure
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Configuration for StorageCoordinatorAO_v2 initialization.
     */
    typedef struct
    {
        IModularConfigStorage *config_storage; /**< Modular config storage (required) */
        uint32_t rate_limit_ms;                /**< Min interval between writes (default: 100ms) */
        ILogger *logger;                       /**< Logger interface (optional, for debug/info/error) */
        IEventNotifier *event_notifier;        /**< Event notifier para ERROR_EEPROM (opcional). */
    } StorageCoordinatorAOConfig_v2_t;

    /* ────────────────────────────────────────────────────────────────────────────
     * Public API
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Initialize Storage Coordinator Active Object v2.
     *
     * Creates thread, command queue, mutex, and loads initial cache from EEPROM.
     *
     * @param[in,out] self AO instance (not NULL).
     * @param[in]     config Configuration (not NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self or config is NULL.
     * @return ERR_ERROR if OS resources fail to create.
     */
    Result_t StorageCoordinatorAO_v2_Init(
        StorageCoordinatorAO_v2_t *self,
        const StorageCoordinatorAOConfig_v2_t *config);

    /**
     * @brief Arranca el thread worker (gate via run_sem).
     * @note Llamar después de Init(). El thread queda bloqueado en run_sem hasta aquí.
     */
    Result_t StorageCoordinatorAO_v2_Start(StorageCoordinatorAO_v2_t *self);

    /**
     * @brief Solicita que el thread worker salga del inner loop.
     * @note No bloquea. Llamar WaitStopped() para sincronizar.
     */
    Result_t StorageCoordinatorAO_v2_Stop(StorageCoordinatorAO_v2_t *self);

    /**
     * @brief Espera a que el thread confirme que el inner loop terminó.
     * @param[in] self       AO instance.
     * @param[in] timeout_ms Timeout en ms (OS_WAIT_FOREVER para espera infinita).
     */
    Result_t StorageCoordinatorAO_v2_WaitStopped(StorageCoordinatorAO_v2_t *self, uint32_t timeout_ms);

    /**
     * @brief De-inicializa AO y libera recursos OSAL.
     * @note Debe estar detenido (Stop+WaitStopped) antes de llamar.
     */
    Result_t StorageCoordinatorAO_v2_Deinit(StorageCoordinatorAO_v2_t *self);

    /**
     * @brief Shutdown Storage Coordinator Active Object v2 (Stop+WaitStopped+Deinit combinados).
     * @param[in] self AO instance.
     * @return ERR_OK on success.
     */
    Result_t StorageCoordinatorAO_v2_Shutdown(StorageCoordinatorAO_v2_t *self);

    /* ────────────────────────────────────────────────────────────────────────────
     * Save Operations (Async, enqueues command)
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Save RelayConfig (async, enqueues command).
     *
     * Thread-safe. Returns immediately. Actual write happens in background thread.
     *
     * @param[in] self AO instance.
     * @param[in] config RelayConfig to save.
     *
     * @return ERR_OK if command enqueued.
     * @return ERR_NULL_POINTER if self or config is NULL.
     * @return ERR_BUSY if queue is full.
     */
    Result_t StorageCoordinatorAO_v2_SaveRelayConfig(
        StorageCoordinatorAO_v2_t *self,
        const RelayConfig_t *config);

    Result_t StorageCoordinatorAO_v2_SaveGPSConfig(
        StorageCoordinatorAO_v2_t *self,
        const GPSConfig_t *config);

    Result_t StorageCoordinatorAO_v2_SaveGeneralConfig(
        StorageCoordinatorAO_v2_t *self,
        const GeneralConfig_t *config);

    /* StorageCoordinatorAO_v2_SaveTimeWindowConfig removed — use SaveRelayConfig; TimeWindow is relay.time_window */

    Result_t StorageCoordinatorAO_v2_SaveWifiConfig(
        StorageCoordinatorAO_v2_t *self,
        const WifiConfig_t *config);

    Result_t StorageCoordinatorAO_v2_SaveSuperUserConfig(
        StorageCoordinatorAO_v2_t *self,
        const SuperUserConfig_t *config);

    /* ────────────────────────────────────────────────────────────────────────────
     * Get Operations (Sync, reads from cache)
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Get RelayConfig (sync, reads from cache).
     *
     * Thread-safe. Instant return (no EEPROM access).
     *
     * @param[in]  self AO instance.
     * @param[out] out_config Buffer to receive config.
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self or out_config is NULL.
     */
    Result_t StorageCoordinatorAO_v2_GetRelayConfig(
        StorageCoordinatorAO_v2_t *self,
        RelayConfig_t *out_config);

    Result_t StorageCoordinatorAO_v2_GetGPSConfig(
        StorageCoordinatorAO_v2_t *self,
        GPSConfig_t *out_config);

    Result_t StorageCoordinatorAO_v2_GetGeneralConfig(
        StorageCoordinatorAO_v2_t *self,
        GeneralConfig_t *out_config);

    /* StorageCoordinatorAO_v2_GetTimeWindowConfig removed — use GetRelayConfig; access out->time_window */

    Result_t StorageCoordinatorAO_v2_GetWifiConfig(
        StorageCoordinatorAO_v2_t *self,
        WifiConfig_t *out_config);

    Result_t StorageCoordinatorAO_v2_GetSuperUserConfig(
        StorageCoordinatorAO_v2_t *self,
        SuperUserConfig_t *out_config);

    /* ────────────────────────────────────────────────────────────────────────────
     * Reset Operations (Async, enqueues command)
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Reset single config to defaults (async).
     *
     * @param[in] self AO instance.
     * @param[in] type Config type to reset.
     *
     * @return ERR_OK if command enqueued.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_BUSY if queue is full.
     */
    Result_t StorageCoordinatorAO_v2_ResetConfig(
        StorageCoordinatorAO_v2_t *self,
        ConfigType_t type);

    /**
     * @brief Factory reset all configs (async).
     *
     * @param[in] self AO instance.
     *
     * @return ERR_OK if command enqueued.
     * @return ERR_NULL_POINTER if self is NULL.
     * @return ERR_BUSY if queue is full.
     */
    Result_t StorageCoordinatorAO_v2_ResetAll(StorageCoordinatorAO_v2_t *self);

    /* ────────────────────────────────────────────────────────────────────────────
     * Observer Pattern APIs (Phase 4.9)
     * ──────────────────────────────────────────────────────────────────────────── */

    /**
     * @brief Suscribirse a notificaciones de cambio de config.
     *
     * Thread-safe. Callback será invocado en thread StorageCoordinator después de
     * escritura EEPROM exitosa.
     *
     * @warning Callback debe ser <1ms, no bloquear (no mutex largos, no I/O, no sleep).
     *
     * @param[in] self AO instance (not NULL).
     * @param[in] callback Función a invocar (not NULL).
     * @param[in] context User context (opcional, puede ser NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER si self o callback es NULL.
     * @return ERR_BUSY si tabla de subscribers está llena.
     */
    Result_t StorageCoordinatorAO_v2_SubscribeConfigChange(
        StorageCoordinatorAO_v2_t *self,
        ConfigChangeCallback_t callback,
        void *context);

    /**
     * @brief Desuscribirse de notificaciones.
     *
     * Remueve primera instancia del callback. Si callback está registrado múltiples
     * veces (con diferentes contexts), solo se remueve el primero.
     *
     * @param[in] self AO instance (not NULL).
     * @param[in] callback Función a remover (not NULL).
     *
     * @return ERR_OK si callback fue removido.
     * @return ERR_NULL_POINTER si self o callback es NULL.
     * @return ERR_NOT_FOUND si callback no estaba suscrito.
     */
    Result_t StorageCoordinatorAO_v2_UnsubscribeConfigChange(
        StorageCoordinatorAO_v2_t *self,
        ConfigChangeCallback_t callback);

#ifdef UNIT_TEST
    /**
     * @brief Test API: Forzar notificación de subscribers (sin escribir EEPROM).
     * @note Solo para unit tests. NO usar en producción.
     */
    void StorageCoordinatorAO_v2_NotifyConfigSubscribers_TestOnly(
        StorageCoordinatorAO_v2_t *self,
        ConfigType_t config_type);
#endif

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_COORDINATOR_AO_V2_H */
