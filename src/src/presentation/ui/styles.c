#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: list_button
//

void init_style_list_button_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0x000000));
    lv_style_set_text_color(style, lv_color_hex(0xffdc00));
    lv_style_set_text_font(style, &ui_font_oswald20);
    lv_style_set_pad_top(style, 2);
    lv_style_set_pad_bottom(style, 2);
    lv_style_set_pad_left(style, 4);
    lv_style_set_pad_right(style, 4);
};

lv_style_t *get_style_list_button_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_list_button_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_list_button_MAIN_PRESSED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xccb000));
    lv_style_set_radius(style, 0);
    lv_style_set_pad_top(style, 2);
    lv_style_set_pad_bottom(style, 2);
    lv_style_set_pad_left(style, 4);
    lv_style_set_pad_right(style, 4);
};

lv_style_t *get_style_list_button_MAIN_PRESSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_list_button_MAIN_PRESSED(style);
    }
    return style;
};

void init_style_list_button_MAIN_FOCUSED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xffdc00));
};

lv_style_t *get_style_list_button_MAIN_FOCUSED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_list_button_MAIN_FOCUSED(style);
    }
    return style;
};

void add_style_list_button(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_list_button_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_list_button_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(obj, get_style_list_button_MAIN_FOCUSED(), LV_PART_MAIN | LV_STATE_FOCUSED);
};

void remove_style_list_button(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_list_button_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_list_button_MAIN_PRESSED(), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_remove_style(obj, get_style_list_button_MAIN_FOCUSED(), LV_PART_MAIN | LV_STATE_FOCUSED);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_list_button,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_list_button,
    };
    remove_style_funcs[styleIndex](obj);
}