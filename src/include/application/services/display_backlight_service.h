/**
 * @file display_backlight_service.h
 * @brief Display backlight timeout service — Fase 4.
 *
 * Manages automatic screen-off after inactivity. Reads
 * `GeneralConfig_t.screen_blacklight_timeout_ms` every Update() call.
 * A value of 0 disables the timeout (backlight always on).
 *
 * Also reads DI_ID_ALARM_OVERTEMP: when an alarm is active the timeout
 * is inhibited so the operator always sees the alarm screen.
 *
 * Integration points:
 *  - InputRouter calls ResetTimer() (button active) or WakeUp() (screen asleep).
 *  - Control thread calls Update() periodically (~1 s is sufficient).
 *  - os_ticks_get() is used for absolute-time comparison (no period dependency).
 *
 * @note Zero lvgl.h — fully testable on PC.
 * @note Thread-safety: ResetTimer() and Update() may be called from different
 *       threads on single-core STM32U575 (Cortex-M33). Pointer writes are
 *       atomic; uint32_t tick reads/writes may not be atomic — acceptable
 *       race window is bounded to one Update() period.
 */
#ifndef INCLUDE_APPLICATION_SERVICES_DISPLAY_BACKLIGHT_SERVICE_H_
#define INCLUDE_APPLICATION_SERVICES_DISPLAY_BACKLIGHT_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_types.h"
#include "interfaces/i_display.h"
#include "interfaces/i_config_storage.h"
#include "interfaces/i_digital_input_source.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ======================================================================
     * CONFIGURATION
     * ====================================================================== */

    /**
     * @brief Initialization parameters for DisplayBacklightService.
     *
     * All pointers required (NULL → ERR_NULL_POINTER from Init).
     */
    typedef struct
    {
        I_Display *display;             /**< Display interface — SetBrightness() */
        IConfigStorage *config_storage; /**< For loading GeneralConfig on each Update() */
        IDigitalInputSource *di_source; /**< For reading DI_ID_ALARM_OVERTEMP */
        uint32_t poll_period_ms;        /**< Period at which Update() is called (informational only) */
    } DisplayBacklightServiceConfig_t;

    /* ======================================================================
     * INSTANCE
     * ====================================================================== */

    /**
     * @brief DisplayBacklightService static instance.
     *
     * Contents are private — zero-init before Init().
     */
    typedef struct
    {
        I_Display *display;
        IConfigStorage *config_storage;
        IDigitalInputSource *di_source;
        uint32_t last_activity_ticks; /**< Timestamp of last ResetTimer() */
        bool is_awake;                /**< true = backlight on */
    } DisplayBacklightService_t;

    /* ======================================================================
     * PUBLIC API
     * ====================================================================== */

    /**
     * @brief Initialize the backlight service.
     *
     * After init the backlight is ON (awake) and the activity timer starts.
     *
     * @param[in,out] self    Service instance (must not be NULL).
     * @param[in]     config  Configuration (all pointers must be non-NULL).
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if any mandatory pointer is NULL.
     */
    Result_t DisplayBacklightService_Init(DisplayBacklightService_t *self,
                                          const DisplayBacklightServiceConfig_t *config);

    /**
     * @brief Call periodically from the control thread (~1 s period).
     *
     * Reads `screen_blacklight_timeout_ms` from config and `DI_ID_ALARM_OVERTEMP`
     * from di_source. If the timeout expires and no alarm is active, turns off
     * the backlight via `Display_SetBrightness(0)`.
     *
     * A timeout value of 0 means "never turn off".
     *
     * @param[in,out] self  Service instance (NULL-safe — returns silently if NULL).
     */
    void DisplayBacklightService_Update(DisplayBacklightService_t *self);

    /**
     * @brief Reset the inactivity timer (called on any button press while awake).
     *
     * Does NOT wake the display — call WakeUp() for that.
     *
     * @param[in,out] self  Service instance (NULL-safe).
     */
    void DisplayBacklightService_ResetTimer(DisplayBacklightService_t *self);

    /**
     * @brief Wake the display (turn on brightness) without resetting the timer.
     *
     * Used by InputRouter when a button is pressed while the display is asleep:
     * the event is consumed (not forwarded to the screen) but the backlight
     * turns on so the user can see the current state.
     *
     * @param[in,out] self  Service instance (NULL-safe).
     */
    void DisplayBacklightService_WakeUp(DisplayBacklightService_t *self);

    /**
     * @brief Return true if the backlight is currently on.
     *
     * @param[in] self  Service instance (NULL → returns false).
     * @return true if awake, false if dimmed/off.
     */
    bool DisplayBacklightService_IsAwake(const DisplayBacklightService_t *self);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_APPLICATION_SERVICES_DISPLAY_BACKLIGHT_SERVICE_H_ */
