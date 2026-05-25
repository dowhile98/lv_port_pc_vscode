/**
 * @file wifi_configuration_screen_presenter.h
 * @brief Presenter for the WiFi Configuration screen.
 *
 * Displays a 2-item list:
 *   1. WiFi module toggle (Enable/Disable) via IWifiModule_t.
 *   2. More... (TBD) — decorative no-op placeholder.
 *
 * ## Architecture
 *  - IScreen_t is the FIRST field (C99 first-field cast).
 *  - Zero lvgl.h — fully testable on PC.
 *  - All LVGL calls go through IListNavScreenView_t.
 *  - Router, wifi_transport, and buzzer are optional (NULL → graceful skip).
 *
 * @author Tecna Smart Lab
 */
#ifndef WIFI_CONFIGURATION_SCREEN_PRESENTER_H
#define WIFI_CONFIGURATION_SCREEN_PRESENTER_H

#include "presentation/interfaces/i_screen.h"
#include "presentation/interfaces/i_list_nav_screen_view.h"
#include "presentation/interfaces/i_screen_router.h"
#include "interfaces/i_wifi_module.h"
#include "hal/hal_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Dependency bundle for WifiConfigurationScreenPresenter_Init().
     */
    typedef struct WifiConfigurationScreenPresenterDeps_t
    {
        /** EEZ-backed list view.  Required — must not be NULL. */
        IListNavScreenView_t *view;

        /** Screen router for navigation.  NULL → navigation silently skipped. */
        IScreenRouter_t *router;

        /** WiFi module control interface. NULL -> toggle action is skipped. */
        IWifiModule_t *wifi_module;

        /** Screen to navigate to on OnBackPressed and after reset action. */
        uint8_t back_screen_id;

    } WifiConfigurationScreenPresenterDeps_t;

    /**
     * @brief WiFi Configuration screen presenter.
     *
     * @note IScreen_t MUST be the first field (C99 first-field cast rule).
     */
    typedef struct WifiConfigurationScreenPresenter_t
    {
        IScreen_t base; /**< MUST be first. */
        IListNavScreenView_t *view;
        IScreenRouter_t *router;
        IWifiModule_t *wifi_module;
        uint8_t back_screen_id;
        uint8_t last_selected_idx; /**< Restored as focused item on re-enter. */
    } WifiConfigurationScreenPresenter_t;

    /**
     * @brief Initialise the WiFi Configuration screen presenter.
     *
     * @param[in,out] self  Presenter instance (must not be NULL).
     * @param[in]     deps  Dependency bundle (must not be NULL; view must not be NULL).
     *
     * @return ERR_OK on success.
     * @return ERR_NULL_POINTER if self, deps, or deps->view is NULL.
     */
    Result_t WifiConfigurationScreenPresenter_Init(
        WifiConfigurationScreenPresenter_t *self,
        const WifiConfigurationScreenPresenterDeps_t *deps);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CONFIGURATION_SCREEN_PRESENTER_H */
