/**
 * @file pc_printf_logger.h
 * @brief ILogger adapter for the PC simulator — output via stdio printf.
 *
 * Drop-in replacement for ElogAdapter on PC:
 *   - Formatted output with ANSI colors, millisecond timestamps, level tag,
 *     module tag, and optional file:line.
 *   - Thread-safe through a POSIX mutex (pthread_mutex).
 *   - No lwprintf dependency.
 *
 * Usage:
 *   PcPrintfLogger_t logger;
 *   PcPrintfLoggerConfig_t cfg = { .min_level = LOG_LEVEL_DEBUG,
 *                                   .enable_colors = true,
 *                                   .show_file_line = true };
 *   PcPrintfLogger_Init(&logger, &cfg);
 *   container->logger = PcPrintfLogger_GetInterface(&logger);
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef PC_PRINTF_LOGGER_H
#define PC_PRINTF_LOGGER_H

#include "interfaces/i_logger.h"
#include "hal_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Configuration for PcPrintfLogger.
     */
    typedef struct
    {
        LogLevel_t min_level; /**< Minimum level to display (inclusive)  */
        bool enable_colors;   /**< Prefix lines with ANSI color codes  */
        bool show_file_line;  /**< Append  file:line  to each log line */
    } PcPrintfLoggerConfig_t;

    /**
     * @brief PcPrintfLogger instance.
     *
     * Embed this struct wherever you need a logger.  The public ILogger
     * interface is obtained via PcPrintfLogger_GetInterface().
     */
    typedef struct
    {
        ILogger interface; /**< Public ILogger (must be first) */
        PcPrintfLoggerConfig_t config;
        bool is_initialized;
    } PcPrintfLogger_t;

    /**
     * @brief Initialise a PcPrintfLogger instance.
     *
     * @param[out] self   Logger instance to initialise.
     * @param[in]  config Configuration.
     * @return ERR_OK on success, ERR_NULL_POINTER if any arg is NULL.
     */
    Result_t PcPrintfLogger_Init(PcPrintfLogger_t *self,
                                 const PcPrintfLoggerConfig_t *config);

    /**
     * @brief Return the ILogger interface pointer.
     *
     * @param[in] self  Initialised logger instance.
     * @return Pointer to ILogger, or NULL if not initialised.
     */
    ILogger *PcPrintfLogger_GetInterface(PcPrintfLogger_t *self);

    /**
     * @brief Deinitialise the logger (idempotent).
     *
     * @param[in] self  Logger instance.
     * @return ERR_OK always.
     */
    Result_t PcPrintfLogger_Deinit(PcPrintfLogger_t *self);

#ifdef __cplusplus
}
#endif

#endif /* PC_PRINTF_LOGGER_H */
