/**
 * @file security_screen_presenter.h
 * @brief Password-entry screen presenter (ACTION-031).
 *
 * ## Responsibilities
 * - Accepts UP/DOWN key presses from an IEncoder_t and builds a key sequence.
 * - Compares the sequence against a stored password on ENTER.
 * - Shows "** VALID ACCESS **" for @c feedback_display_ms, then navigates to
 *   @c success_screen_id; shows "** INVALID ACCESS **" for the same period,
 *   then clears the buffer and returns to INPUT mode.
 * - @c OnBackPressed deletes the last entered key; if the buffer is empty,
 *   immediately navigates to @c cancel_screen_id.
 *
 * ## Reusability
 * The presenter is screen-agnostic: @c success_screen_id, @c cancel_screen_id,
 * and the password are injected via @c SecurityScreenPresenterDeps_t.
 * The same instance can protect any destination screen.
 *
 * ## Architecture
 * - @c IScreen_t is the FIRST field (C99 first-field cast).
 * - Zero @c lvgl.h — fully testable on PC via ISecurityScreenView_t mock.
 * - Encoder polling is non-blocking, called every @c update_period_ms from OnUpdate.
 * - Router is optional (NULL in PC tests — navigation silently skipped).
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef SECURITY_SCREEN_PRESENTER_H
#define SECURITY_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_security_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "hal/hal_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Maximum number of keys the input buffer can hold. */
#define SECURITY_MAX_INPUT_LEN 16U

    /**
     * @brief Key values for the password buffer.
     *
     * Values equal SCREEN_KEY_UP (1) and SCREEN_KEY_DOWN (2) from i_screen.h so that
     * the buffer entries produced by OnKeyEvent match the stored password directly.
     */
#define SEC_KEY_UP 1U   /**< UP button key code (== SCREEN_KEY_UP). */
#define SEC_KEY_DOWN 2U /**< DOWN button key code (== SCREEN_KEY_DOWN). */

    /**
     * @brief Internal state machine for the security screen.
     */
    typedef enum SecurityState_t
    {
        SEC_STATE_INPUT = 0,       /**< Accepting key presses from encoder.   */
        SEC_STATE_FEEDBACK_VALID,  /**< Showing "VALID ACCESS" for 2 s.       */
        SEC_STATE_FEEDBACK_INVALID /**< Showing "INVALID ACCESS" for 2 s.     */
    } SecurityState_t;

    /**
     * @brief Injected dependency bundle for SecurityScreenPresenter_Init().
     *
     * Input events (UP/DOWN/ENTER) are now dispatched via InputRouter→OnKeyEvent;
     * the encoder field has been removed. The router is optional — if NULL,
     * navigation calls are silently skipped (useful for PC unit tests).
     */
    typedef struct SecurityScreenPresenterDeps_t
    {
        ISecurityScreenView_t *view; /**< REQUIRED — widget updates.                  */
        IScreenRouter_t *router;     /**< OPTIONAL (NULL in tests) — navigation.      */
        const uint8_t *password;     /**< REQUIRED — key sequence to match.           */
        uint8_t password_len;        /**< REQUIRED — must be > 0 and ≤ SECURITY_MAX_INPUT_LEN. */
        uint8_t success_screen_id;   /**< SCREEN_ID_MAINMENU (or any target).         */
        uint8_t cancel_screen_id;    /**< SCREEN_ID_HOME (or any origin).             */
        uint32_t update_period_ms;   /**< Control task tick period in ms (> 0).       */
        /** @brief Optional callback executed after password validated, before navigation. */
        void (*on_success_action)(void *ctx);
        void *on_success_ctx; /**< Context passed to on_success_action. */
    } SecurityScreenPresenterDeps_t;

    /**
     * @brief Security screen presenter — password entry and validation.
     *
     * First field MUST be @c IScreen_t (C99 §6.7.2.1 first-field cast rule).
     * Do not access private @c s_* fields directly from outside this module.
     */
    typedef struct SecurityScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first — enables (IScreen_t *)&presenter cast. */

        /* Dependencies */
        ISecurityScreenView_t *view;
        IScreenRouter_t *router; /**< NULL = no navigation (tests). */
        const uint8_t *password;
        uint8_t password_len;
        uint8_t success_screen_id;
        uint8_t cancel_screen_id;
        uint32_t update_period_ms;

        /** @brief Optional callback executed after password validated, before navigation. */
        void (*on_success_action)(void *ctx);
        void *on_success_ctx; /**< Context passed to on_success_action. */

        /* Private runtime state — reset in OnEnter */
        uint8_t s_input[SECURITY_MAX_INPUT_LEN]; /**< Accumulated key presses. */
        uint8_t s_input_count;                   /**< Number of keys entered.  */
        SecurityState_t s_state;                 /**< Current state.           */
        uint32_t s_feedback_ticks;               /**< Ticks since feedback shown. */
    } SecurityScreenPresenter_t;

    /**
     * @brief Initialise the security presenter with injected dependencies.
     *
     * Wires the IScreen_t vtable and stores all injected dependencies.
     * Does NOT draw anything (deferred to OnEnter).
     *
     * @param[in,out] self  Presenter instance (must not be NULL).
     * @param[in]     deps  Dependency bundle (must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, deps, view, or password is NULL.
     * @return ERR_INVALID_PARAM if password_len is 0, > SECURITY_MAX_INPUT_LEN, or update_period_ms is 0.
     */
    Result_t SecurityScreenPresenter_Init(SecurityScreenPresenter_t *self,
                                          const SecurityScreenPresenterDeps_t *deps);

    /* Public lifecycle delegates (call via IScreen_OnXxx in production) */
    void SecurityScreenPresenter_OnEnter(IScreen_t *s);
    void SecurityScreenPresenter_OnExit(IScreen_t *s);
    void SecurityScreenPresenter_OnUpdate(IScreen_t *s);
    void SecurityScreenPresenter_OnBackPressed(IScreen_t *s);

    /**
     * @brief OnKeyEvent — handle UP/DOWN/ENTER key presses from InputRouter.
     *
     * Wired as IScreen_t::OnKeyEvent by SecurityScreenPresenter_Init.
     * Called on DI_ID_BUTTON_UP/DOWN/ENTER events; no polling or edge detection needed.
     * Key values are defined by ScreenKey_t in i_screen.h:
     *   SCREEN_KEY_UP (1) == SEC_KEY_UP, SCREEN_KEY_DOWN (2) == SEC_KEY_DOWN.
     *
     * @param[in] s      IScreen_t base pointer (cast to SecurityScreenPresenter_t internally).
     * @param[in] key    ScreenKey_t value (uint8_t) dispatched by InputRouter.
     * @param[in] event  ScreenKeyEvent_t value (uint8_t); reserved for future KEEPALIVE handling.
     */
    void SecurityScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event);

    /**
     * @brief Reconfigura contraseña y screen IDs en tiempo de ejecución.
     *
     * Llamar ANTES de navegar a SCREEN_ID_SEGURITY para cambiar el contexto
     * de la instancia estática (e.g., super-usuario vs usuario estándar).
     * No restablece el buffer de entrada — eso ocurre en OnEnter.
     *
     * @param[in,out] self            Presenter instance (must not be NULL).
     * @param[in]     password        Nueva contraseña (must not be NULL).
     * @param[in]     password_len    Longitud (1..SECURITY_MAX_INPUT_LEN).
     * @param[in]     success_screen_id  Screen al que navegar si válido.
     * @param[in]     cancel_screen_id   Screen al que navegar si inválido/back.
     */
    void SecurityScreenPresenter_SetContext(SecurityScreenPresenter_t *self,
                                            const uint8_t *password,
                                            uint8_t password_len,
                                            uint8_t success_screen_id,
                                            uint8_t cancel_screen_id);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SCREEN_PRESENTER_H */
