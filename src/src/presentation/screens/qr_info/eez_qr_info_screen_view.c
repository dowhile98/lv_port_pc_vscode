/**
 * @file eez_qr_info_screen_view.c
 * @brief EEZ-backed IQrInfoScreenView_t for the QR Info screen.
 *
 * Implements the QR Info view using EEZ-generated objects:
 *   objects.qr_info_tittle  — lv_label for top title
 *   objects.qr_info_content — lv_qrcode widget for QR code
 *   objects.qr_info_label   — lv_label for bottom description (URL / SSID)
 *
 * @note #include "lvgl.h" is intentional and expected in this file.
 * @note All lv_* calls wrapped with lv_lock() / lv_unlock() (called from non-LVGL thread).
 */
#include "presentation/screens/qr_info/eez_qr_info_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"
#include <string.h>

static void set_title(void *self, const char *title)
{
    (void)self;
    if (title == NULL)
    {
        return;
    }
    lv_lock();
    lv_label_set_text(objects.qr_info_tittle, title);
    lv_unlock();
}

static void set_content(void *self, const char *qr_data)
{
    (void)self;
    if (qr_data == NULL || qr_data[0] == '\0')
    {
        return;
    }
    lv_lock();
    lv_result_t res = lv_qrcode_update(objects.qr_info_content, qr_data, (uint32_t)strlen(qr_data));
    if (res != LV_RESULT_OK)
    {
        lv_label_set_text(objects.qr_info_tittle, "QR Error");
    }
    lv_unlock();
}

static void set_label(void *self, const char *label)
{
    (void)self;
    if (label == NULL)
    {
        return;
    }
    lv_lock();
    lv_label_set_text(objects.qr_info_label, label);
    lv_unlock();
}

static IQrInfoScreenView_Vtable s_vtable = {set_title, set_content, set_label};

static IQrInfoScreenView_t s_iface = {&s_vtable, NULL};

IQrInfoScreenView_t *EezQrInfoScreenView_GetInterface(void)
{
    return &s_iface;
}
