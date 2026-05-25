/**
 * @file wifi_configuration_screen_presenter.c
 * @brief WiFi Configuration screen presenter implementation.
 *
 * @note NO #include "lvgl.h" — intentional. All LVGL access goes through
 *       the injected IListNavScreenView_t implementation.
 *
 * @author Tecna Smart Lab
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/wifi_configuration/wifi_configuration_screen_presenter.h"
#include "presentation/screens/wifi_configuration/eez_wifi_configuration_screen_view.h"
#include "presentation/screens/configuration_input/configuration_input_screen_presenter.h"
#include "presentation/presentation_layer.h"
#include "presentation/interfaces/i_screen_router.h"
#include <string.h>

/* Screen IDs used for navigation in confirmation flow. */
#define SCREEN_ID_INFORMATION_SHOW ((uint8_t)24U)
#define SCREEN_ID_WIFI_CONFIGURATION ((uint8_t)27U)
#define SCREEN_ID_CONFIGURATION_INPUT ((uint8_t)14U)

/*============================================================================*
 * PRIVATE — forward declaration
 *============================================================================*/

static void on_item_selected(uint8_t item_idx, void *context);

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    WifiConfigurationScreenPresenter_t *self = (WifiConfigurationScreenPresenter_t *)base;

    if (self->wifi_module != NULL)
    {
        EezWifiConfigurationScreenView_SetWifiToggleState(true, WifiModule_IsEnabled(self->wifi_module));
    }
    else
    {
        EezWifiConfigurationScreenView_SetWifiToggleState(false, false);
    }

    IListNavScreenView_InitItems(self->view, on_item_selected, self);
    IListNavScreenView_SetFocusedItem(self->view, self->last_selected_idx);
}

static void on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    (void)base;
}

static void on_back_pressed(IScreen_t *base)
{
    WifiConfigurationScreenPresenter_t *self = (WifiConfigurationScreenPresenter_t *)base;
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    (void)base;
    (void)key;
    (void)event;
}

/*============================================================================*
 * PRIVATE — item-selected callback (called from LVGL thread)
 *============================================================================*/

/**
 * @brief Handle list item selection.
 *
 * item 0 — WiFi module toggle (Enable/Disable) via IWifiModule_t:
 *   1. Query current state with WifiModule_IsEnabled().
 *   2. If enabled -> WifiModule_Disable(), else WifiModule_Enable().
 *   3. Configure confirmation screen with resulting message.
 *   4. Navigate to INFORMATION_SHOW which auto-returns after ~1.5s.
 *
 * item 1 — WiFi auto-disable timeout:
 *   1. Set configuration input parameter to CONFIG_INPUT_PARAM_WIFI_TIMEOUT_MIN.
 *   2. Navigate to CONFIGURATION_INPUT screen.
 *   3. User edits value (0=always on, 1-60 min) and saves or cancels.
 *
 * item 2 — "More..." placeholder — no-op.
 */
static void on_item_selected(uint8_t item_idx, void *context)
{
    WifiConfigurationScreenPresenter_t *self = (WifiConfigurationScreenPresenter_t *)context;
    if (self == NULL)
    {
        return;
    }

    self->last_selected_idx = item_idx;

    if (item_idx == 0U)
    {
        /* WiFi module toggle */
        bool wifi_enabled = false;
        Result_t res = ERR_NULL_POINTER;

        if (self->wifi_module != NULL)
        {
            wifi_enabled = WifiModule_IsEnabled(self->wifi_module);
            if (wifi_enabled)
            {
                res = WifiModule_Disable(self->wifi_module);
                PresentationLayer_SetConfirmationMessage((res == ERR_OK) ? "WiFi disabled" : "WiFi disable failed");
            }
            else
            {
                res = WifiModule_Enable(self->wifi_module);
                PresentationLayer_SetConfirmationMessage((res == ERR_OK) ? "WiFi enabled" : "WiFi enable failed");
            }

            EezWifiConfigurationScreenView_SetWifiToggleState(true, WifiModule_IsEnabled(self->wifi_module));
            IListNavScreenView_InitItems(self->view, on_item_selected, self);
            IListNavScreenView_SetFocusedItem(self->view, self->last_selected_idx);
        }
        else
        {
            PresentationLayer_SetConfirmationMessage("WiFi module unavailable");
            EezWifiConfigurationScreenView_SetWifiToggleState(false, false);
            IListNavScreenView_InitItems(self->view, on_item_selected, self);
            IListNavScreenView_SetFocusedItem(self->view, self->last_selected_idx);
        }

        /* Configure confirmation item and back screen */
        PresentationLayer_SetPendingInfoItem(15U);                                  /* INFORMATION_SHOW_ITEM_CONFIRMATION */
        PresentationLayer_SetInfoBackScreen((uint8_t)SCREEN_ID_WIFI_CONFIGURATION); /* Return to WiFi config, not Settings */

        /* Navigate to confirmation screen (auto-returns after ~1.5s) */
        if (self->router != NULL)
        {
            (void)IScreenRouter_NavigateTo(self->router, SCREEN_ID_INFORMATION_SHOW);
        }
    }
    else if (item_idx == 1U)
    {
        /* WiFi timeout configuration — navigate to configuration_input screen */
        PresentationLayer_SetConfigInputContext(
            CONFIG_INPUT_PARAM_WIFI_TIMEOUT_MIN, 0U,
            (uint8_t)SCREEN_ID_WIFI_CONFIGURATION);

        if (self->router != NULL)
        {
            (void)IScreenRouter_NavigateTo(self->router, SCREEN_ID_CONFIGURATION_INPUT);
        }
    }
    /* item 2 ("More...") — no-op */
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t WifiConfigurationScreenPresenter_Init(
    WifiConfigurationScreenPresenter_t *self,
    const WifiConfigurationScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(WifiConfigurationScreenPresenter_t));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed; /* real function, NOT NULL */
    self->base.OnKeyEvent = on_key_event;

    self->view = deps->view;
    self->router = deps->router;
    self->wifi_module = deps->wifi_module;
    self->back_screen_id = deps->back_screen_id;

    return ERR_OK;
}
