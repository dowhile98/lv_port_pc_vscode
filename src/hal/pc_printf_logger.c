/**
 * @file pc_printf_logger.c
 * @brief ILogger adapter for the PC simulator — output via stdio printf.
 *
 * Format (with colors and file:line enabled):
 *   <color>D (012345) [TAG] file.c:42: user message\033[0m\n
 *
 * Thread-safety: protected by a POSIX pthread_mutex so the adapter is safe
 * when called from multiple FreeRTOS POSIX tasks.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "pc_printf_logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * ANSI COLOR CODES
 * ========================================================================== */

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[0;31m"
#define ANSI_YELLOW "\033[0;33m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_CYAN "\033[0;36m"
#define ANSI_WHITE "\033[0;37m"

/* ============================================================================
 * PRIVATE HELPERS
 * ========================================================================== */

/** Single global mutex — all PcPrintfLogger instances share it. */
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_mutex_ok = false; /* true after first Init */

static void ensure_mutex(void)
{
    /* Double-check with a static sentinel is safe here: Init is called
     * once from the main/init task before any logging happens.             */
    if (!s_mutex_ok)
    {
        pthread_mutex_init(&s_mutex, NULL);
        s_mutex_ok = true;
    }
}

/**
 * @brief Returns monotonic milliseconds since program start.
 */
static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000U +
                      (uint64_t)ts.tv_nsec / 1000000U);
}

static const char *level_letter(LogLevel_t level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return "E";
    case LOG_LEVEL_WARN:
        return "W";
    case LOG_LEVEL_INFO:
        return "I";
    case LOG_LEVEL_DEBUG:
        return "D";
    case LOG_LEVEL_VERBOSE:
        return "V";
    default:
        return "?";
    }
}

static const char *level_color(LogLevel_t level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return ANSI_RED;
    case LOG_LEVEL_WARN:
        return ANSI_YELLOW;
    case LOG_LEVEL_INFO:
        return ANSI_GREEN;
    case LOG_LEVEL_DEBUG:
        return ANSI_CYAN;
    case LOG_LEVEL_VERBOSE:
        return ANSI_WHITE;
    default:
        return ANSI_WHITE;
    }
}

/** Extract the basename from a full path (e.g. "/a/b/c.c" → "c.c"). */
static const char *basename_of(const char *path)
{
    if (path == NULL)
    {
        return "?";
    }
    const char *p = path;
    const char *last = path;
    while (*p)
    {
        if (*p == '/' || *p == '\\')
        {
            last = p + 1;
        }
        p++;
    }
    return last;
}

/* ============================================================================
 * VTABLE IMPLEMENTATION
 * ========================================================================== */

static void pc_printf_log_impl(void *impl,
                               LogLevel_t level,
                               const char *tag,
                               const char *file,
                               uint32_t line,
                               const char *fmt,
                               va_list args)
{
    if (impl == NULL || fmt == NULL)
    {
        return;
    }

    PcPrintfLogger_t *self = (PcPrintfLogger_t *)impl;

    /* Level filter */
    if (level > self->config.min_level)
    {
        return;
    }

    uint32_t ts = now_ms();
    const char *lvl = level_letter(level);
    const char *color = self->config.enable_colors ? level_color(level) : "";
    const char *reset = self->config.enable_colors ? ANSI_RESET : "";

    pthread_mutex_lock(&s_mutex);

    /* Header ---------------------------------------------------------------- */
    if (self->config.enable_colors)
    {
        fprintf(stdout, "%s", color);
    }

    fprintf(stdout, "%s (%06lu) [%-8s]",
            lvl,
            (unsigned long)ts,
            tag ? tag : "---");

    if (self->config.show_file_line && file != NULL)
    {
        fprintf(stdout, " %s:%lu", basename_of(file), (unsigned long)line);
    }

    fprintf(stdout, ": ");

    /* User message ---------------------------------------------------------- */
    vfprintf(stdout, fmt, args);

    /* Footer ---------------------------------------------------------------- */
    if (self->config.enable_colors)
    {
        fprintf(stdout, "%s", reset);
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    pthread_mutex_unlock(&s_mutex);
}

/* ============================================================================
 * VTABLE + PUBLIC API
 * ========================================================================== */

static const ILogger_Vtable k_vtable = {
    .Log = pc_printf_log_impl,
};

Result_t PcPrintfLogger_Init(PcPrintfLogger_t *self,
                             const PcPrintfLoggerConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(*self));
    self->config = *config;
    self->interface.vtable = &k_vtable;
    self->interface.impl = self;
    self->is_initialized = true;

    ensure_mutex();

    return ERR_OK;
}

ILogger *PcPrintfLogger_GetInterface(PcPrintfLogger_t *self)
{
    if (self == NULL || !self->is_initialized)
    {
        return NULL;
    }
    return &self->interface;
}

Result_t PcPrintfLogger_Deinit(PcPrintfLogger_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }
    self->is_initialized = false;
    self->interface.vtable = NULL;
    self->interface.impl = NULL;
    return ERR_OK;
}
