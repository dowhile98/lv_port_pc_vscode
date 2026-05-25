/**
 * @file qr_info_screen_presenter.c
 * @brief Presenter for the QR Info screen (SCREEN_ID_QR_INFO = 26).
 *
 * Provider table (indexed by *pending_item_ptr):
 *   idx 0 — Connect via AP (QR)  — WIFI:S:<ssid>;T:WPA;P:<password>;;
 *   idx 1 — Web Server (AP) QR   — http://<ap_ip>
 *   idx 2 — Web Server (STA) QR  — http://<sta_ip_live>
 *
 * Each provider fills both the QR content buffer (s_buf) and the description
 * label buffer (s_label_buf) shown below the QR code.
 *
 * @note NO #include "lvgl.h" — intentional (PC-testable).
 *
 * @author Tecna Smart Lab
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/qr_info/qr_info_screen_presenter.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_device_identity.h"
#include "interfaces/i_network_port.h"
#include "common/wifi_types.h"
#include "lwprintf/lwprintf.h"
#include <string.h>

static char s_buf[QR_INFO_DATA_BUF_SIZE];
static char s_label_buf[QR_INFO_DATA_BUF_SIZE];

/* ════════════════════════════════════════════════════════════════════════════
 * Provider helpers
 * ════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Copy the stored AP SSID into out. EEPROM is the single source of
 *        truth after first-boot initialisation in the DI container.
 */
static void get_ap_ssid(const QrInfoScreenPresenter_t *self,
                        const WifiConfig_t *cfg,
                        char *out,
                        uint32_t out_size)
{
    (void)self; /* device_identity no longer needed for SSID display */
    (void)lwprintf_snprintf(out, (size_t)out_size, "%.32s", cfg->ap_ssid);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 0 — Connect via AP (QR): WiFi credentials QR
 *   QR format: WIFI:S:<ssid>;T:WPA;P:<password>;;
 *   Label:     SSID: <ssid>
 * ════════════════════════════════════════════════════════════════════════════ */

static const char *build_wifi_qr(const QrInfoScreenPresenter_t *self,
                                 char *buf, uint32_t size,
                                 char *label_buf, uint32_t label_size)
{
    buf[0] = '\0';
    label_buf[0] = '\0';

    if (self->config_storage == NULL)
    {
        (void)lwprintf_snprintf(buf, (size_t)size, "WIFI:S:TCS-UNKNOWN;T:WPA;P:;;");
        (void)lwprintf_snprintf(label_buf, (size_t)label_size, "SSID: TCS-UNKNOWN");
        return "Connect via AP (QR)";
    }

    WifiConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &cfg) != ERR_OK)
    {
        (void)lwprintf_snprintf(buf, (size_t)size, "WIFI:S:TCS-ERROR;T:WPA;P:;;");
        (void)lwprintf_snprintf(label_buf, (size_t)label_size, "SSID: --");
        return "Connect via AP (QR)";
    }

    char ssid[40];
    get_ap_ssid(self, &cfg, ssid, (uint32_t)sizeof(ssid));

    (void)lwprintf_snprintf(buf, (size_t)size, "WIFI:S:%.32s;T:WPA;P:%.63s;;",
                            ssid, cfg.ap_pwd);
    (void)lwprintf_snprintf(label_buf, (size_t)label_size, "SSID: %.32s", ssid);

    return "Connect via AP (QR)";
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 1 — Web Server (AP) QR: URL QR code
 *   QR content: http://<ap_ip>
 *   Label:      http://<ap_ip>
 * ════════════════════════════════════════════════════════════════════════════ */

static const char *build_webserver_ap_qr(const QrInfoScreenPresenter_t *self,
                                         char *buf, uint32_t size,
                                         char *label_buf, uint32_t label_size)
{
    buf[0] = '\0';
    label_buf[0] = '\0';

    if (self->config_storage == NULL)
    {
        (void)lwprintf_snprintf(buf, (size_t)size, "http://192.168.8.1");
        (void)lwprintf_snprintf(label_buf, (size_t)label_size, "http://192.168.8.1");
        return "Web Server (AP) QR";
    }

    WifiConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (ConfigStorage_LoadWifiConfig(self->config_storage, &cfg) != ERR_OK)
    {
        (void)lwprintf_snprintf(buf, (size_t)size, "http://192.168.8.1");
        (void)lwprintf_snprintf(label_buf, (size_t)label_size, "http://192.168.8.1");
        return "Web Server (AP) QR";
    }

    (void)lwprintf_snprintf(buf, (size_t)size, "http://%.16s", cfg.ap_ip);
    (void)lwprintf_snprintf(label_buf, (size_t)label_size, "http://%.16s", cfg.ap_ip);

    return "Web Server (AP) QR";
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider 2 — Web Server (STA) QR: live STA IP URL
 *   QR content: http://<sta_ip_live>
 *   Label:      http://<sta_ip_live>
 *   Falls back to "http://--" when port unavailable or IP not assigned.
 * ════════════════════════════════════════════════════════════════════════════ */

static const char *build_webserver_sta_qr(const QrInfoScreenPresenter_t *self,
                                          char *buf, uint32_t size,
                                          char *label_buf, uint32_t label_size)
{
    buf[0] = '\0';
    label_buf[0] = '\0';

    if (self->sta_port != NULL)
    {
        NetworkPortStatus_t status;
        memset(&status, 0, sizeof(status));
        if (NetworkPort_GetStatus(self->sta_port, &status) == ERR_OK &&
            status.has_ip_address)
        {
            const uint8_t *ip = status.ip_addr.octet;
            (void)lwprintf_snprintf(buf, (size_t)size,
                                    "http://%d.%d.%d.%d",
                                    (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
            (void)lwprintf_snprintf(label_buf, (size_t)label_size,
                                    "http://%d.%d.%d.%d",
                                    (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
            return "Web Server (STA) QR";
        }
    }

    /* Fallback — no IP assigned or sta_port not available */
    (void)lwprintf_snprintf(buf, (size_t)size, "http://--");
    (void)lwprintf_snprintf(label_buf, (size_t)label_size, "http://--");
    return "Web Server (STA) QR";
}

/* ════════════════════════════════════════════════════════════════════════════
 * Provider dispatch table
 * ════════════════════════════════════════════════════════════════════════════ */

typedef struct
{
    const char *(*build_fn)(const QrInfoScreenPresenter_t *self,
                            char *buf, uint32_t size,
                            char *label_buf, uint32_t label_size);
} QrProvider_t;

static const QrProvider_t k_providers[] = {
    {build_wifi_qr},          /* idx 0 — Connect via AP (QR)   */
    {build_webserver_ap_qr},  /* idx 1 — Web Server (AP) QR    */
    {build_webserver_sta_qr}, /* idx 2 — Web Server (STA) QR   */
};

#define QR_PROVIDER_COUNT \
    ((uint8_t)(sizeof(k_providers) / sizeof(k_providers[0])))

/* ════════════════════════════════════════════════════════════════════════════
 * Shared refresh logic
 * ════════════════════════════════════════════════════════════════════════════ */

static void refresh_view(QrInfoScreenPresenter_t *self)
{
    uint8_t idx = *self->pending_item_ptr;

    const char *title;

    if (idx < QR_PROVIDER_COUNT)
    {
        title = k_providers[idx].build_fn(self,
                                          s_buf, (uint32_t)sizeof(s_buf),
                                          s_label_buf, (uint32_t)sizeof(s_label_buf));
    }
    else
    {
        title = "---";
        s_buf[0] = '\0';
        s_label_buf[0] = '\0';
    }

    IQrInfoScreenView_SetTitle(self->view, title);
    IQrInfoScreenView_SetContent(self->view, s_buf);
    IQrInfoScreenView_SetLabel(self->view, s_label_buf);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Lifecycle vtable
 * ════════════════════════════════════════════════════════════════════════════ */

static void on_enter(IScreen_t *base)
{
    QrInfoScreenPresenter_t *self = (QrInfoScreenPresenter_t *)base;
    self->active = true;
    refresh_view(self);
}

static void _on_exit(IScreen_t *base)
{
    QrInfoScreenPresenter_t *self = (QrInfoScreenPresenter_t *)base;
    self->active = false;
}

static void on_update(IScreen_t *base)
{
    QrInfoScreenPresenter_t *self = (QrInfoScreenPresenter_t *)base;
    if (!self->active)
    {
        return;
    }
    refresh_view(self);
}

static void on_back_pressed(IScreen_t *base)
{
    QrInfoScreenPresenter_t *self = (QrInfoScreenPresenter_t *)base;
    if (self->router != NULL)
    {
        uint8_t dest = (self->back_screen_id_ptr != NULL)
                           ? *self->back_screen_id_ptr
                           : self->back_screen_id;
        (void)IScreenRouter_NavigateTo(self->router, dest);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * Public API
 * ════════════════════════════════════════════════════════════════════════════ */

Result_t QrInfoScreenPresenter_Init(QrInfoScreenPresenter_t *self,
                                    const QrInfoScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->pending_item_ptr == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));

    self->base.OnEnter = on_enter;
    self->base.OnExit = _on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL;

    self->view = deps->view;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;
    self->pending_item_ptr = deps->pending_item_ptr;
    self->back_screen_id_ptr = deps->back_screen_id_ptr;
    self->config_storage = deps->config_storage;
    self->device_identity = deps->device_identity;
    self->sta_port = deps->sta_port;

    return ERR_OK;
}
