/**
 * @file license_service.c
 * @brief Domain service — License mode and expiration logic.
 *
 * Reads SuperUserConfig_t from IConfigStorage, compares expiration_date
 * against the current ITimeSource time, and returns LicenseStatus_t.
 *
 * ## Fail-safe policy
 * - Storage load fails  → LICENSE_MODE_FREE   (fail open — never lock permanently)
 * - Time source fails   → LICENSE_MODE_RENTAL_ACTIVE (fail open — never lock permanently)
 *
 * @note NO #include "lvgl.h" — intentional: PC-testable domain module.
 *
 * @author Tecna Smart Lab
 * @date   7 de Abril 2026
 */
/* NO #include "lvgl.h" — intentional */
#include "domain/services/license_service.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_license_status.h"
#include "common/date_time.h"
#include <string.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════
 * Internal constants
 * ══════════════════════════════════════════════════════════════════════════ */

#define LICENSE_KEY_FREE 0U   /**< license_key value → free mode */
#define LICENSE_KEY_RENTAL 1U /**< license_key value → rental mode */

#define SECONDS_PER_DAY 86400U

/* ══════════════════════════════════════════════════════════════════════════
 * Private helpers
 * ══════════════════════════════════════════════════════════════════════════ */

static Result_t compute_status(LicenseService_t *self, LicenseStatus_t *out)
{
    out->mode = LICENSE_MODE_FREE;
    out->days_remaining = 0;

    /* 1. Load SuperUserConfig */
    SuperUserConfig_t super_cfg;
    memset(&super_cfg, 0, sizeof(super_cfg));

    if (ConfigStorage_LoadSuperUserConfig(self->config_storage, &super_cfg) != ERR_OK)
    {
        /* Fail open: storage unavailable → treat as free */
        out->mode = LICENSE_MODE_FREE;
        return ERR_OK;
    }

    /* 2. FREE mode: license_key == 0 → no further checks */
    if (super_cfg.license_key == LICENSE_KEY_FREE)
    {
        out->mode = LICENSE_MODE_FREE;
        out->days_remaining = 0;
        return ERR_OK;
    }

    /* 3. RENTAL mode: compare current time against expiration_date */
    DateTime_t now;
    if (self->time_source == NULL || TimeSource_GetTime(self->time_source, &now) != ERR_OK)
    {
        /* Fail open: time unavailable → treat as active to avoid false lockout */
        out->mode = LICENSE_MODE_RENTAL_ACTIVE;
        out->days_remaining = 1; /* Optimistic: 1 day to indicate "unknown but active" */
        return ERR_OK;
    }

    uint32_t now_unix = DateTime_ToUnix(&now);

    /* days_remaining = floor((expiration - now) / 86400) */
    if (now_unix < super_cfg.expiration_date)
    {
        uint32_t diff_seconds = super_cfg.expiration_date - now_unix;
        out->days_remaining = (int32_t)(diff_seconds / SECONDS_PER_DAY);
        /* Ensure at least 1 if any time remains (avoids showing 0 days when < 1 day left) */
        if (out->days_remaining == 0)
        {
            out->days_remaining = 1;
        }
        out->mode = LICENSE_MODE_RENTAL_ACTIVE;
    }
    else
    {
        /* now >= expiration → expired */
        uint32_t diff_seconds = now_unix - super_cfg.expiration_date;
        out->days_remaining = -(int32_t)(diff_seconds / SECONDS_PER_DAY);
        out->mode = LICENSE_MODE_RENTAL_EXPIRED;
    }

    return ERR_OK;
}

/* ══════════════════════════════════════════════════════════════════════════
 * ILicenseStatus vtable implementations
 * ══════════════════════════════════════════════════════════════════════════ */

static Result_t vtable_get_status(void *impl, LicenseStatus_t *out)
{
    if (impl == NULL || out == NULL)
        return ERR_NULL_POINTER;
    LicenseService_t *self = (LicenseService_t *)impl;
    return compute_status(self, out);
}

static bool vtable_is_blocked(void *impl)
{
    if (impl == NULL)
        return false;
    LicenseService_t *self = (LicenseService_t *)impl;
    LicenseStatus_t status;
    if (compute_status(self, &status) != ERR_OK)
        return false;
    return (status.mode == LICENSE_MODE_RENTAL_EXPIRED);
}

static const ILicenseStatus_Vtable k_vtable = {
    .GetStatus = vtable_get_status,
    .IsBlocked = vtable_is_blocked,
};

/* ══════════════════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════════════════ */

Result_t LicenseService_Init(LicenseService_t *self, const LicenseServiceConfig_t *cfg)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (cfg == NULL)
        return ERR_NULL_POINTER;
    if (cfg->config_storage == NULL)
        return ERR_NULL_POINTER;

    memset(self, 0, sizeof(*self));

    self->iface.vtable = &k_vtable;
    self->iface.impl = self;
    self->config_storage = cfg->config_storage;
    self->time_source = cfg->time_source; /* NULL allowed → fail open */

    return ERR_OK;
}

ILicenseStatus *LicenseService_GetInterface(LicenseService_t *self)
{
    if (self == NULL)
        return NULL;
    return &self->iface;
}

Result_t LicenseService_GetStatus(LicenseService_t *self, LicenseStatus_t *out)
{
    if (self == NULL || out == NULL)
        return ERR_NULL_POINTER;
    return compute_status(self, out);
}
