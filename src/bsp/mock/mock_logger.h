/**
 * @file mock_logger.h
 * @brief PC mock implementation of ILogger — prints to stdout.
 */
#ifndef MOCK_LOGGER_H
#define MOCK_LOGGER_H

#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief Return the singleton ILogger instance (printf-based).
     */
    ILogger *MockLogger_GetInstance(void);

    /**
     * @brief Set minimum log level to display (default LOG_LEVEL_DEBUG).
     */
    void MockLogger_SetMinLevel(LogLevel_t level);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_LOGGER_H */
