/**
 * @file security_screen_presenter.c
 * @brief Password-entry screen presenter (ACTION-031).
 *
 * ## Logic summary
 *
 * OnEnter: clears input buffer, shows "...", sets state = SEC_STATE_INPUT.
 *
 * OnUpdate (SEC_STATE_INPUT): no-op (input handled by OnKeyEvent from InputRouter).
 *
 * OnKeyEvent:
 *   - SCREEN_KEY_UP / SCREEN_KEY_DOWN → append key to s_input[], refresh display.
 *   - SCREEN_KEY_ENTER → compare s_input[] to password.
 *      - Match   → SetTitle("** VALID ACCESS **"),  state = FEEDBACK_VALID,  ticks = 0.
 *      - No match → SetTitle("** INVALID ACCESS **"), state = FEEDBACK_INVALID, ticks = 0.
 *
 * OnUpdate (FEEDBACK_VALID / FEEDBACK_INVALID):
 *   - Increment s_feedback_ticks.
 *   - When ticks ≥ (2000 / update_period_ms):
 *       - VALID   → NavigateTo(success_screen_id).
 *       - INVALID → NavigateTo(cancel_screen_id).
 *
 * OnBackPressed (SEC_STATE_INPUT):
 *   - Buffer not empty → remove last entry, refresh display.
 *   - Buffer empty     → NavigateTo(cancel_screen_id).
 *   - Ignored during feedback states (button not active while wait-for-result).
 *
 * @note NO #include "lvgl.h" — intentional. Fully testable on PC.
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */

/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/security/security_screen_presenter.h"
#include <string.h>

/** Milliseconds of valid/invalid feedback display before navigating. */
#define SECURITY_FEEDBACK_DISPLAY_MS 2000U

/*============================================================================*
 * PRIVATE — helpers
 *============================================================================*/

/**
 * @brief Refresh the password-input label to reflect s_input_count.
 *
 * - 0 keys  → "..."
 * - n keys  → n asterisk characters (e.g., "***")
 */
static void update_pswd_display(SecurityScreenPresenter_t *self)
{
    if (self->s_input_count == 0U)
    {
        ISecurityScreenView_SetPswdInput(self->view, "...");
        return;
    }

    char buf[SECURITY_MAX_INPUT_LEN + 1U];
    for (uint8_t i = 0U; i < self->s_input_count; i++)
    {
        buf[i] = '*';
    }
    buf[self->s_input_count] = '\0';
    ISecurityScreenView_SetPswdInput(self->view, buf);
}

/**
 * @brief Return true if the entered sequence matches the stored password exactly.
 */
static bool verify_password(const SecurityScreenPresenter_t *self)
{
    if (self->s_input_count != self->password_len)
    {
        return false;
    }

    for (uint8_t i = 0U; i < self->password_len; i++)
    {
        if (self->s_input[i] != self->password[i])
        {
            return false;
        }
    }
    return true;
}

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

/**
 * @brief Reset all input state and show the initial password prompt.
 */
static void on_enter(IScreen_t *base)
{
    SecurityScreenPresenter_t *self = (SecurityScreenPresenter_t *)base;

    self->s_input_count = 0U;
    self->s_state = SEC_STATE_INPUT;
    self->s_feedback_ticks = 0U;

    memset(self->s_input, 0U, sizeof(self->s_input));

    ISecurityScreenView_SetTitle(self->view, "----------------ENTER KEY----------------");
    ISecurityScreenView_SetPswdInput(self->view, "...");
    ISecurityScreenView_SetLockIcon(self->view, false); /* reset to locked */
}

/**
 * @brief Cleanup on exit — nothing to release for this screen.
 */
static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

/**
 * @brief Periodic update — drives the FEEDBACK countdown; INPUT is handled by OnKeyEvent.
 *
 * Called every @c update_period_ms ms from the Control thread.
 */
static void on_update(IScreen_t *base)
{
    SecurityScreenPresenter_t *self = (SecurityScreenPresenter_t *)base;

    /* ── FEEDBACK state: count down, then navigate ── */
    if (self->s_state == SEC_STATE_FEEDBACK_VALID ||
        self->s_state == SEC_STATE_FEEDBACK_INVALID)
    {
        self->s_feedback_ticks++;
        uint32_t threshold = SECURITY_FEEDBACK_DISPLAY_MS / self->update_period_ms;
        if (self->s_feedback_ticks >= threshold)
        {
            if (self->s_state == SEC_STATE_FEEDBACK_VALID)
            {
                if (self->on_success_action != NULL)
                {
                    self->on_success_action(self->on_success_ctx);
                }
                (void)IScreenRouter_NavigateTo(self->router, self->success_screen_id);
            }
            else
            {
                (void)IScreenRouter_NavigateTo(self->router, self->cancel_screen_id);
            }
        }
    }
    /* SEC_STATE_INPUT: no polling — keys arrive via on_key_event */
}

/**
 * @brief Key event handler — UP/DOWN append to buffer, ENTER triggers verification.
 *
 * Wired as IScreen_t::OnKeyEvent by SecurityScreenPresenter_Init.
 * Called by InputRouter on button press events; no polling or edge detection needed.
 */
static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{

    SecurityScreenPresenter_t *self = (SecurityScreenPresenter_t *)base;

    if (event != SCREEN_KEY_EVENT_PRESS)
    {
        return;
    }
    /* Ignore key presses during feedback — wait for auto-navigation */
    if (self->s_state != SEC_STATE_INPUT)
    {
        return;
    }

    if ((key == (uint8_t)SCREEN_KEY_UP || key == (uint8_t)SCREEN_KEY_DOWN) && self->s_input_count < SECURITY_MAX_INPUT_LEN)
    {
        self->s_input[self->s_input_count] = key; /* SEC_KEY_UP == SCREEN_KEY_UP */
        self->s_input_count++;
        update_pswd_display(self);
    }
    else if (key == (uint8_t)SCREEN_KEY_ENTER)
    {
        if (verify_password(self))
        {
            ISecurityScreenView_SetPswdInput(self->view, "VALID ACCESS");

            ISecurityScreenView_SetLockIcon(self->view, true); /* unlock icon on valid */
            self->s_state = SEC_STATE_FEEDBACK_VALID;
            self->s_feedback_ticks = 0U;
        }
        else
        {
            ISecurityScreenView_SetPswdInput(self->view, "INVALID ACCESS");
            self->s_state = SEC_STATE_FEEDBACK_INVALID;
            self->s_feedback_ticks = 0U;
        }
    }
}

/**
 * @brief Back button — delete last char if buffer non-empty, else navigate to cancel.
 *
 * @note Ignored during feedback states so user cannot interrupt the feedback display.
 */
static void on_back_pressed(IScreen_t *base)
{
    SecurityScreenPresenter_t *self = (SecurityScreenPresenter_t *)base;

    /* Ignore during feedback — wait for auto-navigation */
    if (self->s_state != SEC_STATE_INPUT)
    {
        return;
    }

    if (self->s_input_count > 0U)
    {
        self->s_input_count--;
        update_pswd_display(self);
    }
    else
    {
        /* Empty buffer → return to origin */
        (void)IScreenRouter_NavigateTo(self->router, self->cancel_screen_id);
    }
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t SecurityScreenPresenter_Init(SecurityScreenPresenter_t *self,
                                      const SecurityScreenPresenterDeps_t *deps)
{
    if (self == NULL || deps == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->view == NULL || deps->password == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (deps->password_len == 0U || deps->password_len > SECURITY_MAX_INPUT_LEN)
    {
        return ERR_INVALID_PARAM;
    }
    if (deps->update_period_ms == 0U)
    {
        return ERR_INVALID_PARAM;
    }

    memset(self, 0, sizeof(SecurityScreenPresenter_t));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed; /* real function, NOT NULL */
    self->base.OnKeyEvent = on_key_event;

    /* Required dependencies */
    self->view = deps->view;
    self->password = deps->password;
    self->password_len = deps->password_len;
    self->update_period_ms = deps->update_period_ms;
    self->success_screen_id = deps->success_screen_id;
    self->cancel_screen_id = deps->cancel_screen_id;

    /* Optional (NULL = navigation silently skipped in tests) */
    self->router = deps->router;

    /* Optional success action callback */
    self->on_success_action = deps->on_success_action;
    self->on_success_ctx = deps->on_success_ctx;

    return ERR_OK;
}

/* Public lifecycle delegates — forward to private for external access */
void SecurityScreenPresenter_OnEnter(IScreen_t *s) { on_enter(s); }
void SecurityScreenPresenter_OnExit(IScreen_t *s) { screen_on_exit(s); }
void SecurityScreenPresenter_OnUpdate(IScreen_t *s) { on_update(s); }
void SecurityScreenPresenter_OnBackPressed(IScreen_t *s) { on_back_pressed(s); }
void SecurityScreenPresenter_OnKeyEvent(IScreen_t *s, uint8_t key, uint8_t event) { on_key_event(s, key, event); }

void SecurityScreenPresenter_SetContext(SecurityScreenPresenter_t *self,
                                        const uint8_t *password,
                                        uint8_t password_len,
                                        uint8_t success_screen_id,
                                        uint8_t cancel_screen_id)
{
    if (self == NULL || password == NULL || password_len == 0U ||
        password_len > SECURITY_MAX_INPUT_LEN)
    {
        return;
    }
    self->password = password;
    self->password_len = password_len;
    self->success_screen_id = success_screen_id;
    self->cancel_screen_id = cancel_screen_id;
    /* Clear success action to avoid stale callback from previous context. */
    self->on_success_action = NULL;
    self->on_success_ctx = NULL;
}
