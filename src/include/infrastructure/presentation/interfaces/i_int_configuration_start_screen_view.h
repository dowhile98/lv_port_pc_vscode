/**
 * @file i_int_configuration_start_screen_view.h
 * @brief View contract for the Interrupt Configuration Start screen — zero LVGL.
 *
 * Implemented by EezIntConfigurationStartScreenView, which wraps every lv_*
 * call in lv_lock()/lv_unlock(). The presenter depends only on this interface,
 * keeping it fully testable on PC without any LVGL dependency.
 *
 * ## Widget mapping (from EEZ screens.h)
 *   objects.int_configuration_start_value → lv_switch  → SyncSwitch(bool)
 *   objects.int_configuration_start_label → lv_label   → SetLabel(const char *)
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_INT_CONFIGURATION_START_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_START_SCREEN_VIEW_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct IIntConfigurationStartScreenView_t IIntConfigurationStartScreenView_t;

    /**
     * @brief View contract for the Interrupt Configuration Start screen.
     *
     * Pure function-pointer vtable — zero LVGL dependency.
     */
    struct IIntConfigurationStartScreenView_t
    {
        /**
         * @brief Synchronise the switch widget to the given state.
         *
         * @param[in] self          View instance (must not be NULL).
         * @param[in] start_with_on true  → switch ON  (relay starts closed).
         *                          false → switch OFF (relay starts open).
         */
        void (*SyncSwitch)(IIntConfigurationStartScreenView_t *self, bool start_with_on);

        /**
         * @brief Update the label text (e.g. prompt or "Saved!").
         *
         * @param[in] self  View instance (must not be NULL).
         * @param[in] text  Null-terminated string to display.
         */
        void (*SetLabel)(IIntConfigurationStartScreenView_t *self, const char *text);
    };

    /* ── NULL-safe dispatch helpers ─────────────────────────────────────── */

    static inline void IIntConfigurationStartScreenView_SyncSwitch(
        IIntConfigurationStartScreenView_t *v, bool start_with_on)
    {
        if (v != NULL && v->SyncSwitch != NULL)
        {
            v->SyncSwitch(v, start_with_on);
        }
    }

    static inline void IIntConfigurationStartScreenView_SetLabel(
        IIntConfigurationStartScreenView_t *v, const char *text)
    {
        if (v != NULL && v->SetLabel != NULL)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_START_SCREEN_VIEW_H */
