/**
 * @file eez_home_screen_view.c
 * @brief EEZ-backed IHomeScreenView_t — wraps all LVGL widgets on the Home screen.
 *
 * ## Thread safety
 * lv_lock()/lv_unlock() guards every lv_* call — safe from any thread.
 *
 * ## Widget types (from screens.c)
 *   home_time, home_line1, home_cycle_status, home_contact_type,
 *   home_on/off/start/stop_time_value, home_start_on_value → lv_label
 *   home_wifi_status, home_antenna_icon, home_gps_status     → lv_image
 *   home_battery_level                                        → lv_bar
 *
 * ## Allowed includes
 * This is the ONLY file allowed to include lvgl.h in the presenter layer.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#include "presentation/screens/home/eez_home_screen_view.h"
#include "ui/screens.h"
#include "ui/images.h" /* img_wifi_connected_icon, img_satellite_online, ... */
#include "lvgl.h"

/* External image descriptors (declared in images.h / ui_image_*.c) */
extern const lv_image_dsc_t img_wifi_connected_icon;
extern const lv_image_dsc_t img_wifi_disconnected_icon;
extern const lv_image_dsc_t img_external_antena_icon;
extern const lv_image_dsc_t img_internal_antena_icon;
extern const lv_image_dsc_t img_satellite_disconnect_1;
extern const lv_image_dsc_t img_satellite_disconnect_2;
extern const lv_image_dsc_t img_satellite_online;

/*============================================================================*
 * PRIVATE — label setters
 *============================================================================*/

static void set_time(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_time, s);
    lv_unlock();
}

static void set_line1(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_line1, s);
    lv_unlock();
}

static void set_cycle_status(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_cycle_status, s);
    lv_unlock();
}

static void set_contact_type(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_contact_type, s);
    lv_unlock();
}

static void set_on_time(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_on_time_value, s);
    lv_unlock();
}

static void set_off_time(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_off_time_value, s);
    lv_unlock();
}

static void set_start_time(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_start_time_value, s);
    lv_unlock();
}

static void set_stop_time(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_stop_time_value, s);
    lv_unlock();
}

static void set_start_on(IHomeScreenView_t *self, const char *s)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_start_on_value, s);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — image setters
 *============================================================================*/

static void set_wifi_status(IHomeScreenView_t *self, bool connected)
{
    (void)self;
    lv_lock();
    lv_image_set_src(objects.home_wifi_status,
                     connected ? (const void *)&img_wifi_connected_icon
                               : (const void *)&img_wifi_disconnected_icon);
    lv_unlock();
}

static void set_antenna_icon(IHomeScreenView_t *self, HomeAntennaIconId_t icon)
{
    (void)self;
    const void *src;
    switch (icon)
    {
    case HOME_ANTENNA_ICON_EXTERNAL:
        src = &img_external_antena_icon;
        break;
    case HOME_ANTENNA_ICON_FAILED:
        /* Use satellite disconnect icon to indicate both antennas failed
         * TODO: Replace with dedicated img_antenna_failed icon when available */
        src = &img_satellite_disconnect_1;
        break;
    case HOME_ANTENNA_ICON_INTERNAL:
    default:
        src = &img_internal_antena_icon;
        break;
    }
    lv_lock();
    lv_image_set_src(objects.home_antenna_icon, src);
    lv_unlock();
}

static void set_gps_icon(IHomeScreenView_t *self, HomeGpsIconId_t icon)
{
    (void)self;
    const void *src;
    switch (icon)
    {
    case HOME_GPS_ICON_OFFLINE_2:
        src = &img_satellite_disconnect_2;
        break;
    case HOME_GPS_ICON_ONLINE:
        src = &img_satellite_online;
        break;
    default:
        src = &img_satellite_disconnect_1;
        break;
    }
    lv_lock();
    lv_image_set_src(objects.home_gps_status, src);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — relay state setter (ACTION-030)
 *============================================================================*/

/**
 * @brief Update the relay status label.
 *
 * @param[in] self       Unused (singleton).
 * @param[in] is_closed  true = contact closed (relay energised), false = open.
 */
static void set_relay_state(IHomeScreenView_t *self, bool is_closed)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.home_relay_status, is_closed ? "[on]" : "[off]");
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — bar setter
 *============================================================================*/

static void set_battery_level(IHomeScreenView_t *self, int32_t pct)
{
    (void)self;
    /* -1 = unavailable: set bar to 0 */
    lv_lock();
    lv_bar_set_value(objects.home_battery_level,
                     (pct < 0) ? 0 : (pct > 100 ? 100 : pct),
                     LV_ANIM_OFF);
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IHomeScreenView_t s_vtable = {
    set_time,
    set_line1,
    set_wifi_status,
    set_antenna_icon,
    set_gps_icon,
    set_battery_level,
    set_cycle_status,
    set_contact_type,
    set_on_time,
    set_off_time,
    set_start_time,
    set_stop_time,
    set_start_on,
    set_relay_state,
};

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

/**
 * @brief Return the singleton EEZ-backed IHomeScreenView_t.
 * @return Pointer to static vtable instance (never NULL).
 */
IHomeScreenView_t *EezHomeScreenView_GetInterface(void)
{
    return &s_vtable;
}
