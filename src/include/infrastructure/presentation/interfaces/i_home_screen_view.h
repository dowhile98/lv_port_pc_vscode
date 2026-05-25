/**
 * @file i_home_screen_view.h
 * @brief View interface for the Home (operational) screen.
 *
 * Covers all home screen widgets:
 *   home_time (lv_label), home_wifi_status (lv_image), home_antenna_icon (lv_image),
 *   home_gps_status (lv_image, blink toggle), home_battery_level (lv_bar),
 *   home_cycle_status (lv_label), home_contact_type (lv_label),
 *   home_on/off/start/stop_time_value (lv_label), home_start_on_value (lv_label).
 *
 * @note Zero LVGL includes — fully testable on PC.
 * @note Thread-safety delegated to EezHomeScreenView via lv_lock().
 *
 * @author Tecna Smart Lab
 * @date   26 de Febrero 2026
 */
#ifndef I_HOME_SCREEN_VIEW_H
#define I_HOME_SCREEN_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief GPS status icon selector.
     *
     * Presenter decides which icon based on GPSFixStatus_t + blink tick.
     * View maps each id to the corresponding EEZ image descriptor:
     *   OFFLINE_1 → img_satellite_disconnect_1
     *   OFFLINE_2 → img_satellite_disconnect_2
     *   ONLINE    → img_satellite_online
     */
    typedef enum
    {
        HOME_GPS_ICON_OFFLINE_1 = 0, /**< No fix — blink frame 1 */
        HOME_GPS_ICON_OFFLINE_2 = 1, /**< No fix — blink frame 2 */
        HOME_GPS_ICON_ONLINE = 2,    /**< 3D fix acquired */
    } HomeGpsIconId_t;

    /**
     * @brief Antenna icon selector.
     *
     * Presenter decides which icon based on GPSConfig antenna_type + auto-switch service state.
     * View maps each id to the corresponding EEZ image descriptor:
     *   INTERNAL → img_antenna_internal (default)
     *   EXTERNAL → img_antenna_external
     *   FAILED   → img_antenna_failed (both antennas failed after auto-switch)
     */
    typedef enum
    {
        HOME_ANTENNA_ICON_INTERNAL = 0, /**< Internal antenna active */
        HOME_ANTENNA_ICON_EXTERNAL = 1, /**< External antenna active */
        HOME_ANTENNA_ICON_FAILED = 2,   /**< Both antennas failed (COOLDOWN state) */
    } HomeAntennaIconId_t;

    typedef struct IHomeScreenView_t IHomeScreenView_t;

    struct IHomeScreenView_t
    {
        /** @brief Update time label (e.g. "14:30:55"). */
        void (*SetTime)(IHomeScreenView_t *self, const char *time_str);

        /** @brief Update generic status line (legacy — kept for boot messages). */
        void (*SetLine1)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_wifi_status image icon. */
        void (*SetWifiStatus)(IHomeScreenView_t *self, bool connected);

        /**
         * @brief Update home_antenna_icon image.
         * @param[in] icon  Antenna icon selector (INTERNAL / EXTERNAL / FAILED).
         */
        void (*SetAntennaIcon)(IHomeScreenView_t *self, HomeAntennaIconId_t icon);

        /** @brief Update home_gps_status image with blink/online selector. */
        void (*SetGpsIcon)(IHomeScreenView_t *self, HomeGpsIconId_t icon);

        /**
         * @brief Update home_battery_level bar value.
         * @param[in] pct  0–100 percent. Pass -1 to indicate "unavailable".
         */
        void (*SetBatteryLevel)(IHomeScreenView_t *self, int32_t pct);

        /** @brief Update home_cycle_status label ("enabled" / "disabled"). */
        void (*SetCycleStatus)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_contact_type label ("Normally closed" / "Normally open"). */
        void (*SetContactType)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_on_time_value label (e.g. "0.700 s"). */
        void (*SetOnTime)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_off_time_value label (e.g. "0.300 s"). */
        void (*SetOffTime)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_start_time_value label ("HH:MM:SS"). */
        void (*SetStartTime)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_stop_time_value label ("HH:MM:SS"). */
        void (*SetStopTime)(IHomeScreenView_t *self, const char *text);

        /** @brief Update home_start_on_value label ("yes" / "no"). */
        void (*SetStartOn)(IHomeScreenView_t *self, const char *text);

        /**
         * @brief Update the live relay contact state indicator.
         *
         * @param[in] is_closed  true = contact CLOSED (current flowing);
         *                       false = contact OPEN.
         *
         * @note Called from OnUpdate when a relay state change event is pending.
         *       View implementation wraps lv_* with lv_lock().
         */
        void (*SetRelayState)(IHomeScreenView_t *self, bool is_closed);
    };

    /* ===== NULL-safe dispatch helpers ===== */

#define _IHSV_CALL_STR(fn, v, s) \
    do                           \
    {                            \
        if ((v) && (v)->fn)      \
            (v)->fn((v), (s));   \
    } while (0)
#define _IHSV_CALL_BOOL(fn, v, b) \
    do                            \
    {                             \
        if ((v) && (v)->fn)       \
            (v)->fn((v), (b));    \
    } while (0)
#define _IHSV_CALL_I32(fn, v, n) \
    do                           \
    {                            \
        if ((v) && (v)->fn)      \
            (v)->fn((v), (n));   \
    } while (0)
#define _IHSV_CALL_GPS(fn, v, id) \
    do                            \
    {                             \
        if ((v) && (v)->fn)       \
            (v)->fn((v), (id));   \
    } while (0)

    static inline void IHomeScreenView_SetTime(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetTime, v, s);
    }

    static inline void IHomeScreenView_SetLine1(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetLine1, v, s);
    }

    static inline void IHomeScreenView_SetWifiStatus(IHomeScreenView_t *v, bool connected)
    {
        _IHSV_CALL_BOOL(SetWifiStatus, v, connected);
    }

    static inline void IHomeScreenView_SetAntennaIcon(IHomeScreenView_t *v, HomeAntennaIconId_t icon)
    {
        if (v && v->SetAntennaIcon)
            v->SetAntennaIcon(v, icon);
    }

    static inline void IHomeScreenView_SetGpsIcon(IHomeScreenView_t *v, HomeGpsIconId_t icon)
    {
        _IHSV_CALL_GPS(SetGpsIcon, v, icon);
    }

    static inline void IHomeScreenView_SetBatteryLevel(IHomeScreenView_t *v, int32_t pct)
    {
        _IHSV_CALL_I32(SetBatteryLevel, v, pct);
    }

    static inline void IHomeScreenView_SetCycleStatus(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetCycleStatus, v, s);
    }

    static inline void IHomeScreenView_SetContactType(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetContactType, v, s);
    }

    static inline void IHomeScreenView_SetOnTime(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetOnTime, v, s);
    }

    static inline void IHomeScreenView_SetOffTime(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetOffTime, v, s);
    }

    static inline void IHomeScreenView_SetStartTime(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetStartTime, v, s);
    }

    static inline void IHomeScreenView_SetStopTime(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetStopTime, v, s);
    }

    static inline void IHomeScreenView_SetStartOn(IHomeScreenView_t *v, const char *s)
    {
        _IHSV_CALL_STR(SetStartOn, v, s);
    }

    static inline void IHomeScreenView_SetRelayState(IHomeScreenView_t *v, bool is_closed)
    {
        _IHSV_CALL_BOOL(SetRelayState, v, is_closed);
    }

#undef _IHSV_CALL_STR
#undef _IHSV_CALL_BOOL
#undef _IHSV_CALL_I32
#undef _IHSV_CALL_GPS

#ifdef __cplusplus
}
#endif

#endif /* I_HOME_SCREEN_VIEW_H */
