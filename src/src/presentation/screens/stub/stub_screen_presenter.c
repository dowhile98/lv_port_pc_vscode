/**
 * @file stub_screen_presenter.c
 * @brief Minimal Presenter — validates MVP infrastructure end-to-end.
 *
 * Proves that IScreen_t lifecycle methods dispatch correctly and that
 * the Presenter → View call chain works without any LVGL dependency.
 *
 * @note NO #include "lvgl.h" — intentional. If this file ever includes
 *       lvgl.h the architecture boundary has been violated.
 * @note Deleted in ACTION-026 once HomeScreenPresenter replaces it.
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/stub/stub_screen_presenter.h"
#include <string.h>

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    StubScreenPresenter_t *self = (StubScreenPresenter_t *)base;
    IStubScreenView_SetMessage(self->view, "MVP OK — Enter");
}

static void on_exit(IScreen_t *base)
{
    (void)base;
    /* No resources to release in stub */
}

static void on_update(IScreen_t *base)
{
    StubScreenPresenter_t *self = (StubScreenPresenter_t *)base;
    IStubScreenView_SetMessage(self->view, "MVP OK — Update");
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t StubScreenPresenter_Init(StubScreenPresenter_t *self,
                                  IStubScreenView_t *view)
{
    if (self == NULL || view == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(StubScreenPresenter_t));

    self->base.OnEnter = on_enter;
    self->base.OnExit = on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = NULL; /* Stub is root-level — no back action */
    self->view = view;

    return ERR_OK;
}

/* Public wrappers — expose methods for direct call (bypassing vtable) */
void StubScreenPresenter_OnEnter(IScreen_t *s) { on_enter(s); }
void StubScreenPresenter_OnExit(IScreen_t *s) { on_exit(s); }
void StubScreenPresenter_OnUpdate(IScreen_t *s) { on_update(s); }
