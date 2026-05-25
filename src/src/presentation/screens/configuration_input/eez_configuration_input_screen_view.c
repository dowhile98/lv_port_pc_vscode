/**
 * @file eez_configuration_input_screen_view.c
 * @brief EEZ-backed IConfigurationInputScreenView_t implementation.
 *
 * This is the ONLY file in the configuration_input presenter path
 * that is allowed to include lvgl.h.
 *
 * ## Thread safety
 * lv_lock()/lv_unlock() guards every lv_* call — safe from any thread.
 *
 * ## Widget mapping
 *   objects.configuration_input_label    → lv_label → SetLabel
 *   objects.configuration_input_value    → lv_label → SetValue
 *   objects.configuration_input_up_icon  → lv_obj   → SetUpIconPressed (opacity)
 *   objects.configuration_input_down_icon → lv_obj   → SetDownIconPressed (opacity)
 *
 * @author Tecna Smart Lab
 */
#include "presentation/screens/configuration_input/eez_configuration_input_screen_view.h"
#include "ui/screens.h"
#include "ui/images.h"
#include "lvgl.h"

/*============================================================================*
 * PRIVATE — widget setters
 *============================================================================*/

/**
 * @brief Update the prompt label.
 *
 * @param[in] self  View instance (unused — stateless singleton).
 * @param[in] text  Null-terminated label text (e.g. ">> Start time?").
 */
static void set_label(IConfigurationInputScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.configuration_input_label, text != NULL ? text : "");
    lv_unlock();
}

/**
 * @brief Update the edited time value label.
 *
 * @param[in] self  View instance (unused — stateless singleton).
 * @param[in] text  Null-terminated value string (e.g. "08:30:00" or "  :30:00").
 */
static void set_value(IConfigurationInputScreenView_t *self, const char *text)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.configuration_input_value, text != NULL ? text : "");
    lv_unlock();
}

/**
 * @brief Show or release the UP icon pressed state via opacity.
 *
 * Pressed  → LV_OPA_60 (dimmed, visual feedback)
 * Released → LV_OPA_COVER (fully visible)
 *
 * @param[in] self     View instance (unused).
 * @param[in] pressed  true = icon pressed, false = icon released.
 */
static void set_up_icon_pressed(IConfigurationInputScreenView_t *self, bool pressed)
{
    (void)self;
    const void *src = pressed ? &img_up_button_press : &img_up_button;

    lv_lock();
    lv_image_set_src(objects.configuration_input_up_icon, src);
    lv_unlock();
}

/**
 * @brief Show or release the DOWN icon pressed state via opacity.
 *
 * @param[in] self     View instance (unused).
 * @param[in] pressed  true = icon pressed, false = icon released.
 */
static void set_down_icon_pressed(IConfigurationInputScreenView_t *self, bool pressed)
{
    (void)self;
    const void *src = pressed ? &img_down_button_press : &img_down_button;

    lv_lock();
    lv_image_set_src(objects.configuration_input_down_icon, src);
    lv_unlock();
}

/**
 * @brief Maps a ConfigInputParam_t value to its configuration title string.
 *
 * This helper lives here (EEZ view) because it is the only layer that may
 * include EEZ-generated headers and LVGL types. Presenters remain clean.
 *
 * @param[in] param_id  Parameter identifier (value from ConfigInputParam_t).
 * @return Null-terminated title string; never NULL.
 */
static const char *title_for_param(uint8_t param_id)
{
    /* Map based on what parameter is being edited, not where we came from.
     * CONFIG_INPUT_PARAM_* values defined in configuration_input_screen_presenter.h */
    switch (param_id)
    {
    case 5U:  /* CONFIG_INPUT_PARAM_GPS_TIME_OFFSET */
    case 6U:  /* CONFIG_INPUT_PARAM_UTC_OFFSET_INDEX */
    case 12U: /* CONFIG_INPUT_PARAM_ANTENNA_SWITCH_TIMEOUT_MIN */
        return "GPS CONFIGURATION";
    case 7U: /* CONFIG_INPUT_PARAM_TON_MARGIN_MS */
    case 8U: /* CONFIG_INPUT_PARAM_TOFF_MARGIN_MS */
        return "CONTACT CONFIGURATION";
    case 9U:  /* CONFIG_INPUT_PARAM_SCREEN_TIMEOUT_S */
    case 10U: /* CONFIG_INPUT_PARAM_BUZZER_ON_TIME_MS */
        return "GENERAL CONFIGURATION";
    case 11U: /* CONFIG_INPUT_PARAM_EXPIRATION_DATE */
        return "RENTAL END DATE";
    default: /* 0..4: START_TIME, STOP_TIME, ON_TIME, OFF_TIME, END_DATE (multicycle) */
        return "INTERRUPTION CONFIGURATION";
    }
}

/**
 * @brief Update the screen title widget using the parameter being edited.
 *
 * @param[in] self      View instance (unused — stateless singleton).
 * @param[in] param_id  Parameter ID passed by the presenter.
 */
static void set_title_for_param(IConfigurationInputScreenView_t *self, uint8_t param_id)
{
    (void)self;
    lv_lock();
    lv_label_set_text(objects.configuration_input_title, title_for_param(param_id));
    lv_unlock();
}

/*============================================================================*
 * PRIVATE — singleton vtable
 *============================================================================*/

static IConfigurationInputScreenView_t s_view = {
    .SetLabel = set_label,
    .SetValue = set_value,
    .SetUpIconPressed = set_up_icon_pressed,
    .SetDownIconPressed = set_down_icon_pressed,
    .SetTitleForParam = set_title_for_param,
};

/*============================================================================*
 * PUBLIC — getter
 *============================================================================*/

/**
 * @copydoc EezConfigurationInputScreenView_GetInterface
 */
IConfigurationInputScreenView_t *EezConfigurationInputScreenView_GetInterface(void)
{
    return &s_view;
}
