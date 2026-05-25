/**
 * @file access_denied_screen_presenter.h
 * @brief AccessDenied modal screen presenter.
 *
 * A 2-second auto-dismiss modal that shows an "access denied" message and plays
 * a buzzer notification. Navigates back to return_screen_id on timeout or when
 * OnBackPressed is called.
 *
 * ## Timing model
 * - elapsed_ticks is incremented on each OnUpdate call (reset to 0 in OnEnter).
 * - Navigation fires when elapsed_ticks >= ACCESS_DENIED_TIMEOUT_TICKS.
 * - At 100 ms/tick this equals 2 s.
 *
 * ## Usage
 * ```c
 * AccessDeniedScreenPresenter_t p;
 * AccessDeniedScreenPresenterDeps_t d = {
 *     .view             = EezAccessDeniedScreenView_GetInterface(),
 *     .router           = DI_GetScreenRouter(),
 *     .buzzer           = DI_GetBuzzerService(),
 *     .return_screen_id = (uint8_t)SCREEN_ID_INT_CONFIGURATION,
 * };
 * AccessDeniedScreenPresenter_Init(&p, &d);
 * ```
 *
 * @note IScreen_t MUST remain the first field — C99 first-field cast used by
 *       ScreenRouter.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef ACCESS_DENIED_SCREEN_PRESENTER_H
#define ACCESS_DENIED_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_access_denied_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "domain/services/buzzer_notification_service.h"
#include "hal/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Number of OnUpdate ticks before auto-navigation (100 ms each = 2 s). */
#define ACCESS_DENIED_TIMEOUT_TICKS 1500U

    /**
     * @brief Dependency bundle for AccessDeniedScreenPresenter_Init().
     */
    typedef struct AccessDeniedScreenPresenterDeps_t
    {
        /** View interface.  Required — must not be NULL. */
        IAccessDeniedScreenView_t *view;

        /** Screen router.  Required — must not be NULL. */
        IScreenRouter_t *router;

        /**
         * @brief Buzzer notification service.  NULL → no beep (silently skipped).
         */
        BuzzerNotificationService_t *buzzer;

        /** Screen ID to navigate to after the timeout or on OnBackPressed. */
        uint8_t return_screen_id;

        /**
         * @brief Optional custom message to display.
         *
         * When non-NULL this string is shown instead of the default
         * "CYCLE IN PROGRESS" message.  Caller owns the lifetime — must
         * remain valid while the screen is visible.
         * NULL → default message.
         */
        const char *custom_message;

    } AccessDeniedScreenPresenterDeps_t;

    /**
     * @brief AccessDenied modal screen presenter.
     *
     * IScreen_t MUST be the first field — supports C99 first-field cast.
     */
    typedef struct AccessDeniedScreenPresenter_t
    {
        IScreen_t base; /**< Must be first. */
        IAccessDeniedScreenView_t *view;
        IScreenRouter_t *router;
        BuzzerNotificationService_t *buzzer;
        uint8_t return_screen_id;
        uint32_t elapsed_ticks;      /**< Incremented per OnUpdate. */
        const char *custom_message;  /**< Non-NULL overrides default message. */
    } AccessDeniedScreenPresenter_t;

    /**
     * @brief Initialise the AccessDenied screen presenter.
     *
     * @param[in] self  Presenter instance — must not be NULL.
     * @param[in] deps  Dependency bundle — deps, view, and router must not be NULL.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if self, deps, deps->view, or deps->router is NULL.
     */
    Result_t AccessDeniedScreenPresenter_Init(AccessDeniedScreenPresenter_t *self,
                                              const AccessDeniedScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* ACCESS_DENIED_SCREEN_PRESENTER_H */
