/**
 * @file    tcs_xxx_wifi_task.h
 * @brief   WiFi application task – public API.
 *
 * @details Exposes only the task entry point and the DI injection setter.
 *          All WiFi configuration (mode, SSID, password) is pushed by the
 *          DI container via `tcs_xxx_wifi_task_set_ctrl()` before the task
 *          starts, following the same separation-of-concerns as the original
 *          CycloneTCP ESP-IDF driver (app layer calls esp_wifi_set_mode() /
 *          esp_wifi_connect() independently of the NicDriver).
 */

#ifndef TCS_XXX_WIFI_TASK_H
#define TCS_XXX_WIFI_TASK_H

#include "interfaces/i_wifi_control.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Injects the WiFi control interface used by the task.
     *
     * @details Must be called by the DI container **before** the task is started.
     *          Follows the Dependency Inversion Principle: the task depends on the
     *          `IWifiControl_t` abstraction, not on any concrete adapter.
     *
     * @param[in] ctrl  Initialized IWifiControl_t instance. Must not be NULL.
     */
    void tcs_xxx_wifi_task_set_ctrl(IWifiControl_t *ctrl);

    /**
     * @brief Task entry point (ThreadX / RTOS thread function).
     *
     * @param[in] param  Unused. Reserved for future use.
     */
    void tcs_xxx_wifi_task_entry(void *param);

#ifdef __cplusplus
}
#endif

#endif /* TCS_XXX_WIFI_TASK_H */
