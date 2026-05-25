/**
 * @file display_backlight_service.c
 * @brief Display backlight timeout service implementation — Fase 4.
 *
 * @note NO #include "lvgl.h" — intentional. Must remain LVGL-free.
 * @note NO #include "stm32u5xx_hal.h" — display accessed only via I_Display vtable.
 */
#include "application/services/display_backlight_service.h"
#include "infrastructure/osal/osal.h"
#include "interfaces/i_digital_input_source.h"
#include <string.h>

/* ============================================================================
 * Private helpers
 * ============================================================================ */

/**
 * @brief Turn off the backlight via SetBrightness(0) and mark asleep.
 * @note Called only when currently awake → avoids redundant HAL calls.
 */
static void turn_off(DisplayBacklightService_t *self)
{
    self->is_awake = false;
    (void)Display_SetBrightness(self->display, 0U);
}

/**
 * @brief Turn on the backlight via SetBrightness(100) and mark awake.
 * @note Does NOT reset the activity timer.
 */
static void turn_on(DisplayBacklightService_t *self)
{
    self->is_awake = true;
    (void)Display_SetBrightness(self->display, 100U);
}

/* ============================================================================
 * Public API
 * ============================================================================ */

Result_t DisplayBacklightService_Init(DisplayBacklightService_t *self,
                                      const DisplayBacklightServiceConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (config->display == NULL || config->config_storage == NULL || config->di_source == NULL)
    {
        return ERR_NULL_POINTER;
    }

    memset(self, 0, sizeof(DisplayBacklightService_t));
    self->display = config->display;
    self->config_storage = config->config_storage;
    self->di_source = config->di_source;
    self->last_activity_ticks = os_ticks_get();
    self->is_awake = true;

    return ERR_OK;
}

void DisplayBacklightService_Update(DisplayBacklightService_t *self)
{
    if (self == NULL)
    {
        return;
    }

    /* If already off, nothing to check */
    if (!self->is_awake)
    {
        return;
    }

    /* Read timeout from config — updated every call so live changes take effect */
    GeneralConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    (void)ConfigStorage_LoadGeneralConfig(self->config_storage, &cfg);

    /* timeout == 0 → feature disabled */
    if (cfg.screen_blacklight_timeout_ms == 0U)
    {
        return;
    }

    /* Check alarm — active alarm inhibits backlight off */
    bool overtemp = false;
    (void)DigitalInputSource_ReadInput(self->di_source, DI_ID_ALARM_OVERTEMP, &overtemp);
    if (overtemp)
    {
        return;
    }

    /* Check elapsed time */
    uint32_t now_ms = os_ticks_get();
    if ((now_ms - self->last_activity_ticks) >= cfg.screen_blacklight_timeout_ms)
    {
        turn_off(self);
    }
}

void DisplayBacklightService_ResetTimer(DisplayBacklightService_t *self)
{
    if (self == NULL)
    {
        return;
    }
    self->last_activity_ticks = os_ticks_get();
}

void DisplayBacklightService_WakeUp(DisplayBacklightService_t *self)
{
    if (self == NULL)
    {
        return;
    }
    if (!self->is_awake)
    {
        turn_on(self);

        /*reset timer*/
        self->last_activity_ticks = os_ticks_get();
    }
}

bool DisplayBacklightService_IsAwake(const DisplayBacklightService_t *self)
{
    if (self == NULL)
    {
        return false;
    }
    return self->is_awake;
}
