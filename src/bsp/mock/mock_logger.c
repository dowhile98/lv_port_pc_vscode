/**
 * @file mock_logger.c
 * @brief PC mock ILogger — prints to stdout with level filtering.
 */
#include "mock_logger.h"
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Mock logger state
 * ------------------------------------------------------------------------- */
typedef struct {
    LogLevel_t min_level;
} MockLoggerState_t;

static MockLoggerState_t s_state = { LOG_LEVEL_DEBUG };

static const char *level_label(LogLevel_t lv)
{
    switch (lv) {
        case LOG_LEVEL_ERROR:   return "ERR";
        case LOG_LEVEL_WARN:    return "WRN";
        case LOG_LEVEL_INFO:    return "INF";
        case LOG_LEVEL_DEBUG:   return "DBG";
        case LOG_LEVEL_VERBOSE: return "VRB";
        default:                return "???";
    }
}

static void mock_log(void *impl, LogLevel_t level, const char *tag,
                      const char *file, uint32_t line,
                      const char *fmt, va_list args)
{
    (void)impl;
    if (level > s_state.min_level) return;

    printf("[%s][%s] ", level_label(level), tag ? tag : "---");
    vprintf(fmt, args);
    printf("  (%s:%lu)\n", file ? file : "?", (unsigned long)line);
}

static const ILogger_Vtable s_vtable = {
    .Log = mock_log,
};

static ILogger s_instance = {
    .vtable = &s_vtable,
    .impl   = &s_state,
};

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

ILogger *MockLogger_GetInstance(void)
{
    return &s_instance;
}

void MockLogger_SetMinLevel(LogLevel_t level)
{
    s_state.min_level = level;
}
