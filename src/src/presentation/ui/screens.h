#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_STARTUP = 1,
    SCREEN_ID_OVERVIEW = 2,
    SCREEN_ID_HOME = 3,
    SCREEN_ID_SEGURITY = 4,
    SCREEN_ID_MENU = 5,
    SCREEN_ID_SETTINGS = 6,
    SCREEN_ID_INFORMATION = 7,
    SCREEN_ID_INT_CONFIGURATION = 8,
    SCREEN_ID_GPS_CONFIGURATION = 9,
    SCREEN_ID_CONTACT_CONFIGURATION = 10,
    SCREEN_ID_GENERAL_CONFIGURATION = 11,
    SCREEN_ID_INT_CONFIGURATION_STATE = 12,
    SCREEN_ID_ACCESSDENIED = 13,
    SCREEN_ID_CONFIGURATION_INPUT = 14,
    SCREEN_ID_INT_CONFIGURATION_DAYS = 15,
    SCREEN_ID_INT_CONFIGURATION_PERIOD = 16,
    SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU = 17,
    SCREEN_ID_INT_CONFIGURATION_START = 18,
    SCREEN_ID_INT_CONFIGURATION_PREDEFINED = 19,
    SCREEN_ID_GPS_CONFIGURATION_ANTENNA = 20,
    SCREEN_ID_CONTACT_CONFIGURATION_TYPE = 21,
    SCREEN_ID_GENERAL_CONFIGURATION_ALARM = 22,
    SCREEN_ID_SYSTEM_INFORMATION = 23,
    SCREEN_ID_INFORMATION_SHOW = 24,
    SCREEN_ID_WIFI_MODULE = 25,
    SCREEN_ID_QR_INFO = 26,
    SCREEN_ID_WIFI_CONFIGURATION = 27,
    SCREEN_ID_ADMIN_CONFIGURATION = 28,
    SCREEN_ID_ADMIN_OPERATION_MODE = 29,
    _SCREEN_ID_LAST = 29
};

typedef struct _objects_t {
    lv_obj_t *startup;
    lv_obj_t *overview;
    lv_obj_t *home;
    lv_obj_t *segurity;
    lv_obj_t *menu;
    lv_obj_t *settings;
    lv_obj_t *information;
    lv_obj_t *int_configuration;
    lv_obj_t *gps_configuration;
    lv_obj_t *contact_configuration;
    lv_obj_t *general_configuration;
    lv_obj_t *int_configuration_state;
    lv_obj_t *accessdenied;
    lv_obj_t *configuration_input;
    lv_obj_t *int_configuration_days;
    lv_obj_t *int_configuration_period;
    lv_obj_t *int_configuration_period_menu;
    lv_obj_t *int_configuration_start;
    lv_obj_t *int_configuration_predefined;
    lv_obj_t *gps_configuration_antenna;
    lv_obj_t *contact_configuration_type;
    lv_obj_t *general_configuration_alarm;
    lv_obj_t *system_information;
    lv_obj_t *information_show;
    lv_obj_t *wifi_module;
    lv_obj_t *qr_info;
    lv_obj_t *wifi_configuration;
    lv_obj_t *admin_configuration;
    lv_obj_t *admin_operation_mode;
    lv_obj_t *logo;
    lv_obj_t *model;
    lv_obj_t *link;
    lv_obj_t *device;
    lv_obj_t *companyname;
    lv_obj_t *overview_info;
    lv_obj_t *overview_line1;
    lv_obj_t *overview_line2;
    lv_obj_t *overview_line3;
    lv_obj_t *obj0;
    lv_obj_t *home_time;
    lv_obj_t *home_line1;
    lv_obj_t *home_antenna_icon;
    lv_obj_t *home_gps_status;
    lv_obj_t *home_battery_level;
    lv_obj_t *home_wifi_status;
    lv_obj_t *home_cycle;
    lv_obj_t *home_line2;
    lv_obj_t *home_cycle_status;
    lv_obj_t *home_line3;
    lv_obj_t *home_contact;
    lv_obj_t *home_on_time;
    lv_obj_t *home_off_time;
    lv_obj_t *home_stop_time;
    lv_obj_t *home_start_on;
    lv_obj_t *home_start_time;
    lv_obj_t *home_contact_type;
    lv_obj_t *home_on_time_value;
    lv_obj_t *home_off_time_value;
    lv_obj_t *home_start_time_value;
    lv_obj_t *home_stop_time_value;
    lv_obj_t *home_start_on_value;
    lv_obj_t *home_relay_status;
    lv_obj_t *security_title;
    lv_obj_t *segurity_pswd_input;
    lv_obj_t *segurity_lock_icon;
    lv_obj_t *menu_tittle;
    lv_obj_t *menu_list;
    lv_obj_t *settings_title;
    lv_obj_t *settings_list;
    lv_obj_t *information_label;
    lv_obj_t *int_configuration_title;
    lv_obj_t *int_configuration_list;
    lv_obj_t *gps_configuration_title;
    lv_obj_t *gps_configuration_list;
    lv_obj_t *contact_configuration_title;
    lv_obj_t *contact_configuration_list;
    lv_obj_t *general_configuration_title;
    lv_obj_t *general_configuration_list;
    lv_obj_t *int_configuration_state_title;
    lv_obj_t *int_configuration_state_label;
    lv_obj_t *int_configuration_status;
    lv_obj_t *int_configuration_state_disable;
    lv_obj_t *int_configuration_state_enable;
    lv_obj_t *int_configurationback_icon;
    lv_obj_t *int_configuration_enter_icon;
    lv_obj_t *int_configuration_state_label_1;
    lv_obj_t *int_configuration_state_label_2;
    lv_obj_t *accessdenied_label;
    lv_obj_t *configuration_input_title;
    lv_obj_t *configuration_input_label;
    lv_obj_t *configuration_input_value;
    lv_obj_t *configuration_input_up_icon;
    lv_obj_t *configuration_input_down_icon;
    lv_obj_t *configuration_input_back_icon;
    lv_obj_t *configuration_input_enter_icon;
    lv_obj_t *configuration_input_label_1;
    lv_obj_t *configuration_input_label_2;
    lv_obj_t *int_configuration_days_title;
    lv_obj_t *int_configuration_days_label;
    lv_obj_t *int_configuration_days_back_icon;
    lv_obj_t *int_configuration_days_enter_icon;
    lv_obj_t *int_configuration_days_label_1;
    lv_obj_t *int_configuration_days_label_2;
    lv_obj_t *int_configuration_days_mon;
    lv_obj_t *int_configuration_days_tue;
    lv_obj_t *int_configuration_days_wed;
    lv_obj_t *int_configuration_days_thu;
    lv_obj_t *int_configuration_days_fri;
    lv_obj_t *int_configuration_days_sat;
    lv_obj_t *int_configuration_days_sun;
    lv_obj_t *int_configuration_period_title;
    lv_obj_t *int_configuration_period_label;
    lv_obj_t *int_configuration_days_back_icon_1;
    lv_obj_t *int_configuration_days_enter_icon_1;
    lv_obj_t *int_configuration_period_label_1;
    lv_obj_t *int_configuration_period_label_2;
    lv_obj_t *int_configuration_period_up_icon;
    lv_obj_t *int_configuration_period_down_icon;
    lv_obj_t *int_configuration_period_list;
    lv_obj_t *period_single_title_1;
    lv_obj_t *int_configuration_period_menu_label;
    lv_obj_t *int_configuration_period_menu_list;
    lv_obj_t *int_configuration_start_title;
    lv_obj_t *int_configuration_start_label;
    lv_obj_t *int_configuration_start_value;
    lv_obj_t *int_configuration_start_off;
    lv_obj_t *int_configuration_start_on;
    lv_obj_t *int_configuration_start_back_icon;
    lv_obj_t *int_configuration_start_enter_icon;
    lv_obj_t *int_configuration_start_label_1;
    lv_obj_t *int_configuration_start_label_2;
    lv_obj_t *int_configuration_predefined_title;
    lv_obj_t *int_configuration_predefined_list;
    lv_obj_t *int_configuration_predefined_label;
    lv_obj_t *gps_configuration_antenna_title;
    lv_obj_t *gps_configuration_antenna_label;
    lv_obj_t *int_configuration_days_back_icon_2;
    lv_obj_t *gps_configuration_antenna_enter_icon;
    lv_obj_t *gps_configuration_antenna_label_1;
    lv_obj_t *gps_configuration_antenna_label_2;
    lv_obj_t *gps_configuration_antenna_up_icon;
    lv_obj_t *gps_configuration_antenna_down_icon;
    lv_obj_t *gps_configuration_antenna_list;
    lv_obj_t *contact_configuration_type_title;
    lv_obj_t *contact_configuration_type_label;
    lv_obj_t *contact_configuration_type_icon;
    lv_obj_t *contact_configuration_type_enter_icon;
    lv_obj_t *contact_configuration_type_label_1;
    lv_obj_t *contact_configuration_type_label_2;
    lv_obj_t *contact_configuration_type_up_icon;
    lv_obj_t *contact_configuration_type_down_icon;
    lv_obj_t *contact_configuration_type_list;
    lv_obj_t *general_configuration_alarm_title;
    lv_obj_t *general_configuration_alarm_label;
    lv_obj_t *general_configuration_alarm_back_icon;
    lv_obj_t *general_configuration_alarm_enter_icon;
    lv_obj_t *general_configuration_alarm_label_1;
    lv_obj_t *general_configuration_alarm_label_2;
    lv_obj_t *general_configuration_alarm_up_icon;
    lv_obj_t *general_configuration_alarm_down_icon;
    lv_obj_t *general_configuration_alarm_list;
    lv_obj_t *system_information_tittle;
    lv_obj_t *system_information_list;
    lv_obj_t *information_show_tittle;
    lv_obj_t *information_show_label;
    lv_obj_t *wifi_module_tittle;
    lv_obj_t *wifi_module_list;
    lv_obj_t *qr_info_tittle;
    lv_obj_t *qr_info_label;
    lv_obj_t *qr_info_content;
    lv_obj_t *wifi_configuration_tittle;
    lv_obj_t *wifi_configuration_list;
    lv_obj_t *admin_configuration_tittle;
    lv_obj_t *admin_configuration_list;
    lv_obj_t *admin_operation_mode_title;
    lv_obj_t *admin_operation_mode_label;
    lv_obj_t *admin_operation_mode_back_icon;
    lv_obj_t *admin_operation_mode_enter_icon;
    lv_obj_t *admin_operation_mode_label_1;
    lv_obj_t *admin_operation_mode_label_2;
    lv_obj_t *admin_operation_mode_up_icon;
    lv_obj_t *admin_operation_mode_down_icon;
    lv_obj_t *admin_operation_mode_list;
} objects_t;

extern objects_t objects;

void create_screen_startup();
void delete_screen_startup();
void tick_screen_startup();

void create_screen_overview();
void delete_screen_overview();
void tick_screen_overview();

void create_screen_home();
void delete_screen_home();
void tick_screen_home();

void create_screen_segurity();
void delete_screen_segurity();
void tick_screen_segurity();

void create_screen_menu();
void delete_screen_menu();
void tick_screen_menu();

void create_screen_settings();
void delete_screen_settings();
void tick_screen_settings();

void create_screen_information();
void delete_screen_information();
void tick_screen_information();

void create_screen_int_configuration();
void delete_screen_int_configuration();
void tick_screen_int_configuration();

void create_screen_gps_configuration();
void delete_screen_gps_configuration();
void tick_screen_gps_configuration();

void create_screen_contact_configuration();
void delete_screen_contact_configuration();
void tick_screen_contact_configuration();

void create_screen_general_configuration();
void delete_screen_general_configuration();
void tick_screen_general_configuration();

void create_screen_int_configuration_state();
void delete_screen_int_configuration_state();
void tick_screen_int_configuration_state();

void create_screen_accessdenied();
void delete_screen_accessdenied();
void tick_screen_accessdenied();

void create_screen_configuration_input();
void delete_screen_configuration_input();
void tick_screen_configuration_input();

void create_screen_int_configuration_days();
void delete_screen_int_configuration_days();
void tick_screen_int_configuration_days();

void create_screen_int_configuration_period();
void delete_screen_int_configuration_period();
void tick_screen_int_configuration_period();

void create_screen_int_configuration_period_menu();
void delete_screen_int_configuration_period_menu();
void tick_screen_int_configuration_period_menu();

void create_screen_int_configuration_start();
void delete_screen_int_configuration_start();
void tick_screen_int_configuration_start();

void create_screen_int_configuration_predefined();
void delete_screen_int_configuration_predefined();
void tick_screen_int_configuration_predefined();

void create_screen_gps_configuration_antenna();
void delete_screen_gps_configuration_antenna();
void tick_screen_gps_configuration_antenna();

void create_screen_contact_configuration_type();
void delete_screen_contact_configuration_type();
void tick_screen_contact_configuration_type();

void create_screen_general_configuration_alarm();
void delete_screen_general_configuration_alarm();
void tick_screen_general_configuration_alarm();

void create_screen_system_information();
void delete_screen_system_information();
void tick_screen_system_information();

void create_screen_information_show();
void delete_screen_information_show();
void tick_screen_information_show();

void create_screen_wifi_module();
void delete_screen_wifi_module();
void tick_screen_wifi_module();

void create_screen_qr_info();
void delete_screen_qr_info();
void tick_screen_qr_info();

void create_screen_wifi_configuration();
void delete_screen_wifi_configuration();
void tick_screen_wifi_configuration();

void create_screen_admin_configuration();
void delete_screen_admin_configuration();
void tick_screen_admin_configuration();

void create_screen_admin_operation_mode();
void delete_screen_admin_operation_mode();
void tick_screen_admin_operation_mode();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/