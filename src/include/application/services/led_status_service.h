/**
 * @file led_status_service.h
 * @brief LED Status Indicator Service — polling-based system state visualizer.
 *
 * Drives `led_status[0]` (LED0 on CICX1-V5, SIDE_LED on SSR3-V1) with
 * priority-encoded blink patterns reflecting the system's health:
 *
 *  Priority  Condition                       Pattern
 *  --------  ------------------------------  -----------------------------------
 *  1 (high)  Alarm high-temp active          100ms/100ms fast, continuous
 *  2         License expired                 3× 200ms pulses, 2 s pause
 *  3         Relay FSM error                 2× 200ms pulses, 1 s pause
 *  4         Relay active cycling            200ms ON / 800ms OFF, continuous
 *  5         Relay waiting (GPS synced)      Solid ON
 *  6         No GPS 3D fix                   500ms ON / 1500ms OFF, continuous
 *  7 (low)   Idle / Init                     200ms ON / 3800ms OFF (heartbeat)
 *
 * @note No HAL includes — fully testable on PC (mock OSAL + I_GPIO + interfaces).
 * @note Thread-safety: Update() must be called exclusively from Control Task.
 * @note OSAL dependency: os_ticks_get() is called internally in Update() — acceptable
 *       at Application layer (OSAL is a project-wide abstraction).
 *
 * @note Injected interfaces are all nullable; the service degrades gracefully:
 *       - relay  == NULL  → skips relay-related priority checks
 *       - gps    == NULL  → falls back to LED_STATUS_NO_GPS_SYNC (no fix assumed)
 *       - license== NULL  → skips license expiry check
 */

#ifndef INCLUDE_APPLICATION_SERVICES_LED_STATUS_SERVICE_H_
#define INCLUDE_APPLICATION_SERVICES_LED_STATUS_SERVICE_H_

#include <stdint.h>
#include <stdbool.h>
#include "hal/hal_types.h"
#include "hal/interfaces/i_gpio.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_license_status.h"
#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ======================================================================
     * PRIORITY STATE ENUM
     * ====================================================================== */

    /**
     * @brief Priority-ordered LED status states.
     *
     * Lower index = higher priority. The service evaluates conditions from
     * LED_STATUS_ALARM_HIGH_TEMP (0) downward and emits the first match.
     */
    typedef enum
    {
        LED_STATUS_ALARM_HIGH_TEMP = 0U, /**< Danger: relay forced open by over-temp alarm. */
        LED_STATUS_LICENSE_EXPIRED = 1U, /**< Error: rental license has expired.            */
        LED_STATUS_RELAY_ERROR = 2U,     /**< Error: relay FSM in unrecoverable error.      */
        LED_STATUS_RELAY_ACTIVE = 3U,    /**< Operating: relay executing interrupt cycles.  */
        LED_STATUS_RELAY_WAITING = 4U,   /**< Ready: GPS synced, waiting for time window.   */
        LED_STATUS_NO_GPS_SYNC = 5U,     /**< Searching: no valid GPS 3D fix.               */
        LED_STATUS_IDLE = 6U,            /**< Alive: system idle or still initialising.     */
        LED_STATUS_STATE_COUNT = 7U
    } LedStatusState_t;

    /* ======================================================================
     * BLINK PHASE (exposed for unit-test white-box inspection only)
     * ====================================================================== */

    /**
     * @brief Current phase of the blink state machine.
     */
    typedef enum
    {
        LED_BLINK_PHASE_ON = 0U,    /**< LED is ON; waiting for on_ms to elapse.         */
        LED_BLINK_PHASE_OFF = 1U,   /**< LED is OFF between pulses; waiting for off_ms.  */
        LED_BLINK_PHASE_PAUSE = 2U, /**< LED is OFF after a burst; waiting for pause_ms. */
    } LedBlinkPhase_t;

    /* ======================================================================
     * CONFIG STRUCT
     * ====================================================================== */

    /**
     * @brief Initialization parameters for LedStatusService.
     *
     * @note relay, gps, license are optional (nullable).  When NULL the
     *       corresponding priority levels are skipped.
     * @note gpio_iface must be non-NULL when led_enabled == true.
     */
    typedef struct
    {
        I_GPIO *gpio_iface;   /**< GPIO hardware interface.                         */
        GPIO_Port_t led_port; /**< GPIO port for led_status[0] (from BoardProfile). */
        GPIO_Pin_t led_pin;   /**< GPIO pin  for led_status[0] (from BoardProfile). */
        bool led_enabled;     /**< false → service init'd OK but LED stays silent.  */

        IRelayController *relay; /**< Relay state source   (nullable).                */
        IGPSSource *gps;         /**< GPS fix status source (nullable).               */
        ILicenseStatus *license; /**< License expiry source (nullable).               */
        ILogger *logger;         /**< Logger for debug output (optional, can be NULL). */
    } LedStatusServiceConfig_t;

    /* ======================================================================
     * INSTANCE STRUCT
     * ====================================================================== */

    /**
     * @brief LedStatusService instance.
     *
     * Zero-init the struct before calling LedStatusService_Init().
     * All fields are private — do not access directly.
     */
    typedef struct
    {
        /* Injected hardware */
        I_GPIO *gpio_iface;
        GPIO_Port_t led_port;
        GPIO_Pin_t led_pin;
        bool led_enabled;

        /* Injected domain interfaces */
        IRelayController *relay;
        IGPSSource *gps;
        ILicenseStatus *license;
        ILogger *logger; /**< Optional logger (can be NULL). */

        /* Blink state machine */
        LedStatusState_t current_state;
        LedBlinkPhase_t blink_phase;
        uint8_t pulse_count;
        uint32_t phase_deadline_ms;

        bool is_initialized;
    } LedStatusService_t;

    /* ======================================================================
     * PUBLIC API
     * ====================================================================== */

    /**
     * @brief Initialize the LED status service.
     *
     * Drives led_status[0] to off and arms the state machine in IDLE state.
     *
     * @param[in,out] self    Service instance (must not be NULL; zero-init before call).
     * @param[in]     config  Initialization parameters (must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER    if self or config is NULL.
     * @return ERR_INVALID_PARAM   if led_enabled == true but gpio_iface is NULL.
     */
    Result_t LedStatusService_Init(LedStatusService_t *self, const LedStatusServiceConfig_t *config);

    /**
     * @brief Periodic update — evaluate priority state and advance blink machine.
     *
     * Must be called at ≤ 20 ms intervals for accurate blink timing.
     * Internally calls os_ticks_get() for absolute-time comparison.
     *
     * @param[in,out] self  Initialized service instance. Silently ignores NULL.
     *
     * @note Thread: ControlTask only. Not re-entrant.
     */
    void LedStatusService_Update(LedStatusService_t *self);

    /**
     * @brief Returns the currently active priority state (diagnostics / testing).
     *
     * @param[in] self  Service instance.
     * @return Current LedStatusState_t, or LED_STATUS_IDLE when not initialized.
     */
    LedStatusState_t LedStatusService_GetCurrentState(const LedStatusService_t *self);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_APPLICATION_SERVICES_LED_STATUS_SERVICE_H_ */
