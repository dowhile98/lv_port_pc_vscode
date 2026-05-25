/**
 * @file stub_screen_presenter.h
 * @brief Minimal Presenter used to validate the MVP infrastructure (ACTION-024).
 *
 * Implements IScreen_t. Validates that:
 *   - The first-field cast rule works: (IScreen_t *)&presenter == &presenter
 *   - OnEnter/OnExit/OnUpdate dispatch through the vtable correctly
 *   - OnBackPressed = NULL is safe (root screen pattern)
 *   - No LVGL symbols are pulled into the Presenter layer
 *
 * Deleted in ACTION-026 once HomeScreenPresenter replaces it.
 *
 * @note Zero LVGL includes — fully testable on PC.
 */
#ifndef STUB_SCREEN_PRESENTER_H
#define STUB_SCREEN_PRESENTER_H

#include "hal/hal_types.h"
#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_stub_screen_view.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Stub Presenter — validates MVP infrastructure.
     *
     * base MUST be the first field so that (IScreen_t *)&presenter is valid.
     */
    typedef struct
    {
        IScreen_t base; /**< MUST be first — castable to IScreen_t* */
        IStubScreenView_t *view;
    } StubScreenPresenter_t;

    /**
     * @brief Initialise the stub presenter with its View dependency.
     *
     * @param[in,out] self  Presenter instance (must not be NULL).
     * @param[in]     view  View interface (must not be NULL).
     * @return ERR_OK on success, ERR_NULL_POINTER if any arg is NULL.
     */
    Result_t StubScreenPresenter_Init(StubScreenPresenter_t *self,
                                      IStubScreenView_t *view);

    /* IScreen_t method implementations — called via base vtable */
    void StubScreenPresenter_OnEnter(IScreen_t *self);
    void StubScreenPresenter_OnExit(IScreen_t *self);
    void StubScreenPresenter_OnUpdate(IScreen_t *self);
    /* OnBackPressed is NULL for the stub (root-level screen — no back action) */

#ifdef __cplusplus
}
#endif

#endif /* STUB_SCREEN_PRESENTER_H */
