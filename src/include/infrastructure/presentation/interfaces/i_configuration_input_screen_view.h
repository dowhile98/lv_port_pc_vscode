/**
 * @file i_configuration_input_screen_view.h
 * @brief View interface for the ConfigurationInput time-editor screen.
 *
 * Implemented by EezConfigurationInputScreenView. Zero lvgl.h dependency here.
 *
 * @author Tecna Smart Lab
 * @date   2026
 */
#ifndef I_CONFIGURATION_INPUT_SCREEN_VIEW_H
#define I_CONFIGURATION_INPUT_SCREEN_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief View contract for the ConfigurationInput screen.
     */
    typedef struct IConfigurationInputScreenView_t IConfigurationInputScreenView_t;

    struct IConfigurationInputScreenView_t
    {
        /**
         * @brief Update the prompt label (e.g. ">> Start time?").
         * @param[in] self  View instance.
         * @param[in] text  Null-terminated label string.
         */
        void (*SetLabel)(IConfigurationInputScreenView_t *self, const char *text);

        /**
         * @brief Update the time value display (e.g. "08:30:00" or "  :30:00").
         * @param[in] self  View instance.
         * @param[in] text  Null-terminated value string.
         */
        void (*SetValue)(IConfigurationInputScreenView_t *self, const char *text);

        /**
         * @brief Show/hide the UP arrow pressed state.
         * @param[in] self     View instance.
         * @param[in] pressed  true = pressed (highlighted), false = normal.
         */
        void (*SetUpIconPressed)(IConfigurationInputScreenView_t *self, bool pressed);

        /**
         * @brief Show/hide the DOWN arrow pressed state.
         * @param[in] self     View instance.
         * @param[in] pressed  true = pressed (highlighted), false = normal.
         */
        void (*SetDownIconPressed)(IConfigurationInputScreenView_t *self, bool pressed);

        /**
         * @brief Update the screen title based on the parameter being edited.
         *
         * The view is responsible for mapping @p param_id to the appropriate
         * title string. Presenters pass their @c param_id directly;
         * they never need to include EEZ-generated headers or LVGL headers.
         *
         * @param[in] self      View instance.
         * @param[in] param_id  Parameter identifier (value from ConfigInputParam_t).
         */
        void (*SetTitleForParam)(IConfigurationInputScreenView_t *self, uint8_t param_id);
    };

    /* ---------- NULL-safe inline dispatch helpers ---------- */

    static inline void IConfigurationInputScreenView_SetLabel(
        IConfigurationInputScreenView_t *v, const char *text)
    {
        if (v && v->SetLabel)
        {
            v->SetLabel(v, text);
        }
    }

    static inline void IConfigurationInputScreenView_SetValue(
        IConfigurationInputScreenView_t *v, const char *text)
    {
        if (v && v->SetValue)
        {
            v->SetValue(v, text);
        }
    }

    static inline void IConfigurationInputScreenView_SetTitleForParam(
        IConfigurationInputScreenView_t *v, uint8_t param_id)
    {
        if (v && v->SetTitleForParam)
        {
            v->SetTitleForParam(v, param_id);
        }
    }

    static inline void IConfigurationInputScreenView_SetUpIconPressed(
        IConfigurationInputScreenView_t *v, bool pressed)
    {
        if (v && v->SetUpIconPressed)
        {
            v->SetUpIconPressed(v, pressed);
        }
    }

    static inline void IConfigurationInputScreenView_SetDownIconPressed(
        IConfigurationInputScreenView_t *v, bool pressed)
    {
        if (v && v->SetDownIconPressed)
        {
            v->SetDownIconPressed(v, pressed);
        }
    }

#ifdef __cplusplus
}
#endif

#endif /* I_CONFIGURATION_INPUT_SCREEN_VIEW_H */
