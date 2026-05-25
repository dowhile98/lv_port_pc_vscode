#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_startup() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.startup = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // logo
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.logo = obj;
            lv_obj_set_pos(obj, 1, -229);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_logo);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // model
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.model = obj;
            lv_obj_set_pos(obj, 1, -125);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_text_font(obj, &ui_font_oswald30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "TCS Current Interrupter");
        }
        {
            // link
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.link = obj;
            lv_obj_set_pos(obj, 1, -88);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "www.tecna.co");
        }
    }
    
    tick_screen_startup();
}

void delete_screen_startup() {
    lv_obj_delete(objects.startup);
    objects.startup = 0;
    objects.logo = 0;
    objects.model = 0;
    objects.link = 0;
}

void tick_screen_startup() {
}

void create_screen_overview() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.overview = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // device
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.device = obj;
            lv_obj_set_pos(obj, 1, -266);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "GPS Programming Unit\nTCS CICX1");
        }
        {
            // companyname
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.companyname = obj;
            lv_obj_set_pos(obj, 1, -207);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "TECNA S.A.");
        }
        {
            // overview_info
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.overview_info = obj;
            lv_obj_set_pos(obj, 1, -71);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "...");
        }
        {
            // overview_line1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.overview_line1 = obj;
            lv_obj_set_pos(obj, 1, -302);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------------------------------");
        }
        {
            // overview_line2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.overview_line2 = obj;
            lv_obj_set_pos(obj, 1, -238);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------------------------------");
        }
        {
            // overview_line3
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.overview_line3 = obj;
            lv_obj_set_pos(obj, 1, -176);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------------------------------");
        }
        {
            lv_obj_t *obj = lv_spinner_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 1, -128);
            lv_obj_set_size(obj, 38, 37);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE|LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_PRESS_LOCK|LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM|LV_OBJ_FLAG_SCROLL_WITH_ARROW|LV_OBJ_FLAG_SNAPPABLE);
            lv_obj_set_style_arc_width(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xffdc00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 5, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_overview();
}

void delete_screen_overview() {
    lv_obj_delete(objects.overview);
    objects.overview = 0;
    objects.device = 0;
    objects.companyname = 0;
    objects.overview_info = 0;
    objects.overview_line1 = 0;
    objects.overview_line2 = 0;
    objects.overview_line3 = 0;
    objects.obj0 = 0;
}

void tick_screen_overview() {
}

void create_screen_home() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.home = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // home_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_time = obj;
            lv_obj_set_pos(obj, -111, -285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "00:00:00");
        }
        {
            // home_line1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_line1 = obj;
            lv_obj_set_pos(obj, 3, -269);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------------------------------");
        }
        {
            // home_antenna_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.home_antenna_icon = obj;
            lv_obj_set_pos(obj, 82, -283);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_external_antena_icon);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // home_gps_status
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.home_gps_status = obj;
            lv_obj_set_pos(obj, 108, -284);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_satellite_disconnect_1);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // home_battery_level
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.home_battery_level = obj;
            lv_obj_set_pos(obj, 143, -282);
            lv_obj_set_size(obj, 30, 15);
            lv_obj_set_style_bg_image_src(obj, &img_battery_incon, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_src(obj, &img_battery_level_icon, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        {
            // home_wifi_status
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.home_wifi_status = obj;
            lv_obj_set_pos(obj, 57, -285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_wifi_disconnected_icon);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // home_cycle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_cycle = obj;
            lv_obj_set_pos(obj, -132, -250);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Cycle");
        }
        {
            // home_line2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_line2 = obj;
            lv_obj_set_pos(obj, 4, -235);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------------------------------");
        }
        {
            // home_cycle_status
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_cycle_status = obj;
            lv_obj_set_pos(obj, 39, -250);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "enabled");
        }
        {
            // home_line3
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_line3 = obj;
            lv_obj_set_pos(obj, -6, -166);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|\n|");
        }
        {
            // home_contact
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_contact = obj;
            lv_obj_set_pos(obj, -99, -217);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Contact Type");
        }
        {
            // home_on_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_on_time = obj;
            lv_obj_set_pos(obj, -118, -189);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "ON Time");
        }
        {
            // home_off_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_off_time = obj;
            lv_obj_set_pos(obj, -116, -163);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "OFF Time");
        }
        {
            // home_stop_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_stop_time = obj;
            lv_obj_set_pos(obj, -112, -106);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Stop Time");
        }
        {
            // home_start_on
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_start_on = obj;
            lv_obj_set_pos(obj, -110, -80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Start ON ?");
        }
        {
            // home_start_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_start_time = obj;
            lv_obj_set_pos(obj, -110, -136);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Start Time");
        }
        {
            // home_contact_type
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_contact_type = obj;
            lv_obj_set_pos(obj, 76, -216);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Normally closed");
        }
        {
            // home_on_time_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_on_time_value = obj;
            lv_obj_set_pos(obj, 37, -190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "0.700 s");
        }
        {
            // home_off_time_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_off_time_value = obj;
            lv_obj_set_pos(obj, 38, -163);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "0.300 s");
        }
        {
            // home_start_time_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_start_time_value = obj;
            lv_obj_set_pos(obj, 46, -136);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "06:00:00");
        }
        {
            // home_stop_time_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_stop_time_value = obj;
            lv_obj_set_pos(obj, 44, -106);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "18:00:00");
        }
        {
            // home_start_on_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_start_on_value = obj;
            lv_obj_set_pos(obj, 19, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "yes");
        }
        {
            // home_relay_status
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.home_relay_status = obj;
            lv_obj_set_pos(obj, 113, -249);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "[on]");
        }
    }
    
    tick_screen_home();
}

void delete_screen_home() {
    lv_obj_delete(objects.home);
    objects.home = 0;
    objects.home_time = 0;
    objects.home_line1 = 0;
    objects.home_antenna_icon = 0;
    objects.home_gps_status = 0;
    objects.home_battery_level = 0;
    objects.home_wifi_status = 0;
    objects.home_cycle = 0;
    objects.home_line2 = 0;
    objects.home_cycle_status = 0;
    objects.home_line3 = 0;
    objects.home_contact = 0;
    objects.home_on_time = 0;
    objects.home_off_time = 0;
    objects.home_stop_time = 0;
    objects.home_start_on = 0;
    objects.home_start_time = 0;
    objects.home_contact_type = 0;
    objects.home_on_time_value = 0;
    objects.home_off_time_value = 0;
    objects.home_start_time_value = 0;
    objects.home_stop_time_value = 0;
    objects.home_start_on_value = 0;
    objects.home_relay_status = 0;
}

void tick_screen_home() {
}

void create_screen_segurity() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.segurity = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // security_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.security_title = obj;
            lv_obj_set_pos(obj, 1, -289);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "----------------ENTER KEY----------------");
        }
        {
            // segurity_pswd_input
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.segurity_pswd_input = obj;
            lv_obj_set_pos(obj, -5, -84);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "...");
        }
        {
            // segurity_lock_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.segurity_lock_icon = obj;
            lv_obj_set_pos(obj, 1, -211);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_lock_icon);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_segurity();
}

void delete_screen_segurity() {
    lv_obj_delete(objects.segurity);
    objects.segurity = 0;
    objects.security_title = 0;
    objects.segurity_pswd_input = 0;
    objects.segurity_lock_icon = 0;
}

void tick_screen_segurity() {
}

void create_screen_menu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.menu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // menu_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.menu_tittle = obj;
            lv_obj_set_pos(obj, 1, -294);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "MAIN MENU");
        }
        {
            // menu_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.menu_list = obj;
            lv_obj_set_pos(obj, 1, -170);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_menu();
}

void delete_screen_menu() {
    lv_obj_delete(objects.menu);
    objects.menu = 0;
    objects.menu_tittle = 0;
    objects.menu_list = 0;
}

void tick_screen_menu() {
}

void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // settings_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.settings_title = obj;
            lv_obj_set_pos(obj, 1, -290);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "SETTINGS");
        }
        {
            // settings_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.settings_list = obj;
            lv_obj_set_pos(obj, 1, -169);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_settings();
}

void delete_screen_settings() {
    lv_obj_delete(objects.settings);
    objects.settings = 0;
    objects.settings_title = 0;
    objects.settings_list = 0;
}

void tick_screen_settings() {
}

void create_screen_information() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.information = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // information_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.information_label = obj;
            lv_obj_set_pos(obj, 1, -198);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "-----------------------------------------------------\nAPLICATION\nMODE\n-----------------------------------------------------");
        }
    }
    
    tick_screen_information();
}

void delete_screen_information() {
    lv_obj_delete(objects.information);
    objects.information = 0;
    objects.information_label = 0;
}

void tick_screen_information() {
}

void create_screen_int_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_title = obj;
            lv_obj_set_pos(obj, 1, -284);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.int_configuration_list = obj;
            lv_obj_set_pos(obj, 1, -167);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_int_configuration();
}

void delete_screen_int_configuration() {
    lv_obj_delete(objects.int_configuration);
    objects.int_configuration = 0;
    objects.int_configuration_title = 0;
    objects.int_configuration_list = 0;
}

void tick_screen_int_configuration() {
}

void create_screen_gps_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.gps_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // gps_configuration_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.gps_configuration_title = obj;
            lv_obj_set_pos(obj, 1, -289);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "GPS CONFIGURATION");
        }
        {
            // gps_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.gps_configuration_list = obj;
            lv_obj_set_pos(obj, 1, -168);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_gps_configuration();
}

void delete_screen_gps_configuration() {
    lv_obj_delete(objects.gps_configuration);
    objects.gps_configuration = 0;
    objects.gps_configuration_title = 0;
    objects.gps_configuration_list = 0;
}

void tick_screen_gps_configuration() {
}

void create_screen_contact_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.contact_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // contact_configuration_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contact_configuration_title = obj;
            lv_obj_set_pos(obj, 1, -288);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "CONTACT CONFIGURATION");
        }
        {
            // contact_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.contact_configuration_list = obj;
            lv_obj_set_pos(obj, 2, -165);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_contact_configuration();
}

void delete_screen_contact_configuration() {
    lv_obj_delete(objects.contact_configuration);
    objects.contact_configuration = 0;
    objects.contact_configuration_title = 0;
    objects.contact_configuration_list = 0;
}

void tick_screen_contact_configuration() {
}

void create_screen_general_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.general_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // general_configuration_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.general_configuration_title = obj;
            lv_obj_set_pos(obj, 1, -290);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "GENERAL CONFIGURATION");
        }
        {
            // general_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.general_configuration_list = obj;
            lv_obj_set_pos(obj, 1, -170);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_general_configuration();
}

void delete_screen_general_configuration() {
    lv_obj_delete(objects.general_configuration);
    objects.general_configuration = 0;
    objects.general_configuration_title = 0;
    objects.general_configuration_list = 0;
}

void tick_screen_general_configuration() {
}

void create_screen_int_configuration_state() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_state = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_state_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_title = obj;
            lv_obj_set_pos(obj, 1, -286);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_state_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_label = obj;
            lv_obj_set_pos(obj, -100, -259);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Interrupt");
        }
        {
            // int_configuration_status
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.int_configuration_status = obj;
            lv_obj_set_pos(obj, 2, -179);
            lv_obj_set_size(obj, 56, 30);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_state_disable
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_disable = obj;
            lv_obj_set_pos(obj, -70, -174);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Disable");
        }
        {
            // int_configuration_state_enable
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_enable = obj;
            lv_obj_set_pos(obj, 74, -174);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Enable");
        }
        {
            // int_configurationback_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configurationback_icon = obj;
            lv_obj_set_pos(obj, -119, -92);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_enter_icon = obj;
            lv_obj_set_pos(obj, 141, -92);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_state_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_label_1 = obj;
            lv_obj_set_pos(obj, -75, -91);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // int_configuration_state_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_state_label_2 = obj;
            lv_obj_set_pos(obj, 94, -88);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "change");
        }
    }
    
    tick_screen_int_configuration_state();
}

void delete_screen_int_configuration_state() {
    lv_obj_delete(objects.int_configuration_state);
    objects.int_configuration_state = 0;
    objects.int_configuration_state_title = 0;
    objects.int_configuration_state_label = 0;
    objects.int_configuration_status = 0;
    objects.int_configuration_state_disable = 0;
    objects.int_configuration_state_enable = 0;
    objects.int_configurationback_icon = 0;
    objects.int_configuration_enter_icon = 0;
    objects.int_configuration_state_label_1 = 0;
    objects.int_configuration_state_label_2 = 0;
}

void tick_screen_int_configuration_state() {
}

void create_screen_accessdenied() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.accessdenied = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // accessdenied_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.accessdenied_label = obj;
            lv_obj_set_pos(obj, 1, -192);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "-----------------------------------------------------\nSECURITY WARNING\nCYCLE IN PROGRESS\n-----------------------------------------------------");
        }
    }
    
    tick_screen_accessdenied();
}

void delete_screen_accessdenied() {
    lv_obj_delete(objects.accessdenied);
    objects.accessdenied = 0;
    objects.accessdenied_label = 0;
}

void tick_screen_accessdenied() {
}

void create_screen_configuration_input() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.configuration_input = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // configuration_input_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.configuration_input_title = obj;
            lv_obj_set_pos(obj, 1, -276);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // configuration_input_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.configuration_input_label = obj;
            lv_obj_set_pos(obj, 0, -251);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Start time?");
        }
        {
            // configuration_input_value
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.configuration_input_value = obj;
            lv_obj_set_pos(obj, 1, -167);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "...");
        }
        {
            // configuration_input_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.configuration_input_up_icon = obj;
            lv_obj_set_pos(obj, 1, -214);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // configuration_input_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.configuration_input_down_icon = obj;
            lv_obj_set_pos(obj, 1, -123);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // configuration_input_back_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.configuration_input_back_icon = obj;
            lv_obj_set_pos(obj, -141, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // configuration_input_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.configuration_input_enter_icon = obj;
            lv_obj_set_pos(obj, 144, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // configuration_input_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.configuration_input_label_1 = obj;
            lv_obj_set_pos(obj, -98, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // configuration_input_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.configuration_input_label_2 = obj;
            lv_obj_set_pos(obj, 112, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "save");
        }
    }
    
    tick_screen_configuration_input();
}

void delete_screen_configuration_input() {
    lv_obj_delete(objects.configuration_input);
    objects.configuration_input = 0;
    objects.configuration_input_title = 0;
    objects.configuration_input_label = 0;
    objects.configuration_input_value = 0;
    objects.configuration_input_up_icon = 0;
    objects.configuration_input_down_icon = 0;
    objects.configuration_input_back_icon = 0;
    objects.configuration_input_enter_icon = 0;
    objects.configuration_input_label_1 = 0;
    objects.configuration_input_label_2 = 0;
}

void tick_screen_configuration_input() {
}

void create_screen_int_configuration_days() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_days = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_days_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_days_title = obj;
            lv_obj_set_pos(obj, 0, -290);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_days_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_days_label = obj;
            lv_obj_set_pos(obj, -76, -266);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Days interrupt?");
        }
        {
            // int_configuration_days_back_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_days_back_icon = obj;
            lv_obj_set_pos(obj, -120, -89);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_days_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_days_enter_icon = obj;
            lv_obj_set_pos(obj, 140, -89);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_days_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_days_label_1 = obj;
            lv_obj_set_pos(obj, -77, -89);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // int_configuration_days_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_days_label_2 = obj;
            lv_obj_set_pos(obj, 108, -89);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "save");
        }
        {
            // int_configuration_days_mon
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_mon = obj;
            lv_obj_set_pos(obj, -93, -240);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Monday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_tue
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_tue = obj;
            lv_obj_set_pos(obj, -92, -207);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Tuesday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_wed
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_wed = obj;
            lv_obj_set_pos(obj, -81, -173);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Wednesday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_thu
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_thu = obj;
            lv_obj_set_pos(obj, -88, -140);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Thursday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_fri
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_fri = obj;
            lv_obj_set_pos(obj, 58, -240);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Friday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_sat
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_sat = obj;
            lv_obj_set_pos(obj, 68, -207);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Saturday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
        {
            // int_configuration_days_sun
            lv_obj_t *obj = lv_checkbox_create(parent_obj);
            objects.int_configuration_days_sun = obj;
            lv_obj_set_pos(obj, 63, -173);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_checkbox_set_text_static(obj, "Sunday ");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_FOCUSED);
        }
    }
    
    tick_screen_int_configuration_days();
}

void delete_screen_int_configuration_days() {
    lv_obj_delete(objects.int_configuration_days);
    objects.int_configuration_days = 0;
    objects.int_configuration_days_title = 0;
    objects.int_configuration_days_label = 0;
    objects.int_configuration_days_back_icon = 0;
    objects.int_configuration_days_enter_icon = 0;
    objects.int_configuration_days_label_1 = 0;
    objects.int_configuration_days_label_2 = 0;
    objects.int_configuration_days_mon = 0;
    objects.int_configuration_days_tue = 0;
    objects.int_configuration_days_wed = 0;
    objects.int_configuration_days_thu = 0;
    objects.int_configuration_days_fri = 0;
    objects.int_configuration_days_sat = 0;
    objects.int_configuration_days_sun = 0;
}

void tick_screen_int_configuration_days() {
}

void create_screen_int_configuration_period() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_period = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_period_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_period_title = obj;
            lv_obj_set_pos(obj, 1, -290);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_period_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_period_label = obj;
            lv_obj_set_pos(obj, -73, -265);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Interrupt mode?");
        }
        {
            // int_configuration_days_back_icon_1
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_days_back_icon_1 = obj;
            lv_obj_set_pos(obj, -126, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_days_enter_icon_1
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_days_enter_icon_1 = obj;
            lv_obj_set_pos(obj, 134, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_period_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_period_label_1 = obj;
            lv_obj_set_pos(obj, -83, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // int_configuration_period_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_period_label_2 = obj;
            lv_obj_set_pos(obj, 93, -82);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "select ");
        }
        {
            // int_configuration_period_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_period_up_icon = obj;
            lv_obj_set_pos(obj, 0, -222);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_period_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_period_down_icon = obj;
            lv_obj_set_pos(obj, 0, -109);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_period_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.int_configuration_period_list = obj;
            lv_obj_set_pos(obj, 0, -166);
            lv_obj_set_size(obj, 293, 64);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_int_configuration_period();
}

void delete_screen_int_configuration_period() {
    lv_obj_delete(objects.int_configuration_period);
    objects.int_configuration_period = 0;
    objects.int_configuration_period_title = 0;
    objects.int_configuration_period_label = 0;
    objects.int_configuration_days_back_icon_1 = 0;
    objects.int_configuration_days_enter_icon_1 = 0;
    objects.int_configuration_period_label_1 = 0;
    objects.int_configuration_period_label_2 = 0;
    objects.int_configuration_period_up_icon = 0;
    objects.int_configuration_period_down_icon = 0;
    objects.int_configuration_period_list = 0;
}

void tick_screen_int_configuration_period() {
}

void create_screen_int_configuration_period_menu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_period_menu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // period_single_title_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.period_single_title_1 = obj;
            lv_obj_set_pos(obj, -6, -288);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_period_menu_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_period_menu_label = obj;
            lv_obj_set_pos(obj, -69, -263);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Select item for edit");
        }
        {
            // int_configuration_period_menu_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.int_configuration_period_menu_list = obj;
            lv_obj_set_pos(obj, 0, -159);
            lv_obj_set_size(obj, 293, 178);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_int_configuration_period_menu();
}

void delete_screen_int_configuration_period_menu() {
    lv_obj_delete(objects.int_configuration_period_menu);
    objects.int_configuration_period_menu = 0;
    objects.period_single_title_1 = 0;
    objects.int_configuration_period_menu_label = 0;
    objects.int_configuration_period_menu_list = 0;
}

void tick_screen_int_configuration_period_menu() {
}

void create_screen_int_configuration_start() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_start = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_start_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_title = obj;
            lv_obj_set_pos(obj, 1, -286);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_start_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_label = obj;
            lv_obj_set_pos(obj, -34, -260);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>How does the cycle begin?");
        }
        {
            // int_configuration_start_value
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.int_configuration_start_value = obj;
            lv_obj_set_pos(obj, -4, -177);
            lv_obj_set_size(obj, 56, 30);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_start_off
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_off = obj;
            lv_obj_set_pos(obj, -57, -177);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "OFF");
        }
        {
            // int_configuration_start_on
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_on = obj;
            lv_obj_set_pos(obj, 60, -177);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "ON");
        }
        {
            // int_configuration_start_back_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_start_back_icon = obj;
            lv_obj_set_pos(obj, -129, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_start_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_start_enter_icon = obj;
            lv_obj_set_pos(obj, 131, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_start_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_label_1 = obj;
            lv_obj_set_pos(obj, -85, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // int_configuration_start_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_start_label_2 = obj;
            lv_obj_set_pos(obj, 99, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "save");
        }
    }
    
    tick_screen_int_configuration_start();
}

void delete_screen_int_configuration_start() {
    lv_obj_delete(objects.int_configuration_start);
    objects.int_configuration_start = 0;
    objects.int_configuration_start_title = 0;
    objects.int_configuration_start_label = 0;
    objects.int_configuration_start_value = 0;
    objects.int_configuration_start_off = 0;
    objects.int_configuration_start_on = 0;
    objects.int_configuration_start_back_icon = 0;
    objects.int_configuration_start_enter_icon = 0;
    objects.int_configuration_start_label_1 = 0;
    objects.int_configuration_start_label_2 = 0;
}

void tick_screen_int_configuration_start() {
}

void create_screen_int_configuration_predefined() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.int_configuration_predefined = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // int_configuration_predefined_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_predefined_title = obj;
            lv_obj_set_pos(obj, -2, -290);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "INTERRUPTION CONFIGURATION");
        }
        {
            // int_configuration_predefined_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.int_configuration_predefined_list = obj;
            lv_obj_set_pos(obj, 10, -160);
            lv_obj_set_size(obj, 306, 182);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // int_configuration_predefined_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.int_configuration_predefined_label = obj;
            lv_obj_set_pos(obj, -101, -264);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Ton - Toff");
        }
    }
    
    tick_screen_int_configuration_predefined();
}

void delete_screen_int_configuration_predefined() {
    lv_obj_delete(objects.int_configuration_predefined);
    objects.int_configuration_predefined = 0;
    objects.int_configuration_predefined_title = 0;
    objects.int_configuration_predefined_list = 0;
    objects.int_configuration_predefined_label = 0;
}

void tick_screen_int_configuration_predefined() {
}

void create_screen_gps_configuration_antenna() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.gps_configuration_antenna = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // gps_configuration_antenna_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.gps_configuration_antenna_title = obj;
            lv_obj_set_pos(obj, 0, -293);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "GPS CONFIGURATION");
        }
        {
            // gps_configuration_antenna_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.gps_configuration_antenna_label = obj;
            lv_obj_set_pos(obj, -30, -268);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Select the antenna to use");
        }
        {
            // int_configuration_days_back_icon_2
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.int_configuration_days_back_icon_2 = obj;
            lv_obj_set_pos(obj, -129, -80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // gps_configuration_antenna_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.gps_configuration_antenna_enter_icon = obj;
            lv_obj_set_pos(obj, 131, -80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // gps_configuration_antenna_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.gps_configuration_antenna_label_1 = obj;
            lv_obj_set_pos(obj, -86, -80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // gps_configuration_antenna_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.gps_configuration_antenna_label_2 = obj;
            lv_obj_set_pos(obj, 90, -80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "select ");
        }
        {
            // gps_configuration_antenna_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.gps_configuration_antenna_up_icon = obj;
            lv_obj_set_pos(obj, 1, -220);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // gps_configuration_antenna_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.gps_configuration_antenna_down_icon = obj;
            lv_obj_set_pos(obj, 1, -107);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // gps_configuration_antenna_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.gps_configuration_antenna_list = obj;
            lv_obj_set_pos(obj, 0, -162);
            lv_obj_set_size(obj, 293, 64);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_gps_configuration_antenna();
}

void delete_screen_gps_configuration_antenna() {
    lv_obj_delete(objects.gps_configuration_antenna);
    objects.gps_configuration_antenna = 0;
    objects.gps_configuration_antenna_title = 0;
    objects.gps_configuration_antenna_label = 0;
    objects.int_configuration_days_back_icon_2 = 0;
    objects.gps_configuration_antenna_enter_icon = 0;
    objects.gps_configuration_antenna_label_1 = 0;
    objects.gps_configuration_antenna_label_2 = 0;
    objects.gps_configuration_antenna_up_icon = 0;
    objects.gps_configuration_antenna_down_icon = 0;
    objects.gps_configuration_antenna_list = 0;
}

void tick_screen_gps_configuration_antenna() {
}

void create_screen_contact_configuration_type() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.contact_configuration_type = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // contact_configuration_type_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contact_configuration_type_title = obj;
            lv_obj_set_pos(obj, 0, -287);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "CONTACT CONFIGURATION");
        }
        {
            // contact_configuration_type_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contact_configuration_type_label = obj;
            lv_obj_set_pos(obj, -46, -253);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>Select the type of contact");
        }
        {
            // contact_configuration_type_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.contact_configuration_type_icon = obj;
            lv_obj_set_pos(obj, -129, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // contact_configuration_type_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.contact_configuration_type_enter_icon = obj;
            lv_obj_set_pos(obj, 131, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // contact_configuration_type_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contact_configuration_type_label_1 = obj;
            lv_obj_set_pos(obj, -86, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // contact_configuration_type_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contact_configuration_type_label_2 = obj;
            lv_obj_set_pos(obj, 90, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "select ");
        }
        {
            // contact_configuration_type_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.contact_configuration_type_up_icon = obj;
            lv_obj_set_pos(obj, 1, -218);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // contact_configuration_type_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.contact_configuration_type_down_icon = obj;
            lv_obj_set_pos(obj, 1, -105);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // contact_configuration_type_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.contact_configuration_type_list = obj;
            lv_obj_set_pos(obj, 0, -160);
            lv_obj_set_size(obj, 293, 64);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_contact_configuration_type();
}

void delete_screen_contact_configuration_type() {
    lv_obj_delete(objects.contact_configuration_type);
    objects.contact_configuration_type = 0;
    objects.contact_configuration_type_title = 0;
    objects.contact_configuration_type_label = 0;
    objects.contact_configuration_type_icon = 0;
    objects.contact_configuration_type_enter_icon = 0;
    objects.contact_configuration_type_label_1 = 0;
    objects.contact_configuration_type_label_2 = 0;
    objects.contact_configuration_type_up_icon = 0;
    objects.contact_configuration_type_down_icon = 0;
    objects.contact_configuration_type_list = 0;
}

void tick_screen_contact_configuration_type() {
}

void create_screen_general_configuration_alarm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.general_configuration_alarm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // general_configuration_alarm_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.general_configuration_alarm_title = obj;
            lv_obj_set_pos(obj, 1, -287);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "GENERAL CONFIGURATION");
        }
        {
            // general_configuration_alarm_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.general_configuration_alarm_label = obj;
            lv_obj_set_pos(obj, -20, -261);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>High temperature alarm enabled?");
        }
        {
            // general_configuration_alarm_back_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.general_configuration_alarm_back_icon = obj;
            lv_obj_set_pos(obj, -129, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // general_configuration_alarm_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.general_configuration_alarm_enter_icon = obj;
            lv_obj_set_pos(obj, 131, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // general_configuration_alarm_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.general_configuration_alarm_label_1 = obj;
            lv_obj_set_pos(obj, -86, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // general_configuration_alarm_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.general_configuration_alarm_label_2 = obj;
            lv_obj_set_pos(obj, 90, -85);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "select ");
        }
        {
            // general_configuration_alarm_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.general_configuration_alarm_up_icon = obj;
            lv_obj_set_pos(obj, 1, -222);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // general_configuration_alarm_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.general_configuration_alarm_down_icon = obj;
            lv_obj_set_pos(obj, 1, -109);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // general_configuration_alarm_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.general_configuration_alarm_list = obj;
            lv_obj_set_pos(obj, -3, -166);
            lv_obj_set_size(obj, 293, 64);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_general_configuration_alarm();
}

void delete_screen_general_configuration_alarm() {
    lv_obj_delete(objects.general_configuration_alarm);
    objects.general_configuration_alarm = 0;
    objects.general_configuration_alarm_title = 0;
    objects.general_configuration_alarm_label = 0;
    objects.general_configuration_alarm_back_icon = 0;
    objects.general_configuration_alarm_enter_icon = 0;
    objects.general_configuration_alarm_label_1 = 0;
    objects.general_configuration_alarm_label_2 = 0;
    objects.general_configuration_alarm_up_icon = 0;
    objects.general_configuration_alarm_down_icon = 0;
    objects.general_configuration_alarm_list = 0;
}

void tick_screen_general_configuration_alarm() {
}

void create_screen_system_information() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.system_information = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // system_information_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.system_information_tittle = obj;
            lv_obj_set_pos(obj, 0, -291);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "System Information");
        }
        {
            // system_information_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.system_information_list = obj;
            lv_obj_set_pos(obj, 1, -173);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_system_information();
}

void delete_screen_system_information() {
    lv_obj_delete(objects.system_information);
    objects.system_information = 0;
    objects.system_information_tittle = 0;
    objects.system_information_list = 0;
}

void tick_screen_system_information() {
}

void create_screen_information_show() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.information_show = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // information_show_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.information_show_tittle = obj;
            lv_obj_set_pos(obj, 1, -287);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "Tittle");
        }
        {
            // information_show_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.information_show_label = obj;
            lv_obj_set_pos(obj, 1, -160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "->");
        }
    }
    
    tick_screen_information_show();
}

void delete_screen_information_show() {
    lv_obj_delete(objects.information_show);
    objects.information_show = 0;
    objects.information_show_tittle = 0;
    objects.information_show_label = 0;
}

void tick_screen_information_show() {
}

void create_screen_wifi_module() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.wifi_module = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // wifi_module_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.wifi_module_tittle = obj;
            lv_obj_set_pos(obj, 0, -288);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "WiFi Module");
        }
        {
            // wifi_module_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.wifi_module_list = obj;
            lv_obj_set_pos(obj, 1, -164);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_wifi_module();
}

void delete_screen_wifi_module() {
    lv_obj_delete(objects.wifi_module);
    objects.wifi_module = 0;
    objects.wifi_module_tittle = 0;
    objects.wifi_module_list = 0;
}

void tick_screen_wifi_module() {
}

void create_screen_qr_info() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.qr_info = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // qr_info_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.qr_info_tittle = obj;
            lv_obj_set_pos(obj, 1, -281);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "tittle");
        }
        {
            // qr_info_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.qr_info_label = obj;
            lv_obj_set_pos(obj, 1, -74);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "information");
        }
        {
            // qr_info_content
            lv_obj_t *obj = lv_qrcode_create(parent_obj);
            objects.qr_info_content = obj;
            lv_obj_set_pos(obj, 2, -177);
            lv_obj_set_size(obj, 160, 160);
            lv_qrcode_set_size(obj, 160);
            lv_qrcode_set_dark_color(obj, lv_color_hex(0xffdc00));
            lv_qrcode_set_light_color(obj, lv_color_hex(0x000000));
            lv_qrcode_update(obj, "http://192.168.1.77", 19);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_qr_info();
}

void delete_screen_qr_info() {
    lv_obj_delete(objects.qr_info);
    objects.qr_info = 0;
    objects.qr_info_tittle = 0;
    objects.qr_info_label = 0;
    objects.qr_info_content = 0;
}

void tick_screen_qr_info() {
}

void create_screen_wifi_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.wifi_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // wifi_configuration_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.wifi_configuration_tittle = obj;
            lv_obj_set_pos(obj, 0, -287);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "WiFi Configuration");
        }
        {
            // wifi_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.wifi_configuration_list = obj;
            lv_obj_set_pos(obj, 1, -165);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_wifi_configuration();
}

void delete_screen_wifi_configuration() {
    lv_obj_delete(objects.wifi_configuration);
    objects.wifi_configuration = 0;
    objects.wifi_configuration_tittle = 0;
    objects.wifi_configuration_list = 0;
}

void tick_screen_wifi_configuration() {
}

void create_screen_admin_configuration() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.admin_configuration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // admin_configuration_tittle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.admin_configuration_tittle = obj;
            lv_obj_set_pos(obj, 0, -287);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "Admin Configutation");
        }
        {
            // admin_configuration_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.admin_configuration_list = obj;
            lv_obj_set_pos(obj, 1, -164);
            lv_obj_set_size(obj, 306, 201);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_admin_configuration();
}

void delete_screen_admin_configuration() {
    lv_obj_delete(objects.admin_configuration);
    objects.admin_configuration = 0;
    objects.admin_configuration_tittle = 0;
    objects.admin_configuration_list = 0;
}

void tick_screen_admin_configuration() {
}

void create_screen_admin_operation_mode() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.admin_operation_mode = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 599, 1026);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_src(obj, &img_background_1, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // admin_operation_mode_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.admin_operation_mode_title = obj;
            lv_obj_set_pos(obj, 0, -289);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_label_set_text_static(obj, "ADMIN CONFIGURATION");
        }
        {
            // admin_operation_mode_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.admin_operation_mode_label = obj;
            lv_obj_set_pos(obj, 159, 239);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffdc00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, ">>mode");
        }
        {
            // admin_operation_mode_back_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.admin_operation_mode_back_icon = obj;
            lv_obj_set_pos(obj, -129, -84);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_back_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // admin_operation_mode_enter_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.admin_operation_mode_enter_icon = obj;
            lv_obj_set_pos(obj, 131, -84);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_enter_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // admin_operation_mode_label_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.admin_operation_mode_label_1 = obj;
            lv_obj_set_pos(obj, -86, -84);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "back");
        }
        {
            // admin_operation_mode_label_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.admin_operation_mode_label_2 = obj;
            lv_obj_set_pos(obj, 90, -84);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "select ");
        }
        {
            // admin_operation_mode_up_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.admin_operation_mode_up_icon = obj;
            lv_obj_set_pos(obj, 0, -226);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_up_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // admin_operation_mode_down_icon
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.admin_operation_mode_down_icon = obj;
            lv_obj_set_pos(obj, 0, -113);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_down_button);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // admin_operation_mode_list
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.admin_operation_mode_list = obj;
            lv_obj_set_pos(obj, 0, -172);
            lv_obj_set_size(obj, 293, 64);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_oswald20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_admin_operation_mode();
}

void delete_screen_admin_operation_mode() {
    lv_obj_delete(objects.admin_operation_mode);
    objects.admin_operation_mode = 0;
    objects.admin_operation_mode_title = 0;
    objects.admin_operation_mode_label = 0;
    objects.admin_operation_mode_back_icon = 0;
    objects.admin_operation_mode_enter_icon = 0;
    objects.admin_operation_mode_label_1 = 0;
    objects.admin_operation_mode_label_2 = 0;
    objects.admin_operation_mode_up_icon = 0;
    objects.admin_operation_mode_down_icon = 0;
    objects.admin_operation_mode_list = 0;
}

void tick_screen_admin_operation_mode() {
}

typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_startup,
    create_screen_overview,
    create_screen_home,
    create_screen_segurity,
    create_screen_menu,
    create_screen_settings,
    create_screen_information,
    create_screen_int_configuration,
    create_screen_gps_configuration,
    create_screen_contact_configuration,
    create_screen_general_configuration,
    create_screen_int_configuration_state,
    create_screen_accessdenied,
    create_screen_configuration_input,
    create_screen_int_configuration_days,
    create_screen_int_configuration_period,
    create_screen_int_configuration_period_menu,
    create_screen_int_configuration_start,
    create_screen_int_configuration_predefined,
    create_screen_gps_configuration_antenna,
    create_screen_contact_configuration_type,
    create_screen_general_configuration_alarm,
    create_screen_system_information,
    create_screen_information_show,
    create_screen_wifi_module,
    create_screen_qr_info,
    create_screen_wifi_configuration,
    create_screen_admin_configuration,
    create_screen_admin_operation_mode,
};
void create_screen(int screen_index) {
    create_screen_funcs[screen_index]();
}
void create_screen_by_id(enum ScreensEnum screenId) {
    create_screen_funcs[screenId - 1]();
}

typedef void (*delete_screen_func_t)();
delete_screen_func_t delete_screen_funcs[] = {
    delete_screen_startup,
    delete_screen_overview,
    delete_screen_home,
    delete_screen_segurity,
    delete_screen_menu,
    delete_screen_settings,
    delete_screen_information,
    delete_screen_int_configuration,
    delete_screen_gps_configuration,
    delete_screen_contact_configuration,
    delete_screen_general_configuration,
    delete_screen_int_configuration_state,
    delete_screen_accessdenied,
    delete_screen_configuration_input,
    delete_screen_int_configuration_days,
    delete_screen_int_configuration_period,
    delete_screen_int_configuration_period_menu,
    delete_screen_int_configuration_start,
    delete_screen_int_configuration_predefined,
    delete_screen_gps_configuration_antenna,
    delete_screen_contact_configuration_type,
    delete_screen_general_configuration_alarm,
    delete_screen_system_information,
    delete_screen_information_show,
    delete_screen_wifi_module,
    delete_screen_qr_info,
    delete_screen_wifi_configuration,
    delete_screen_admin_configuration,
    delete_screen_admin_operation_mode,
};
void delete_screen(int screen_index) {
    delete_screen_funcs[screen_index]();
}
void delete_screen_by_id(enum ScreensEnum screenId) {
    delete_screen_funcs[screenId - 1]();
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_startup,
    tick_screen_overview,
    tick_screen_home,
    tick_screen_segurity,
    tick_screen_menu,
    tick_screen_settings,
    tick_screen_information,
    tick_screen_int_configuration,
    tick_screen_gps_configuration,
    tick_screen_contact_configuration,
    tick_screen_general_configuration,
    tick_screen_int_configuration_state,
    tick_screen_accessdenied,
    tick_screen_configuration_input,
    tick_screen_int_configuration_days,
    tick_screen_int_configuration_period,
    tick_screen_int_configuration_period_menu,
    tick_screen_int_configuration_start,
    tick_screen_int_configuration_predefined,
    tick_screen_gps_configuration_antenna,
    tick_screen_contact_configuration_type,
    tick_screen_general_configuration_alarm,
    tick_screen_system_information,
    tick_screen_information_show,
    tick_screen_wifi_module,
    tick_screen_qr_info,
    tick_screen_wifi_configuration,
    tick_screen_admin_configuration,
    tick_screen_admin_operation_mode,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 29) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "oswald22", &ui_font_oswald22 },
    { "oswald24", &ui_font_oswald24 },
    { "oswald26", &ui_font_oswald26 },
    { "oswald14", &ui_font_oswald14 },
    { "oswald30", &ui_font_oswald30 },
    { "oswald18", &ui_font_oswald18 },
    { "oswald20", &ui_font_oswald20 },
    { "oswald40", &ui_font_oswald40 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_startup();
    create_screen_overview();
    create_screen_home();
    create_screen_segurity();
    create_screen_menu();
    create_screen_settings();
    create_screen_information();
    create_screen_int_configuration();
    create_screen_gps_configuration();
    create_screen_contact_configuration();
    create_screen_general_configuration();
    create_screen_int_configuration_state();
    create_screen_accessdenied();
    create_screen_configuration_input();
    create_screen_int_configuration_days();
    create_screen_int_configuration_period();
    create_screen_int_configuration_period_menu();
    create_screen_int_configuration_start();
    create_screen_int_configuration_predefined();
    create_screen_gps_configuration_antenna();
    create_screen_contact_configuration_type();
    create_screen_general_configuration_alarm();
    create_screen_system_information();
    create_screen_information_show();
    create_screen_wifi_module();
    create_screen_qr_info();
    create_screen_wifi_configuration();
    create_screen_admin_configuration();
    create_screen_admin_operation_mode();
}