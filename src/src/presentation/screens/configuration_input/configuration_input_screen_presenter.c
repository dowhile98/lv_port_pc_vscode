/**
 * @file configuration_input_screen_presenter.c
 * @brief ConfigurationInput HH:MM:SS time-field editor presenter — GREEN implementation.
 *
 * Allows editing a single DateTime_t (start_time or stop_time) from
 * TimeWindowConfig_t stored in IModularConfigStorage.
 *
 * Field navigation: HH → MM → SS. ENTER on SS saves and navigates back.
 * Active field blinks every CONFIG_INPUT_BLINK_TICKS OnUpdate calls.
 * UP/DOWN increment/decrement the active field with wrap-around.
 * OnBackPressed discards edits and navigates back without saving.
 *
 * @note NO #include "lvgl.h" — intentional.  LVGL access goes through view.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
/* NO #include "lvgl.h" — intentional */
#include "presentation/screens/configuration_input/configuration_input_screen_presenter.h"
#include "presentation/interfaces/i_configuration_input_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_config_storage.h"
#include "common/relay_types.h"
#include "common/general_types.h"
#include "common/gps_types.h"
#include "common/wifi_types.h"
#include "domain/time/utc_offsets.h"
#include <lwprintf/lwprintf.h>
#include <osal/osal.h>
#include <string.h>

/*============================================================================*
 * PRIVATE — constants
 *============================================================================*/

#define VALUE_BUF_SIZE 16U /**< "HH:MM:SS\0" = 9 bytes — 16 is safe. */
#define HH_MAX 23U
#define MM_MAX 59U
#define SS_MAX 59U

/* Private field-index constants used by the HH:MM:SS drivers.
 * current_field == 0 → hours, 1 → minutes, 2 → seconds. */
#define INPUT_FIELD_HH 0U
#define INPUT_FIELD_MM 1U
#define INPUT_FIELD_SS 2U

/*============================================================================*
 * PRIVATE — formatting helpers
 *============================================================================*/

/**
 * @brief HH:MM:SS driver — format edit_value.time as "HH:MM:SS" with active-field blink.
 *
 * When blink_on == 0 the active sub-field is replaced with "__".
 * Called via driver->format_fn(self).
 */
static void time_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const DateTime_t *t = &self->edit_value.time;

    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u:%02u:%02u",
                                (unsigned)t->hour, (unsigned)t->minute, (unsigned)t->second);
    }
    else
    {
        switch (self->current_field)
        {
        case INPUT_FIELD_HH:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "__:%02u:%02u",
                                    (unsigned)t->minute, (unsigned)t->second);
            break;

        case INPUT_FIELD_MM:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u:__:%02u",
                                    (unsigned)t->hour, (unsigned)t->second);
            break;

        case INPUT_FIELD_SS:
        default:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u:%02u:__",
                                    (unsigned)t->hour, (unsigned)t->minute);
            break;
        }
    }

    IConfigurationInputScreenView_SetValue(self->view, buf);
}

/*============================================================================*
 * PRIVATE — field arithmetic
 *============================================================================*/

/** HH:MM:SS driver — increment the sub-field at current_field with wrap-around. */
static void time_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case INPUT_FIELD_HH:
        self->edit_value.time.hour = (self->edit_value.time.hour >= HH_MAX)
                                         ? 0U
                                         : (uint8_t)(self->edit_value.time.hour + 1U);
        break;

    case INPUT_FIELD_MM:
        self->edit_value.time.minute = (self->edit_value.time.minute >= MM_MAX)
                                           ? 0U
                                           : (uint8_t)(self->edit_value.time.minute + 1U);
        break;

    case INPUT_FIELD_SS:
    default:
        self->edit_value.time.second = (self->edit_value.time.second >= SS_MAX)
                                           ? 0U
                                           : (uint8_t)(self->edit_value.time.second + 1U);
        break;
    }
}

/** HH:MM:SS driver — decrement the sub-field at current_field with wrap-around. */
static void time_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case INPUT_FIELD_HH:
        self->edit_value.time.hour = (self->edit_value.time.hour == 0U)
                                         ? HH_MAX
                                         : (uint8_t)(self->edit_value.time.hour - 1U);
        break;

    case INPUT_FIELD_MM:
        self->edit_value.time.minute = (self->edit_value.time.minute == 0U)
                                           ? MM_MAX
                                           : (uint8_t)(self->edit_value.time.minute - 1U);
        break;

    case INPUT_FIELD_SS:
    default:
        self->edit_value.time.second = (self->edit_value.time.second == 0U)
                                           ? SS_MAX
                                           : (uint8_t)(self->edit_value.time.second - 1U);
        break;
    }
}

/*============================================================================*
 * PRIVATE — save helper
 *============================================================================*/

/*============================================================================*
 * PRIVATE — navigation helper
 *============================================================================*/

static void navigate_back(ConfigurationInputScreenPresenter_t *self)
{
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

/*============================================================================*
 * PRIVATE — HH:MM:SS driver: load / save functions
 *
 * start_time and stop_time share the same format/increment/decrement functions
 * (same HH:MM:SS logic) but each has its own load_fn and save_fn.
 *============================================================================*/

static void time_load_start_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    self->edit_value.time = cfg.time_window.start_time;
}

static void time_load_stop_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    self->edit_value.time = cfg.time_window.stop_time;
}

static void time_save_start_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    /* Load first to preserve stop_time and weekday_mask */

    /*get start_time from edit_value and save to storage*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    /*update start_time with edited value*/
    cfg.time_window.start_time = self->edit_value.time;
    /*save updated config back to storage*/
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

static void time_save_stop_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    /*get start_time from edit_value and save to storage*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    /*update start_time with edited value*/
    cfg.time_window.stop_time = self->edit_value.time;
    /*save updated config back to storage*/
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

/*============================================================================*
 * PRIVATE — static driver table
 *
 * Index == ConfigInputParam_t value.
 * To add a new parameter:
 *   1. Add an entry to ConfigInputParam_t (before CONFIG_INPUT_PARAM_COUNT).
 *   2. Implement the five driver functions.
 *   3. Add one entry below.
 *   No changes to the generic presenter loop are required.
 *============================================================================*/

/*---------------------------------------------------------------------------- *
 * MILLIS driver — On time / Off time
 * Stores uint32_t milliseconds (displayed as "X.XXXs").
 * Min: 50 ms, Max: 60000 ms (60 s).
 * Step: accelerated based on how long the button has been held (×5 per level).
 *----------------------------------------------------------------------------*/
#define MILLIS_MIN_MS 50U
#define MILLIS_MAX_MS 60000U

/**
 * @brief Computes the increment/decrement step for the millis driver based on
 *        how long the UP or DOWN button has been continuously held.
 *
 * Acceleration table (×5 per level):
 *   0   –  800 ms  →    1 ms
 *   800 – 2000 ms  →    5 ms
 *  2000 – 4000 ms  →   25 ms
 *  4000 – 7000 ms  →  125 ms
 *  > 7000 ms       → 1000 ms  (max, stays constant)
 *
 * @param press_tick  Value of up_press_tick / down_press_tick set on PRESS event.
 * @return Step in milliseconds.
 */
static uint32_t millis_compute_step(uint32_t press_tick)
{
    const uint32_t hold_ms = os_ticks_get() - press_tick;

    if (hold_ms < 800U)
    {
        return 1U;
    }
    if (hold_ms < 2000U)
    {
        return 5U;
    }
    if (hold_ms < 4000U)
    {
        return 25U;
    }
    if (hold_ms < 7000U)
    {
        return 125U;
    }
    return 1000U;
}

static void millis_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%.3f s",
                                (float)self->edit_value.millis / 1000.0f);
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "_.___s");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void millis_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    const uint32_t step = millis_compute_step(self->up_press_tick);

    if (self->edit_value.millis + step <= MILLIS_MAX_MS)
    {
        self->edit_value.millis += step;
    }
    else
    {
        self->edit_value.millis = MILLIS_MIN_MS;
    }
}

static void millis_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    const uint32_t step = millis_compute_step(self->down_press_tick);

    if (self->edit_value.millis >= MILLIS_MIN_MS + step)
    {
        self->edit_value.millis -= step;
    }
    else
    {
        self->edit_value.millis = MILLIS_MAX_MS;
    }
}

/* On time: RelayConfig_t.multicycle.ton[cycle_idx] */
static void on_time_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    uint32_t val = 0U;
    memset(&cfg, 0, sizeof(cfg));
    /*load configuration*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);

    if (cfg.multicycle.enabled == 0U)
    {
        /*single cycle*/
        val = cfg.simple_cycle.ton;
    }
    else
    {
        /*multiple cycles*/
        const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                                ? self->cycle_idx
                                : 0U;
        val = cfg.multicycle.ton[idx];
    }

    self->edit_value.millis = (val > 0U) ? val : MILLIS_MIN_MS;
}

static void on_time_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);

    if (cfg.multicycle.enabled == 0U)
    {
        cfg.simple_cycle.ton = self->edit_value.millis;
    }
    else
    {
        const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                                ? self->cycle_idx
                                : 0U;
        cfg.multicycle.ton[idx] = self->edit_value.millis;
    }
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

/* Off time: RelayConfig_t.multicycle.toff[cycle_idx] */
static void off_time_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    uint32_t val = 0U;

    memset(&cfg, 0, sizeof(cfg));
    /*load configuration*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);

    if (cfg.multicycle.enabled == 0U)
    {
        val = cfg.simple_cycle.toff;
    }
    else
    {
        const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                                ? self->cycle_idx
                                : 0U;
        val = cfg.multicycle.toff[idx];
    }

    self->edit_value.millis = (val > 0U) ? val : MILLIS_MIN_MS;
}

static void off_time_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    /*load configuration*/
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);

    if (cfg.multicycle.enabled == 0U)
    {
        cfg.simple_cycle.toff = self->edit_value.millis;
    }
    else
    {
        const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                                ? self->cycle_idx
                                : 0U;
        cfg.multicycle.toff[idx] = self->edit_value.millis;
    }
    /*save configuration*/
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

/*---------------------------------------------------------------------------- *
 * END DATE driver — boundary_dates[cycle_idx] (MM/DD/YYYY, 3 fields, English)
 * field 0 = month (1..12), field 1 = day (1..31), field 2 = year (2025..2099).
 *----------------------------------------------------------------------------*/
#define END_DATE_FIELD_MONTH 0U
#define END_DATE_FIELD_DAY 1U
#define END_DATE_FIELD_YEAR 2U
#define END_DATE_MONTH_MIN 1U
#define END_DATE_MONTH_MAX 12U
#define END_DATE_DAY_MIN 1U
#define END_DATE_DAY_MAX 31U
#define END_DATE_YEAR_MIN 2025U
#define END_DATE_YEAR_MAX 2099U

static void end_date_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const uint8_t m = (uint8_t)self->edit_value.time.month;
    const uint8_t d = (uint8_t)self->edit_value.time.day;
    const uint16_t y = self->edit_value.time.year;

    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/%02u/%04u",
                                (unsigned)m, (unsigned)d, (unsigned)y);
    }
    else
    {
        switch (self->current_field)
        {
        case END_DATE_FIELD_MONTH:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "__/%02u/%04u",
                                    (unsigned)d, (unsigned)y);
            break;
        case END_DATE_FIELD_DAY:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/__/%04u",
                                    (unsigned)m, (unsigned)y);
            break;
        case END_DATE_FIELD_YEAR:
        default:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/%02u/____",
                                    (unsigned)m, (unsigned)d);
            break;
        }
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void end_date_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case END_DATE_FIELD_MONTH:
    {
        const uint8_t next = (uint8_t)(self->edit_value.time.month + 1U);
        self->edit_value.time.month = (next > END_DATE_MONTH_MAX) ? END_DATE_MONTH_MIN : next;
        break;
    }
    case END_DATE_FIELD_DAY:
    {
        const uint8_t next = (uint8_t)(self->edit_value.time.day + 1U);
        self->edit_value.time.day = (next > END_DATE_DAY_MAX) ? END_DATE_DAY_MIN : next;
        break;
    }
    case END_DATE_FIELD_YEAR:
    default:
    {
        const uint16_t next = (uint16_t)(self->edit_value.time.year + 1U);
        self->edit_value.time.year = (next > (uint16_t)END_DATE_YEAR_MAX)
                                         ? (uint16_t)END_DATE_YEAR_MIN
                                         : next;
        break;
    }
    }
}

static void end_date_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case END_DATE_FIELD_MONTH:
    {
        const uint8_t prev = (uint8_t)(self->edit_value.time.month - 1U);
        self->edit_value.time.month = (self->edit_value.time.month <= END_DATE_MONTH_MIN)
                                          ? END_DATE_MONTH_MAX
                                          : prev;
        break;
    }
    case END_DATE_FIELD_DAY:
    {
        const uint8_t prev = (uint8_t)(self->edit_value.time.day - 1U);
        self->edit_value.time.day = (self->edit_value.time.day <= END_DATE_DAY_MIN)
                                        ? END_DATE_DAY_MAX
                                        : prev;
        break;
    }
    case END_DATE_FIELD_YEAR:
    default:
    {
        const uint16_t prev = (uint16_t)(self->edit_value.time.year - 1U);
        self->edit_value.time.year = (self->edit_value.time.year <= (uint16_t)END_DATE_YEAR_MIN)
                                         ? (uint16_t)END_DATE_YEAR_MAX
                                         : prev;
        break;
    }
    }
}

static void end_date_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                            ? self->cycle_idx
                            : 0U;
    self->edit_value.time = cfg.multicycle.boundary_dates[idx];
    /* Guard: ensure valid defaults */
    if (self->edit_value.time.month == 0U)
    {
        self->edit_value.time.month = 1U;
    }
    if (self->edit_value.time.day == 0U)
    {
        self->edit_value.time.day = 1U;
    }
    if (self->edit_value.time.year < (uint16_t)END_DATE_YEAR_MIN)
    {
        self->edit_value.time.year = (uint16_t)END_DATE_YEAR_MIN;
    }
}

static void end_date_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    const uint8_t idx = (self->cycle_idx < (uint8_t)RELAY_MAX_CYCLES)
                            ? self->cycle_idx
                            : 0U;
    cfg.multicycle.boundary_dates[idx].month = self->edit_value.time.month;
    cfg.multicycle.boundary_dates[idx].day = self->edit_value.time.day;
    cfg.multicycle.boundary_dates[idx].year = self->edit_value.time.year;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

/*---------------------------------------------------------------------------- *
 * GPS TIME OFFSET driver — GPSConfig_t.time_offset (-10..+10 seconds).
 * Single field, signed integer displayed as "+N s" / "-N s".
 *----------------------------------------------------------------------------*/
#define GPS_TIME_OFFSET_MIN (-10)
#define GPS_TIME_OFFSET_MAX (10)

static void gps_time_offset_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const int8_t val = self->edit_value.signed_int;

    if (self->blink_on != 0U)
    {
        if (val >= 0)
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "+%d s", (int)val);
        }
        else
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%d s", (int)val);
        }
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "__ s");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void gps_time_offset_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.signed_int < (int8_t)GPS_TIME_OFFSET_MAX)
    {
        self->edit_value.signed_int++;
    }
}

static void gps_time_offset_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.signed_int > (int8_t)GPS_TIME_OFFSET_MIN)
    {
        self->edit_value.signed_int--;
    }
}

static void gps_time_offset_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    self->edit_value.signed_int = cfg.time_offset;
}

static void gps_time_offset_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    cfg.time_offset = self->edit_value.signed_int;
    (void)ConfigStorage_SaveGPSConfig(self->config_storage, &cfg);
}

/*---------------------------------------------------------------------------- *
 * ANTENNA SWITCH TIMEOUT driver — GPSConfig_t.antenna_switch_timeout_min (0-60 min).
 * Single field; 0 = disabled, 1-60 = timeout in minutes. Display: "X min" or "disabled".
 *----------------------------------------------------------------------------*/
#define ANTENNA_SWITCH_TIMEOUT_MIN 0U
#define ANTENNA_SWITCH_TIMEOUT_MAX 60U
#define ANTENNA_SWITCH_TIMEOUT_DEFAULT 10U

static void antenna_switch_timeout_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const uint8_t val = self->edit_value.byte_val;

    if (self->blink_on != 0U)
    {
        if (val == 0U)
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "disabled");
        }
        else
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%u min", (unsigned)val);
        }
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "__ min");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void antenna_switch_timeout_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val < ANTENNA_SWITCH_TIMEOUT_MAX)
    {
        self->edit_value.byte_val++;
    }
    else
    {
        self->edit_value.byte_val = 0U; /* wrap to disabled */
    }
}

static void antenna_switch_timeout_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val > 0U)
    {
        self->edit_value.byte_val--;
    }
    else
    {
        self->edit_value.byte_val = ANTENNA_SWITCH_TIMEOUT_MAX; /* wrap to max */
    }
}

static void antenna_switch_timeout_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    /* Clamp to valid range (0 or 1-30) */
    if (cfg.antenna_switch_timeout_min > ANTENNA_SWITCH_TIMEOUT_MAX)
    {
        self->edit_value.byte_val = ANTENNA_SWITCH_TIMEOUT_DEFAULT;
    }
    else
    {
        self->edit_value.byte_val = cfg.antenna_switch_timeout_min;
    }
}

static void antenna_switch_timeout_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    cfg.antenna_switch_timeout_min = self->edit_value.byte_val;
    (void)ConfigStorage_SaveGPSConfig(self->config_storage, &cfg);
}

/*---------------------------------------------------------------------------- *
 * UTC OFFSET INDEX driver — GPSConfig_t.utc_offset_index (0..UTC_OFFSET_COUNT-1).
 * Single field; display shows "UTC±H.D" using the UTC_OFFSETS[] lookup table.
 *----------------------------------------------------------------------------*/

/**
 * @brief Format a float UTC offset (in hours) as "UTC±H.D" into buf.
 * @note  Avoids %%f / float printf — uses integer quarter-hour arithmetic.
 *        All values in UTC_OFFSETS[] are exact multiples of 0.25h.
 */
static void format_utc_hours(char *buf, uint16_t buf_size, float hours)
{
    /* Multiply by 4 to obtain integer quarter-hours; handle sign explicitly. */
    int32_t total_q;
    if (hours < 0.0f)
    {
        total_q = -(int32_t)((-hours) * 4.0f + 0.001f); /* rounding guard */
    }
    else
    {
        total_q = (int32_t)(hours * 4.0f + 0.001f);
    }
    const int32_t abs_q = total_q < 0 ? -total_q : total_q;
    const int32_t h = abs_q / 4;
    const int32_t frac_q = abs_q % 4; /* 0=.0  1=.25  2=.5  3=.75 */
    const char sign_c = (total_q < 0) ? '-' : '+';

    switch (frac_q)
    {
    case 1:
        (void)lwprintf_snprintf(buf, buf_size, "UTC%c%d.25", sign_c, (int)h);
        break;
    case 2:
        (void)lwprintf_snprintf(buf, buf_size, "UTC%c%d.5", sign_c, (int)h);
        break;
    case 3:
        (void)lwprintf_snprintf(buf, buf_size, "UTC%c%d.75", sign_c, (int)h);
        break;
    default: /* 0 */
        (void)lwprintf_snprintf(buf, buf_size, "UTC%c%d.0", sign_c, (int)h);
        break;
    }
}

static void utc_offset_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const uint8_t idx = self->edit_value.byte_val;

    if (self->blink_on != 0U)
    {
        if (idx < (uint8_t)UTC_OFFSET_COUNT)
        {
            format_utc_hours(buf, VALUE_BUF_SIZE, UTC_OFFSETS[idx]);
        }
        else
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "UTC?");
        }
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "UTC___");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void utc_offset_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val < (uint8_t)(UTC_OFFSET_COUNT - 1U))
    {
        self->edit_value.byte_val++;
    }
}

static void utc_offset_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val > 0U)
    {
        self->edit_value.byte_val--;
    }
}

static void utc_offset_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    /* Clamp to valid range */
    self->edit_value.byte_val = (cfg.utc_offset_index < (uint8_t)UTC_OFFSET_COUNT)
                                    ? cfg.utc_offset_index
                                    : 14U; /* default UTC+0 */
}

static void utc_offset_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    GPSConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGPSConfig(self->config_storage, &cfg);
    cfg.utc_offset_index = self->edit_value.byte_val;
    (void)ConfigStorage_SaveGPSConfig(self->config_storage, &cfg);
}

/*============================================================================*
 * MARGIN MS driver — ton_margin_ms / toff_margin_ms (0..50 ms, step 1 ms).
 * Single field; display shows "X ms" (integer, no decimals).
 * Shared format/increment/decrement; load/save differ only by which field.
 *============================================================================*/
#define MARGIN_MS_MIN 0U
#define MARGIN_MS_MAX 50U
#define MARGIN_MS_STEP 1U

static void margin_ms_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%lu ms",
                                (unsigned long)self->edit_value.millis);
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "_ ms");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void margin_ms_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis < MARGIN_MS_MAX)
    {
        self->edit_value.millis += MARGIN_MS_STEP;
    }
    else
    {
        self->edit_value.millis = MARGIN_MS_MAX;
    }
}

static void margin_ms_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis >= MARGIN_MS_MIN + MARGIN_MS_STEP)
    {
        self->edit_value.millis -= MARGIN_MS_STEP;
    }
    else
    {
        self->edit_value.millis = MARGIN_MS_MIN;
    }
}

static void ton_margin_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    self->edit_value.millis =
        (cfg.ton_margin_ms <= MARGIN_MS_MAX) ? cfg.ton_margin_ms : 0U;
}

static void ton_margin_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    cfg.ton_margin_ms = self->edit_value.millis;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

static void toff_margin_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    self->edit_value.millis =
        (cfg.toff_margin_ms <= MARGIN_MS_MAX) ? cfg.toff_margin_ms : 0U;
}

static void toff_margin_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    RelayConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadRelayConfig(self->config_storage, &cfg);
    cfg.toff_margin_ms = self->edit_value.millis;
    (void)ConfigStorage_SaveRelayConfig(self->config_storage, &cfg);
}

/*============================================================================*
 * SCREEN TIMEOUT driver — screen_blacklight_timeout_ms (15..100 s, step 5 s).
 * Value stored as ms internally; displayed and edited in whole seconds.
 * edit_value.millis holds the raw ms value (15000..100000, step 5000).
 *============================================================================*/
#define SCREEN_TIMEOUT_MIN_MS 10000U  /**< 10 s in ms */
#define SCREEN_TIMEOUT_MAX_MS 100000U /**< 100 s in ms */
#define SCREEN_TIMEOUT_STEP_MS 5000U  /**< 5 s step in ms */

static void screen_timeout_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%lu s",
                                (unsigned long)(self->edit_value.millis / 1000UL));
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "_ s");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void screen_timeout_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis < SCREEN_TIMEOUT_MAX_MS)
    {
        self->edit_value.millis += SCREEN_TIMEOUT_STEP_MS;
    }
    else
    {
        self->edit_value.millis = SCREEN_TIMEOUT_MIN_MS;
    }
}

static void screen_timeout_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis >= SCREEN_TIMEOUT_MIN_MS + SCREEN_TIMEOUT_STEP_MS)
    {
        self->edit_value.millis -= SCREEN_TIMEOUT_STEP_MS;
    }
    else
    {
        self->edit_value.millis = SCREEN_TIMEOUT_MAX_MS;
    }
}

static void screen_timeout_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    GeneralConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    /* Clamp to valid range; default to minimum if out of range */
    if (cfg.screen_blacklight_timeout_ms >= SCREEN_TIMEOUT_MIN_MS &&
        cfg.screen_blacklight_timeout_ms <= SCREEN_TIMEOUT_MAX_MS)
    {
        self->edit_value.millis = cfg.screen_blacklight_timeout_ms;
    }
    else
    {
        self->edit_value.millis = SCREEN_TIMEOUT_MIN_MS;
    }
}

static void screen_timeout_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    GeneralConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    cfg.screen_blacklight_timeout_ms = self->edit_value.millis;
    (void)ConfigStorage_SaveGeneralConfig(self->config_storage, &cfg);
}

/*============================================================================*
 * BUZZER ON TIME driver — buzzer_on_time_ms (40..100 ms, step 1 ms).
 * Single field; display shows "X ms" (integer ms).
 *============================================================================*/
#define BUZZER_ON_MIN_MS 40U
#define BUZZER_ON_MAX_MS 100U
#define BUZZER_ON_STEP_MS 1U

static void buzzer_on_time_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%lu ms",
                                (unsigned long)self->edit_value.millis);
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "_ ms");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void buzzer_on_time_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis < BUZZER_ON_MAX_MS)
    {
        self->edit_value.millis += BUZZER_ON_STEP_MS;
    }
    else
    {
        self->edit_value.millis = BUZZER_ON_MAX_MS;
    }
}

static void buzzer_on_time_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.millis >= BUZZER_ON_MIN_MS + BUZZER_ON_STEP_MS)
    {
        self->edit_value.millis -= BUZZER_ON_STEP_MS;
    }
    else
    {
        self->edit_value.millis = BUZZER_ON_MIN_MS;
    }
}

static void buzzer_on_time_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    GeneralConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    self->edit_value.millis = (cfg.buzzer_on_time_ms >= BUZZER_ON_MIN_MS &&
                               cfg.buzzer_on_time_ms <= BUZZER_ON_MAX_MS)
                                  ? cfg.buzzer_on_time_ms
                                  : BUZZER_ON_MIN_MS;
}

static void buzzer_on_time_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    GeneralConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);
    cfg.buzzer_on_time_ms = self->edit_value.millis;
    (void)ConfigStorage_SaveGeneralConfig(self->config_storage, &cfg);
}

/*---------------------------------------------------------------------------- *
 * EXPIRATION DATE driver — SuperUserConfig_t.expiration_date (DD/MM/YYYY → unix epoch).
 * field 0 = day (1..31), field 1 = month (1..12), field 2 = year (2025..2099).
 * On save: converts DD/MM/YYYY at 23:59:59 to unix epoch via DateTime_ToUnix().
 *----------------------------------------------------------------------------*/
#define EXPIR_DATE_FIELD_DAY 0U
#define EXPIR_DATE_FIELD_MONTH 1U
#define EXPIR_DATE_FIELD_YEAR 2U
#define EXPIR_DATE_DAY_MIN 1U
#define EXPIR_DATE_DAY_MAX 31U
#define EXPIR_DATE_MONTH_MIN 1U
#define EXPIR_DATE_MONTH_MAX 12U
#define EXPIR_DATE_YEAR_MIN 2025U
#define EXPIR_DATE_YEAR_MAX 2099U

#include "common/date_time.h"

static void expiration_date_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const uint8_t d = (uint8_t)self->edit_value.time.day;
    const uint8_t m = (uint8_t)self->edit_value.time.month;
    const uint16_t y = self->edit_value.time.year;

    if (self->blink_on != 0U)
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/%02u/%04u",
                                (unsigned)d, (unsigned)m, (unsigned)y);
    }
    else
    {
        switch (self->current_field)
        {
        case EXPIR_DATE_FIELD_DAY:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "__/%02u/%04u",
                                    (unsigned)m, (unsigned)y);
            break;
        case EXPIR_DATE_FIELD_MONTH:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/__/%04u",
                                    (unsigned)d, (unsigned)y);
            break;
        case EXPIR_DATE_FIELD_YEAR:
        default:
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%02u/%02u/____",
                                    (unsigned)d, (unsigned)m);
            break;
        }
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void expiration_date_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case EXPIR_DATE_FIELD_DAY:
    {
        const uint8_t next = (uint8_t)(self->edit_value.time.day + 1U);
        self->edit_value.time.day = (next > EXPIR_DATE_DAY_MAX) ? EXPIR_DATE_DAY_MIN : next;
        break;
    }
    case EXPIR_DATE_FIELD_MONTH:
    {
        const uint8_t next = (uint8_t)(self->edit_value.time.month + 1U);
        self->edit_value.time.month = (next > EXPIR_DATE_MONTH_MAX) ? EXPIR_DATE_MONTH_MIN : next;
        break;
    }
    case EXPIR_DATE_FIELD_YEAR:
    default:
    {
        const uint16_t next = (uint16_t)(self->edit_value.time.year + 1U);
        self->edit_value.time.year =
            (next > (uint16_t)EXPIR_DATE_YEAR_MAX) ? (uint16_t)EXPIR_DATE_YEAR_MIN : next;
        break;
    }
    }
}

static void expiration_date_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    switch (self->current_field)
    {
    case EXPIR_DATE_FIELD_DAY:
    {
        self->edit_value.time.day =
            (self->edit_value.time.day <= EXPIR_DATE_DAY_MIN)
                ? EXPIR_DATE_DAY_MAX
                : (uint8_t)(self->edit_value.time.day - 1U);
        break;
    }
    case EXPIR_DATE_FIELD_MONTH:
    {
        self->edit_value.time.month =
            (self->edit_value.time.month <= EXPIR_DATE_MONTH_MIN)
                ? EXPIR_DATE_MONTH_MAX
                : (uint8_t)(self->edit_value.time.month - 1U);
        break;
    }
    case EXPIR_DATE_FIELD_YEAR:
    default:
    {
        self->edit_value.time.year =
            (self->edit_value.time.year <= (uint16_t)EXPIR_DATE_YEAR_MIN)
                ? (uint16_t)EXPIR_DATE_YEAR_MAX
                : (uint16_t)(self->edit_value.time.year - 1U);
        break;
    }
    }
}

static void expiration_date_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    SuperUserConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadSuperUserConfig(self->config_storage, &cfg);

    DateTime_t dt;
    DateTime_FromUnix(cfg.expiration_date, &dt);

    self->edit_value.time.day = dt.day;
    self->edit_value.time.month = dt.month;
    self->edit_value.time.year = dt.year;

    /* Guard: ensure valid defaults if epoch was 0 */
    if (self->edit_value.time.day == 0U)
    {
        self->edit_value.time.day = 1U;
    }
    if (self->edit_value.time.month == 0U)
    {
        self->edit_value.time.month = 1U;
    }
    if (self->edit_value.time.year < (uint16_t)EXPIR_DATE_YEAR_MIN)
    {
        self->edit_value.time.year = (uint16_t)EXPIR_DATE_YEAR_MIN;
    }
}

static void expiration_date_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    SuperUserConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadSuperUserConfig(self->config_storage, &cfg);

    /* Convert DD/MM/YYYY at 23:59:59 to unix epoch. */
    DateTime_t dt;
    memset(&dt, 0, sizeof(dt));
    dt.day = self->edit_value.time.day;
    dt.month = self->edit_value.time.month;
    dt.year = self->edit_value.time.year;
    dt.hour = 23U;
    dt.minute = 59U;
    dt.second = 59U;

    cfg.expiration_date = DateTime_ToUnix(&dt);
    (void)ConfigStorage_SaveSuperUserConfig(self->config_storage, &cfg);
}

/*============================================================================*
 * WIFI TIMEOUT driver — WifiConfig_t.wifi_timeout_minutes (0-60 min).
 * 0 = always on, 1-60 = auto-disable after N minutes. Display: "always on" or "X min".
 *============================================================================*/
#define WIFI_TIMEOUT_MIN 0U
#define WIFI_TIMEOUT_MAX 60U

static void wifi_timeout_format_fn(ConfigurationInputScreenPresenter_t *self)
{
    char buf[VALUE_BUF_SIZE];
    const uint8_t val = self->edit_value.byte_val;

    if (self->blink_on != 0U)
    {
        if (val == 0U)
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "always on");
        }
        else
        {
            (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "%u min", (unsigned)val);
        }
    }
    else
    {
        (void)lwprintf_snprintf(buf, VALUE_BUF_SIZE, "_________");
    }
    IConfigurationInputScreenView_SetValue(self->view, buf);
}

static void wifi_timeout_increment_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val < WIFI_TIMEOUT_MAX)
    {
        self->edit_value.byte_val++;
    }
    else
    {
        self->edit_value.byte_val = WIFI_TIMEOUT_MIN;
    }
}

static void wifi_timeout_decrement_fn(ConfigurationInputScreenPresenter_t *self)
{
    if (self->edit_value.byte_val > WIFI_TIMEOUT_MIN)
    {
        self->edit_value.byte_val--;
    }
    else
    {
        self->edit_value.byte_val = WIFI_TIMEOUT_MAX;
    }
}

static void wifi_timeout_load_fn(ConfigurationInputScreenPresenter_t *self)
{
    WifiConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadWifiConfig(self->config_storage, &cfg);
    self->edit_value.byte_val = (cfg.wifi_timeout_minutes <= WIFI_TIMEOUT_MAX)
                                    ? cfg.wifi_timeout_minutes
                                    : WIFI_TIMEOUT_MIN;
}

static void wifi_timeout_save_fn(ConfigurationInputScreenPresenter_t *self)
{
    WifiConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadWifiConfig(self->config_storage, &cfg);
    cfg.wifi_timeout_minutes = self->edit_value.byte_val;
    (void)ConfigStorage_SaveWifiConfig(self->config_storage, &cfg);
}

/*============================================================================*/

static const ConfigInputDriver_t s_drivers[CONFIG_INPUT_PARAM_COUNT] =
    {
        [CONFIG_INPUT_PARAM_START_TIME] =
            {
                .label = ">> Start time?",
                .field_count = 3U,
                .load_fn = time_load_start_fn,
                .save_fn = time_save_start_fn,
                .format_fn = time_format_fn,
                .increment_fn = time_increment_fn,
                .decrement_fn = time_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_STOP_TIME] =
            {
                .label = ">> Stop time?",
                .field_count = 3U,
                .load_fn = time_load_stop_fn,
                .save_fn = time_save_stop_fn,
                .format_fn = time_format_fn,       /* same HH:MM:SS rendering */
                .increment_fn = time_increment_fn, /* same field arithmetic   */
                .decrement_fn = time_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_ON_TIME] =
            {
                .label = ">> On time?",
                .field_count = 1U,
                .load_fn = on_time_load_fn,
                .save_fn = on_time_save_fn,
                .format_fn = millis_format_fn,
                .increment_fn = millis_increment_fn,
                .decrement_fn = millis_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_OFF_TIME] =
            {
                .label = ">> Off time?",
                .field_count = 1U,
                .load_fn = off_time_load_fn,
                .save_fn = off_time_save_fn,
                .format_fn = millis_format_fn,
                .increment_fn = millis_increment_fn,
                .decrement_fn = millis_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_END_DATE] =
            {
                .label = ">> End date?",
                .field_count = 3U, /* field 0=month, field 1=day, field 2=year (MM/DD/YYYY) */
                .load_fn = end_date_load_fn,
                .save_fn = end_date_save_fn,
                .format_fn = end_date_format_fn,
                .increment_fn = end_date_increment_fn,
                .decrement_fn = end_date_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_GPS_TIME_OFFSET] =
            {
                .label = ">> Time offset?",
                .field_count = 1U,
                .load_fn = gps_time_offset_load_fn,
                .save_fn = gps_time_offset_save_fn,
                .format_fn = gps_time_offset_format_fn,
                .increment_fn = gps_time_offset_increment_fn,
                .decrement_fn = gps_time_offset_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_UTC_OFFSET_INDEX] =
            {
                .label = ">> UTC setting?",
                .field_count = 1U,
                .load_fn = utc_offset_load_fn,
                .save_fn = utc_offset_save_fn,
                .format_fn = utc_offset_format_fn,
                .increment_fn = utc_offset_increment_fn,
                .decrement_fn = utc_offset_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_TON_MARGIN_MS] =
            {
                .label = ">> Comp. ON->OFF?",
                .field_count = 1U,
                .load_fn = ton_margin_load_fn,
                .save_fn = ton_margin_save_fn,
                .format_fn = margin_ms_format_fn,
                .increment_fn = margin_ms_increment_fn,
                .decrement_fn = margin_ms_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_TOFF_MARGIN_MS] =
            {
                .label = ">> Comp. OFF->ON?",
                .field_count = 1U,
                .load_fn = toff_margin_load_fn,
                .save_fn = toff_margin_save_fn,
                .format_fn = margin_ms_format_fn,
                .increment_fn = margin_ms_increment_fn,
                .decrement_fn = margin_ms_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_SCREEN_TIMEOUT_S] =
            {
                .label = ">> Screen ON time?",
                .field_count = 1U,
                .load_fn = screen_timeout_load_fn,
                .save_fn = screen_timeout_save_fn,
                .format_fn = screen_timeout_format_fn,
                .increment_fn = screen_timeout_increment_fn,
                .decrement_fn = screen_timeout_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_BUZZER_ON_TIME_MS] =
            {
                .label = ">> Buzzer time?",
                .field_count = 1U,
                .load_fn = buzzer_on_time_load_fn,
                .save_fn = buzzer_on_time_save_fn,
                .format_fn = buzzer_on_time_format_fn,
                .increment_fn = buzzer_on_time_increment_fn,
                .decrement_fn = buzzer_on_time_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_EXPIRATION_DATE] =
            {
                .label = ">> End date?",
                .field_count = 3U, /* field 0=day, field 1=month, field 2=year (DD/MM/YYYY) */
                .load_fn = expiration_date_load_fn,
                .save_fn = expiration_date_save_fn,
                .format_fn = expiration_date_format_fn,
                .increment_fn = expiration_date_increment_fn,
                .decrement_fn = expiration_date_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_ANTENNA_SWITCH_TIMEOUT_MIN] =
            {
                .label = ">> GPS auto-switch timeout?",
                .field_count = 1U,
                .load_fn = antenna_switch_timeout_load_fn,
                .save_fn = antenna_switch_timeout_save_fn,
                .format_fn = antenna_switch_timeout_format_fn,
                .increment_fn = antenna_switch_timeout_increment_fn,
                .decrement_fn = antenna_switch_timeout_decrement_fn,
            },
        [CONFIG_INPUT_PARAM_WIFI_TIMEOUT_MIN] =
            {
                .label = ">> WiFi auto-disable timeout?",
                .field_count = 1U,
                .load_fn = wifi_timeout_load_fn,
                .save_fn = wifi_timeout_save_fn,
                .format_fn = wifi_timeout_format_fn,
                .increment_fn = wifi_timeout_increment_fn,
                .decrement_fn = wifi_timeout_decrement_fn,
            },
};

/*============================================================================*
 * PRIVATE — IScreen_t vtable implementations
 *============================================================================*/

static void on_enter(IScreen_t *base)
{
    ConfigurationInputScreenPresenter_t *self = (ConfigurationInputScreenPresenter_t *)base;

    /* Select the driver for the configured parameter (clamp to valid range) */
    const uint8_t idx = ((uint8_t)self->param_id < (uint8_t)CONFIG_INPUT_PARAM_COUNT)
                            ? (uint8_t)self->param_id
                            : 0U;
    self->driver = &s_drivers[idx];

    /* Reset editor state */
    self->current_field = 0U;
    self->blink_ticks = 0U;
    self->blink_on = 1U;
    self->up_held = 0U;
    self->down_held = 0U;
    self->up_press_tick = 0U;
    self->down_press_tick = 0U;
    self->show_saved = 0U; /* clear any leftover "Saved" state from a prev visit */
    self->saved_tick = 0U;

    /* Update the title to reflect what parameter is being edited.
     * The view handles the param_id → title string mapping internally. */
    IConfigurationInputScreenView_SetTitleForParam(self->view, (uint8_t)self->param_id);

    /* Load value from storage and refresh the display */

    self->driver->load_fn(self);
    IConfigurationInputScreenView_SetLabel(self->view, self->driver->label);
    self->driver->format_fn(self);
}

static void screen_on_exit(IScreen_t *base)
{
    (void)base;
}

static void on_update(IScreen_t *base)
{
    ConfigurationInputScreenPresenter_t *self = (ConfigurationInputScreenPresenter_t *)base;

    /* Auto-return after "Saved" message timeout */
    if ((self->show_saved != 0U) &&
        ((os_ticks_get() - self->saved_tick) >= CONFIG_INPUT_SAVED_DISPLAY_MS))
    {
        self->show_saved = 0U;
        navigate_back(self);
        return; /* nothing else to do after navigating away */
    }

    /* Advance blink counter */
    if ((os_ticks_get() - self->blink_ticks) >= CONFIG_INPUT_BLINK_TICKS)
    {
        self->blink_ticks = os_ticks_get();
        self->blink_on ^= 1U;

        if (self->up_held != 0U)
        {
            self->up_held = 0U;
            IConfigurationInputScreenView_SetUpIconPressed(self->view, false);
        }

        if (self->down_held != 0U)
        {
            self->down_held = 0U;
            IConfigurationInputScreenView_SetDownIconPressed(self->view, false);
        }
    }

    /* Refresh value display on every tick (blink or steady) */
    self->driver->format_fn(self);
}

static void on_back_pressed(IScreen_t *base)
{
    ConfigurationInputScreenPresenter_t *self = (ConfigurationInputScreenPresenter_t *)base;

    /* Discard edits — navigate back without saving */
    if (self->router != NULL)
    {
        (void)IScreenRouter_NavigateTo(self->router, self->back_screen_id);
    }
}

static void on_key_event(IScreen_t *base, uint8_t key, uint8_t event)
{
    ConfigurationInputScreenPresenter_t *self = (ConfigurationInputScreenPresenter_t *)base;

    const bool is_up = (key == (uint8_t)SCREEN_KEY_UP);
    const bool is_down = (key == (uint8_t)SCREEN_KEY_DOWN);
    bool act = false;

    /* ── UP / DOWN: immediate on PRESS, repeat after long-press threshold ──────── */
    if (is_up || is_down)
    {
        uint32_t *press_tick = is_up ? &self->up_press_tick : &self->down_press_tick;

        if ((event & SCREEN_KEY_EVENT_PRESS) != 0U)
        {
            *press_tick = os_ticks_get();
            act = true; /* immediate action on first press */
        }
        else if ((event & SCREEN_KEY_EVENT_KEEPALIVE) != 0U)
        {
            if ((os_ticks_get() - *press_tick) >= CONFIG_INPUT_LONG_PRESS_TICKS)
            {
                act = true;
            }
        }
        /* RELEASE: act remains false — no action */
    }

    if (act)
    {
        /* Ignore UP/DOWN while the "Saved" confirmation is on screen. */
        if (self->show_saved != 0U)
        {
            return;
        }

        if (is_up)
        {
            self->driver->increment_fn(self);
            self->up_held = 1U;
            self->blink_on = 1U; /* keep field visible after modification */
            self->blink_ticks = os_ticks_get();
            IConfigurationInputScreenView_SetUpIconPressed(self->view, true);
        }
        else
        {
            self->driver->decrement_fn(self);
            self->down_held = 1U;
            self->blink_on = 1U;
            self->blink_ticks = os_ticks_get();
            IConfigurationInputScreenView_SetDownIconPressed(self->view, true);
        }
        self->driver->format_fn(self);
    }
    else if ((key == (uint8_t)SCREEN_KEY_ENTER) &&
             ((event & SCREEN_KEY_EVENT_PRESS) != 0U) &&
             (self->show_saved == 0U)) /* ignore ENTER while "Saved" is visible */
    {
        if ((uint8_t)(self->current_field + 1U) >= self->driver->field_count)
        {
            /* Last sub-field: save, show confirmation, start auto-return timer.
             * Do NOT navigate here — the "Saved" message absorbs the key so the
             * destination screen never sees a ghost ENTER event. */
            self->driver->save_fn(self);
            self->show_saved = 1U;
            self->saved_tick = os_ticks_get();
            IConfigurationInputScreenView_SetLabel(self->view, ">> Saved!");
        }
        else
        {
            /* Advance to the next sub-field */
            self->current_field++;
            self->blink_on = 1U; /* show new active field immediately */
            self->blink_ticks = 0U;
        }
    }
    else
    {
        /* Unknown / unhandled key — ignore */
    }
}

/*============================================================================*
 * PUBLIC — API
 *============================================================================*/

Result_t ConfigurationInputScreenPresenter_Init(
    ConfigurationInputScreenPresenter_t *self,
    const ConfigurationInputScreenPresenterDeps_t *deps)
{
    if (!self)
        return ERR_NULL_POINTER;
    if (!deps)
        return ERR_NULL_POINTER;
    if (!deps->view)
        return ERR_NULL_POINTER;
    if (!deps->config_storage)
        return ERR_NULL_POINTER;

    memset(self, 0, sizeof(*self));

    /* Wire IScreen_t vtable */
    self->base.OnEnter = on_enter;
    self->base.OnExit = screen_on_exit;
    self->base.OnUpdate = on_update;
    self->base.OnBackPressed = on_back_pressed;
    self->base.OnKeyEvent = on_key_event;

    /* Inject dependencies */
    self->view = deps->view;
    self->router = deps->router;
    self->config_storage = deps->config_storage;
    self->back_screen_id = deps->back_screen_id;

    /* Initial editor state — driver pre-set so on_update is safe before first OnEnter */
    self->param_id = CONFIG_INPUT_PARAM_START_TIME;
    self->driver = &s_drivers[CONFIG_INPUT_PARAM_START_TIME];
    self->current_field = 0U;
    self->blink_on = 1U;

    return ERR_OK;
}

void ConfigurationInputScreenPresenter_SetParam(
    ConfigurationInputScreenPresenter_t *self,
    ConfigInputParam_t param)
{
    if (!self)
    {
        return;
    }
    self->param_id = param;
    self->driver = &s_drivers[param];
}

void ConfigurationInputScreenPresenter_SetContext(
    ConfigurationInputScreenPresenter_t *self,
    ConfigInputParam_t param,
    uint8_t cycle_idx,
    uint8_t back_screen_id)
{
    if (!self)
    {
        return;
    }
    self->param_id = param;
    self->driver = &s_drivers[param];
    self->cycle_idx = (cycle_idx < (uint8_t)RELAY_MAX_CYCLES) ? cycle_idx : 0U;
    self->back_screen_id = back_screen_id;
}
