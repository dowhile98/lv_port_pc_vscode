/**
 * @file presentation_layer.c
 * @brief Presentation layer DI shim — owns the static Presenter instances.
 *
 * Bridges:
 *   DI container (infrastructure)  →  HomeScreenPresenter (presentation)
 *   ui_actions.c (LVGL callbacks)  →  IScreen_t lifecycle events
 *   Control AO thread              →  IScreen_OnUpdate(active screen)
 *
 * ## Threading
 * - PresentationLayer_Init()        — called from DI context (init thread).
 * - PresentationLayer_SetRouter()   — called from DI context after router init.
 * - PresentationLayer_OnHomeEnter() — called from LVGL thread (action callback).
 * - PresentationLayer_Update()      — called from Control AO thread every 100 ms.
 *
 * @note Zero LVGL includes — fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/presentation_layer.h"
#include "src/presentation/ui/screens.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/screens/int_configuration_state/int_configuration_state_screen_presenter.h"
#include "presentation/screens/int_configuration_start/int_configuration_start_screen_presenter.h"
#include "presentation/screens/int_configuration_predefined/int_configuration_predefined_screen_presenter.h"
#include "presentation/screens/gps_configuration_antenna/gps_configuration_antenna_screen_presenter.h"
#include "presentation/screens/access_denied/access_denied_screen_presenter.h"
#include "presentation/screens/configuration_input/configuration_input_screen_presenter.h"
#include "presentation/screens/general_configuration_alarm/general_configuration_alarm_screen_presenter.h"
#include "presentation/screens/int_configuration_days/int_configuration_days_screen_presenter.h"
#include "presentation/screens/int_configuration_period/int_configuration_period_screen_presenter.h"
#include "presentation/screens/int_configuration_period_menu/int_configuration_period_menu_screen_presenter.h"
#include "presentation/screens/admin_operation_mode/admin_operation_mode_screen_presenter.h"
#include "src/presentation/ui/screens.h" /* SCREEN_ID_* (EEZ-generated) */
#include <stdbool.h>
#include <string.h>
/*============================================================================*
 * PRIVATE — static presenter instances and router (DI-owned)
 *============================================================================*/

static HomeScreenPresenter_t s_home_presenter;
static SecurityScreenPresenter_t s_security_presenter;
static ListNavScreenPresenter_t s_menu_presenter;
static ListNavScreenPresenter_t s_settings_presenter;
static ListNavScreenPresenter_t s_system_information_presenter;
static InformationShowScreenPresenter_t s_information_show_presenter;
static QrInfoScreenPresenter_t s_qr_info_presenter;
static ListNavScreenPresenter_t s_int_config_presenter;
static IntConfigurationStateScreenPresenter_t s_int_config_state_presenter;
static IntConfigurationStartScreenPresenter_t s_int_config_start_presenter;
static IntConfigurationPredefinedScreenPresenter_t s_int_config_predefined_presenter;
static ListNavScreenPresenter_t s_gps_config_presenter;
static GpsConfigurationAntennaScreenPresenter_t s_gps_config_antenna_presenter;
static ListNavScreenPresenter_t s_contact_config_presenter;
static ContactConfigurationTypeScreenPresenter_t s_contact_config_type_presenter;
static GeneralConfigurationAlarmScreenPresenter_t s_general_config_alarm_presenter;
static ListNavScreenPresenter_t s_general_config_presenter;
static AccessDeniedScreenPresenter_t s_access_denied_presenter;
static ConfigurationInputScreenPresenter_t s_config_input_presenter;
static IntConfigurationDaysScreenPresenter_t s_int_config_days_presenter;
static IntConfigurationPeriodScreenPresenter_t s_int_config_period_presenter;
static IntConfigurationPeriodMenuScreenPresenter_t s_int_config_period_menu_presenter;
static ListNavScreenPresenter_t s_wifi_module_presenter;
static WifiConfigurationScreenPresenter_t s_wifi_configuration_presenter;
static ListNavScreenPresenter_t s_admin_config_presenter;
static AdminOperationModeScreenPresenter_t s_admin_operation_mode_presenter;
static IScreenRouter_t *s_router = NULL;
static bool s_initialized = false;
static bool s_security_initialized = false;
static bool s_menu_initialized = false;
static bool s_settings_initialized = false;
static bool s_system_information_initialized = false;
static bool s_information_show_initialized = false;
static uint8_t s_pending_info_item = 0U;
static uint8_t s_pending_info_back_screen_id = (uint8_t)SCREEN_ID_SYSTEM_INFORMATION;
static char s_confirmation_msg[64] = {0};
static bool s_qr_info_initialized = false;
static uint8_t s_pending_qr_item = 0U;
static uint8_t s_qr_back_screen_id = 0U;
static bool s_int_config_initialized = false;
static bool s_int_config_state_initialized = false;
static bool s_int_config_start_initialized = false;
static bool s_int_config_predefined_initialized = false;
static bool s_gps_config_initialized = false;
static bool s_gps_config_antenna_initialized = false;
static bool s_contact_config_initialized = false;
static bool s_contact_config_type_initialized = false;
static bool s_general_config_alarm_initialized = false;
static bool s_general_config_initialized = false;
static bool s_access_denied_initialized = false;
static bool s_config_input_initialized = false;
static bool s_int_config_days_initialized = false;
static bool s_int_config_period_initialized = false;
static bool s_int_config_period_menu_initialized = false;
static bool s_wifi_module_initialized = false;
static bool s_wifi_configuration_initialized = false;
static bool s_admin_config_initialized = false;
static bool s_admin_operation_mode_initialized = false;
static bool s_system_ready = false;

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t PresentationLayer_Init(const HomeScreenPresenterDeps_t *deps)
{
    Result_t res = HomeScreenPresenter_Init(&s_home_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }

    s_initialized = true;
    return ERR_OK;
}

Result_t PresentationLayer_InitSecurity(const SecurityScreenPresenterDeps_t *deps)
{
    Result_t res = SecurityScreenPresenter_Init(&s_security_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_security_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetSecurityScreen(void)
{
    return (IScreen_t *)&s_security_presenter;
}

IScreen_t *PresentationLayer_GetHomeScreen(void)
{
    return (IScreen_t *)&s_home_presenter;
}

Result_t PresentationLayer_InitMenu(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_menu_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_menu_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetMenuScreen(void)
{
    return (IScreen_t *)&s_menu_presenter;
}

void PresentationLayer_OnMenuEnter(void)
{
    if (!s_menu_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_menu_presenter);
}

Result_t PresentationLayer_InitSettings(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_settings_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_settings_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetSettingsScreen(void)
{
    return (IScreen_t *)&s_settings_presenter;
}

void PresentationLayer_OnSettingsEnter(void)
{
    if (!s_settings_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_settings_presenter);
}

Result_t PresentationLayer_InitSystemInformation(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_system_information_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_system_information_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetSystemInformationScreen(void)
{
    return (IScreen_t *)&s_system_information_presenter;
}

void PresentationLayer_OnSystemInformationEnter(void)
{
    if (!s_system_information_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_system_information_presenter);
}

void PresentationLayer_SetPendingInfoItem(uint8_t item_idx)
{
    s_pending_info_item = item_idx;
}

const uint8_t *PresentationLayer_GetPendingInfoItemPtr(void)
{
    return &s_pending_info_item;
}

void PresentationLayer_SetInfoBackScreen(uint8_t screen_id)
{
    s_pending_info_back_screen_id = screen_id;
}

void PresentationLayer_SetConfirmationMessage(const char *msg)
{
    if (msg == NULL)
    {
        return;
    }
    (void)strncpy(s_confirmation_msg, msg, sizeof(s_confirmation_msg) - 1U);
    s_confirmation_msg[sizeof(s_confirmation_msg) - 1U] = '\0';
}

void PresentationLayer_SetSecuritySuccessAction(void (*fn)(void *ctx), void *ctx)
{
    s_security_presenter.on_success_action = fn;
    s_security_presenter.on_success_ctx = ctx;
}

void PresentationLayer_SetSecuritySuccessScreenId(uint8_t screen_id)
{
    s_security_presenter.success_screen_id = screen_id;
}

void PresentationLayer_SetSecurityCancelScreenId(uint8_t screen_id)
{
    s_security_presenter.cancel_screen_id = screen_id;
}

void PresentationLayer_SetSecurityContext(const uint8_t *password,
                                          uint8_t password_len,
                                          uint8_t success_screen_id,
                                          uint8_t cancel_screen_id)
{
    SecurityScreenPresenter_SetContext(&s_security_presenter,
                                       password,
                                       password_len,
                                       success_screen_id,
                                       cancel_screen_id);
}

void PresentationLayer_ResetSecurityToDefault(void)
{
    /* Default password: UP DOWN DOWN UP (4 keys) */
    static const uint8_t default_pw[] = {1U, 2U, 2U, 1U}; /* SEC_KEY_UP=1, SEC_KEY_DOWN=2 */
    SecurityScreenPresenter_SetContext(&s_security_presenter, default_pw, 4U,
                                       (uint8_t)SCREEN_ID_MENU,
                                       (uint8_t)SCREEN_ID_HOME);
}

const char *PresentationLayer_GetConfirmationMessagePtr(void)
{
    return s_confirmation_msg;
}

const uint8_t *PresentationLayer_GetInfoBackScreenPtr(void)
{
    return &s_pending_info_back_screen_id;
}

Result_t PresentationLayer_InitInformationShow(const InformationShowScreenPresenterDeps_t *deps)
{
    Result_t res = InformationShowScreenPresenter_Init(&s_information_show_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_information_show_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetInformationShowScreen(void)
{
    return (IScreen_t *)&s_information_show_presenter;
}

void PresentationLayer_OnInformationShowEnter(void)
{
    if (!s_information_show_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_information_show_presenter);
}

void PresentationLayer_OnInformationShowUpdate(void)
{
    if (!s_information_show_initialized)
    {
        return;
    }
    IScreen_OnUpdate((IScreen_t *)&s_information_show_presenter);
}

/* ── QR Info ─────────────────────────────────────────────────────────────── */

void PresentationLayer_SetPendingQrItem(uint8_t item_idx)
{
    s_pending_qr_item = item_idx;
}

const uint8_t *PresentationLayer_GetPendingQrItemPtr(void)
{
    return &s_pending_qr_item;
}

void PresentationLayer_SetQrBackScreen(uint8_t screen_id)
{
    s_qr_back_screen_id = screen_id;
}

const uint8_t *PresentationLayer_GetQrBackScreenPtr(void)
{
    return &s_qr_back_screen_id;
}

Result_t PresentationLayer_InitQrInfo(const QrInfoScreenPresenterDeps_t *deps)
{
    Result_t res = QrInfoScreenPresenter_Init(&s_qr_info_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_qr_info_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetQrInfoScreen(void)
{
    return (IScreen_t *)&s_qr_info_presenter;
}

void PresentationLayer_OnQrInfoEnter(void)
{
    if (!s_qr_info_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_qr_info_presenter);
}

Result_t PresentationLayer_InitIntConfiguration(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_int_config_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetIntConfigurationScreen(void)
{
    return (IScreen_t *)&s_int_config_presenter;
}

void PresentationLayer_OnIntConfigurationEnter(void)
{
    if (!s_int_config_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_presenter);
}

Result_t PresentationLayer_InitIntConfigurationState(
    const IntConfigurationStateScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationStateScreenPresenter_Init(
        &s_int_config_state_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_state_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetIntConfigurationStateScreen(void)
{
    return (IScreen_t *)&s_int_config_state_presenter;
}

void PresentationLayer_OnIntConfigurationStateEnter(void)
{
    if (!s_int_config_state_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_state_presenter);
}

void PresentationLayer_OnIntConfigurationStateToggle(bool new_state)
{
    if (!s_int_config_state_initialized)
    {
        return;
    }
    IntConfigurationStateScreenPresenter_OnToggle(
        &s_int_config_state_presenter, new_state);
}

Result_t PresentationLayer_InitIntConfigurationStart(
    const IntConfigurationStartScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationStartScreenPresenter_Init(
        &s_int_config_start_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_start_initialized = true;
    return ERR_OK;
}

void PresentationLayer_OnIntConfigurationStartEnter(void)
{
    if (!s_int_config_start_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_start_presenter);
}

void PresentationLayer_OnIntConfigurationStartToggle(bool start_with_on)
{
    if (!s_int_config_start_initialized)
    {
        return;
    }
    IntConfigurationStartScreenPresenter_OnToggle(
        &s_int_config_start_presenter, start_with_on);
}

IScreen_t *PresentationLayer_GetIntConfigurationStartScreen(void)
{
    return (IScreen_t *)&s_int_config_start_presenter;
}

Result_t PresentationLayer_InitIntConfigurationPredefined(
    const IntConfigurationPredefinedScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationPredefinedScreenPresenter_Init(
        &s_int_config_predefined_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_predefined_initialized = true;
    return ERR_OK;
}

void PresentationLayer_OnIntConfigurationPredefinedEnter(void)
{
    if (!s_int_config_predefined_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_predefined_presenter);
}

IScreen_t *PresentationLayer_GetIntConfigurationPredefinedScreen(void)
{
    return (IScreen_t *)&s_int_config_predefined_presenter;
}

Result_t PresentationLayer_InitGpsConfiguration(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_gps_config_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_gps_config_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetGpsConfigurationScreen(void)
{
    return (IScreen_t *)&s_gps_config_presenter;
}

void PresentationLayer_OnGpsConfigurationEnter(void)
{
    if (!s_gps_config_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_gps_config_presenter);
}

Result_t PresentationLayer_InitGpsConfigurationAntenna(
    const GpsConfigurationAntennaScreenPresenterDeps_t *deps)
{
    Result_t res = GpsConfigurationAntennaScreenPresenter_Init(&s_gps_config_antenna_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_gps_config_antenna_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetGpsConfigurationAntennaScreen(void)
{
    return (IScreen_t *)&s_gps_config_antenna_presenter;
}

void PresentationLayer_OnGpsConfigurationAntennaEnter(void)
{
    if (!s_gps_config_antenna_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_gps_config_antenna_presenter);
}

Result_t PresentationLayer_InitContactConfiguration(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_contact_config_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_contact_config_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetContactConfigurationScreen(void)
{
    return (IScreen_t *)&s_contact_config_presenter;
}

void PresentationLayer_OnContactConfigurationEnter(void)
{
    if (!s_contact_config_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_contact_config_presenter);
}

Result_t PresentationLayer_InitContactConfigurationType(
    const ContactConfigurationTypeScreenPresenterDeps_t *deps)
{
    Result_t res = ContactConfigurationTypeScreenPresenter_Init(&s_contact_config_type_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_contact_config_type_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetContactConfigurationTypeScreen(void)
{
    return (IScreen_t *)&s_contact_config_type_presenter;
}

void PresentationLayer_OnContactConfigurationTypeEnter(void)
{
    if (!s_contact_config_type_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_contact_config_type_presenter);
}

Result_t PresentationLayer_InitGeneralConfigurationAlarm(
    const GeneralConfigurationAlarmScreenPresenterDeps_t *deps)
{
    Result_t res = GeneralConfigurationAlarmScreenPresenter_Init(&s_general_config_alarm_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_general_config_alarm_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetGeneralConfigurationAlarmScreen(void)
{
    return (IScreen_t *)&s_general_config_alarm_presenter;
}

void PresentationLayer_OnGeneralConfigurationAlarmEnter(void)
{
    if (!s_general_config_alarm_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_general_config_alarm_presenter);
}

Result_t PresentationLayer_InitGeneralConfiguration(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_general_config_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_general_config_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetGeneralConfigurationScreen(void)
{
    return (IScreen_t *)&s_general_config_presenter;
}

void PresentationLayer_OnGeneralConfigurationEnter(void)
{
    if (!s_general_config_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_general_config_presenter);
}

Result_t PresentationLayer_InitAccessDenied(const AccessDeniedScreenPresenterDeps_t *deps)
{
    Result_t res = AccessDeniedScreenPresenter_Init(&s_access_denied_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_access_denied_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetAccessDeniedScreen(void)
{
    return (IScreen_t *)&s_access_denied_presenter;
}

void PresentationLayer_OnAccessDeniedEnter(void)
{
    if (!s_access_denied_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_access_denied_presenter);
}

void PresentationLayer_SetAccessDeniedReturnScreen(uint8_t screen_id)
{
    s_access_denied_presenter.return_screen_id = screen_id;
}

void PresentationLayer_SetAccessDeniedMessage(const char *msg)
{
    s_access_denied_presenter.custom_message = msg;
}

Result_t PresentationLayer_InitConfigurationInput(
    const ConfigurationInputScreenPresenterDeps_t *deps)
{
    Result_t res = ConfigurationInputScreenPresenter_Init(&s_config_input_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_config_input_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetConfigurationInputScreen(void)
{
    return (IScreen_t *)&s_config_input_presenter;
}

void PresentationLayer_OnConfigurationInputEnter(void)
{
    if (!s_config_input_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_config_input_presenter);
}

void PresentationLayer_SetConfigInputParam(ConfigInputParam_t param)
{
    ConfigurationInputScreenPresenter_SetParam(&s_config_input_presenter, param);
}

void PresentationLayer_SetConfigInputContext(
    ConfigInputParam_t param, uint8_t cycle_idx, uint8_t back_screen_id)
{
    ConfigurationInputScreenPresenter_SetContext(
        &s_config_input_presenter, param, cycle_idx, back_screen_id);
}

Result_t PresentationLayer_InitIntConfigurationDays(
    const IntConfigurationDaysScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationDaysScreenPresenter_Init(&s_int_config_days_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_days_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetIntConfigurationDaysScreen(void)
{
    return (IScreen_t *)&s_int_config_days_presenter;
}

void PresentationLayer_OnIntConfigurationDaysEnter(void)
{
    if (!s_int_config_days_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_days_presenter);
}

void PresentationLayer_OnIntConfigurationDayClicked(uint8_t day_idx, bool checked)
{
    if (!s_int_config_days_initialized)
    {
        return;
    }
    IntConfigurationDaysScreenPresenter_OnDayClicked(
        &s_int_config_days_presenter, day_idx, checked);
}

Result_t PresentationLayer_InitIntConfigurationPeriod(
    const IntConfigurationPeriodScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationPeriodScreenPresenter_Init(
        &s_int_config_period_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_period_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetIntConfigurationPeriodScreen(void)
{
    return (IScreen_t *)&s_int_config_period_presenter;
}

void PresentationLayer_OnIntConfigurationPeriodEnter(void)
{
    if (!s_int_config_period_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_period_presenter);
}

/**
 * @brief Translate PeriodEditParam_t (0-2) → ConfigInputParam_t (2-4) and
 *        call SetContext on the config-input presenter.
 *
 * Registered as set_edit_context_fn in IntConfigurationPeriodMenuDeps.
 * Called from period-menu presenter when the user selects an edit item.
 * Executes in LVGL thread — no concurrency guard required.
 */
static void period_edit_context_fn(void *ctx, uint8_t param, uint8_t cycle_idx)
{
    (void)ctx;
    /* PeriodEditParam_t (0,1,2) → ConfigInputParam_t (2,3,4) */
    static const ConfigInputParam_t k_param_map[] = {
        CONFIG_INPUT_PARAM_ON_TIME,  /* PERIOD_EDIT_ON_TIME  = 0 */
        CONFIG_INPUT_PARAM_OFF_TIME, /* PERIOD_EDIT_OFF_TIME = 1 */
        CONFIG_INPUT_PARAM_END_DATE, /* PERIOD_EDIT_END_DATE = 2 */
    };
    const ConfigInputParam_t p = (param < 3U) ? k_param_map[param]
                                              : CONFIG_INPUT_PARAM_ON_TIME;
    ConfigurationInputScreenPresenter_SetContext(
        &s_config_input_presenter, p, cycle_idx,
        (uint8_t)SCREEN_ID_INT_CONFIGURATION_PERIOD_MENU);
}

void (*PresentationLayer_GetPeriodEditContextFn(void))(void *, uint8_t, uint8_t)
{
    return period_edit_context_fn;
}

void *PresentationLayer_GetPeriodEditContextCtx(void)
{
    return NULL;
}

/**
 * @brief Forwards SetMode to the static period-menu presenter instance.
 *
 * Registered as set_period_menu_mode_fn in IntConfigurationPeriodScreenPresenterDeps.
 * Called by the period presenter before navigating to the menu, publishing
 * the chosen mode so the menu never needs a storage read on entry.
 */
static void period_menu_set_mode_fn(void *ctx, uint8_t mode)
{
    IntConfigurationPeriodMenuScreenPresenter_t *menu =
        (IntConfigurationPeriodMenuScreenPresenter_t *)ctx;
    IntConfigurationPeriodMenuScreenPresenter_SetMode(menu, mode);
}

void (*PresentationLayer_GetPeriodMenuSetModeFn(void))(void *, uint8_t)
{
    return period_menu_set_mode_fn;
}

void *PresentationLayer_GetPeriodMenuSetModeCtx(void)
{
    return &s_int_config_period_menu_presenter;
}

Result_t PresentationLayer_InitIntConfigurationPeriodMenu(
    const IntConfigurationPeriodMenuScreenPresenterDeps_t *deps)
{
    Result_t res = IntConfigurationPeriodMenuScreenPresenter_Init(
        &s_int_config_period_menu_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_int_config_period_menu_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetIntConfigurationPeriodMenuScreen(void)
{
    return (IScreen_t *)&s_int_config_period_menu_presenter;
}

void PresentationLayer_OnIntConfigurationPeriodMenuEnter(void)
{
    if (!s_int_config_period_menu_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_int_config_period_menu_presenter);
}

void PresentationLayer_SetRouter(IScreenRouter_t *router)
{
    s_router = router;
}

void PresentationLayer_OnHomeEnter(void)
{
    if (!s_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_home_presenter);
}

void PresentationLayer_OnSecurityEnter(void)
{
    if (!s_security_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_security_presenter);
}

void PresentationLayer_Update(void)
{
    if (!s_initialized)
    {
        return;
    }

    if (s_router != NULL)
    {
        /* Screen-agnostic: dispatch to whichever screen is currently active */
        IScreen_t *active = IScreenRouter_GetActiveScreen(s_router);
        IScreen_OnUpdate(active);
    }
    else
    {
        /* Fallback (no router wired yet): update Home directly */
        IScreen_OnUpdate((IScreen_t *)&s_home_presenter);
    }
}

void PresentationLayer_SetSystemReady(bool ready)
{
    s_system_ready = ready;
}

bool PresentationLayer_IsSystemReady(void)
{
    return s_system_ready;
}

void PresentationLayer_OnBootComplete(void)
{
    if (!s_initialized || s_router == NULL)
    {
        return;
    }
    /* Navigate through the router so active_screen and InputRouter are updated. */
    (void)IScreenRouter_NavigateTo(s_router, (uint8_t)SCREEN_ID_HOME);
}

/* ── WiFi Module screen presenter ─────────────────────────────────────── */

Result_t PresentationLayer_InitWifiModule(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_wifi_module_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_wifi_module_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetWifiModuleScreen(void)
{
    return (IScreen_t *)&s_wifi_module_presenter;
}

void PresentationLayer_OnWifiModuleEnter(void)
{
    if (!s_wifi_module_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_wifi_module_presenter);
}

/* ── WiFi Configuration screen presenter ─────────────────────────────── */

Result_t PresentationLayer_InitWifiConfiguration(const WifiConfigurationScreenPresenterDeps_t *deps)
{
    Result_t res = WifiConfigurationScreenPresenter_Init(&s_wifi_configuration_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_wifi_configuration_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetWifiConfigurationScreen(void)
{
    return (IScreen_t *)&s_wifi_configuration_presenter;
}

void PresentationLayer_OnWifiConfigurationEnter(void)
{
    if (!s_wifi_configuration_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_wifi_configuration_presenter);
}

/* ── Admin Configuration screen presenter ────────────────────────────── */

Result_t PresentationLayer_InitAdminConfiguration(const ListNavScreenPresenterDeps_t *deps)
{
    Result_t res = ListNavScreenPresenter_Init(&s_admin_config_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_admin_config_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetAdminConfigurationScreen(void)
{
    return (IScreen_t *)&s_admin_config_presenter;
}

void PresentationLayer_OnAdminConfigurationEnter(void)
{
    if (!s_admin_config_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_admin_config_presenter);
}

/* ── Admin Operation Mode screen presenter ───────────────────────────── */

Result_t PresentationLayer_InitAdminOperationMode(
    const AdminOperationModeScreenPresenterDeps_t *deps)
{
    Result_t res = AdminOperationModeScreenPresenter_Init(&s_admin_operation_mode_presenter, deps);
    if (res != ERR_OK)
    {
        return res;
    }
    s_admin_operation_mode_initialized = true;
    return ERR_OK;
}

IScreen_t *PresentationLayer_GetAdminOperationModeScreen(void)
{
    return (IScreen_t *)&s_admin_operation_mode_presenter;
}

void PresentationLayer_OnAdminOperationModeEnter(void)
{
    if (!s_admin_operation_mode_initialized)
    {
        return;
    }
    IScreen_OnEnter((IScreen_t *)&s_admin_operation_mode_presenter);
}
