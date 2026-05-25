/**
 * @file admin_operation_mode_screen_presenter.h
 * @brief Presenter for the Admin Operation Mode selection screen.
 *
 * Shows a lv_list with "Free mode" / "Rent mode".
 * Item selection updates SuperUserConfig_t.license_key and shows a feedback
 * label, then auto-returns to back_screen_id after
 * ADMIN_OP_MODE_SAVED_DISPLAY_MS milliseconds.
 *
 * ## Lifecycle
 *   OnEnter   → loads SuperUserConfig_t, rebuilds list with current mode focused
 *   Item tap  → saves license_key, shows feedback label, arms delay timer
 *   OnUpdate  → when timer expires, navigates to back_screen_id
 *   OnBack    → navigates to back_screen_id without saving
 *
 * @note #include "lvgl.h" is FORBIDDEN in this file.
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef ADMIN_OPERATION_MODE_SCREEN_PRESENTER_H
#define ADMIN_OPERATION_MODE_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_screen_router.h"
#include "presentation/interfaces/i_admin_operation_mode_screen_view.h"
#include "interfaces/i_config_storage.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Duration (ms) to display feedback label before auto-returning. */
#define ADMIN_OP_MODE_SAVED_DISPLAY_MS 1500U

    /**
     * @brief Dependencies required to initialise the operation mode selection presenter.
     */
    typedef struct AdminOperationModeScreenPresenterDeps_t
    {
        /** EEZ-backed view (required — must not be NULL). */
        IAdminOperationModeScreenView_t *view;

        /** Config storage (required). */
        IConfigStorage *config_storage;

        /** Screen router. NULL → navigation silently skipped (tests). */
        IScreenRouter_t *router;

        /** Screen ID to navigate to on back or after save delay. */
        uint8_t back_screen_id;

    } AdminOperationModeScreenPresenterDeps_t;

    /**
     * @brief Admin Operation Mode selection presenter.
     *
     * @note @c base MUST be the first field — C99 first-field cast rule.
     */
    typedef struct AdminOperationModeScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */

        IAdminOperationModeScreenView_t *view;
        IConfigStorage *config_storage;
        IScreenRouter_t *router;
        uint8_t back_screen_id;

        /** Cached current mode (0=FREE, 1=RENT) read from SuperUserConfig_t on OnEnter. */
        uint8_t current_mode;

        /** true when the save-feedback delay timer is running. */
        bool s_save_pending;

        /** Accumulator for the feedback delay (ms). */
        uint32_t s_save_ticks_ms;

    } AdminOperationModeScreenPresenter_t;

    /**
     * @brief Initialise the operation mode selection presenter.
     *
     * @return ERR_OK, ERR_NULL_POINTER, or ERR_INVALID_PARAM.
     */
    Result_t AdminOperationModeScreenPresenter_Init(
        AdminOperationModeScreenPresenter_t *self,
        const AdminOperationModeScreenPresenterDeps_t *deps);

    /* Public lifecycle delegates */
    void AdminOperationModeScreenPresenter_OnEnter(IScreen_t *s);
    void AdminOperationModeScreenPresenter_OnExit(IScreen_t *s);
    void AdminOperationModeScreenPresenter_OnUpdate(IScreen_t *s);
    void AdminOperationModeScreenPresenter_OnBackPressed(IScreen_t *s);
    void AdminOperationModeScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event);

#ifdef __cplusplus
}
#endif

#endif /* ADMIN_OPERATION_MODE_SCREEN_PRESENTER_H */
