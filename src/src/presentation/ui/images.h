#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_logo;
extern const lv_img_dsc_t img_internal_antena_icon;
extern const lv_img_dsc_t img_external_antena_icon;
extern const lv_img_dsc_t img_satellite_disconnect_1;
extern const lv_img_dsc_t img_satellite_disconnect_2;
extern const lv_img_dsc_t img_satellite_online;
extern const lv_img_dsc_t img_battery_incon;
extern const lv_img_dsc_t img_battery_level_icon;
extern const lv_img_dsc_t img_wifi_connected_icon;
extern const lv_img_dsc_t img_wifi_disconnected_icon;
extern const lv_img_dsc_t img_lock_icon;
extern const lv_img_dsc_t img_unlock_icon;
extern const lv_img_dsc_t img_up_button;
extern const lv_img_dsc_t img_down_button;
extern const lv_img_dsc_t img_back_button;
extern const lv_img_dsc_t img_enter_button;
extern const lv_img_dsc_t img_up_button_press;
extern const lv_img_dsc_t img_down_button_press;
extern const lv_img_dsc_t img_gps_antenna_failed;
extern const lv_img_dsc_t img_background_1;
extern const lv_img_dsc_t img_background_0;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[21];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/