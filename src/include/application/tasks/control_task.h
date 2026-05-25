/**
 * @file control_task.h
 * @brief Main Control Task - System Monitoring and Periodic Updates
 *
 * @note Priority 12 (between RelayAO=10 and GPSAo=15).
 * @note Executes every 100ms for health monitoring and periodic updates.
 */

#ifndef APPLICATION_TASKS_CONTROL_TASK_H
#define APPLICATION_TASKS_CONTROL_TASK_H

#include "hal/hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Forward declarations */
    typedef struct WifiEnableService_t WifiEnableService_t;

    /**
     * @brief Dependency bundle for ControlTask_Init().
     */
    typedef struct
    {
        WifiEnableService_t *wifi_service; /**< WiFi enable service for startup sync. Can be NULL. */
    } ControlTaskDeps_t;

    /**
     * @brief Initialize and start the main control task.
     * @note Called from System_Init() after all adapters are initialized.
     *
     * @param[in] deps  Dependency bundle (can be NULL for minimal init).
     *
     * @return ERR_OK on success, error code otherwise.
     */
    Result_t ControlTask_Init(const ControlTaskDeps_t *deps);

    /**
     * @brief Stop the control task (for testing/shutdown).
     * @note Blocks until task terminates.
     * @return ERR_OK on success.
     */
    Result_t ControlTask_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_TASKS_CONTROL_TASK_H */
