/**
 * @file eez_int_configuration_state_screen_view.c
 * @brief EEZ-backed IIntConfigurationStateScreenView_t implementation.
 *
 * This is the ONLY file in the int_configuration_state presenter path
 * that is allowed to include lvgl.h.
 *
 * ## Thread safety
 * lv_lock()/lv_unlock() guards every lv_* call — safe from any thread.
 *
 * ## Widget mapping
 *   objects.int_configuration_status → lv_switch → SyncSwitch(bool)
 *
 * @author Tecna Smart Lab
 */
#include "presentation/screens/int_configuration_state/eez_int_configuration_state_screen_view.h"
#include "ui/screens.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

/**
 * @brief Synchronise the switch widget to the given enabled state.
 *
 * Sets or clears LV_STATE_CHECKED on objects.int_configuration_status.
 * Wrapped in lv_lock()/lv_unlock() for thread safety.
 *
 * @param[in] self     View instance (unused — stateless singleton).
 * @param[in] enabled  true → switch ON, false → switch OFF.
 */
static void sync_switch(IIntConfigurationStateScreenView_t *self, bool enabled)
{
    (void)self;
    lv_lock();
    if (enabled)
    {
        lv_obj_add_state(objects.int_configuration_status, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(objects.int_configuration_status, LV_STATE_CHECKED);
    }
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IIntConfigurationStateScreenView_t s_view = {
    .SyncSwitch = sync_switch,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

/**
 * @copydoc EezIntConfigurationStateScreenView_GetInterface
 */
IIntConfigurationStateScreenView_t *EezIntConfigurationStateScreenView_GetInterface(void)
{
    return &s_view;
}
