/**
 * @file    wifi_enable_service.c
 * @brief   WiFi module power control service implementation.
 *
 * Implements IWifiModule interface with:
 * - Enable: De-assert ESP32 reset, save config, start timeout, buzzer (2 beeps)
 * - Disable: Assert ESP32 reset, save config, stop timeout, buzzer (1 long beep)
 * - Update: Check timeout watchdog, auto-disable if expired, buzzer (3 beeps)
 * - IsEnabled: Query current state
 *
 * @author  TCS Team
 * @date    2026-04-11
 */

#include "application/services/wifi_enable_service.h"
#include "common/wifi_types.h"
#include "infrastructure/osal/osal.h"
#include "Third_Party/esp_hosted/port/esp_hosted_os_port.h"
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════ */
/* ── Forward Declarations (V-Table Methods) ──────────────────────────────── */
/* ══════════════════════════════════════════════════════════════════════════ */

static Result_t wifi_enable_service_enable(void *self);
static Result_t wifi_enable_service_disable(void *self);
static bool wifi_enable_service_is_enabled(const void *self);
static Result_t wifi_enable_service_update(void *self);

/* ══════════════════════════════════════════════════════════════════════════ */
/* ── V-Table Instance ────────────────────────────────────────────────────── */
/* ══════════════════════════════════════════════════════════════════════════ */

static const IWifiModule_Vtable s_vtable = {
    .Enable = wifi_enable_service_enable,
    .Disable = wifi_enable_service_disable,
    .IsEnabled = wifi_enable_service_is_enabled,
    .Update = wifi_enable_service_update,
};

/* ══════════════════════════════════════════════════════════════════════════ */
/* ── Public API Implementation ───────────────────────────────────────────── */
/* ══════════════════════════════════════════════════════════════════════════ */

Result_t WifiEnableService_Init(WifiEnableService_t *self, const WifiEnableServiceConfig_t *config)
{
    /* Validate parameters */
    if (self == NULL)
        return ERR_NULL_POINTER;

    if (config == NULL)
        return ERR_NULL_POINTER;

    if (config->transport == NULL || config->config_storage == NULL || config->buzzer == NULL)
        return ERR_INVALID_PARAM;

    /* Clear instance */
    memset(self, 0, sizeof(*self));

    /* Store dependencies */
    self->transport = config->transport;
    self->config_storage = config->config_storage;
    self->buzzer = config->buzzer;
    self->logger = config->logger; /* Can be NULL */

    /* Store optional callbacks */
    self->on_wifi_disabled = config->on_wifi_disabled;
    self->on_wifi_disabled_ctx = config->on_wifi_disabled_ctx;

    /* Initialize state */
    self->enabled = false;
    self->enable_timestamp_ms = 0;
    self->timeout_minutes = 0;

    /* Setup interface */
    self->iface.vtable = &s_vtable;
    self->iface.impl = self;

    return ERR_OK;
}

IWifiModule_t *WifiEnableService_GetInterface(WifiEnableService_t *self)
{
    if (self == NULL)
        return NULL;

    return &self->iface;
}

Result_t WifiEnableService_SyncStartupState(WifiEnableService_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Load persisted config */
    WifiConfig_t wifi_cfg;
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &wifi_cfg) == ERR_OK)
    {
        const bool wifi_enabled = (wifi_cfg.wifi_enable != 0U);
        self->enabled = wifi_enabled;
        self->timeout_minutes = wifi_cfg.wifi_timeout_minutes;
        self->enable_timestamp_ms = wifi_enabled ? os_ticks_get() : 0U;

        ESP_Hosted_SetWifiEnabled(wifi_enabled);
        if (!wifi_enabled)
        {
            /* Keep ESP32 in reset when feature is disabled from config. */
            (void)WifiTransport_ResetESP32(self->transport, true);
        }
        else
        {
            /* Exit reset */
            (void)WifiTransport_ResetESP32(self->transport, false);
        }

        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__,
                       "Startup state synced: %s, timeout=%u min",
                       wifi_enabled ? "enabled" : "disabled", self->timeout_minutes);
        }
    }
    else
    {
        /* Fail-safe: allow hosted init if config cannot be read. */
        ESP_Hosted_SetWifiEnabled(true);

        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_WARN, "WIFI_ENABLE", __FILE__, __LINE__,
                       "Failed to load config at startup - defaulting to enabled");
        }
    }

    return ERR_OK;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/* ── V-Table Method Implementations ──────────────────────────────────────── */
/* ══════════════════════════════════════════════════════════════════════════ */

static Result_t wifi_enable_service_enable(void *self)
{
    WifiEnableService_t *svc = (WifiEnableService_t *)self;

    if (svc == NULL)
        return ERR_NULL_POINTER;

    /* Idempotent: If already enabled, return OK */
    if (svc->enabled)
    {
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_DEBUG, "WIFI_ENABLE", __FILE__, __LINE__, "WiFi already enabled, skipping");
        return ERR_OK;
    }

    if (svc->logger)
        Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__, "Enabling WiFi module (de-assert ESP32 reset)");

    /* 1. De-assert ESP32 reset (false = de-assert, start module) */
    Result_t res = WifiTransport_ResetESP32(svc->transport, false);
    if (res != ERR_OK)
        return res;

    /* 2. Update state */
    svc->enabled = true;
    svc->enable_timestamp_ms = os_ticks_get();

    /* 3. Load config to get timeout value */
    static WifiConfig_t config;
    res = ConfigStorage_LoadWifiConfig(svc->config_storage, &config);
    if (res == ERR_OK)
    {
        /* Clamp timeout to 0-60 min range */
        if (config.wifi_timeout_minutes > 60U)
        {
            svc->timeout_minutes = 60U;
        }
        else
        {
            svc->timeout_minutes = config.wifi_timeout_minutes;
        }
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__,
                       "Loaded timeout config: %u minutes", svc->timeout_minutes);
    }
    else
    {
        svc->timeout_minutes = 0; /* Default: always on */
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_WARN, "WIFI_ENABLE", __FILE__, __LINE__,
                       "Failed to load config, defaulting to always-on mode");
    }

    /* 4. Save wifi_enable=1 to EEPROM */
    config.wifi_enable = 1;
    res = ConfigStorage_SaveWifiConfig(svc->config_storage, &config);
    if (res != ERR_OK)
    {
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_WARN, "WIFI_ENABLE", __FILE__, __LINE__,
                       "Failed to save config, but module is already enabled");
    }

    /* 5. Buzzer feedback (module started) */
    BuzzerNotificationService_NotifyEvent(svc->buzzer, BUZZER_EVENT_BOOT_COMPLETE);

    if (svc->logger)
        Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__, "WiFi module enabled successfully");

    return ERR_OK;
}

static Result_t wifi_enable_service_disable(void *self)
{
    WifiEnableService_t *svc = (WifiEnableService_t *)self;

    if (svc == NULL)
        return ERR_NULL_POINTER;

    /* Idempotent: If already disabled, return OK */
    if (!svc->enabled)
    {
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_DEBUG, "WIFI_ENABLE", __FILE__, __LINE__, "WiFi already disabled, skipping");
        return ERR_OK;
    }

    if (svc->logger)
        Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__, "Disabling WiFi module (assert ESP32 reset)");

    /* 1. Assert ESP32 reset (true = assert, hold in reset) */
    Result_t res = WifiTransport_ResetESP32(svc->transport, true);
    if (res != ERR_OK)
        return res;

    /* 2. Update state */
    svc->enabled = false;

    /* 3. Save wifi_enable=0 to EEPROM */
    static WifiConfig_t config;
    res = ConfigStorage_LoadWifiConfig(svc->config_storage, &config);
    if (res == ERR_OK)
    {
        config.wifi_enable = 0;
        ConfigStorage_SaveWifiConfig(svc->config_storage, &config);
    }
    else if (svc->logger)
    {
        Logger_Log(svc->logger, LOG_LEVEL_WARN, "WIFI_ENABLE", __FILE__, __LINE__,
                   "Failed to save config after disable");
    }

    /* 4. Buzzer feedback (module shutdown) */
    BuzzerNotificationService_NotifyEvent(svc->buzzer, BUZZER_EVENT_SHUTDOWN_START);

    /* 5. Notify listener (e.g. WifiStatusAdapter) that WiFi is gone */
    if (svc->on_wifi_disabled != NULL)
    {
        svc->on_wifi_disabled(svc->on_wifi_disabled_ctx);
    }

    if (svc->logger)
        Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__, "WiFi module disabled successfully");

    return ERR_OK;
}

static bool wifi_enable_service_is_enabled(const void *self)
{
    const WifiEnableService_t *svc = (const WifiEnableService_t *)self;

    if (svc == NULL)
        return false;

    return svc->enabled;
}

static Result_t wifi_enable_service_update(void *self)
{
    WifiEnableService_t *svc = (WifiEnableService_t *)self;

    if (svc == NULL)
        return ERR_NULL_POINTER;

    /* If disabled, nothing to do */
    if (!svc->enabled)
        return ERR_OK;

    /* Reload config every update to detect changes in real-time */
    static WifiConfig_t config;
    Result_t res = ConfigStorage_LoadWifiConfig(svc->config_storage, &config);
    if (res == ERR_OK)
    {
        /* Clamp timeout to 0-60 min range */
        if (config.wifi_timeout_minutes > 60U)
        {
            svc->timeout_minutes = 60U;
        }
        else
        {
            svc->timeout_minutes = config.wifi_timeout_minutes;
        }
    }
    else
    {
        /* If config load fails, default to always-on */
        svc->timeout_minutes = 0;
    }

    /* If timeout is 0 (always on), nothing to do */
    if (svc->timeout_minutes == 0)
        return ERR_OK;

    /* Check timeout */
    uint32_t now_ms = os_ticks_get();
    uint32_t elapsed_ms = now_ms - svc->enable_timestamp_ms;
    uint32_t timeout_ms = (uint32_t)svc->timeout_minutes * 60000UL;

    if (elapsed_ms >= timeout_ms)
    {
        /* Timeout expired → auto-disable */
        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_WARN, "WIFI_ENABLE", __FILE__, __LINE__,
                       "WiFi timeout expired (%u min) - auto-disabling module", svc->timeout_minutes);

        /* Assert ESP32 reset */
        WifiTransport_ResetESP32(svc->transport, true);

        /* Update state */
        svc->enabled = false;

        /* Save config */
        if (ConfigStorage_LoadWifiConfig(svc->config_storage, &config) == ERR_OK)
        {
            config.wifi_enable = 0;
            ConfigStorage_SaveWifiConfig(svc->config_storage, &config);
        }

        /* Buzzer feedback (timeout = error condition) */
        BuzzerNotificationService_NotifyEvent(svc->buzzer, BUZZER_EVENT_ERROR_GENERIC);

        /* Notify listener (e.g. WifiStatusAdapter) that WiFi is gone */
        if (svc->on_wifi_disabled != NULL)
        {
            svc->on_wifi_disabled(svc->on_wifi_disabled_ctx);
        }

        if (svc->logger)
            Logger_Log(svc->logger, LOG_LEVEL_INFO, "WIFI_ENABLE", __FILE__, __LINE__,
                       "WiFi module auto-disabled after timeout");

        return ERR_TIMEOUT;
    }

    return ERR_OK;
}
