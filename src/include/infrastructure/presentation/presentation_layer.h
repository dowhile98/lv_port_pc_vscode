/**
 * @file presentation_layer.h
 * @brief Thin shim — bridges LVGL actions (ui_actions.c) to Presenters.
 *
 * Holds static Presenter instances and exposes:
 *  - PresentationLayer_Init()        — called once from DI_InitUISubsystem()
 *  - PresentationLayer_SetRouter()   — wires the active-screen router (ACTION-029)
 *  - PresentationLayer_OnHomeEnter() — called from action_home_loaded()
 *  - PresentationLayer_Update()      — called from Control thread; dispatches to
 *                                      the currently active screen via IScreenRouter_t
 *
 * This file is the only place where Presentation-layer objects are allocated.
 * It deliberately has no LVGL dependency — all LVGL calls go through the view.
 */
#ifndef PRESENTATION_LAYER_H
#define PRESENTATION_LAYER_H

#include "hal/hal_types.h"
#include "presentation/screens/home/home_screen_presenter.h"
#include "presentation/screens/security/security_screen_presenter.h"
#include "presentation/screens/list_nav/list_nav_screen_presenter.h"
#include "presentation/screens/int_configuration_state/int_configuration_state_screen_presenter.h"
#include "presentation/screens/int_configuration_start/int_configuration_start_screen_presenter.h"
#include "presentation/screens/int_configuration_predefined/int_configuration_predefined_screen_presenter.h"
#include "presentation/screens/access_denied/access_denied_screen_presenter.h"
#include "presentation/screens/configuration_input/configuration_input_screen_presenter.h"
#include "presentation/screens/int_configuration_days/int_configuration_days_screen_presenter.h"
#include "presentation/screens/int_configuration_period/int_configuration_period_screen_presenter.h"
#include "presentation/screens/int_configuration_period_menu/int_configuration_period_menu_screen_presenter.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/screens/gps_configuration_antenna/gps_configuration_antenna_screen_presenter.h"
#include "presentation/screens/contact_configuration_type/contact_configuration_type_screen_presenter.h"
#include "presentation/screens/general_configuration_alarm/general_configuration_alarm_screen_presenter.h"
#include "presentation/screens/information_show/information_show_screen_presenter.h"
#include "presentation/screens/qr_info/qr_info_screen_presenter.h"
#include "presentation/screens/wifi_configuration/wifi_configuration_screen_presenter.h"
#include "presentation/screens/admin_operation_mode/admin_operation_mode_screen_presenter.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialise the presentation layer with injected dependencies.
     *
     * Creates and wires the HomeScreenPresenter with all domain interfaces.
     * Must be called once from DI_InitUISubsystem() after UiAO_Init().
     *
     * @param[in] deps  Fully populated dependency bundle (must not be NULL;
     *                  all fields must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if deps or any field is NULL.
     */
    Result_t PresentationLayer_Init(const HomeScreenPresenterDeps_t *deps);

    /**
     * @brief Initialise the Security screen presenter (ACTION-031).
     *
     * @param[in] deps  Fully populated security deps bundle.
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitSecurity(const SecurityScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Security presenter.
     *
     * Used by DI container to register the screen in EezScreenRouter.
     */
    IScreen_t *PresentationLayer_GetSecurityScreen(void);

    /**
     * @brief Return the IScreen_t pointer for the Home presenter.
     *
     * Used by DI container to register the screen in EezScreenRouter (ACTION-031+).
     */
    IScreen_t *PresentationLayer_GetHomeScreen(void);

    /**
     * @brief Initialise the Menu screen presenter.
     *
     * @param[in] deps  Fully populated menu deps bundle.
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitMenu(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Menu presenter.
     *
     * Used by the DI container to register the screen in EezScreenRouter.
     */
    IScreen_t *PresentationLayer_GetMenuScreen(void);

    /**
     * @brief Called by action_menu_loaded() when MENU screen begins loading.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnMenuEnter(void);

    /**
     * @brief Initialise the Settings screen presenter.
     *
     * @param[in] deps  Fully populated settings deps bundle.
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitSettings(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Settings presenter.
     *
     * Used by the DI container to register the screen in EezScreenRouter.
     */
    IScreen_t *PresentationLayer_GetSettingsScreen(void);

    /**
     * @brief Called by action_settings_loaded() when SETTINGS screen begins loading.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnSettingsEnter(void);

    /**
     * @brief Initialise the System Information screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle.
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitSystemInformation(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the System Information presenter.
     */
    IScreen_t *PresentationLayer_GetSystemInformationScreen(void);

    /**
     * @brief Called by action_system_information_loaded() when screen loads.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnSystemInformationEnter(void);

    /* ------------------------------------------------------------------ */
    /* Information Show (unified: Device Status, Interrupt Configuration,  */
    /* GPS Status, Battery Status)                                         */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Set the pending information item index.
     *
     * Called via ListNavScreenPresenter pre_navigate_fn before EEZ routes to
     * SCREEN_ID_INFORMATION_SHOW.  Safe to call from LVGL thread.
     *
     * @param[in] item_idx  Zero-based index into the provider table (0..3).
     */
    void PresentationLayer_SetPendingInfoItem(uint8_t item_idx);

    /**
     * @brief Get a pointer to the shared pending item byte.
     *
     * Injected into InformationShowScreenPresenterDeps_t.pending_item_ptr
     * so the presenter can read the current selection at OnEnter time.
     *
     * @return Pointer to the static pending item byte (never NULL).
     */
    const uint8_t *PresentationLayer_GetPendingInfoItemPtr(void);

    /**
     * @brief Set the dynamic back-screen ID for the information_show presenter.
     *
     * Must be called from the pre_navigate_fn of whichever list-nav screen
     * is navigating to SCREEN_ID_INFORMATION_SHOW so that the back button
     * returns to the correct parent screen.
     *
     * @param[in] screen_id  Target screen ID to return to on back press.
     */
    void PresentationLayer_SetInfoBackScreen(uint8_t screen_id);

    /**
     * @brief Set the confirmation message for the INFORMATION_SHOW_ITEM_CONFIRMATION provider.
     *
     * Copies @p msg into an internal static buffer.  The buffer is injected into
     * InformationShowScreenPresenterDeps_t.pending_confirmation_msg as a pointer,
     * so calling this before navigating to item 15 makes the message available to
     * the presenter.
     *
     * @param[in] msg  NUL-terminated string (must not be NULL; truncated at 63 chars).
     */
    void PresentationLayer_SetConfirmationMessage(const char *msg);

    /**
     * @brief Set the optional callback that the SecurityScreenPresenter invokes on successful
     *        password validation (before navigating to success_screen_id).
     *
     * Should be called from a general-config pre_navigate_fn just before routing to
     * SCREEN_ID_SEGURITY so that the correct action fires on the next successful login.
     *
     * @param[in] fn   Callback function (may be NULL to clear a previous action).
     * @param[in] ctx  Context pointer passed to @p fn.
     */
    void PresentationLayer_SetSecuritySuccessAction(void (*fn)(void *ctx), void *ctx);

    /**
     * @brief Override the screen navigated to after a successful password entry.
     *
     * Call from a pre_navigate_fn to redirect the security screen's success
     * destination on a per-action basis.
     *
     * @param[in] screen_id  Target screen ID for successful validation.
     */
    void PresentationLayer_SetSecuritySuccessScreenId(uint8_t screen_id);

    /**
     * @brief Override the screen navigated to after pressing BACK or entering wrong password.
     *
     * @param[in] screen_id  Target screen ID for cancelled / failed validation.
     */
    void PresentationLayer_SetSecurityCancelScreenId(uint8_t screen_id);

    /**
     * @brief Reconfigura contraseña y destinos de la instancia Security en una sola llamada.
     *
     * Equivale a llamar SetContext() en el SecurityScreenPresenter estático.
     * Llamar en el pre_navigate_fn del menú, ANTES de navegar a SCREEN_ID_SEGURITY.
     *
     * @param[in] password          Nueva contraseña (must not be NULL).
     * @param[in] password_len      Longitud (1..SECURITY_MAX_INPUT_LEN).
     * @param[in] success_screen_id Screen destino si contraseña válida.
     * @param[in] cancel_screen_id  Screen destino si BACK o contraseña inválida.
     */
    void PresentationLayer_SetSecurityContext(const uint8_t *password,
                                              uint8_t password_len,
                                              uint8_t success_screen_id,
                                              uint8_t cancel_screen_id);

    /**
     * @brief Reset security context to default state (UP DOWN DOWN UP → MENU | HOME).
     *
     * Call this before navigating to SCREEN_ID_SEGURITY to clear any stale state
     * from previous failed/cancelled attempts. Ensures consistent behavior when
     * entering security from different entry points (Home, Menu, etc.).
     *
     * Default context:
     *   - Password: UP DOWN DOWN UP (4 keys)
    *   - Success screen: MENU
    *   - Cancel screen: HOME
     */
    void PresentationLayer_ResetSecurityToDefault(void);

    /**
     * @brief Get a pointer to the static confirmation message buffer.
     *
     * Injected into InformationShowScreenPresenterDeps_t.pending_confirmation_msg.
     *
     * @return Pointer to the static string buffer (never NULL).
     */
    const char *PresentationLayer_GetConfirmationMessagePtr(void);

    /**
     * @brief Get a pointer to the shared info back-screen ID byte.
     *
     * Injected into InformationShowScreenPresenterDeps_t.back_screen_id_ptr.
     *
     * @return Pointer to the static back-screen ID byte (never NULL).
     */
    const uint8_t *PresentationLayer_GetInfoBackScreenPtr(void);

    /**
     * @brief Initialise the unified information_show screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + pending_item_ptr required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitInformationShow(const InformationShowScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the information_show presenter.
     *
     * Used by the DI container to register the screen in EezScreenRouter.
     */
    IScreen_t *PresentationLayer_GetInformationShowScreen(void);

    /**
     * @brief Called by action_information_show_loaded() when screen loads.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnInformationShowEnter(void);

    /**
     * @brief Called periodically to refresh the displayed data.
     *
     * Must be called from the control thread (~1 s period) while the screen
     * is visible.  The presenter checks the active flag internally.
     */
    void PresentationLayer_OnInformationShowUpdate(void);

    /* ── QR Info (WiFi QR / Web Server QR) ──────────────────────────────── */

    /**
     * @brief Set the pending QR item index before navigating to SCREEN_ID_QR_INFO.
     *
     * Called by the ListNavScreenPresenter pre_navigate_fn.
     *   idx 0 → Connect via AP (QR)
     *   idx 1 → Web Server (AP) QR
     */
    void PresentationLayer_SetPendingQrItem(uint8_t item_idx);

    /**
     * @brief Return pointer to the pending QR item byte.
     *
     * Injected into QrInfoScreenPresenterDeps_t.pending_item_ptr.
     */
    const uint8_t *PresentationLayer_GetPendingQrItemPtr(void);

    /**
     * @brief Set the back-screen ID for the QR Info screen.
     *
     * Called by the ListNavScreenPresenter pre_navigate_fn so the QR screen
     * can navigate back to the correct list (e.g., WiFi Module).
     */
    void PresentationLayer_SetQrBackScreen(uint8_t screen_id);

    /**
     * @brief Return pointer to the QR back-screen ID byte.
     *
     * Injected into QrInfoScreenPresenterDeps_t.back_screen_id_ptr.
     */
    const uint8_t *PresentationLayer_GetQrBackScreenPtr(void);

    /**
     * @brief Initialise the QR Info screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + pending_item_ptr required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitQrInfo(const QrInfoScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the QR Info presenter.
     */
    IScreen_t *PresentationLayer_GetQrInfoScreen(void);

    /**
     * @brief Called by action_qr_info_loaded() when the QR Info screen loads.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnQrInfoEnter(void);

    /* ── WiFi Configuration screen ─────────────────────────────────────── */

    /**
     * @brief Initialise the WiFi Configuration screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitWifiConfiguration(
        const WifiConfigurationScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the WiFi Configuration presenter.
     *
     * Used by the DI container to register the screen in EezScreenRouter.
     */
    IScreen_t *PresentationLayer_GetWifiConfigurationScreen(void);

    /**
     * @brief Called by action_wifi_configuration_loaded() when screen loads.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnWifiConfigurationEnter(void);

    /**
     * @brief Initialise the Interrupt Configuration screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle (7 items, back=SETTINGS).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfiguration(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Interrupt Configuration presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationScreen(void);

    /**
     * @brief Called by action_int_configuration_loaded().
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnIntConfigurationEnter(void);

    /**
     * @brief Initialise the Interrupt Configuration State screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationState(
        const IntConfigurationStateScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Int Config State presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationStateScreen(void);

    /**
     * @brief Called by action_int_configuration_state_loaded() (LVGL thread).
     * Reads relay.enabled from config and syncs the switch widget.
     */
    void PresentationLayer_OnIntConfigurationStateEnter(void);

    /**
     * @brief Called by action_int_configuration_state_switch_toggle() (LVGL thread).
     *
     * Loads current SystemConfig, updates relay.enabled, and saves back.
     *
     * @param[in] new_state  New switch state (true = enabled).
     */
    void PresentationLayer_OnIntConfigurationStateToggle(bool new_state);

    /**
     * @brief Initialise the Interrupt Configuration Start screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationStart(
        const IntConfigurationStartScreenPresenterDeps_t *deps);

    /**
     * @brief Called by action_int_configuration_start_loaded() (LVGL thread).
     * Reads relay.start_with_on from config and syncs the switch widget.
     */
    void PresentationLayer_OnIntConfigurationStartEnter(void);

    /**
     * @brief Called by action_int_configuration_start_switch_toggle() (LVGL thread).
     *
     * Saves relay.start_with_on, shows "Saved!", and arms auto-return timer.
     *
     * @param[in] start_with_on  New switch state (true = start ON).
     */
    void PresentationLayer_OnIntConfigurationStartToggle(bool start_with_on);

    /**
     * @brief Return the IScreen_t pointer for the Int Config Start presenter.
     *
     * Used by the EEZ router to dispatch OnUpdate to the active screen.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationStartScreen(void);

    /**
     * @brief Initialise the Predefined Cycles screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationPredefined(
        const IntConfigurationPredefinedScreenPresenterDeps_t *deps);

    /**
     * @brief Called by action_int_configuration_predefined_loaded() (LVGL thread).
     * Populates the cycle list and resets display state.
     */
    void PresentationLayer_OnIntConfigurationPredefinedEnter(void);

    /**
     * @brief Return the IScreen_t pointer for the Predefined Cycles presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationPredefinedScreen(void);

    /**
     * @brief Initialise the GPS Configuration screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle (3 items, back=SETTINGS).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitGpsConfiguration(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the GPS Configuration presenter.
     */
    IScreen_t *PresentationLayer_GetGpsConfigurationScreen(void);

    /**
     * @brief Called by action_gps_configuration_loaded().
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnGpsConfigurationEnter(void);

    /**
     * @brief Initialise the GPS Configuration Antenna selection screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitGpsConfigurationAntenna(
        const GpsConfigurationAntennaScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the GPS Configuration Antenna presenter.
     */
    IScreen_t *PresentationLayer_GetGpsConfigurationAntennaScreen(void);

    /**
     * @brief Called by action_gps_configuration_antenna_loaded() (LVGL thread).
     * Loads GPSConfig_t, rebuilds list (current antenna focused).
     */
    void PresentationLayer_OnGpsConfigurationAntennaEnter(void);

    /**
     * @brief Initialise the Contact Configuration screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle (3 items, back=SETTINGS).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitContactConfiguration(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Contact Configuration presenter.
     */
    IScreen_t *PresentationLayer_GetContactConfigurationScreen(void);

    /**
     * @brief Called by action_contact_configuration_loaded().
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnContactConfigurationEnter(void);

    /**
     * @brief Initialise the Contact Configuration Type selection screen presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitContactConfigurationType(
        const ContactConfigurationTypeScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Contact Configuration Type presenter.
     */
    IScreen_t *PresentationLayer_GetContactConfigurationTypeScreen(void);

    /**
     * @brief Called by action_contact_configuration_type_loaded() (LVGL thread).
     * Loads RelayConfig_t, rebuilds list (current type focused).
     */
    void PresentationLayer_OnContactConfigurationTypeEnter(void);

    /* ------------------------------------------------------------------ */
    /* General Configuration Alarm                                         */
    /* ------------------------------------------------------------------ */

    /**
     * @brief Initialise the General Configuration Alarm selection presenter.
     *
     * @param[in] deps  Fully populated deps bundle (view, config_storage, router,
     *                  back_screen_id = SCREEN_ID_GENERAL_CONFIGURATION).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitGeneralConfigurationAlarm(
        const GeneralConfigurationAlarmScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the General Configuration Alarm presenter.
     */
    IScreen_t *PresentationLayer_GetGeneralConfigurationAlarmScreen(void);

    /**
     * @brief Called by action_general_configuration_alarm_loaded() (LVGL thread).
     *        Loads GeneralConfig_t, rebuilds list (current value focused).
     */
    void PresentationLayer_OnGeneralConfigurationAlarmEnter(void);

    /**
     * @brief Initialise the General Configuration screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle (6 items, back=SETTINGS).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitGeneralConfiguration(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the General Configuration presenter.
     */
    IScreen_t *PresentationLayer_GetGeneralConfigurationScreen(void);

    /**
     * @brief Called by action_general_configuration_loaded().
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnGeneralConfigurationEnter(void);

    /**
     * @brief Initialise the Access Denied modal screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + router required, buzzer optional).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitAccessDenied(const AccessDeniedScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Access Denied presenter.
     */
    IScreen_t *PresentationLayer_GetAccessDeniedScreen(void);

    /**
     * @brief Called by action_accessdenied_loaded() (LVGL thread).
     * Resets the countdown timer, fires buzzer, and shows the message.
     */
    void PresentationLayer_OnAccessDeniedEnter(void);

    /**
     * @brief Override the return screen for the next Access Denied visit.
     *
     * Call BEFORE navigating to SCREEN_ID_ACCESSDENIED so that the
     * auto-dismiss navigates to the correct caller screen.
     *
     * @param[in] screen_id  Screen to return to after the 2-second timeout.
     */
    void PresentationLayer_SetAccessDeniedReturnScreen(uint8_t screen_id);

    /**
     * @brief Override the message shown on the Access Denied modal.
     *
     * Call before navigating to SCREEN_ID_ACCESSDENIED to display a
     * context-specific message (e.g. license expired).
     * NULL restores the default "CYCLE IN PROGRESS" message.
     *
     * @param[in] msg  String literal (caller must keep alive while screen is visible).
     *                 NULL → default message.
     */
    void PresentationLayer_SetAccessDeniedMessage(const char *msg);

    /**
     * @brief Initialise the Configuration Input (HH:MM:SS) screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitConfigurationInput(
        const ConfigurationInputScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Configuration Input presenter.
     */
    IScreen_t *PresentationLayer_GetConfigurationInputScreen(void);

    /**
     * @brief Called by action_configuration_input_loaded() (LVGL thread).
     * Loads time config, sets prompt label and initial value.
     */
    void PresentationLayer_OnConfigurationInputEnter(void);

    /**
     * @brief Select which DateTime_t will be edited on the next OnEnter.
     *
     * Must be called BEFORE navigating to SCREEN_ID_CONFIGURATION_INPUT.
     * Typically invoked from the ListNav pre_navigate hook.
     *
     * @param[in] param  CONFIG_INPUT_PARAM_START_TIME or STOP_TIME.
     */
    void PresentationLayer_SetConfigInputParam(ConfigInputParam_t param);

    /**
     * @brief Configure the ConfigurationInput presenter with full context before
     *        navigating to it.  Sets param, cycle_idx AND back_screen_id so that
     *        OnBackPressed always returns to the correct originating screen.
     *
     * @param[in] param          Which value to edit.
     * @param[in] cycle_idx      Cycle index (0 for single / start / stop time).
     * @param[in] back_screen_id Screen to return to on back/save.
     */
    void PresentationLayer_SetConfigInputContext(
        ConfigInputParam_t param, uint8_t cycle_idx, uint8_t back_screen_id);

    /**
     * @brief Initialise the IntConfigurationDays screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + config_storage required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationDays(
        const IntConfigurationDaysScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the IntConfigurationDays presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationDaysScreen(void);

    /**
     * @brief Called by action_int_configuration_days_loaded() (LVGL thread).
     * Loads weekday_mask and refreshes all 7 checkboxes.
     */
    void PresentationLayer_OnIntConfigurationDaysEnter(void);

    /**
     * @brief Called when a day checkbox is clicked (LV_EVENT_CLICKED).
     *
     * Invoked from ui_actions.c in the checkbox event callback.
     * Executes in LVGL thread context.
     *
     * @param[in] day_idx  0 = Monday … 6 = Sunday.
     * @param[in] checked  New state of the checkbox after the click.
     */
    void PresentationLayer_OnIntConfigurationDayClicked(uint8_t day_idx, bool checked);

    /* ---------------------------------------------------------------------- */
    /* IntConfigurationPeriod (list — Single / Multiple)                     */
    /* ---------------------------------------------------------------------- */

    /**
     * @brief Initialise the IntConfigurationPeriod list screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + config_storage + router required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationPeriod(
        const IntConfigurationPeriodScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the IntConfigurationPeriod presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationPeriodScreen(void);

    /**
     * @brief Called by action_int_configuration_period_loaded() (LVGL thread).
     * Loads multicycle.enabled and rebuilds the list with the current selection.
     */
    void PresentationLayer_OnIntConfigurationPeriodEnter(void);

    /**
     * @brief Returns the callback for period-menu → configuration_input context
     *        wiring.  Translates PeriodEditParam_t to ConfigInputParam_t and
     *        calls ConfigurationInputScreenPresenter_SetContext().
     */
    void (*PresentationLayer_GetPeriodEditContextFn(void))(void *, uint8_t, uint8_t);

    /**
     * @brief Returns the context pointer for PresentationLayer_GetPeriodEditContextFn()
     *        (always NULL — context-free static function).
     */
    void *PresentationLayer_GetPeriodEditContextCtx(void);

    /**
     * @brief Returns the callback that pushes mode (0=Single, 1=Multiple) to
     *        the period menu presenter, so it never reads storage on entry.
     *
     * Registered as set_period_menu_mode_fn in IntConfigurationPeriodScreenPresenterDeps.
     */
    void (*PresentationLayer_GetPeriodMenuSetModeFn(void))(void *, uint8_t);

    /**
     * @brief Returns the context pointer for PresentationLayer_GetPeriodMenuSetModeFn()
     *        (pointer to the static menu presenter instance).
     */
    void *PresentationLayer_GetPeriodMenuSetModeCtx(void);

    /* ---------------------------------------------------------------------- */
    /* IntConfigurationPeriodMenu (dynamic cycle list)                        */
    /* ---------------------------------------------------------------------- */

    /**
     * @brief Initialise the IntConfigurationPeriodMenu screen presenter.
     *
     * @param[in] deps  Dependency bundle (view + config_storage + router +
     *                  set_edit_context_fn required).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitIntConfigurationPeriodMenu(
        const IntConfigurationPeriodMenuScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the IntConfigurationPeriodMenu presenter.
     */
    IScreen_t *PresentationLayer_GetIntConfigurationPeriodMenuScreen(void);

    /**
     * @brief Called by action_int_configuration_period_menu_loaded() (LVGL thread).
     * Loads mode, resets current_index to 0, rebuilds the list.
     */
    void PresentationLayer_OnIntConfigurationPeriodMenuEnter(void);

    /**
     * @brief Wire the active-screen router (ACTION-029).
     *
     * Must be called before PresentationLayer_Update() is first invoked.
     * Typically called from DI_InitUISubsystem() after ScreenRouter_Init().
     *
     * @param[in] router  Fully initialised IScreenRouter_t (must not be NULL).
     */
    void PresentationLayer_SetRouter(IScreenRouter_t *router);

    /**
     * @brief Called by action_home_loaded() when HOME screen begins loading.
     *
     * Executes in LVGL thread context (inside lv_task_handler).
     * No lv_lock() needed.
     *
     * @note Safe to call before PresentationLayer_Init() — silently ignored.
     */
    void PresentationLayer_OnHomeEnter(void);

    /**
     * @brief Called by action_security_loaded() when SECURITY screen begins loading.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnSecurityEnter(void);

    /**
     * @brief Screen-agnostic periodic update — call from Control thread every 100 ms.
     *
     * Dispatches OnUpdate() to whichever screen is currently active according to
     * the router set by PresentationLayer_SetRouter(). If no router is wired,
     * falls back to updating the Home screen directly.
     *
     * View implementations handle lv_lock() internally.
     *
     * @note Safe to call before PresentationLayer_Init() — silently ignored.
     */
    void PresentationLayer_Update(void);

    /**
     * @brief Signal that all critical subsystems initialised successfully.
     *
     * Called from System_Init() just before returning ERR_OK.
     * If System_Init() returns early due to a subsystem error, this is never
     * called and IsSystemReady() returns false → Overview shows "System Error".
     *
     * @param[in] ready  true = all systems OK; false = initialisation failed.
     */
    void PresentationLayer_SetSystemReady(bool ready);

    /**
     * @brief Query the system-ready flag set by PresentationLayer_SetSystemReady().
     *
     * Consumed by ui_actions.c in action_overview_loaded() to choose the
     * status message shown on the Overview screen.
     *
     * @return true if System_Init() completed without errors.
     */
    bool PresentationLayer_IsSystemReady(void);

    /**
     * @brief Navigate to HOME via the router at the end of the boot sequence.
     *
     * Called by nav_to_home_cb() (lv_timer, LVGL thread) when the Overview
     * display time expires.  Going through the router ensures:
     *   - active_screen is updated to HomePresenter.
     *   - InputRouter is wired to HomePresenter (encoder/back enabled).
     *   - IScreen_OnExit(NULL) is called safely on the boot placeholder.
     *
     * @note Executes in LVGL thread — no lv_lock() needed.
     * @note Safe to call before PresentationLayer_Init() — silently ignored.
     */
    void PresentationLayer_OnBootComplete(void);

    /**
     * @brief Initialise the WiFi Module screen presenter.
     *
     * @param[in] deps  Fully populated list-nav deps bundle (5 items, back=SYSTEM_INFORMATION).
     * @return ERR_OK on success, ERR_NULL_POINTER if required fields are NULL.
     */
    Result_t PresentationLayer_InitWifiModule(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the WiFi Module presenter.
     */
    IScreen_t *PresentationLayer_GetWifiModuleScreen(void);

    /**
     * @brief Called by action_wifi_module_loaded() when WiFi Module screen loads.
     *
     * Executes in LVGL thread context. No lv_lock() needed.
     */
    void PresentationLayer_OnWifiModuleEnter(void);

    /**
     * @brief Initialise the Admin Configuration screen presenter (ListNavScreenPresenter).
     */
    Result_t PresentationLayer_InitAdminConfiguration(const ListNavScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Admin Configuration presenter.
     */
    IScreen_t *PresentationLayer_GetAdminConfigurationScreen(void);

    /**
     * @brief Called by action_admin_configuration_loaded() (LVGL thread).
     */
    void PresentationLayer_OnAdminConfigurationEnter(void);

    /**
     * @brief Initialise the Admin Operation Mode screen presenter.
     */
    Result_t PresentationLayer_InitAdminOperationMode(
        const AdminOperationModeScreenPresenterDeps_t *deps);

    /**
     * @brief Return the IScreen_t pointer for the Admin Operation Mode presenter.
     */
    IScreen_t *PresentationLayer_GetAdminOperationModeScreen(void);

    /**
     * @brief Called by action_admin_operation_mode_loaded() (LVGL thread).
     */
    void PresentationLayer_OnAdminOperationModeEnter(void);

#ifdef __cplusplus
}
#endif

#endif /* PRESENTATION_LAYER_H */
