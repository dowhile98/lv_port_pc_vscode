/**
 * @file admin_operation_mode_screen_presenter.c
 * @brief Presenter for the Admin Operation Mode selection screen.
 *
 * ## Flujo
 *   OnEnter  → carga SuperUserConfig_t → reconstruye lista con modo actual enfocado.
 *   Item tap → license_key = (id==0) ? 0 : ADMIN_OP_MODE_RENT_KEY_DEFAULT
 *            → guarda SuperUserConfig_t → muestra feedback → arma timer.
 *   OnUpdate → cuando expire timer → navega a back_screen_id.
 *   OnBack   → navega a back_screen_id sin guardar.
 *
 * @note NO #include "lvgl.h" — intentional.
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/admin_operation_mode/admin_operation_mode_screen_presenter.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include <string.h>
#include <osal/osal.h>

/*============================================================================*
 * PRIVATE — constants
 *============================================================================*/

#define ADMIN_OP_MODE_ITEMS_COUNT 2U

/**
 * @brief Default license_key used when switching to RENT without a real license.
 *
 * Non-zero so the selector distinguishes FREE (0) from RENT (!=0).
 * Can be overridden by a future license injection flow.
 */
#define ADMIN_OP_MODE_RENT_KEY_DEFAULT 1U

static const char *const s_mode_labels[ADMIN_OP_MODE_ITEMS_COUNT] = {
    "Free mode", /* id=0 → license_key = 0         */
    "Rent mode", /* id=1 → license_key = non-zero  */
};
/*============================================================================*
 * PRIVATE — list item callback forward declaration
 *============================================================================*/
/* Forward declaration needed because rebuild_list references it. */
static void on_item_selected(void *ctx, uint8_t item_id);
/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

static void navigate_to(AdminOperationModeScreenPresenter_t *self, uint8_t screen_id)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, screen_id);
    }
}

static void rebuild_list(AdminOperationModeScreenPresenter_t *self)
{
    IAdminOperationModeScreenView_ClearList(self->view);
    for (uint8_t i = 0U; i < ADMIN_OP_MODE_ITEMS_COUNT; i++)
    {
        IAdminOperationModeScreenView_AddListItem(
            self->view,
            i,
            s_mode_labels[i],
            (i == self->current_mode),
            (AdminOperationModeItemCallback_t)on_item_selected,
            self);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    AdminOperationModeScreenPresenter_t *self =
        (AdminOperationModeScreenPresenter_t *)base;

    /* Clear feedback and timer state. */
    IAdminOperationModeScreenView_SetLabel(self->view, "");
    self->s_save_pending = false;
    self->s_save_ticks_ms = 0U;

    /* Load current mode from super user config. */
    SuperUserConfig_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    if (self->config_storage != NULL)
    {
        (void)ConfigStorage_LoadSuperUserConfig(self->config_storage, &cfg);
    }
    self->current_mode = (cfg.license_key == 0U) ? 0U : 1U;

    rebuild_list(self);
}

static void _on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    AdminOperationModeScreenPresenter_t *self =
        (AdminOperationModeScreenPresenter_t *)base;

    if (!self->s_save_pending)
    {
        return;
    }
    self->s_save_ticks_ms += 50U; /* approximate; called every ~50 ms */
    if (self->s_save_ticks_ms >= ADMIN_OP_MODE_SAVED_DISPLAY_MS)
    {
        self->s_save_pending = false;
        self->s_save_ticks_ms = 0U;
        navigate_to(self, self->back_screen_id);
    }
}

static void on_back_pressed(IScreen_t *base)
{
    AdminOperationModeScreenPresenter_t *self =
        (AdminOperationModeScreenPresenter_t *)base;
    navigate_to(self, self->back_screen_id);
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    (void)base;
    (void)key;
    (void)event;
    /* lv_group handles focus within the list. */
}

/*============================================================================*
 * PRIVATE — list item callback
 *============================================================================*/

static void on_item_selected(void *ctx, uint8_t item_id)
{
    AdminOperationModeScreenPresenter_t *self =
        (AdminOperationModeScreenPresenter_t *)ctx;
    if (self == NULL || item_id >= ADMIN_OP_MODE_ITEMS_COUNT)
    {
        return;
    }

    /* Load → update license_key → save. */
    SuperUserConfig_t cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    if (self->config_storage != NULL)
    {
        (void)ConfigStorage_LoadSuperUserConfig(self->config_storage, &cfg);
    }

    if (item_id == 0U)
    {
        cfg.license_key = 0U; /* FREE */
    }
    else
    {
        /* Preserve existing key if already non-zero (don't overwrite real license). */
        if (cfg.license_key == 0U)
        {
            cfg.license_key = ADMIN_OP_MODE_RENT_KEY_DEFAULT;
        }
    }

    if (self->config_storage != NULL)
    {
        (void)ConfigStorage_SaveSuperUserConfig(self->config_storage, &cfg);
    }

    self->current_mode = item_id;

    IAdminOperationModeScreenView_SetLabel(self->view, "Saved");
    self->s_save_pending = true;
    self->s_save_ticks_ms = 0U;
}

/*============================================================================*
 * PUBLIC — Init
 *============================================================================*/

Result_t AdminOperationModeScreenPresenter_Init(
    AdminOperationModeScreenPresenter_t *self,
    const AdminOperationModeScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL || deps->config_storage == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Wire IScreen_t vtable. */
    self->base.OnEnter = on_enter;
    self->base.OnExit = _on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = on_key_event;

    self->view = deps->view;
    self->config_storage = deps->config_storage;
    self->router = deps->router;
    self->back_screen_id = deps->back_screen_id;

    self->current_mode = 0U;
    self->s_save_pending = false;
    self->s_save_ticks_ms = 0U;

    return ERR_OK;
}

/* Public lifecycle delegates */
void AdminOperationModeScreenPresenter_OnEnter(IScreen_t *s) { on_enter(s); }
void AdminOperationModeScreenPresenter_OnExit(IScreen_t *s) { _on_exit(s); }
void AdminOperationModeScreenPresenter_OnUpdate(IScreen_t *s) { on_update(s); }
void AdminOperationModeScreenPresenter_OnBackPressed(IScreen_t *s) { on_back_pressed(s); }
void AdminOperationModeScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event)
{
    on_key_event(s, key, event);
}
