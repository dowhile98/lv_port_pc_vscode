/**
 * @file i_logger.h
 * @brief Interfaz abstracta para logging de depuración (terminal/UART/SWO).
 * @version 1.0.0
 *
 * @note Esta interfaz permite logging volátil para desarrollo/debug.
 *       NO debe usarse para eventos persistentes (usar IEventNotifier).
 *       Cumple con DIP: Domain/Application no dependen de implementación concreta.
 */

#ifndef I_LOGGER_H
#define I_LOGGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_types.h"

    /**
     * @brief Niveles de severidad para logs de depuración.
     */
    typedef enum
    {
        LOG_LEVEL_ERROR = 0,  /**< Errores críticos */
        LOG_LEVEL_WARN = 1,   /**< Advertencias */
        LOG_LEVEL_INFO = 2,   /**< Información general */
        LOG_LEVEL_DEBUG = 3,  /**< Debug detallado */
        LOG_LEVEL_VERBOSE = 4 /**< Traza completa */
    } LogLevel_t;

    /**
     * @brief V-Table para ILogger.
     */
    typedef struct ILogger_Vtable
    {
        /**
         * @brief Escribe un mensaje de log con formato.
         * @note Thread-safe si la implementación usa mutex.
         *
         * @param[in] impl      Instancia de implementación concreta.
         * @param[in] level     Nivel de severidad del mensaje.
         * @param[in] tag       Etiqueta/módulo emisor (ej: "GPS", "RELAY").
         * @param[in] file      Archivo fuente (__FILE__).
         * @param[in] line      Línea fuente (__LINE__).
         * @param[in] fmt       Formato printf-style.
         * @param[in] args      Argumentos variables (va_list).
         */
        void (*Log)(void *impl,
                    LogLevel_t level,
                    const char *tag,
                    const char *file,
                    uint32_t line,
                    const char *fmt,
                    va_list args);
    } ILogger_Vtable;

    /**
     * @brief Instancia de interfaz ILogger.
     */
    typedef struct ILogger
    {
        const ILogger_Vtable *vtable;
        void *impl;
    } ILogger;

    /* ===== Helper Functions ===== */

    /**
     * @brief Valida si la instancia de logger es válida.
     * @return true si válida, false si null o corrupta.
     */
    static inline bool Logger_IsValid(const ILogger *logger)
    {
        return (logger != NULL) &&
               (logger->vtable != NULL) &&
               (logger->vtable->Log != NULL) &&
               (logger->impl != NULL);
    }

    /**
     * @brief Wrapper que convierte variadic args a va_list.
     * @note Esta función debe usarse vía macros LOG_*.
     */
    static inline void Logger_Log(const ILogger *logger,
                                  LogLevel_t level,
                                  const char *tag,
                                  const char *file,
                                  uint32_t line,
                                  const char *fmt, ...)
    {
        if (!Logger_IsValid(logger))
            return;

        va_list args;
        va_start(args, fmt);
        logger->vtable->Log(logger->impl, level, tag, file, line, fmt, args);
        va_end(args);
    }

/* ===== Convenience Macros (auto-capture __FILE__/__LINE__) ===== */

/**
 * @brief Log de ERROR (nivel 0).
 * @param logger Puntero a ILogger*.
 * @param tag    Tag del módulo (string literal).
 * @param fmt    Formato printf.
 * @param ...    Argumentos variables.
 */
#define LOG_ERROR(logger, tag, fmt, ...) \
    Logger_Log(logger, LOG_LEVEL_ERROR, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log de WARNING (nivel 1).
 */
#define LOG_WARN(logger, tag, fmt, ...) \
    Logger_Log(logger, LOG_LEVEL_WARN, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log de INFO (nivel 2).
 */
#define LOG_INFO(logger, tag, fmt, ...) \
    Logger_Log(logger, LOG_LEVEL_INFO, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log de DEBUG (nivel 3).
 */
#define LOG_DEBUG(logger, tag, fmt, ...) \
    Logger_Log(logger, LOG_LEVEL_DEBUG, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/**
 * @brief Log de VERBOSE (nivel 4).
 */
#define LOG_VERBOSE(logger, tag, fmt, ...) \
    Logger_Log(logger, LOG_LEVEL_VERBOSE, tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* I_LOGGER_H */
