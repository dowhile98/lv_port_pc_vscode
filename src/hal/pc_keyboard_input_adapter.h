/**
 * @file pc_keyboard_input_adapter.h
 * @brief PC keyboard-based IDigitalInputSource for the PC simulator.
 *
 * Drop-in replacement for DigitalInputAdapter (GPIO-based) in PC builds.
 * Maps SDL keyboard scancodes to the 4 firmware buttons via lwbtn debounce:
 *   UP, DOWN, ENTER, BACK (= ONOFF on firmware).
 *
 * Usage:
 *   1. PCKeyboardInputAdapter_Init(&kbd, &cfg)
 *   2. Pass PCKeyboardInputAdapter_GetInterface(&kbd) as UiAO_Config_t.di_source
 *   3. Call IDigitalInputSource_Process(di_source) every ~20ms from a FreeRTOS task
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef PC_KEYBOARD_INPUT_ADAPTER_H
#define PC_KEYBOARD_INPUT_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "interfaces/i_digital_input_source.h"
#include "lwbtn/lwbtn.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================
     * CONFIGURATION
     * ======================================================================== */

    /**
     * @brief SDL scancode mapping for the four firmware buttons.
     *
     * Use SDL_SCANCODE_* integer values from <SDL2/SDL_scancode.h>.
     * Common defaults:
     *   SDL_SCANCODE_UP     = 82
     *   SDL_SCANCODE_DOWN   = 81
     *   SDL_SCANCODE_RETURN = 40
     *   SDL_SCANCODE_ESCAPE = 41
     */
    typedef struct
    {
        int scancode_up;    /**< UP button scancode    (DI_ID_BUTTON_UP)    */
        int scancode_down;  /**< DOWN button scancode  (DI_ID_BUTTON_DOWN)  */
        int scancode_enter; /**< ENTER button scancode (DI_ID_BUTTON_ENTER) */
        int scancode_back;  /**< BACK button scancode  (DI_ID_BUTTON_ONOFF) */
    } PCKeyboardInputConfig_t;

    /* ========================================================================
     * INTERNAL TYPES (exposed for static allocation, not part of public API)
     * ======================================================================== */

#ifndef PC_KBD_MAX_CALLBACKS_PER_INPUT
#define PC_KBD_MAX_CALLBACKS_PER_INPUT 20U
#endif

#define PC_KBD_BTN_COUNT 4U /**< ONOFF, ENTER, UP, DOWN */

    /**
     * @brief Internal subscriber slot — mirrors DigitalInputAdapter pattern.
     */
    typedef struct
    {
        DigitalInputCallback_t callback;
        void *context;
        bool active;
        uint32_t event_mask; /**< Bitmask of DI_EVENT_* values */
    } PCKbdSubscriber_t;

    /* Forward declaration for context back-pointer */
    typedef struct PCKeyboardInputAdapter PCKeyboardInputAdapter_t;

    /**
     * @brief Per-button context passed as lwbtn_btn_t::arg.
     */
    typedef struct
    {
        PCKeyboardInputAdapter_t *adapter;
        DigitalInputID_t id;
        int scancode;
    } PCKbdBtnContext_t;

    /**
     * @brief PCKeyboardInputAdapter instance — stack-allocatable (~700 bytes).
     *
     * @note The @c iface field MUST remain first to allow safe cast to
     *       @c IDigitalInputSource*.
     */
    struct PCKeyboardInputAdapter
    {
        IDigitalInputSource iface; /**< Public vtable — MUST be first field */

        PCKeyboardInputConfig_t config;
        lwbtn_t lwbtn_instance;
        lwbtn_btn_t buttons[PC_KBD_BTN_COUNT];
        PCKbdBtnContext_t contexts[PC_KBD_BTN_COUNT];

        /** Observer callbacks: [input_id][slot] */
        PCKbdSubscriber_t subscribers[DI_ID_COUNT][PC_KBD_MAX_CALLBACKS_PER_INPUT];
    };

    /* ========================================================================
     * PUBLIC API
     * ======================================================================== */

    /**
     * @brief Initialise the adapter with a scancode mapping.
     *
     * @param[out] self    Instance to initialise (caller owns memory).
     * @param[in]  config  Scancode mapping — must not be NULL.
     *
     * @return ERR_OK           Success.
     * @return ERR_NULL_POINTER self or config is NULL.
     */
    Result_t PCKeyboardInputAdapter_Init(PCKeyboardInputAdapter_t *self,
                                         const PCKeyboardInputConfig_t *config);

    /**
     * @brief Return the IDigitalInputSource interface for DI injection.
     *
     * @return Pointer to interface, or NULL if self is NULL.
     */
    IDigitalInputSource *PCKeyboardInputAdapter_GetInterface(PCKeyboardInputAdapter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* PC_KEYBOARD_INPUT_ADAPTER_H */
