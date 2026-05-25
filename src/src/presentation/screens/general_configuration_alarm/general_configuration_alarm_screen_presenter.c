/**
 * @file general_configuration_alarm_screen_presenter.c
 * @brief Presenter for the General Configuration Alarm selection screen.
 *
 * @note Intentionally does NOT include lvgl.h — all widget access is
 *       delegated to IGeneralConfigurationAlarmScreenView_t.
 *
 * ## Flow
 *   OnEnter  → load GeneralConfig_t → rebuild list (current value focused).
 *   OnSelect → load config → set buzzer_high_temp_alarm → save → show feedback → arm timer.
 *   OnUpdate → wait CONFIG_ALARM_SAVED_DISPLAY_MS → navigate back.
 *   OnBack   → navigate back without changes.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/general_configuration_alarm/general_configuration_alarm_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "common/general_types.h"
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — constant labels
 *============================================================================*/

#define ALARM_ITEMS_COUNT 2U

static const char *const s_alarm_labels[ALARM_ITEMS_COUNT] = {
    "Disable", /* id=0, buzzer_high_temp_alarm == 0 */
    "Enable",  /* id=1, buzzer_high_temp_alarm == 1 */
};

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(GeneralConfigurationAlarmScreenPresenter_t *self, uint8_t screen_id)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, screen_id);
    }
}

/*============================================================================*
 * PRIVATE — list item callback
 *============================================================================*/

/**
 * @brief Called by the EEZ view when the user selects a list item.
 *
 * @param[in] ctx      Presenter pointer (passed via AddListItem).
 * @param[in] item_id  0=Disable, 1=Enable.
 */
static void on_item_selected(void *ctx, uint8_t item_id)
{
    GeneralConfigurationAlarmScreenPresenter_t *self =
        (GeneralConfigurationAlarmScreenPresenter_t *)ctx;
    if (self == NULL)
    {
        return;
    }
    if (item_id >= ALARM_ITEMS_COUNT)
    {
        return; /* guard against stale callbacks */
    }

    /* Load current config → update buzzer_high_temp_alarm → save. */
    GeneralConfig_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    cfg.buzzer_high_temp_alarm = item_id;
    (void)ConfigStorage_SaveGeneralConfig(self->config_storage, &cfg);

    /* Cache the new value so OnEnter correctly re-focuses on re-entry. */
    self->alarm_enabled = item_id;

    /* Show feedback label. */
    IGeneralConfigurationAlarmScreenView_SetLabel(self->view, s_alarm_labels[item_id]);

    /* Arm auto-return timer. */
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
}

/*============================================================================*
 * PRIVATE — list rebuild
 *============================================================================*/

static void rebuild_list(GeneralConfigurationAlarmScreenPresenter_t *self, uint8_t current_value)
{
    IGeneralConfigurationAlarmScreenView_ClearList(self->view);

    for (uint8_t i = 0U; i < ALARM_ITEMS_COUNT; i++)
    {
        IGeneralConfigurationAlarmScreenView_AddListItem(
            self->view,
            i,
            s_alarm_labels[i],
            (i == current_value), /* focus if current selection */
            on_item_selected,
            self);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    GeneralConfigurationAlarmScreenPresenter_t *self =
        (GeneralConfigurationAlarmScreenPresenter_t *)base;

    /* Reset save feedback state. */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    /* Load current alarm setting. */
    GeneralConfig_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    const Result_t res = ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    self->alarm_enabled = (res == ERR_OK) ? cfg.buzzer_high_temp_alarm : 0U;
    /* Clamp to valid range (0 or 1). */
    if (self->alarm_enabled > 1U)
    {
        self->alarm_enabled = 0U;
    }

    /* Clear the feedback label. */
    IGeneralConfigurationAlarmScreenView_SetLabel(self->view, ">>Select alarm setting");

    /* Build and focus list. */
    rebuild_list(self, self->alarm_enabled);
}

static void on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    GeneralConfigurationAlarmScreenPresenter_t *self =
        (GeneralConfigurationAlarmScreenPresenter_t *)base;

    if (self->show_saved == 0U)
    {
        return;
    }

    const uint32_t elapsed = os_ticks_get() - self->saved_tick;
    if (elapsed >= CONFIG_ALARM_SAVED_DISPLAY_MS)
    {
        self->show_saved = 0U;
        navigate_to(self, self->back_screen_id);
    }
}

static void on_back_pressed(IScreen_t *base)
{
    GeneralConfigurationAlarmScreenPresenter_t *self =
        (GeneralConfigurationAlarmScreenPresenter_t *)base;
    navigate_to(self, self->back_screen_id);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t GeneralConfigurationAlarmScreenPresenter_Init(
    GeneralConfigurationAlarmScreenPresenter_t *self,
    const GeneralConfigurationAlarmScreenPresenterDeps_t *deps)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->config_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    (void)memset(self, 0, sizeof(*self));

    self->base.OnEnter = on_enter;
    self->base.OnExit = on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL;

    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}
