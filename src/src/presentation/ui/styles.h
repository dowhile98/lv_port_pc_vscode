#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: list_button
lv_style_t *get_style_list_button_MAIN_DEFAULT();
lv_style_t *get_style_list_button_MAIN_PRESSED();
lv_style_t *get_style_list_button_MAIN_FOCUSED();
void add_style_list_button(lv_obj_t *obj);
void remove_style_list_button(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/