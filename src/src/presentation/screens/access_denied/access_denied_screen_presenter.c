/**
 * @file access_denied_screen_presenter.c
 * @brief AccessDenied modal screen presenter — GREEN implementation.
 *
 * 2-second auto-dismiss modal: fires buzzer, shows message, then navigates
 * back to return_screen_id when elapsed_ticks >= ACCESS_DENIED_TIMEOUT_TICKS.
 * OnBackPressed skips the countdown and navigates immediately.
 *
 * @note NO #include "lvgl.h" — intentional. LVGL access goes through view.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/access_denied/access_denied_screen_presenter.h"
#include "presentation/interfaces/i_access_denied_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "domain/services/buzzer_notification_service.h"
#include "osal/osal.h"
#include <string.h>

/*============================================================================*
 * PRIVATE helpers
 *============================================================================*/

static void navigate_back(AccessDeniedScreenPresenter_t *self)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->return_screen_id);
    }
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    AccessDeniedScreenPresenter_t *self = (AccessDeniedScreenPresenter_t *)base;

    /* Reset timeout counter */
    self->elapsed_ticks = os_ticks_get();

    /* Fire buzzer (NULL-safe — buzzer is optional) */
    if (self->buzzer != NULL)
    {
        (void)BuzzerNotificationService_NotifyEvent(self->buzzer,
                                                    BUZZER_EVENT_ACCESSDENIED);
    }

    /* Update view message — use custom override if provided */
    static const char k_default_msg[] =
        "-----------------------------------------------------\n"
        "SECURITY WARNING\n"
        "CYCLE IN PROGRESS\n"
        "-----------------------------------------------------";
    const char *msg = (self->custom_message != NULL) ? self->custom_message : k_default_msg;
    IAccessDeniedScreenView_SetMessage(self->view, msg);
}

static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    AccessDeniedScreenPresenter_t *self = (AccessDeniedScreenPresenter_t *)base;


    if ((os_ticks_get() - self->elapsed_ticks)>= ACCESS_DENIED_TIMEOUT_TICKS)
    {
        navigate_back(self);
    }
}

static void on_back_pressed(IScreen_t *base)
{
    AccessDeniedScreenPresenter_t *self = (AccessDeniedScreenPresenter_t *)base;
    navigate_back(self);
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    (void)base;
    (void)key;
    (void)event;
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t AccessDeniedScreenPresenter_Init(AccessDeniedScreenPresenter_t *self,
                                          const AccessDeniedScreenPresenterDeps_t *deps)
{
    if (!self)
        return ERR_NULL_POINTER;
    if (!deps)
        return ERR_NULL_POINTER;
    if (!deps->view)
        return ERR_NULL_POINTER;
    if (!deps->router)
        return ERR_NULL_POINTER;

    memset(self, 0, sizeof(*self));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = on_key_event;

    /* Inject dependencies */
    self->view = deps->view;
    self->router = deps->router;
    self->buzzer = deps->buzzer;
    self->return_screen_id = deps->return_screen_id;
    self->elapsed_ticks = 0U;
    self->custom_message = deps->custom_message; /* NULL → use default */

    return ERR_OK;
}
