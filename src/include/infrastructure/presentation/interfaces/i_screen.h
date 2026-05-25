/**
 * @file i_screen.h
 * @brief Base contract for all application screens (MVP architecture).
 *
 * Every concrete screen (HomeScreenPresenter, SettingsPresenter, etc.)
 * must implement this interface so the Screen Router can manage them
 * uniformly without knowing their concrete type.
 *
 * @note Zero LVGL includes. All lv_*() calls live in EezXxxView implementations.
 * @note Thread-Safety: lifecycle methods (OnEnter/OnExit) are called from the
 *       LVGL thread (action context). OnUpdate is called from the Control AO
 *       thread every 500 ms.
 *
 * Architecture reference: docs/architecture/lvgl/LVGL_MVP_GAP_ANALYSIS.md §5 / P-1.1
 */
#ifndef I_SCREEN_H
#define I_SCREEN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Base contract for all application screens.
     */
    typedef struct IScreen_t IScreen_t;

    struct IScreen_t
    {
        /**
         * @brief Called once when the screen becomes active.
         *        Load initial state into the view here.
         * @param[in] self  Screen instance.
         */
        void (*OnEnter)(IScreen_t *self);

        /**
         * @brief Called once when the screen is about to be replaced.
         *        Release resources, unregister input callbacks, cancel operations.
         * @param[in] self  Screen instance.
         */
        void (*OnExit)(IScreen_t *self);

        /**
         * @brief Called periodically while this screen is active.
         *        Refresh domain data into the View.
         *        Typical period: 500 ms from the Control AO thread.
         * @param[in] self  Screen instance.
         */
        void (*OnUpdate)(IScreen_t *self);

        /**
         * @brief Called when the physical Back/Cancel button (DI_ID_BUTTON_ONOFF) is pressed.
         *
         * Root screens (e.g. Home) set this to a no-op function — dispatch is NULL-safe.
         * Child screens (Settings, TextEntry) use it to navigate back or discard input.
         *
         * @note Invoked by InputRouter_t on DI_ID_BUTTON_ONOFF / DI_EVENT_CLICK.
         *       Runs in the InputRouter dispatch context (NOT an ISR).
         * @param[in] self  Screen instance.
         */
        void (*OnBackPressed)(IScreen_t *self);

        /**
         * @brief Called by InputRouter_t when UP, DOWN, or ENTER button events fire.
         *
         * Replaces encoder polling in presenters — callback-based, one event per press.
         * Runs in the DigitalInputAO polling context (NOT an ISR).
         *
         * Screens that do not use directional input may leave this NULL (NULL-safe dispatch).
         *
         * @param[in] self   Screen instance.
         * @param[in] key    Key identifier (SCREEN_KEY_UP / DOWN / ENTER).
         * @param[in] event  Event type (SCREEN_KEY_EVENT_PRESS / KEEPALIVE).
         *                   Passed through from DigitalInputEvent_t — presenters can
         *                   distinguish a first press from an auto-repeat keepalive.
         */
        void (*OnKeyEvent)(IScreen_t *self, uint8_t key, uint8_t event);
    };

    /**
     * @brief Dispatch OnEnter — safe when self or vtable entry is NULL.
     */
    static inline void IScreen_OnEnter(IScreen_t *s)
    {
        if (s && s->OnEnter)
        {
            s->OnEnter(s);
        }
    }

    /**
     * @brief Dispatch OnExit — safe when self or vtable entry is NULL.
     */
    static inline void IScreen_OnExit(IScreen_t *s)
    {
        if (s && s->OnExit)
        {
            s->OnExit(s);
        }
    }

    /**
     * @brief Dispatch OnUpdate — safe when self or vtable entry is NULL.
     */
    static inline void IScreen_OnUpdate(IScreen_t *s)
    {
        if (s && s->OnUpdate)
        {
            s->OnUpdate(s);
        }
    }

    /**
     * @brief Dispatch OnBackPressed — safe when self or vtable entry is NULL.
     */
    static inline void IScreen_OnBackPressed(IScreen_t *s)
    {
        if (s && s->OnBackPressed)
        {
            s->OnBackPressed(s);
        }
    }

    /**
     * @brief Physical key identifiers dispatched by InputRouter_t.
     *
     * Values deliberately match SEC_KEY_UP (1) and SEC_KEY_DOWN (2) so that
     * SecurityScreenPresenter can store them directly in the password buffer
     * without translation.
     */
    typedef enum
    {
        SCREEN_KEY_UP = 1U,    /**< UP button (DI_ID_BUTTON_UP / DI_EVENT_PRESS or KEEPALIVE). */
        SCREEN_KEY_DOWN = 2U,  /**< DOWN button (DI_ID_BUTTON_DOWN / DI_EVENT_PRESS or KEEPALIVE). */
        SCREEN_KEY_ENTER = 3U, /**< ENTER button (DI_ID_BUTTON_ENTER / DI_EVENT_PRESS). */
    } ScreenKey_t;

    /**
     * @brief Event type for OnKeyEvent — mirrors DigitalInputEvent_t values.
     *
     * Defined here (not via i_digital_input_source.h) so that i_screen.h
     * stays fully HAL-free and usable in PC unit tests.
     *
     * Values must remain in sync with DigitalInputEvent_t in
     * include/interfaces/i_digital_input_source.h.
     */
    typedef enum
    {
        SCREEN_KEY_EVENT_PRESS = 1<<0U,     /**< Initial press post-debounce. Mirrors DI_EVENT_PRESS (0). */
        SCREEN_KEY_EVENT_LONG_PRESS = 1<<1, /**< Presión mantenida (legacy, deprecado) */
        SCREEN_KEY_DI_EVENT_RELEASE = 1<<2, /**< Liberación del input */
        SCREEN_KEY_DI_EVENT_CLICK = 1<<3,   /**< Click completo (press + release rápido) */
        SCREEN_KEY_EVENT_KEEPALIVE = 1<<4U, /**< Auto-repeat while held.   Mirrors DI_EVENT_KEEPALIVE (4). */
    } ScreenKeyEvent_t;

    /**
     * @brief Dispatch OnKeyEvent — safe when self or vtable entry is NULL.
     */
    static inline void IScreen_OnKeyEvent(IScreen_t *s, ScreenKey_t key, ScreenKeyEvent_t event)
    {
        if (s && s->OnKeyEvent)
        {
            s->OnKeyEvent(s, (uint8_t)key, (uint8_t)event);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_SCREEN_H */
