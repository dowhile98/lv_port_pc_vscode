/**
 * @file i_int_configuration_predefined_screen_view.h
 * @brief View contract for the Predefined Cycles selection screen.
 *
 * ## Provided operations
 *   - InitItems  — populate lv_list with all predefined cycle labels.
 *   - SetLabel   — update the status/feedback label (e.g. ">> Saved! Ton:0.8s / Toff:0.2s").
 *
 * @note Zero #include "lvgl.h" — fully PC-buildable.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H
#define I_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct IIntConfigurationPredefinedScreenView_t;

    /**
     * @brief Fired by the EEZ view when the user selects a list item.
     *
     * @param[in] item_idx  Zero-based index of the selected predefined cycle.
     * @param[in] context   Opaque pointer supplied to InitItems().
     */
    typedef void (*PredefinedCycleSelectedCallback_t)(uint8_t item_idx, void *context);

    /**
     * @brief View interface for the Predefined Cycles screen.
     */
    typedef struct IIntConfigurationPredefinedScreenView_t
    {
        /**
         * @brief Populate the lv_list with all predefined cycle entries.
         *
         * Clears previous items, then adds the full static cycle label table.
         * Each button fires @p on_selected with its zero-based index.
         *
         * @note lv_lock/lv_unlock handled internally — safe from any thread.
         *
         * @param[in] self        View instance.
         * @param[in] on_selected Callback fired on item release.
         * @param[in] context     Opaque pointer forwarded to @p on_selected.
         */
        void (*InitItems)(struct IIntConfigurationPredefinedScreenView_t *self,
                          PredefinedCycleSelectedCallback_t on_selected,
                          void *context);

        /**
         * @brief Update the status label text.
         *
         * @param[in] self  View instance.
         * @param[in] text  Null-terminated string (e.g. ">> Predefined Periods").
         */
        void (*SetLabel)(struct IIntConfigurationPredefinedScreenView_t *self,
                         const char *text);

    } IIntConfigurationPredefinedScreenView_t;

    /* -----------------------------------------------------------------------
     * NULL-safe inline dispatch helpers
     * --------------------------------------------------------------------- */

    /** @brief Populate the cycle list. */
    static inline void IIntConfigurationPredefinedScreenView_InitItems(
        IIntConfigurationPredefinedScreenView_t *self,
        PredefinedCycleSelectedCallback_t on_selected,
        void *context)
    {
        if (self != NULL && self->InitItems != NULL)
        {
            self->InitItems(self, on_selected, context);
        }
    }

    /** @brief Update the status label. */
    static inline void IIntConfigurationPredefinedScreenView_SetLabel(
        IIntConfigurationPredefinedScreenView_t *self,
        const char *text)
    {
        if (self != NULL && self->SetLabel != NULL)
        {
            self->SetLabel(self, text);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_INT_CONFIGURATION_PREDEFINED_SCREEN_VIEW_H */
