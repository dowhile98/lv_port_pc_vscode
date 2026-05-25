/**
 * @file ui_actions.c
 * @brief Implements EEZ Studio action callbacks, LVGL animations, and
 *        one-shot navigation timers for the boot screen sequence.
 *
 * ## Threading
 * All action_*() functions execute in the LVGL thread (ui_ao), inside
 * lv_task_handler(). They MUST NOT acquire lv_lock() — it is already held.
 *
 * ## Screen Sequence
 *   STARTUP ──[4 000 ms]──► OVERVIEW ──[3 000 ms]──► HOME
 *
 * ## EEZ Event Wiring
 * Already wired by EEZ screens.c (empty stubs satisfy linker — DO NOT change):
 *   objects.startup → LV_EVENT_SCREEN_LOAD_START   → action_logo_animation   (stub)
 *   objects.startup → LV_EVENT_SCREEN_UNLOAD_START → action_model_animation  (stub)
 * Registered manually in ui_actions_init() (our owned logic):
 *   objects.startup → LV_EVENT_SCREEN_LOAD_START   → action_startup_loaded
 *   objects.startup → LV_EVENT_SCREEN_UNLOAD_START → action_startup_unloaded
 *   objects.overview        → LV_EVENT_SCREEN_LOAD_START  → action_overview_loaded
 *   objects.home             → LV_EVENT_SCREEN_LOAD_START  → action_home_loaded
 *   objects.segurity         → LV_EVENT_SCREEN_LOAD_START  → action_security_loaded
 *   objects.menu             → LV_EVENT_SCREEN_LOAD_START  → action_menu_loaded
 *   objects.settings         → LV_EVENT_SCREEN_LOAD_START  → action_settings_loaded
 *   objects.system_information → LV_EVENT_SCREEN_LOAD_START → action_system_information_loaded
 *   objects.int_configuration    → LV_EVENT_SCREEN_LOAD_START  → action_int_configuration_loaded
 *   objects.gps_configuration    → LV_EVENT_SCREEN_LOAD_START  → action_gps_configuration_loaded
 *   objects.contact_configuration → LV_EVENT_SCREEN_LOAD_START → action_contact_configuration_loaded
 *   objects.general_configuration → LV_EVENT_SCREEN_LOAD_START → action_general_configuration_loaded
 *   objects.accessdenied            → LV_EVENT_SCREEN_LOAD_START → action_accessdenied_loaded
 *   objects.configuration_input     → LV_EVENT_SCREEN_LOAD_START → action_configuration_input_loaded
 *   objects.int_configuration_days  → LV_EVENT_SCREEN_LOAD_START → action_int_configuration_days_loaded
 *   objects.int_configuration_days_{mon…sun} → LV_EVENT_CLICKED  → action_int_configuration_day_clicked
 *   objects.int_configuration_period      → LV_EVENT_SCREEN_LOAD_START → action_int_configuration_period_loaded
 *   objects.int_configuration_period_menu → LV_EVENT_SCREEN_LOAD_START → action_int_configuration_period_menu_loaded
 *   objects.int_configuration_predefined   → LV_EVENT_SCREEN_LOAD_START → action_int_configuration_predefined_loaded
 *   objects.wifi_module              → LV_EVENT_SCREEN_LOAD_START → action_wifi_module_loaded
 *   objects.qr_info                  → LV_EVENT_SCREEN_LOAD_START → action_qr_info_loaded *   objects.admin_configuration      → LV_EVENT_SCREEN_LOAD_START → action_admin_configuration_loaded
 *   objects.admin_operation_mode     → LV_EVENT_SCREEN_LOAD_START → action_admin_operation_mode_loaded *
 * @author Tecna Smart Lab
 * @date   24 de Febrero 2026
 */
#include "presentation/ui_actions.h"
#include "presentation/presentation_layer.h" /* PresentationLayer_OnHomeEnter() */
#include "presentation/ui_helpers/lvgl_screen_helper.h"
/* EEZ / LVGL — allowed in this file only */
#include "ui/ui.h"      /* loadScreen(), ScreensEnum */
#include "ui/screens.h" /* objects.* */
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — Animation helper
 *============================================================================*/

/**
 * @brief lv_anim_exec_xcb_t wrapper for lv_obj_set_style_opa.
 *
 * lv_obj_set_style_opa takes 3 arguments and cannot be cast directly to
 * lv_anim_exec_xcb_t (which expects 2). This wrapper adapts the signature.
 *
 * @param[in] obj  lv_obj_t pointer (passed as void* by LVGL anim engine).
 * @param[in] v    Opacity value to apply (0 = transparent … 255 = opaque).
 */
static void set_opa_cb(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)v,
                         LV_PART_MAIN | LV_STATE_DEFAULT);
}

/*============================================================================*
 * PRIVATE — Navigation timer callbacks
 *============================================================================*/

/**
 * @brief One-shot timer callback: navigate STARTUP → OVERVIEW.
 * @param[in] t  Unused timer handle (auto-deleted after firing).
 */
static void nav_to_overview_cb(lv_timer_t *t)
{
    (void)t;
    LvglScreenHelper_Change(SCREEN_ID_OVERVIEW);
}

/**
 * @brief One-shot timer callback: navigate OVERVIEW → HOME.
 *
 * Routes through PresentationLayer_OnBootComplete() → IScreenRouter_NavigateTo()
 * so that:
 *   - active_screen is updated to HomePresenter.
 *   - InputRouter is wired (encoder / back button enabled).
 *   - IScreen_OnExit(NULL) is called safely on the boot placeholder.
 *
 * @param[in] t  Unused timer handle (auto-deleted after firing).
 */
static void nav_to_home_cb(lv_timer_t *t)
{
    (void)t;
    PresentationLayer_OnBootComplete();
}

/*============================================================================*
 * PUBLIC — EEZ action functions (declared in ui/actions.h)
 *============================================================================*/

/*============================================================================*
 * PUBLIC — EEZ extern stubs (declared in ui/actions.h, wired by screens.c)
 *
 * EEZ registers these on objects.startup via lv_obj_add_event_cb() in screen.c.
 * We intentionally leave them empty — the actual startup logic lives in the
 * static callbacks below, registered by ui_actions_init().
 * These stubs must remain public (non-static) so the linker satisfies the
 * extern declarations in ui/actions.h.
 *============================================================================*/

/*============================================================================*
 * PRIVATE — Our owned startup callbacks (static — not callable by EEZ)
 *============================================================================*/

/**
 * @brief STARTUP screen loaded — start logo animation and auto-navigate timer.
 *
 * Registered in ui_actions_init() via lv_obj_add_event_cb().
 * Runs in LVGL thread. No lv_lock() needed.
 *
 * Sequence:
 *   1. Logo fade-in (600 ms, no movement).
 *   2. Model label fade-in  (1 000 ms, 400 ms delay).
 *   3. One-shot timer → navigate to OVERVIEW after 2 500 ms.
 */
static void action_startup_loaded(lv_event_t *e)
{
    (void)e;

    /* ── 1. Logo: fade in (OPA 0 → 255, 600 ms, no movement) ─────────── */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, objects.logo);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    /* ── 2. Model label: fade in (OPA 0 → 255, 400 ms delay) ─────────── */
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, objects.model);
    lv_anim_set_exec_cb(&b, set_opa_cb);
    lv_anim_set_values(&b, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&b, 1000);
    lv_anim_set_delay(&b, 400);
    lv_anim_start(&b);

    /* ── 3. Auto-navigate to OVERVIEW after 2 500 ms ───────────────────── */
    lv_timer_t *t = lv_timer_create(nav_to_overview_cb, 2500U, NULL);
    lv_timer_set_repeat_count(t, 1);
}

/**
 * @brief STARTUP screen unloading — fade out logo.
 *
 * Registered in ui_actions_init() via lv_obj_add_event_cb().
 * Runs in LVGL thread. No lv_lock() needed.
 */
static void action_startup_unloaded(lv_event_t *e)
{
    (void)e;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, objects.logo);
    lv_anim_set_exec_cb(&a, set_opa_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 200);
    lv_anim_start(&a);
}

/**
 * @brief Second stage of Overview: write status and start HOME timer.
 *
 * Fires 4 000 ms after the Overview screen loaded.
 * Writes "System Ready" or "System Error" to objects.overview_info, then
 * starts a final one-shot 2 000 ms timer that navigates to HOME.
 *
 * @param[in] t  Timer handle (one-shot, auto-deleted after firing).
 */
static void on_overview_show_status_cb(lv_timer_t *t)
{
    (void)t;

    /* ── Write status message ─────────────────────────────────────────── */
    const char *msg = PresentationLayer_IsSystemReady()
                          ? "System Ready"
                          : "System Error";
    lv_label_set_text(objects.overview_info, msg);

    /* ── Navigate to HOME after 2 s so user can read the message ──────── */
    lv_timer_t *nav = lv_timer_create(nav_to_home_cb, 2000U, NULL);
    lv_timer_set_repeat_count(nav, 1);
}

/**
 * @brief OVERVIEW / LV_EVENT_SCREEN_LOAD_START handler (registered manually).
 *
 * ## Screen sequence on Overview
 *   t = 0 s  — screen loads, overview_info is blank
 *   t = 4 s  — "System Ready" / "System Error" written to overview_info
 *   t = 6 s  — navigate to HOME
 *
 * Two timers in chain:
 *   4 000 ms → on_overview_show_status_cb (writes label)
 *              └─► 2 000 ms → nav_to_home_cb
 *
 * Registered in ui_actions_init() because EEZ has not yet added this event
 * to the generated screens.c.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_overview_loaded(lv_event_t *e)
{
    (void)e;

    /* Clear any residual text so the label is blank while the system checks */
    lv_label_set_text(objects.overview_info, "...");

    /* First stage: wait 4 s before revealing the status */
    lv_timer_t *t = lv_timer_create(on_overview_show_status_cb, 4000U, NULL);
    lv_timer_set_repeat_count(t, 1);
}

/**
 * @brief HOME / LV_EVENT_SCREEN_LOAD_START handler (registered manually).
 *
 * Fires when the Home (operational) screen begins loading. Delegates to
 * PresentationLayer_OnHomeEnter() which calls IScreen_OnEnter on the
 * HomeScreenPresenter — sets initial labels so the screen is never blank.
 *
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_home_loaded(lv_event_t *e)
{
    (void)e;
    /* HOME is a status-display screen — no focusable widgets. Clear the
     * default group so the encoder/mouse don't try to focus stale objects
     * from a previous screen, which would trigger lv_group_get_editing(NULL). */
    lv_group_t *group = lv_group_get_default();
    if (group != NULL)
    {
        lv_group_remove_all_objs(group);
    }
    PresentationLayer_OnHomeEnter();
}

/**
 * @brief SECURITY / LV_EVENT_SCREEN_LOAD_START handler (registered manually).
 *
 * Fires when the Security screen begins loading. Delegates to
 * PresentationLayer_OnSecurityEnter() which calls IScreen_OnEnter on the
 * SecurityScreenPresenter — clears the password buffer and resets state.
 *
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_security_loaded(lv_event_t *e)
{
    (void)e;
    PresentationLayer_OnSecurityEnter();
}

/**
 * @brief MENU / LV_EVENT_SCREEN_LOAD_START handler (registered manually).
 *
 * Fires when the Main Menu screen begins loading.
 *
 * ## Sequence
 *   1. PresentationLayer_OnMenuEnter() → IScreen_OnEnter on MenuScreenPresenter
 *       → IMenuScreenView_InitItems → lv_list buttons populated.
 *   2. Group wiring: remove all objects from the default group, add objects.menu_list,
 *      and focus it — ensures UP/DOWN/ENTER from the encoder or keyboard driver
 *      navigate within the list.
 *
 * Executes in LVGL thread context (inside lv_task_handler).
 * No lv_lock() needed — lv_lock() is a no-op when already held by LVGL thread.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_menu_loaded(lv_event_t *e)
{
    (void)e;

    /* 1. Wire default input group — only the list is focusable on this screen */
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    /* 2. Populate list items via presenter → view (lv_lock handled inside) */
    PresentationLayer_OnMenuEnter();
}

/**
 * @brief SETTINGS / LV_EVENT_SCREEN_LOAD_START handler (registered manually).
 *
 * Fires when the Settings screen begins loading. Delegates to
 * PresentationLayer_OnSettingsEnter() which calls IScreen_OnEnter on the
 * SettingsScreenPresenter — populates the list items.
 *
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_settings_loaded(lv_event_t *e)
{
    (void)e;

    /* 1. Wire default input group — only the settings list is focusable */
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);

    /* 2. Populate list items via presenter → view */
    PresentationLayer_OnSettingsEnter();
}

/**
 * @brief SYSTEM_INFORMATION / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Populates the list-nav entries and restores last focused item.
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_system_information_loaded(lv_event_t *e)
{
    (void)e;

    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);

    PresentationLayer_OnSystemInformationEnter();
}

/**
 * @brief DEVICE_STATUS / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Clears the encoder group and calls the presenter OnEnter.
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
/**
 * @brief INFORMATION_SHOW / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Clears the encoder group (info-only screen) and notifies the unified
 * information presenter to render the pending item.
 *
 * Executes in LVGL thread context. No lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_information_show_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnInformationShowEnter();
}

/**
 * @brief QR_INFO / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Clears encoder focus group (no interactive widgets on QR screens)
 * then hands off to presenter.
 */
static void action_qr_info_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnQrInfoEnter();
}

/**
 * @brief INT_CONFIGURATION / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_int_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnIntConfigurationEnter();
}

/**
 * @brief GPS_CONFIGURATION / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_gps_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);

    PresentationLayer_OnGpsConfigurationEnter();
}

/**
 * @brief CONTACT_CONFIGURATION / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_contact_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);

    PresentationLayer_OnContactConfigurationEnter();
}

/**
 * @brief GENERAL_CONFIGURATION / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_general_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);

    PresentationLayer_OnGeneralConfigurationEnter();
}

/**
 * @brief ACCESSDENIED / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Notifies the AccessDenied presenter so it starts the 2-second auto-return
 * timer and triggers the buzzer feedback.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_accessdenied_loaded(lv_event_t *e)
{
    (void)e;
    PresentationLayer_OnAccessDeniedEnter();
}

/**
 * @brief CONFIGURATION_INPUT / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Notifies the ConfigurationInput presenter so it loads the current
 * DateTime_t for the configured parameter and refreshes the display.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_configuration_input_loaded(lv_event_t *e)
{
    (void)e;
    PresentationLayer_OnConfigurationInputEnter();
}

/**
 * @brief INT_CONFIGURATION_DAYS / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Adds all 7 day checkboxes to the default encoder group (so the encoder
 * can navigate them), then notifies the presenter to load weekday_mask.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_int_configuration_days_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    lv_group_add_obj(group, objects.int_configuration_days_mon);
    lv_group_add_obj(group, objects.int_configuration_days_tue);
    lv_group_add_obj(group, objects.int_configuration_days_wed);
    lv_group_add_obj(group, objects.int_configuration_days_thu);
    lv_group_add_obj(group, objects.int_configuration_days_fri);
    lv_group_add_obj(group, objects.int_configuration_days_sat);
    lv_group_add_obj(group, objects.int_configuration_days_sun);

    PresentationLayer_OnIntConfigurationDaysEnter();
}

/**
 * @brief Generic LV_EVENT_CLICKED callback for any day checkbox.
 *
 * The day index (0 = Monday … 6 = Sunday) is passed as @c user_data.
 * Reads the new checked state from the widget and delegates to the
 * presenter via PresentationLayer_OnIntConfigurationDayClicked().
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event; target = checkbox widget, user_data = day index.
 */
static void action_int_configuration_day_clicked(lv_event_t *e)
{
    lv_obj_t *checkbox = lv_event_get_target(e);
    uint8_t day_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    bool checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    PresentationLayer_OnIntConfigurationDayClicked(day_idx, checked);
}

/**
 * @brief INT_CONFIGURATION_PERIOD / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Adds the list widget to the default encoder group so the encoder can
 * navigate between Single / Multiple items.  Then calls
 * OnIntConfigurationPeriodEnter() which rebuilds the list with the
 * current selection. Selection callbacks handle save + navigation.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_int_configuration_period_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnIntConfigurationPeriodEnter();
}

/**
 * @brief INT_CONFIGURATION_PERIOD_MENU / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Adds the list widget to the default encoder group so the encoder can
 * navigate cycle items.  Then calls OnIntConfigurationPeriodMenuEnter() which
 * resets current_index to 0 and rebuilds the list for the start of the menu.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_int_configuration_period_menu_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnIntConfigurationPeriodMenuEnter();
}

/**
 * @brief LV_EVENT_SCREEN_LOAD_START handler for int_configuration_state screen.
 *
 * Reads relay.enabled from config storage via the presenter and synchronises
 * the switch widget. Executes in LVGL thread — no lv_lock() needed here.
 */
static void action_int_configuration_state_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    lv_group_add_obj(group, objects.int_configuration_status);

    PresentationLayer_OnIntConfigurationStateEnter();
}

/**
 * @brief LV_EVENT_VALUE_CHANGED handler for the int_configuration_status switch.
 *
 * Reads the new switch state and delegates persistence to the presenter
 * which saves relay.enabled to EEPROM asynchronously.
 */
static void action_int_configuration_state_switch_toggle(lv_event_t *e)
{
    (void)e;
    bool checked = lv_obj_has_state(objects.int_configuration_status,
                                    LV_STATE_CHECKED);
    PresentationLayer_OnIntConfigurationStateToggle(checked);
}

/**
 * @brief LV_EVENT_SCREEN_LOAD_START handler for int_configuration_start screen.
 *
 * Reads relay.start_with_on from config storage via the presenter and
 * synchronises the switch widget. Executes in LVGL thread.
 */
static void action_int_configuration_start_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    lv_group_add_obj(group, objects.int_configuration_start_value);

    PresentationLayer_OnIntConfigurationStartEnter();
}

/**
 * @brief LV_EVENT_VALUE_CHANGED handler for the int_configuration_start_value switch.
 *
 * Reads the new switch state, persists relay.start_with_on, and arms the
 * auto-return timer via the presenter.
 */
static void action_int_configuration_start_switch_toggle(lv_event_t *e)
{
    (void)e;
    bool checked = lv_obj_has_state(objects.int_configuration_start_value,
                                    LV_STATE_CHECKED);
    PresentationLayer_OnIntConfigurationStartToggle(checked);
}

/**
 * @brief LV_EVENT_SCREEN_LOAD_START handler for int_configuration_predefined screen.
 *
 * Populates the predefined cycles list and resets the feedback label.
 * Executes in LVGL thread — no lv_lock() needed.
 */
static void action_int_configuration_predefined_loaded(lv_event_t *e)
{
    (void)e;
    PresentationLayer_OnIntConfigurationPredefinedEnter();
}

/**
 * @brief GPS_CONFIGURATION_ANTENNA / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Clears the encoder group, then notifies the presenter to load
 * GPSConfig_t.antenna_type and rebuild the list with current selection focused.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_gps_configuration_antenna_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnGpsConfigurationAntennaEnter();
}

/**
 * @brief LVGL screen-load callback for the Contact Configuration Type screen.
 *
 * Clears the encoder group so list items can self-register, then calls
 * the presenter OnEnter to load RelayConfig_t.contact_type and rebuild
 * the selection list.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_contact_configuration_type_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnContactConfigurationTypeEnter();
}

/**
 * @brief Fired by LVGL when the General Configuration Alarm screen loads.
 *
 * Clears the encoder group so that AddListItem() can re-populate it with
 * the selection list.
 *
 * Executes in LVGL thread — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_general_configuration_alarm_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnGeneralConfigurationAlarmEnter();
}
/**
 * @brief WIFI_MODULE / LV_EVENT_SCREEN_LOAD_START handler.
 *
 * Clears the encoder group and notifies the WiFi Module list presenter
 * to populate the 5-item navigation list.
 *
 * Executes in LVGL thread context — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_wifi_module_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnWifiModuleEnter();
}

/**
 * @brief LVGL event callback: WiFi Configuration screen load start.
 *
 * Clears the LVGL encoder group and delegates to the presenter OnEnter
 * to populate the 2-item action list.
 *
 * Executes in LVGL thread context — no lv_lock() needed.
 *
 * @param[in] e  LVGL event (unused).
 */
static void action_wifi_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnWifiConfigurationEnter();
}

static void action_admin_configuration_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnAdminConfigurationEnter();
}

static void action_admin_operation_mode_loaded(lv_event_t *e)
{
    (void)e;
    lv_group_t *group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    PresentationLayer_OnAdminOperationModeEnter();
}
/*============================================================================*
 * PUBLIC — Manual registration
 *============================================================================*/

/**
 * @brief PC-only: periodic LVGL timer callback that drives PresentationLayer_Update().
 *
 * On real firmware the Control AO calls PresentationLayer_Update() every 100ms.
 * On PC that AO does not exist, so this timer keeps screen data (time, wifi,
 * GPS, etc.) updated without modifying any real-firmware source files.
 */
static void presentation_update_timer_cb(lv_timer_t *t)
{
    (void)t;
    PresentationLayer_Update();
}

/**
 * @brief Register LVGL event callbacks not yet wired by EEZ Studio.
 *
 * EEZ wires action_logo_animation / action_model_animation for objects.startup
 * in screens.c — those are empty stubs.  Our static action_startup_loaded /
 * action_startup_unloaded are registered here and contain the actual logic.
 *
 * @note Call once from ui_ao.c immediately after ui_init().
 */
void ui_actions_init(void)
{
    /* Startup: register our owned callbacks.
     * EEZ's action_logo_animation / action_model_animation (wired by screens.c)
     * are empty stubs — these static callbacks contain the actual logic. */
    lv_obj_add_event_cb(objects.startup,
                        action_startup_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);
    lv_obj_add_event_cb(objects.startup,
                        action_startup_unloaded,
                        LV_EVENT_SCREEN_UNLOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.overview,
                        action_overview_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.home,
                        action_home_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.segurity,
                        action_security_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.menu,
                        action_menu_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.settings,
                        action_settings_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.system_information,
                        action_system_information_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.information_show,
                        action_information_show_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.qr_info,
                        action_qr_info_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration,
                        action_int_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.gps_configuration,
                        action_gps_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.contact_configuration,
                        action_contact_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.general_configuration,
                        action_general_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration_state,
                        action_int_configuration_state_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration_status,
                        action_int_configuration_state_switch_toggle,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration_start,
                        action_int_configuration_start_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration_start_value,
                        action_int_configuration_start_switch_toggle,
                        LV_EVENT_VALUE_CHANGED,
                        NULL);

    lv_obj_add_event_cb(objects.accessdenied,
                        action_accessdenied_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.configuration_input,
                        action_configuration_input_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* IntConfigurationDays: screen-load + per-checkbox click handlers */
    lv_obj_add_event_cb(objects.int_configuration_days,
                        action_int_configuration_days_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    lv_obj_add_event_cb(objects.int_configuration_days_mon,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)0U);
    lv_obj_add_event_cb(objects.int_configuration_days_tue,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)1U);
    lv_obj_add_event_cb(objects.int_configuration_days_wed,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)2U);
    lv_obj_add_event_cb(objects.int_configuration_days_thu,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)3U);
    lv_obj_add_event_cb(objects.int_configuration_days_fri,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)4U);
    lv_obj_add_event_cb(objects.int_configuration_days_sat,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)5U);
    lv_obj_add_event_cb(objects.int_configuration_days_sun,
                        action_int_configuration_day_clicked,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)6U);

    /* IntConfigurationPeriod: screen-load (list items register their own callbacks) */
    lv_obj_add_event_cb(objects.int_configuration_period,
                        action_int_configuration_period_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* IntConfigurationPeriodMenu: screen-load */
    lv_obj_add_event_cb(objects.int_configuration_period_menu,
                        action_int_configuration_period_menu_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* IntConfigurationPredefined: screen-load (list items register their own callbacks
     * via EEZ view InitItems() — no per-item registrations needed here) */
    lv_obj_add_event_cb(objects.int_configuration_predefined,
                        action_int_configuration_predefined_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* GpsConfigurationAntenna: screen-load (list items register their own callbacks
     * via EEZ view AddListItem() — no per-item registrations needed here) */
    lv_obj_add_event_cb(objects.gps_configuration_antenna,
                        action_gps_configuration_antenna_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* ContactConfigurationType: screen-load (list items register their own callbacks
     * via EEZ view AddListItem() — no per-item registrations needed here) */
    lv_obj_add_event_cb(objects.contact_configuration_type,
                        action_contact_configuration_type_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* GeneralConfigurationAlarm: screen-load (list items register their own callbacks
     * via EEZ view AddListItem() — no per-item registrations needed here) */
    lv_obj_add_event_cb(objects.general_configuration_alarm,
                        action_general_configuration_alarm_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* WiFi Module: screen-load — populates 5-item navigation list */
    lv_obj_add_event_cb(objects.wifi_module,
                        action_wifi_module_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* WiFi Configuration: screen-load — populates 2-item action list */
    lv_obj_add_event_cb(objects.wifi_configuration,
                        action_wifi_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* Admin Configuration: screen-load — populates 2-item admin list */
    lv_obj_add_event_cb(objects.admin_configuration,
                        action_admin_configuration_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* Admin Operation Mode: screen-load — populates Free/Rent selection list */
    lv_obj_add_event_cb(objects.admin_operation_mode,
                        action_admin_operation_mode_loaded,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);

    /* PC simulator: the Control AO does not exist, so nobody calls
     * PresentationLayer_Update(). Drive it here with a 100 ms LVGL timer
     * so all screen on_update() handlers (time, wifi, GPS, etc.) run. */
    lv_timer_t *update_timer = lv_timer_create(
        presentation_update_timer_cb,
        100U, NULL);
    (void)update_timer;
}
