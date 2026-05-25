/**
 * @file date_time.c
 * @brief Utilidades para conversión y manipulación de DateTime_t.
 * @version 1.0.0
 * @date 6 abril 2026
 */

#include "common/date_time.h"
#include <string.h>
#include <stdbool.h>
/* ══════════════════════════════════════════════════════════════════════════
 *  Constants
 * ══════════════════════════════════════════════════════════════════════════ */

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400
#define DAYS_PER_YEAR 365
#define DAYS_PER_LEAP_YEAR 366
#define EPOCH_YEAR 1970 /**< Unix epoch base year */

/* Days in each month (non-leap year) */
static const uint8_t days_in_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* ══════════════════════════════════════════════════════════════════════════
 *  Helper Functions
 * ══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Check if a year is a leap year.
 * @param year Year to check (e.g., 2024).
 * @return true if leap year, false otherwise.
 */
static bool is_leap_year(uint16_t year)
{
    if (year % 400 == 0)
        return true;
    if (year % 100 == 0)
        return false;
    if (year % 4 == 0)
        return true;
    return false;
}

/**
 * @brief Get number of days in a month.
 * @param month Month (1-12).
 * @param year Year (for leap year calculation).
 * @return Number of days in month.
 */
static uint8_t get_days_in_month(uint8_t month, uint16_t year)
{
    if (month < 1 || month > 12)
        return 0;

    if (month == 2 && is_leap_year(year))
        return 29;

    return days_in_month[month - 1];
}

/**
 * @brief Validate DateTime_t structure.
 * @param dt Pointer to DateTime_t.
 * @return true if valid, false otherwise.
 */
static bool is_datetime_valid(const DateTime_t *dt)
{
    if (dt == NULL)
        return false;

    if (dt->year < EPOCH_YEAR || dt->year > 2106)
        return false;

    if (dt->month < 1 || dt->month > 12)
        return false;

    uint8_t max_day = get_days_in_month(dt->month, dt->year);
    if (dt->day < 1 || dt->day > max_day)
        return false;

    if (dt->hour > 23)
        return false;
    if (dt->minute > 59)
        return false;
    if (dt->second > 59)
        return false;

    return true;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Public API - Unix Timestamp Conversion
 * ══════════════════════════════════════════════════════════════════════════ */

uint32_t DateTime_ToUnix(const DateTime_t *dt)
{
    if (!is_datetime_valid(dt))
    {
        return 0; /* Invalid date → return epoch 0 */
    }

    uint32_t timestamp = 0;

    /* Count days from EPOCH_YEAR to (year - 1) */
    for (uint16_t y = EPOCH_YEAR; y < dt->year; y++)
    {
        if (is_leap_year(y))
        {
            timestamp += DAYS_PER_LEAP_YEAR * SECONDS_PER_DAY;
        }
        else
        {
            timestamp += DAYS_PER_YEAR * SECONDS_PER_DAY;
        }
    }

    /* Add days from start of current year to current month */
    for (uint8_t m = 1; m < dt->month; m++)
    {
        timestamp += get_days_in_month(m, dt->year) * SECONDS_PER_DAY;
    }

    /* Add days in current month (day - 1 because day 1 = 0 elapsed days) */
    timestamp += (dt->day - 1) * SECONDS_PER_DAY;

    /* Add hours, minutes, seconds */
    timestamp += dt->hour * SECONDS_PER_HOUR;
    timestamp += dt->minute * SECONDS_PER_MINUTE;
    timestamp += dt->second;

    return timestamp;
}

void DateTime_FromUnix(uint32_t timestamp, DateTime_t *out_dt)
{
    if (out_dt == NULL)
    {
        return;
    }

    /* Initialize output */
    memset(out_dt, 0, sizeof(DateTime_t));

    /* Extract time components (seconds, minutes, hours) */
    out_dt->second = timestamp % 60;
    timestamp /= 60;

    out_dt->minute = timestamp % 60;
    timestamp /= 60;

    out_dt->hour = timestamp % 24;
    timestamp /= 24; /* Now timestamp = days since epoch */

    /* Calculate year and day-of-year */
    uint16_t year = EPOCH_YEAR;
    uint32_t days_remaining = timestamp;

    while (true)
    {
        uint16_t days_in_year = is_leap_year(year) ? DAYS_PER_LEAP_YEAR : DAYS_PER_YEAR;

        if (days_remaining < days_in_year)
        {
            break; /* Found the correct year */
        }

        days_remaining -= days_in_year;
        year++;
    }

    out_dt->year = year;

    /* Calculate month and day */
    uint8_t month = 1;
    while (month <= 12)
    {
        uint8_t days_this_month = get_days_in_month(month, year);

        if (days_remaining < days_this_month)
        {
            break; /* Found the correct month */
        }

        days_remaining -= days_this_month;
        month++;
    }

    out_dt->month = month;
    out_dt->day = days_remaining + 1; /* +1 because day 1 = 0 days elapsed */

    /* Calculate weekday (0 = Thursday for epoch 1970-01-01) */
    /* Unix epoch started on Thursday (day 4 in 0-based week starting Sunday) */
    out_dt->weekDay = (timestamp + 4) % 7; /* 0=Sunday, 1=Monday, ..., 6=Saturday */
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Public API - DateTime Arithmetic
 * ══════════════════════════════════════════════════════════════════════════ */

void DateTime_AddSeconds(DateTime_t *dt, int32_t seconds)
{
    if (dt == NULL || !is_datetime_valid(dt))
    {
        return;
    }

    /* Convert to Unix timestamp, add seconds, convert back */
    uint32_t timestamp = DateTime_ToUnix(dt);

    /* Handle negative seconds (underflow protection) */
    if (seconds < 0 && (uint32_t)(-seconds) > timestamp)
    {
        /* Underflow → clamp to epoch */
        timestamp = 0;
    }
    else
    {
        timestamp += seconds;
    }

    DateTime_FromUnix(timestamp, dt);
}
