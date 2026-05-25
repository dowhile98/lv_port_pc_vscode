/**
 * @file int_configuration_days_screen_presenter.c
 * @brief Weekday-mask editor presenter for the IntConfigurationDays screen.
 *
 * Behaviour summary
 * -----------------
 * - **OnEnter**   : Load RelayConfig_t from storage → copy weekday_mask →
 *                   refresh all 7 checkboxes via the view interface. LVGL
 *                   encoder group (wired in ui_actions.c) handles navigation.
 * - **OnDayClicked(day_idx, checked)** : Update working_mask bit, save entire
 *                   RelayConfig_t, show ">> Saved!" for
 *                   @c INT_CONFIG_DAYS_SAVED_MS ms, then auto-return.  Ignored
 *                   while show_saved == 1 (ghost-click prevention).
 * - **OnUpdate**  : Drives the auto-return timer when show_saved == 1.
 * - **OnBackPressed**: Discard and navigate back immediately.
 *
 * @note NO #include "lvgl.h" — intentional.  LVGL access goes through view.
 * @note Static allocation only — no malloc/free.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/int_configuration_days/int_configuration_days_screen_presenter.h"
#include "presentation/interfaces/i_int_configuration_days_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "i_modular_config_storage.h"
#include "common/relay_types.h"
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — constants
 *============================================================================*/

/** @brief Prompt shown on screen entry. */
#define DAYS_PROMPT_TEXT ">>Days interrupt?"

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

/**
 * @brief Navigate back to the configured back_screen_id (if router is set).
 *
 * @param[in] self  Presenter instance.
 */
static void navigate_back(IntConfigurationDaysScreenPresenter_t *self)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

/**
 * @brief Refresh all 7 day checkboxes from working_mask.
 *
 * @param[in] self  Presenter instance.
 */
static void refresh_all_checkboxes(IntConfigurationDaysScreenPresenter_t *self)
{
    for (uint8_t i = 0U; i < INT_CONFIG_DAYS_COUNT; i++)
    {
        const bool checked = ((self->working_mask >> i) & 0x01U) != 0U;
        IIntConfigurationDaysScreenView_SetDayChecked(self->view, i, checked);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

/**
 * @brief Load the weekday_mask from storage and initialise the view.
 *
 * On storage-load failure, working_mask defaults to 0x00 (all disabled) and
 * the view is still fully initialised — safe-fail.
 *
 * @param[in] base  First field of IntConfigurationDaysScreenPresenter_t.
 */
static void on_enter(IScreen_t *base)
{
    IntConfigurationDaysScreenPresenter_t *self =
        (IntConfigurationDaysScreenPresenter_t *)base;

    /* Clear saved-state to ensure clicks work on re-entry */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    /* Load relay config and extract the weekday_mask */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));

    const Result_t res = ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    if (res == ERR_OK)
    {
        self->working_mask = relay_cfg.time_window.weekday_mask;
    }
    else
    {
        self->working_mask = 0U; /* safe-fail: all days disabled */
    }

    /* Sync all checkbox states */
    refresh_all_checkboxes(self);

    /* Show prompt */
    IIntConfigurationDaysScreenView_SetLabel(self->view, DAYS_PROMPT_TEXT);
}

/**
 * @brief Screen exit — nothing to clean up.
 *
 * @param[in] base  First field of IntConfigurationDaysScreenPresenter_t.
 */
static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

/**
 * @brief Periodic update — drives the "Saved" auto-return timer.
 *
 * @param[in] base  First field of IntConfigurationDaysScreenPresenter_t.
 */
static void on_update(IScreen_t *base)
{
    IntConfigurationDaysScreenPresenter_t *self =
        (IntConfigurationDaysScreenPresenter_t *)base;

    if ((self->show_saved != 0U) &&
        ((os_ticks_get() - self->saved_tick) >= INT_CONFIG_DAYS_SAVED_MS))
    {
        self->show_saved = 0U;
        IIntConfigurationDaysScreenView_SetLabel(self->view, DAYS_PROMPT_TEXT);
    }
}

/**
 * @brief Discard working_mask and navigate back without saving.
 *
 * @param[in] base  First field of IntConfigurationDaysScreenPresenter_t.
 */
static void on_back_pressed(IScreen_t *base)
{
    IntConfigurationDaysScreenPresenter_t *self =
        (IntConfigurationDaysScreenPresenter_t *)base;
    navigate_back(self);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

/**
 * @copydoc IntConfigurationDaysScreenPresenter_Init
 */
Result_t IntConfigurationDaysScreenPresenter_Init(
    IntConfigurationDaysScreenPresenter_t *self,
    const IntConfigurationDaysScreenPresenterDeps_t *deps)
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

    /* Wire IScreen_t vtable — no OnKeyEvent: encoder group handles it */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = NULL;

    /* Inject dependencies */
    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}

/**
 * @copydoc IntConfigurationDaysScreenPresenter_OnDayClicked
 */
void IntConfigurationDaysScreenPresenter_OnDayClicked(
    IntConfigurationDaysScreenPresenter_t *self,
    uint8_t day_idx,
    bool checked)
{
    if (self == NULL)
    {
        return;
    }

    /* Ignore clicks while "Saved" banner is still visible (ghost-click prevention) */
    if (self->show_saved != 0U)
    {
        return;
    }

    /* Clamp to valid range */
    if (day_idx >= INT_CONFIG_DAYS_COUNT)
    {
        return;
    }

    /* Update working_mask to reflect the new checkbox state */
    if (checked)
    {
        self->working_mask |= (uint8_t)(1U << day_idx);
    }
    else
    {
        self->working_mask &= (uint8_t)(~(1U << day_idx));
    }

    /* Persist: load full RelayConfig_t to preserve all other fields, patch mask, save */
    RelayConfig_t relay_cfg;
    (void)memset(&relay_cfg, 0, sizeof(relay_cfg));
    /*get relay config to preserve all other fields, then patch weekday_mask and save*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &relay_cfg);
    /*update weekday_mask and save*/
    relay_cfg.time_window.weekday_mask = self->working_mask;
    /*save full relay config with updated weekday_mask*/
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &relay_cfg);

    /* Show confirmation and start auto-return timer */
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
    IIntConfigurationDaysScreenView_SetLabel(self->view, ">> Saved!");
}
