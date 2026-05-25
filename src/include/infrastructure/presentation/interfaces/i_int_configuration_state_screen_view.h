/**
 * @file i_int_configuration_state_screen_view.h
 * @brief View contract for the Interrupt Configuration State screen — zero LVGL.
 *
 * Implemented by EezIntConfigurationStateScreenView_t which wraps LVGL widget
 * calls under lv_lock()/lv_unlock(). The presenter depends only on this
 * interface, making it fully testable on PC without any LVGL dependency.
 *
 * ## Widget mapping (from EEZ screens.c)
 *   objects.int_configuration_status → lv_switch — SyncSwitch(bool enabled)
 *
 * @author Tecna Smart Lab
 */
#ifndef I_INT_CONFIGURATION_STATE_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_STATE_SCREEN_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IIntConfigurationStateScreenView_t IIntConfigurationStateScreenView_t;

    /**
     * @brief View contract for the Interrupt Configuration State screen.
     *
     * Pure function-pointer vtable — zero LVGL dependency.
     */
    struct IIntConfigurationStateScreenView_t
    {
        /**
         * @brief Synchronise the hardware switch widget to the given state.
         *
         * Sets or clears LV_STATE_CHECKED on the int_configuration_status switch.
         * Must be called from inside lv_lock()/lv_unlock() in the concrete impl.
         *
         * @param[in] self     View instance (must not be NULL).
         * @param[in] enabled  true  → switch ON (LV_STATE_CHECKED added).
         *                     false → switch OFF (LV_STATE_CHECKED cleared).
         */
        void (*SyncSwitch)(IIntConfigurationStateScreenView_t *self, bool enabled);
    };

    /* ── NULL-safe dispatch helpers ─────────────────────────────────────── */

    /**
     * @brief NULL-safe SyncSwitch dispatch.
     *
     * @param[in] v        View instance (may be NULL — silently ignored).
     * @param[in] enabled  Desired switch state.
     */
    static inline void IIntConfigurationStateScreenView_SyncSwitch(
        IIntConfigurationStateScreenView_t *v, bool enabled)
    {
        if (v != NULL && v->SyncSwitch != NULL)
        {
            v->SyncSwitch(v, enabled);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_STATE_SCREEN_VIEW_H */
