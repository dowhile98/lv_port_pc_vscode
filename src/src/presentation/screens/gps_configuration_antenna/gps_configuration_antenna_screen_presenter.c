/**
 * @file gps_configuration_antenna_screen_presenter.c
 * @brief Presenter for the GPS Configuration Antenna selection screen.
 *
 * @note Intentionally does NOT include lvgl.h — all widget access is
 *       delegated to IGpsConfigurationAntennaScreenView_t.
 *
 * ## Flow
 *   OnEnter  → load GPSConfig_t → rebuild list (current antenna focused).
 *   OnSelect → load config → set antenna_type → save → show feedback → arm timer.
 *   OnUpdate → wait CONFIG_ANTENNA_SAVED_DISPLAY_MS → navigate back.
 *   OnBack   → navigate back without changes.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/gps_configuration_antenna/gps_configuration_antenna_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "common/gps_types.h"
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — constant labels
 *============================================================================*/

#define ANTENNA_ITEMS_COUNT 2U

static const char *const s_antenna_labels[ANTENNA_ITEMS_COUNT] = {
    "Internal antenna", /* id=0, GPSConfig_t.antenna_type == 0 */
    "External antenna", /* id=1, GPSConfig_t.antenna_type == 1 */
};

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(GpsConfigurationAntennaScreenPresenter_t *self, uint8_t screen_id)
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
 * @param[in] item_id  AntennaSelectItemId_t: 0=Internal, 1=External.
 */
static void on_item_selected(void *ctx, uint8_t item_id)
{
    GpsConfigurationAntennaScreenPresenter_t *self =
        (GpsConfigurationAntennaScreenPresenter_t *)ctx;
    if (self == NULL)
    {
        return;
    }
    if (item_id >= ANTENNA_ITEMS_COUNT)
    {
        return; /* guard against stale callbacks */
    }

    /* Load current config → update antenna_type → save. */
    GPSConfig_t gps_cfg;
    (void)memset(&gps_cfg, 0, sizeof(gps_cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &gps_cfg);
    gps_cfg.antenna_type = item_id;
    (void)ConfigStorage_SaveGPSConfig(self->config_storage, &gps_cfg);

    /* Cache the new value so OnEnter correctly re-focuses on re-entry. */
    self->antenna_type = item_id;

    /* Show feedback label. */
    const char *name = s_antenna_labels[item_id];
    IGpsConfigurationAntennaScreenView_SetLabel(self->view, name);

    /* Arm auto-return timer. */
    self->show_saved = 1U;
    self->saved_tick = os_ticks_get();
}

/*============================================================================*
 * PRIVATE — list rebuild
 *============================================================================*/

static void rebuild_list(GpsConfigurationAntennaScreenPresenter_t *self, uint8_t current_type)
{
    IGpsConfigurationAntennaScreenView_ClearList(self->view);

    for (uint8_t i = 0U; i < ANTENNA_ITEMS_COUNT; i++)
    {
        IGpsConfigurationAntennaScreenView_AddListItem(
            self->view,
            i,
            s_antenna_labels[i],
            (i == current_type), /* focus if current selection */
            on_item_selected,
            self);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    GpsConfigurationAntennaScreenPresenter_t *self =
        (GpsConfigurationAntennaScreenPresenter_t *)base;

    /* Reset save feedback state. */
    self->show_saved = 0U;
    self->saved_tick = 0U;

    /* Load current antenna type. */
    GPSConfig_t gps_cfg;
    (void)memset(&gps_cfg, 0, sizeof(gps_cfg));
    const Result_t res = ConfigStorage_LoadGPSConfig(self->config_storage, &gps_cfg);
    self->antenna_type = (res == ERR_OK) ? gps_cfg.antenna_type : 0U;

    /* Clear the feedback label. */
    IGpsConfigurationAntennaScreenView_SetLabel(self->view, ">>Select the antenna to use");

    /* Build and focus list. */
    rebuild_list(self, self->antenna_type);
}

static void on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    GpsConfigurationAntennaScreenPresenter_t *self =
        (GpsConfigurationAntennaScreenPresenter_t *)base;

    if (self->show_saved == 0U)
    {
        return;
    }

    const uint32_t elapsed = os_ticks_get() - self->saved_tick;
    if (elapsed >= CONFIG_ANTENNA_SAVED_DISPLAY_MS)
    {
        self->show_saved = 0U;
        navigate_to(self, self->back_screen_id);
    }
}

static void on_back_pressed(IScreen_t *base)
{
    GpsConfigurationAntennaScreenPresenter_t *self =
        (GpsConfigurationAntennaScreenPresenter_t *)base;
    navigate_to(self, self->back_screen_id);
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t GpsConfigurationAntennaScreenPresenter_Init(
    GpsConfigurationAntennaScreenPresenter_t *self,
    const GpsConfigurationAntennaScreenPresenterDeps_t *deps)
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
