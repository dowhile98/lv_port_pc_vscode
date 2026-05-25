/**
 * @file storage_coordinator_ao_v2.c
 * @brief Simplified Storage Coordinator Active Object v2 (Phase 3, write-through cache).
 *
 * Implementation follows TDD Green phase: minimal code to pass 11 unit tests.
 */

#include "application/activeobjects/storage_coordinator_ao_v2.h"
#include "infrastructure/osal/osal.h"
#include "include/interfaces/i_logger.h"
#include <string.h>

/* ────────────────────────────────────────────────────────────────────────────
 * constants
 * ──────────────────────────────────────────────────────────────────────────── */
static const char *TAG = "SCAO";

/* ────────────────────────────────────────────────────────────────────────────
 * Forward Declarations
 * ──────────────────────────────────────────────────────────────────────────── */

static void notify_config_subscribers(
    StorageCoordinatorAO_v2_t *self,
    ConfigType_t config_type);

/* ────────────────────────────────────────────────────────────────────────────
 * Private: Thread Function
 * ──────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Worker thread processing storage commands.
 */
static void storage_thread_func(void *context)
{
    StorageCoordinatorAO_v2_t *self = (StorageCoordinatorAO_v2_t *)context;

    if (!self)
    {
        return;
    }

    ILogger *logger = self->logger;

    StorageCommand_t cmd;

    while (!self->terminate)
    {
        os_semaphore_get(self->run_sem, OS_WAIT_FOREVER);
        if (self->terminate)
        {
            break;
        }

        LOG_INFO(logger, TAG, "Storage Coordinator AO v2 thread started");

        while (self->is_running)
        {
            /* Try to receive command (non-blocking) */
            Result_t res = os_queue_receive(self->command_queue, &cmd, 100);
            if (res != ERR_OK)
            {
                continue; /* No command available */
            }

            /* Check rate limiting */
            uint32_t now = os_ticks_get();
            uint32_t last_write = self->rate_limiter.last_write_tick[cmd.config_type];
            uint32_t min_interval = self->rate_limiter.min_interval_ms;

            if ((now - last_write) < min_interval && last_write != 0)
            {
                /* Too soon, skip write */
                continue;
            }

            /* Process command */
            switch (cmd.type)
            {
            case STORAGE_CMD_SAVE_CONFIG:
            {
                LOG_INFO(logger, TAG, "Processing save command for config type %d", cmd.config_type);
                /* Write to EEPROM using type-safe inline wrappers */
                IModularConfigStorage *storage = self->config_storage;

                /* Dispatch to appropriate wrapper based on config_type */
                switch (cmd.config_type)
                {
                case CONFIG_TYPE_RELAY:
                    res = ModularConfigStorage_SaveRelay(storage, (const RelayConfig_t *)cmd.config_data);
                    break;
                case CONFIG_TYPE_GPS:
                    res = ModularConfigStorage_SaveGPS(storage, (const GPSConfig_t *)cmd.config_data);
                    break;
                case CONFIG_TYPE_GENERAL:
                    res = ModularConfigStorage_SaveGeneral(storage, (const GeneralConfig_t *)cmd.config_data);
                    break;
                case CONFIG_TYPE_WIFI:
                    res = ModularConfigStorage_SaveWifi(storage, (const WifiConfig_t *)cmd.config_data);
                    break;
                case CONFIG_TYPE_SUPERUSER:
                    res = ModularConfigStorage_SaveSuperUser(storage, (const SuperUserConfig_t *)cmd.config_data);
                    break;
                default:
                    res = ERR_INVALID_PARAM;
                    break;
                }

                /* Write-through: Update cache ONLY on success */
                if (res == ERR_OK)
                {
                    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);

                    switch (cmd.config_type)
                    {
                    case CONFIG_TYPE_RELAY:
                        memcpy(&self->cache.relay, cmd.config_data, sizeof(RelayConfig_t));
                        break;
                    case CONFIG_TYPE_GPS:
                        memcpy(&self->cache.gps, cmd.config_data, sizeof(GPSConfig_t));
                        break;
                    case CONFIG_TYPE_GENERAL:
                        memcpy(&self->cache.general, cmd.config_data, sizeof(GeneralConfig_t));
                        break;
                    case CONFIG_TYPE_WIFI:
                        memcpy(&self->cache.wifi, cmd.config_data, sizeof(WifiConfig_t));
                        break;
                    case CONFIG_TYPE_SUPERUSER:
                        memcpy(&self->cache.superuser, cmd.config_data, sizeof(SuperUserConfig_t));
                        break;
                    default:
                        break;
                    }

                    /* Phase 4.9: Notificar subscribers ANTES de liberar mutex */
                    LOG_INFO(logger, TAG, "Notifying subscribers of config type %d change", cmd.config_type);

                    notify_config_subscribers(self, cmd.config_type);

                    os_mutex_release(self->cache.mutex);

                    /* Update rate limiter */
                    self->rate_limiter.last_write_tick[cmd.config_type] = now;
                }
                else
                {
                    LOG_ERROR(logger, TAG, "Failed to save config type %d: %d", cmd.config_type, res);
                    if (EventNotifier_IsValid(self->event_notifier))
                    {
                        DateTime_t err_ts = {0};
                        (void)EventNotifier_NotifyEvent(self->event_notifier, &err_ts,
                                                        EVENT_TYPE_ERROR_EEPROM,
                                                        EVENT_SEVERITY_ERROR,
                                                        (uint16_t)cmd.config_type, 0U);
                    }
                }

                /* Free allocated memory (caller allocated via os_alloc) */
                if (cmd.config_data)
                {
                    os_free(cmd.config_data);
                }
                break;
            }

            case STORAGE_CMD_RESET_CONFIG:
            {
                LOG_INFO(logger, TAG, "Processing reset command for config type %d", cmd.config_type);
                /* Reset single config using type-safe inline wrappers */
                IModularConfigStorage *storage = self->config_storage;

                switch (cmd.config_type)
                {
                case CONFIG_TYPE_RELAY:
                    res = ModularConfigStorage_ResetRelay(storage);
                    break;
                case CONFIG_TYPE_GPS:
                    res = ModularConfigStorage_ResetGPS(storage);
                    break;
                case CONFIG_TYPE_GENERAL:
                    res = ModularConfigStorage_ResetGeneral(storage);
                    break;
                case CONFIG_TYPE_WIFI:
                    res = ModularConfigStorage_ResetWifi(storage);
                    break;
                case CONFIG_TYPE_SUPERUSER:
                    res = ModularConfigStorage_ResetSuperUser(storage);
                    break;
                default:
                    res = ERR_INVALID_PARAM;
                    break;
                }

                /* Reload defaults into cache and notify subscribers */
                if (res == ERR_OK)
                {
                    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);

                    switch (cmd.config_type)
                    {
                    case CONFIG_TYPE_RELAY:
                        res = ModularConfigStorage_LoadRelay(storage, &self->cache.relay);
                        break;
                    case CONFIG_TYPE_GPS:
                        res = ModularConfigStorage_LoadGPS(storage, &self->cache.gps);
                        break;
                    case CONFIG_TYPE_GENERAL:
                        res = ModularConfigStorage_LoadGeneral(storage, &self->cache.general);
                        break;
                    case CONFIG_TYPE_WIFI:
                        res = ModularConfigStorage_LoadWifi(storage, &self->cache.wifi);
                        break;
                    case CONFIG_TYPE_SUPERUSER:
                        res = ModularConfigStorage_LoadSuperUser(storage, &self->cache.superuser);
                        break;
                    default:
                        break;
                    }

                    if (res == ERR_OK)
                    {
                        notify_config_subscribers(self, cmd.config_type);
                    }
                    else
                    {
                        LOG_ERROR(logger, TAG, "Failed to reload cache after reset, type %d: %d", cmd.config_type, res);
                    }

                    os_mutex_release(self->cache.mutex);
                }
                else
                {
                    LOG_ERROR(logger, TAG, "Failed to reset config type %d: %d", cmd.config_type, res);
                }
                break;
            }

            case STORAGE_CMD_RESET_ALL:
            {
                /* Factory reset all configs using type-safe wrapper */
                LOG_INFO(logger, TAG, "Processing reset all command");
                IModularConfigStorage *storage = self->config_storage;
                res = ModularConfigStorage_ResetAll(storage);

                if (res != ERR_OK)
                {
                    LOG_ERROR(logger, TAG, "Failed to reset all configs: %d", res);
                }
                break;
            }

            default:
                break;
            }
        }
        LOG_INFO(logger, TAG, "Storage Coordinator AO v2 thread stopped");
        (void)os_semaphore_put(self->stopped_sem); /* ← WaitStopped() */
    }
    (void)os_semaphore_put(self->stopped_sem); /* ← Deinit() */
}

/* ────────────────────────────────────────────────────────────────────────────
 * Private: Load initial cache from EEPROM
 * ──────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Load initial cache from EEPROM.
 * @note On first boot (ERR_CHECKSUM), writes defaults to EEPROM.
 * @note Handlers load defaults into cache when magic invalid, so cache is always populated.
 */
static Result_t load_initial_cache(StorageCoordinatorAO_v2_t *self)
{
    if (!self || !self->config_storage)
    {
        return ERR_NULL_POINTER;
    }

    IModularConfigStorage *storage = self->config_storage;
    Result_t res;
    bool first_boot = false;

    /* ========== Header: Configuration Loading ========== */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE", "========== Configuration Loading from EEPROM ==========");
    }

    /* Load Relay Config */
    res = ModularConfigStorage_LoadRelay(storage, &self->cache.relay);
    if (res == ERR_CHECKSUM)
    {
        /* First boot or corrupted - write defaults (already in cache) */
        first_boot = true;
        res = ModularConfigStorage_SaveRelay(storage, &self->cache.relay);
        if (res != ERR_OK)
            return res;
    }
    else if (res != ERR_OK)
    {
        return res; /* Other error (hardware failure) */
    }

    /* Log loaded Relay configuration */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE",
                 "RELAY      | Type: %-2s | Enabled: %-3s | Ton: %5lums | Toff: %5lums | Multi: %s",
                 self->cache.relay.contact_type == RELAY_TYPE_NO ? "NO" : "NC",
                 self->cache.relay.enabled ? "Yes" : "No",
                 self->cache.relay.simple_cycle.ton,
                 self->cache.relay.simple_cycle.toff,
                 self->cache.relay.multicycle.enabled ? "On" : "Off");

        /* Show multicycle details if enabled */
        if (self->cache.relay.multicycle.enabled)
        {
            LOG_INFO(self->logger, "STORAGE",
                     "           | StopAtEnd: %-3s | Cycles: %u configured",
                     self->cache.relay.multicycle.stop_at_end ? "Yes" : "No",
                     RELAY_MAX_CYCLES);

            for (uint8_t i = 0; i < RELAY_MAX_CYCLES; i++)
            {
                LOG_INFO(self->logger, "STORAGE",
                         "           | Cycle[%u]: Ton=%5lums | Toff=%5lums | Until: %02u/%02u",
                         i,
                         self->cache.relay.multicycle.ton[i],
                         self->cache.relay.multicycle.toff[i],
                         self->cache.relay.multicycle.boundary_dates[i].day,
                         self->cache.relay.multicycle.boundary_dates[i].month);
            }
        }
    }

    /* Load GPS Config */
    res = ModularConfigStorage_LoadGPS(storage, &self->cache.gps);
    if (res == ERR_CHECKSUM)
    {
        first_boot = true;
        res = ModularConfigStorage_SaveGPS(storage, &self->cache.gps);
        if (res != ERR_OK)
            return res;
    }
    else if (res != ERR_OK)
    {
        return res;
    }

    /* Log loaded GPS configuration */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE",
                 "GPS        | Antenna: %-8s | UTC Index: #%-2u | Time Offset: %+3ds",
                 self->cache.gps.antenna_type == 0 ? "Internal" : "External",
                 self->cache.gps.utc_offset_index,
                 self->cache.gps.time_offset);
    }

    /* Load General Config */
    res = ModularConfigStorage_LoadGeneral(storage, &self->cache.general);
    if (res == ERR_CHECKSUM)
    {
        first_boot = true;
        res = ModularConfigStorage_SaveGeneral(storage, &self->cache.general);
        if (res != ERR_OK)
            return res;
    }
    else if (res != ERR_OK)
    {
        return res;
    }

    /* Log loaded General configuration */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE",
                 "GENERAL    | Buzzer: %4lums | TempAlarm: %-3s | Backlight: %5lums",
                 self->cache.general.buzzer_on_time_ms,
                 self->cache.general.buzzer_high_temp_alarm ? "On" : "Off",
                 self->cache.general.screen_blacklight_timeout_ms);
    }

    /* Load Wifi Config */
    /* Note: TimeWindow eliminated — now part of RelayConfig_t.time_window */
    res = ModularConfigStorage_LoadWifi(storage, &self->cache.wifi);
    if (res == ERR_CHECKSUM)
    {
        first_boot = true;
        res = ModularConfigStorage_SaveWifi(storage, &self->cache.wifi);
        if (res != ERR_OK)
            return res;
    }
    else if (res != ERR_OK)
    {
        return res;
    }

    /* Log loaded WiFi configuration */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE",
                 "WIFI       | STA: '%-20s' | AP: '%-20s' | Mode: %u | DHCP: %s",
                 self->cache.wifi.sta_ssid,
                 self->cache.wifi.ap_ssid,
                 self->cache.wifi.mode,
                 self->cache.wifi.sta_use_dhcp ? "On" : "Off");
    }

    /* Load SuperUser Config */
    res = ModularConfigStorage_LoadSuperUser(storage, &self->cache.superuser);
    if (res == ERR_CHECKSUM)
    {
        first_boot = true;
        res = ModularConfigStorage_SaveSuperUser(storage, &self->cache.superuser);
        if (res != ERR_OK)
            return res;
    }
    else if (res != ERR_OK)
    {
        return res;
    }
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE",
                 "SUPERUSER  | License: 0x%08lX | Expiration: %10lu | GPS_PPS_Offset: %+4d us",
                 self->cache.superuser.license_key,
                 self->cache.superuser.expiration_date,
                 self->cache.superuser.gps_pps_offset_us);
    }

    /* ========== Footer ========== */
    if (self->logger)
    {
        LOG_INFO(self->logger, "STORAGE", "========================================================");
    }

    /* Log first boot if it occurred */
    if (first_boot && self->logger)
    {
        LOG_WARN(self->logger, "STORAGE", "⚠️  FIRST BOOT DETECTED - Defaults written to EEPROM");
    }

    return ERR_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API: Init
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t StorageCoordinatorAO_v2_Init(
    StorageCoordinatorAO_v2_t *self,
    const StorageCoordinatorAOConfig_v2_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    if (!config->config_storage)
    {
        return ERR_NULL_POINTER;
    }

    /* Initialize struct */
    memset(self, 0, sizeof(StorageCoordinatorAO_v2_t));
    self->config_storage = config->config_storage;

    self->logger = config->logger;                 /* Optional, can be NULL */
    self->event_notifier = config->event_notifier; /* Optional, can be NULL */
    /* Setup rate limiter */
    self->rate_limiter.min_interval_ms = config->rate_limit_ms > 0 ? config->rate_limit_ms : STORAGE_RATE_LIMIT_MS;
    for (size_t i = 0; i <= CONFIG_TYPE_SUPERUSER; i++)
    {
        self->rate_limiter.last_write_tick[i] = 0;
    }

    /* Create mutex for cache */
    Result_t res = os_mutex_create(&self->cache.mutex, "storage_cache_mutex");
    if (res != ERR_OK)
        return ERR_ERROR;

    /* Create command queue */
    static uint8_t queue_buffer[STORAGE_COMMAND_QUEUE_SIZE * sizeof(StorageCommand_t)];
    os_queue_config_t queue_cfg = {
        .name = "storage_cmd_queue",
        .buffer = queue_buffer,
        .buffer_size = sizeof(queue_buffer),
        .item_size = sizeof(StorageCommand_t)};
    res = os_queue_create(&self->command_queue, &queue_cfg);
    if (res != ERR_OK)
    {
        os_mutex_delete(self->cache.mutex);
        return ERR_ERROR;
    }

    /* stopped_sem: binary, init=0 */
    if (os_semaphore_create(&self->stopped_sem, "SCAO_STOP_SEM", 0U) != ERR_OK)
    {
        os_queue_delete(self->command_queue);
        os_mutex_delete(self->cache.mutex);
        return ERR_ERROR;
    }

    /* run_sem: binary, init=0 — gate para el outer loop */
    if (os_semaphore_create(&self->run_sem, "SCAO_RUN_SEM", 0U) != ERR_OK)
    {
        os_semaphore_delete(self->stopped_sem);
        os_queue_delete(self->command_queue);
        os_mutex_delete(self->cache.mutex);
        return ERR_ERROR;
    }

    /* Load initial cache from EEPROM */
    res = load_initial_cache(self);
    if (res != ERR_OK)
    {
        os_semaphore_delete(self->run_sem);
        os_semaphore_delete(self->stopped_sem);
        os_queue_delete(self->command_queue);
        os_mutex_delete(self->cache.mutex);
        return ERR_ERROR;
    }

    /* Create worker thread with static stack from struct — queda bloqueado en run_sem hasta Start() */
    os_thread_config_t thread_cfg = {
        .name = "storage_worker",
        .entry = storage_thread_func,
        .arg = self,
        .stack_ptr = self->thread_stack,
        .stack_size = sizeof(self->thread_stack),
        .priority = 10,
        .auto_start = true};
    res = os_thread_create(&self->thread, &thread_cfg);
    if (res != ERR_OK)
    {
        os_semaphore_delete(self->run_sem);
        os_semaphore_delete(self->stopped_sem);
        os_queue_delete(self->command_queue);
        os_mutex_delete(self->cache.mutex);
        return ERR_ERROR;
    }

    return ERR_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API: Shutdown
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t StorageCoordinatorAO_v2_Start(StorageCoordinatorAO_v2_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    if (self->is_running)
        return ERR_BUSY;

    self->is_running = true;
    (void)os_semaphore_put(self->run_sem);
    LOG_INFO(self->logger, TAG, "Started");
    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_Stop(StorageCoordinatorAO_v2_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    self->is_running = false;
    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_WaitStopped(StorageCoordinatorAO_v2_t *self, uint32_t timeout_ms)
{
    if (!self)
        return ERR_NULL_POINTER;
    return os_semaphore_get(self->stopped_sem, timeout_ms);
}

Result_t StorageCoordinatorAO_v2_Deinit(StorageCoordinatorAO_v2_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    if (self->is_running)
        return ERR_BUSY; /* Llamar Stop() + WaitStopped() antes */

    /* Despertar el outer loop con la señal de terminación */
    self->terminate = true;
    (void)os_semaphore_put(self->run_sem);
    (void)os_semaphore_get(self->stopped_sem, OS_WAIT_FOREVER);

    os_semaphore_delete(self->run_sem);
    os_semaphore_delete(self->stopped_sem);
    if (self->command_queue)
    {
        os_queue_delete(self->command_queue);
        self->command_queue = NULL;
    }
    if (self->cache.mutex)
    {
        os_mutex_delete(self->cache.mutex);
        self->cache.mutex = NULL;
    }
    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_Shutdown(StorageCoordinatorAO_v2_t *self)
{
    if (!self)
        return ERR_NULL_POINTER;
    (void)StorageCoordinatorAO_v2_Stop(self);
    (void)StorageCoordinatorAO_v2_WaitStopped(self, OS_WAIT_FOREVER);
    return StorageCoordinatorAO_v2_Deinit(self);
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API: Save Operations (Async, enqueues command)
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t StorageCoordinatorAO_v2_SaveRelayConfig(
    StorageCoordinatorAO_v2_t *self,
    const RelayConfig_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    /* Allocate memory for config copy (freed by worker thread) */
    void *config_copy = NULL;
    Result_t res = os_alloc(sizeof(RelayConfig_t), &config_copy);
    if (res != ERR_OK || !config_copy)
    {
        return ERR_NO_MEMORY;
    }

    memcpy(config_copy, config, sizeof(RelayConfig_t));

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_SAVE_CONFIG,
        .config_type = CONFIG_TYPE_RELAY,
        .config_data = config_copy,
        .config_size = sizeof(RelayConfig_t)};

    res = os_queue_send(self->command_queue, &cmd, 0);
    if (res != ERR_OK)
    {
        os_free(config_copy); /* Free on send failure */
    }

    return res;
}

Result_t StorageCoordinatorAO_v2_SaveGPSConfig(
    StorageCoordinatorAO_v2_t *self,
    const GPSConfig_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    /* Allocate memory for config copy (freed by worker thread) */
    void *config_copy = NULL;
    Result_t res = os_alloc(sizeof(GPSConfig_t), &config_copy);
    if (res != ERR_OK || !config_copy)
    {
        return ERR_NO_MEMORY;
    }

    memcpy(config_copy, config, sizeof(GPSConfig_t));

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_SAVE_CONFIG,
        .config_type = CONFIG_TYPE_GPS,
        .config_data = config_copy,
        .config_size = sizeof(GPSConfig_t)};

    res = os_queue_send(self->command_queue, &cmd, 0);
    if (res != ERR_OK)
    {
        os_free(config_copy); /* Free on send failure */
    }

    return res;
}

Result_t StorageCoordinatorAO_v2_SaveGeneralConfig(
    StorageCoordinatorAO_v2_t *self,
    const GeneralConfig_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    /* Allocate memory for config copy (freed by worker thread) */
    void *config_copy = NULL;
    Result_t res = os_alloc(sizeof(GeneralConfig_t), &config_copy);
    if (res != ERR_OK || !config_copy)
    {
        return ERR_NO_MEMORY;
    }

    memcpy(config_copy, config, sizeof(GeneralConfig_t));

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_SAVE_CONFIG,
        .config_type = CONFIG_TYPE_GENERAL,
        .config_data = config_copy,
        .config_size = sizeof(GeneralConfig_t)};

    res = os_queue_send(self->command_queue, &cmd, 0);
    if (res != ERR_OK)
    {
        os_free(config_copy); /* Free on send failure */
    }

    return res;
}

/* StorageCoordinatorAO_v2_SaveTimeWindowConfig removed — TimeWindow now embedded in RelayConfig_t */

Result_t StorageCoordinatorAO_v2_SaveWifiConfig(
    StorageCoordinatorAO_v2_t *self,
    const WifiConfig_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    /* Allocate memory for config copy (freed by worker thread) */
    void *config_copy = NULL;
    Result_t res = os_alloc(sizeof(WifiConfig_t), &config_copy);
    if (res != ERR_OK || !config_copy)
    {
        return ERR_NO_MEMORY;
    }

    memcpy(config_copy, config, sizeof(WifiConfig_t));

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_SAVE_CONFIG,
        .config_type = CONFIG_TYPE_WIFI,
        .config_data = config_copy,
        .config_size = sizeof(WifiConfig_t)};

    res = os_queue_send(self->command_queue, &cmd, 0);
    if (res != ERR_OK)
    {
        os_free(config_copy); /* Free on send failure */
    }

    return res;
}

Result_t StorageCoordinatorAO_v2_SaveSuperUserConfig(
    StorageCoordinatorAO_v2_t *self,
    const SuperUserConfig_t *config)
{
    if (!self || !config)
    {
        return ERR_NULL_POINTER;
    }

    /* Allocate memory for config copy (freed by worker thread) */
    void *config_copy = NULL;
    Result_t res = os_alloc(sizeof(SuperUserConfig_t), &config_copy);
    if (res != ERR_OK || !config_copy)
    {
        return ERR_NO_MEMORY;
    }

    memcpy(config_copy, config, sizeof(SuperUserConfig_t));

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_SAVE_CONFIG,
        .config_type = CONFIG_TYPE_SUPERUSER,
        .config_data = config_copy,
        .config_size = sizeof(SuperUserConfig_t)};

    res = os_queue_send(self->command_queue, &cmd, 0);
    if (res != ERR_OK)
    {
        os_free(config_copy); /* Free on send failure */
    }

    return res;
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API: Get Operations (Sync, reads from cache)
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t StorageCoordinatorAO_v2_GetRelayConfig(
    StorageCoordinatorAO_v2_t *self,
    RelayConfig_t *out_config)
{
    if (!self || !out_config)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);
    memcpy(out_config, &self->cache.relay, sizeof(RelayConfig_t));
    os_mutex_release(self->cache.mutex);

    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_GetGPSConfig(
    StorageCoordinatorAO_v2_t *self,
    GPSConfig_t *out_config)
{
    if (!self || !out_config)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);
    memcpy(out_config, &self->cache.gps, sizeof(GPSConfig_t));
    os_mutex_release(self->cache.mutex);

    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_GetGeneralConfig(
    StorageCoordinatorAO_v2_t *self,
    GeneralConfig_t *out_config)
{
    if (!self || !out_config)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);
    memcpy(out_config, &self->cache.general, sizeof(GeneralConfig_t));
    os_mutex_release(self->cache.mutex);

    return ERR_OK;
}

/* StorageCoordinatorAO_v2_GetTimeWindowConfig removed — access via relay.time_window */

Result_t StorageCoordinatorAO_v2_GetWifiConfig(
    StorageCoordinatorAO_v2_t *self,
    WifiConfig_t *out_config)
{
    if (!self || !out_config)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);
    memcpy(out_config, &self->cache.wifi, sizeof(WifiConfig_t));
    os_mutex_release(self->cache.mutex);

    return ERR_OK;
}

Result_t StorageCoordinatorAO_v2_GetSuperUserConfig(
    StorageCoordinatorAO_v2_t *self,
    SuperUserConfig_t *out_config)
{
    if (!self || !out_config)
    {
        return ERR_NULL_POINTER;
    }

    os_mutex_acquire(self->cache.mutex, OS_WAIT_FOREVER);
    memcpy(out_config, &self->cache.superuser, sizeof(SuperUserConfig_t));
    os_mutex_release(self->cache.mutex);

    return ERR_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
 * Public API: Reset Operations (Async, enqueues command)
 * ──────────────────────────────────────────────────────────────────────────── */

Result_t StorageCoordinatorAO_v2_ResetConfig(
    StorageCoordinatorAO_v2_t *self,
    ConfigType_t type)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_RESET_CONFIG,
        .config_type = type};

    return os_queue_send(self->command_queue, &cmd, 0);
}

Result_t StorageCoordinatorAO_v2_ResetAll(StorageCoordinatorAO_v2_t *self)
{
    if (!self)
    {
        return ERR_NULL_POINTER;
    }

    StorageCommand_t cmd = {
        .type = STORAGE_CMD_RESET_ALL,
        .config_type = CONFIG_TYPE_RELAY /* Ignored for RESET_ALL */
    };

    return os_queue_send(self->command_queue, &cmd, 0);
}

/* ────────────────────────────────────────────────────────────────────────────
 * Observer Pattern Implementation (Phase 4.9)
 * ──────────────────────────────────────────────────────────────────────────── */

/**
 * @brief Helper: Notificar todos los subscribers de un cambio de config.
 * @note Llamado DESPUÉS de actualizar cache exitosamente (mutex debe estar acquired).
 * @param self AO instance.
 * @param config_type Tipo de config que cambió.
 */
static void notify_config_subscribers(
    StorageCoordinatorAO_v2_t *self,
    ConfigType_t config_type)
{
    if (!self)
    {
        return;
    }

    /* Obtener puntero al config en cache */
    const void *config_data = NULL;
    size_t config_size = 0;

    switch (config_type)
    {
    case CONFIG_TYPE_RELAY:
        config_data = &self->cache.relay;
        config_size = sizeof(RelayConfig_t);
        break;
    case CONFIG_TYPE_GPS:
        config_data = &self->cache.gps;
        config_size = sizeof(GPSConfig_t);
        break;
    case CONFIG_TYPE_GENERAL:
        config_data = &self->cache.general;
        config_size = sizeof(GeneralConfig_t);
        break;
    case CONFIG_TYPE_WIFI:
        config_data = &self->cache.wifi;
        config_size = sizeof(WifiConfig_t);
        break;
    case CONFIG_TYPE_SUPERUSER:
        config_data = &self->cache.superuser;
        config_size = sizeof(SuperUserConfig_t);
        break;
    default:
        return; /* Config type desconocido */
    }

    /* Invocar todos los callbacks registrados */
    for (uint8_t i = 0; i < CONFIG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback != NULL)
        {
            self->subscribers[i].callback(
                self->subscribers[i].context,
                config_type,
                config_data,
                config_size);
        }
    }
}

Result_t StorageCoordinatorAO_v2_SubscribeConfigChange(
    StorageCoordinatorAO_v2_t *self,
    ConfigChangeCallback_t callback,
    void *context)
{
    if (!self || !callback)
    {
        return ERR_NULL_POINTER;
    }

    /* Buscar slot libre en tabla de subscribers */
    for (uint8_t i = 0; i < CONFIG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback == NULL)
        {
            self->subscribers[i].callback = callback;
            self->subscribers[i].context = context;
            return ERR_OK;
        }
    }

    return ERR_BUSY; /* Tabla llena */
}

Result_t StorageCoordinatorAO_v2_UnsubscribeConfigChange(
    StorageCoordinatorAO_v2_t *self,
    ConfigChangeCallback_t callback)
{
    if (!self || !callback)
    {
        return ERR_NULL_POINTER;
    }

    /* Buscar primera instancia del callback y remover */
    for (uint8_t i = 0; i < CONFIG_SUBSCRIBERS_MAX; i++)
    {
        if (self->subscribers[i].callback == callback)
        {
            self->subscribers[i].callback = NULL;
            self->subscribers[i].context = NULL;
            return ERR_OK;
        }
    }

    return ERR_NOT_FOUND; /* Callback no estaba suscrito */
}

#ifdef UNIT_TEST
/**
 * @brief Test API: Forzar notificación sin escribir EEPROM.
 */
void StorageCoordinatorAO_v2_NotifyConfigSubscribers_TestOnly(
    StorageCoordinatorAO_v2_t *self,
    ConfigType_t config_type)
{
    if (!self)
    {
        return;
    }

    notify_config_subscribers(self, config_type);
}
#endif
