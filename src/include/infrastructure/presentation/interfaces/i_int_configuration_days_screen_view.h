/**
 * @file i_int_configuration_days_screen_view.h
 * @brief View interface for the Interrupt Configuration Days screen.
 *
 * Implemented by EezIntConfigurationDaysScreenView. Zero lvgl.h dependency.
 *
 * ## Widget mapping
 *   int_configuration_days_mon … sun  → lv_checkbox (SetDayChecked — init only;
 *                                        LVGL encoder group manages runtime focus)
 *   int_configuration_days_label      → lv_label    (SetLabel  — prompt / ">> Saved!")
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Number of selectable day checkboxes (Mon … Sun). */
#define INT_CONFIG_DAYS_COUNT 7U

    /**
     * @brief View contract for the IntConfigurationDays screen.
     */
    typedef struct IIntConfigurationDaysScreenView_t IIntConfigurationDaysScreenView_t;

    struct IIntConfigurationDaysScreenView_t
    {
        /**
         * @brief Set or clear the checked state of one day checkbox.
         *
         * @param[in] self     View instance.
         * @param[in] day_idx  0 = Monday … 6 = Sunday.
         * @param[in] checked  true = checked (day active), false = unchecked.
         */
        void (*SetDayChecked)(IIntConfigurationDaysScreenView_t *self,
                              uint8_t day_idx, bool checked);

        /**
         * @brief Update the prompt label text.
         *
         * Used to show ">>Days interrupt?" on enter and ">> Saved!" on save.
         *
         * @param[in] self  View instance.
         * @param[in] text  Null-terminated string.
         */
        void (*SetLabel)(IIntConfigurationDaysScreenView_t *self, const char *text);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IIntConfigurationDaysScreenView_SetDayChecked(
        IIntConfigurationDaysScreenView_t *v, uint8_t day_idx, bool checked)
    {
        if (v && v->SetDayChecked)
        {
            v->SetDayChecked(v, day_idx, checked);
        }
    }

    static inline void IIntConfigurationDaysScreenView_SetLabel(
        IIntConfigurationDaysScreenView_t *v, const char *text)
    {
        if (v && v->SetLabel)
        {
            v->SetLabel(v, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_DAYS_SCREEN_VIEW_H */
