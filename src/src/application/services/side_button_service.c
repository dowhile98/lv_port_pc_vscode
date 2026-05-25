/**
 * @file side_button_service.c
 * @brief Implementation of SideButtonService (multi-function button FSM).
 */

#include "application/services/side_button_service.h"
#include "infrastructure/osal/osal.h" /* os_ticks_get() for timing */

#include <string.h>

#define MULTI_CLICK_WINDOW_MS 1000U
#define LONG_PRESS_THRESHOLD_500MS 500U
#define LONG_PRESS_THRESHOLD_2S 2000U
#define LONG_PRESS_THRESHOLD_15S 15000U

static void dispatch_pattern(SideButtonService_t *self, uint8_t clicks);
static void execute_relay_toggle(SideButtonService_t *self);
static void execute_gps_reset(SideButtonService_t *self);
static void execute_emergency_relay_toggle(SideButtonService_t *self);
static void execute_wifi_toggle(SideButtonService_t *self);
static void execute_factory_reset(SideButtonService_t *self);
static void execute_status_report(SideButtonService_t *self);
static void on_side_button_event_internal(void *context,
                                          DigitalInputID_t id,
                                          DigitalInputEvent_t event,
                                          const DigitalInputEventData_t *data);

Result_t SideButtonService_Init(SideButtonService_t *service, const SideButtonServiceConfig_t *config)
{
    if ((service == NULL) || (config == NULL))
    {
        return ERR_NULL_POINTER;
    }

    if ((config->relay == NULL) ||
        (config->wifi_module == NULL) ||
        (config->gps == NULL) ||
        (config->buzzer == NULL) ||
        (config->config == NULL))
    {
        return ERR_NULL_POINTER;
    }

    memset(service, 0, sizeof(*service));

    service->relay = config->relay;
    service->wifi_module = config->wifi_module;
    service->gps = config->gps;
    service->buzzer = config->buzzer;
    service->config = config->config;
    service->logger = config->logger; /* Can be NULL */
    service->multi_click_window_ms = MULTI_CLICK_WINDOW_MS;
    service->on_button_event = on_side_button_event_internal;

    return ERR_OK;
}

Result_t SideButtonService_Update(SideButtonService_t *service)
{
    uint32_t now_ms;
    uint32_t press_duration_ms;
    uint32_t elapsed_since_first_click;

    if (service == NULL)
    {
        return ERR_NULL_POINTER;
    }

    now_ms = os_ticks_get();

    /* Check for long press thresholds (feedback only, action on release) */
    if (service->is_pressed)
    {
        press_duration_ms = now_ms - service->press_start_timestamp_ms;

        /* 15-second threshold: factory reset indicator */
        if ((press_duration_ms >= LONG_PRESS_THRESHOLD_15S) && !service->long_press_15s_triggered)
        {
            service->long_press_15s_triggered = true;
            /* Feedback: buzzer alert (critical operation pending) */
            (void)BuzzerNotificationService_NotifyEvent(service->buzzer, BUZZER_EVENT_ERROR_GENERIC);
            if (service->logger)
            {
                Logger_Log(service->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                           "Long press 15s threshold reached (factory reset), release to confirm");
            }
        }

        /* 2-second threshold: WiFi toggle indicator */
        if ((press_duration_ms >= LONG_PRESS_THRESHOLD_2S) && !service->long_press_2s_triggered)
        {
            service->long_press_2s_triggered = true;
            /* Feedback: buzzer alert (WiFi toggle pending) */
            (void)BuzzerNotificationService_NotifyEvent(service->buzzer, BUZZER_EVENT_BUTTON_LONG_PRESS);
            if (service->logger)
            {
                Logger_Log(service->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                           "Long press 2s threshold reached (WiFi toggle), release to confirm");
            }
        }
    }

    /* Multi-click window timeout check */
    if ((!service->is_pressed) && (service->click_count > 0U))
    {
        elapsed_since_first_click = now_ms - service->first_click_timestamp_ms;
        if (elapsed_since_first_click >= service->multi_click_window_ms)
        {
            if (service->logger)
            {
                Logger_Log(service->logger, LOG_LEVEL_DEBUG, "SIDE_BTN", __FILE__, __LINE__,
                           "Multi-click window timeout: dispatching %u click(s)", service->click_count);
            }
            dispatch_pattern(service, service->click_count);
            service->click_count = 0U;
        }
    }

    return ERR_OK;
}

Result_t SideButtonService_RegisterEvents(SideButtonService_t *service, IDigitalInputSource *input_source)
{
    if ((service == NULL) || (input_source == NULL))
    {
        return ERR_NULL_POINTER;
    }

    return DigitalInputSource_RegisterCallback(
        input_source,
        DI_ID_SIDE_BUTTON,
        DI_EVENT_PRESS | DI_EVENT_RELEASE,
        service->on_button_event,
        service);
}

static void on_side_button_event_internal(void *context,
                                          DigitalInputID_t id,
                                          DigitalInputEvent_t event,
                                          const DigitalInputEventData_t *data)
{
    SideButtonService_t *self;
    uint32_t now_ms;
    uint32_t press_duration_ms;

    (void)data;

    if (id != DI_ID_SIDE_BUTTON)
    {
        return;
    }

    self = (SideButtonService_t *)context;
    if (self == NULL)
    {
        return;
    }

    now_ms = os_ticks_get();

    if ((event & DI_EVENT_PRESS) != 0U)
    {
        self->is_pressed = true;
        self->press_start_timestamp_ms = now_ms;
        self->long_press_500ms_triggered = false;
        self->long_press_2s_triggered = false;
        self->long_press_15s_triggered = false;

        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_DEBUG, "SIDE_BTN", __FILE__, __LINE__,
                       "PRESS event at %lu ms", now_ms);
        }

        if (self->click_count == 0U)
        {
            self->first_click_timestamp_ms = now_ms;
        }

        return;
    }

    if ((event & DI_EVENT_RELEASE) != 0U)
    {
        self->is_pressed = false;
        press_duration_ms = now_ms - self->press_start_timestamp_ms;

        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_DEBUG, "SIDE_BTN", __FILE__, __LINE__,
                       "RELEASE event after %lu ms press duration", press_duration_ms);
        }

        /* Long press 15s: factory reset confirmed */
        if (self->long_press_15s_triggered)
        {
            if (self->logger)
            {
                Logger_Log(self->logger, LOG_LEVEL_WARN, "SIDE_BTN", __FILE__, __LINE__,
                           "Executing factory reset (15s long press confirmed)");
            }
            execute_factory_reset(self);
            self->click_count = 0U;
            return;
        }

        /* Long press 2s: WiFi toggle confirmed */
        if (self->long_press_2s_triggered)
        {
            if (self->logger)
            {
                Logger_Log(self->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                           "Executing WiFi toggle (2s long press confirmed)");
            }
            execute_wifi_toggle(self);
            self->click_count = 0U;
            return;
        }

        /* Short press (<500ms): increment click counter */
        if (press_duration_ms < LONG_PRESS_THRESHOLD_500MS)
        {
            self->click_count++;
            if (self->logger)
            {
                Logger_Log(self->logger, LOG_LEVEL_DEBUG, "SIDE_BTN", __FILE__, __LINE__,
                           "Click registered: count=%u", self->click_count);
            }
            return;
        }

        /* Medium press (500ms-2s): status report */
        if (press_duration_ms < LONG_PRESS_THRESHOLD_2S)
        {
            if (self->logger)
            {
                Logger_Log(self->logger, LOG_LEVEL_DEBUG, "SIDE_BTN", __FILE__, __LINE__,
                           "Executing status report (500ms-2s press)");
            }
            execute_status_report(self);
            self->click_count = 0U;
        }
    }
}

static void dispatch_pattern(SideButtonService_t *self, uint8_t clicks)
{
    if (self->logger)
    {
        Logger_Log(self->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                   "Dispatching pattern: %u click(s)", clicks);
    }

    switch (clicks)
    {
    case 1:
        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                       "Executing relay toggle (1 click)");
        }
        execute_relay_toggle(self);
        break;

    case 2:
        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                       "Executing GPS reset (2 clicks)");
        }
        execute_gps_reset(self);
        break;

    case 3:
        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_INFO, "SIDE_BTN", __FILE__, __LINE__,
                       "Executing emergency relay toggle (3 clicks)");
        }
        execute_emergency_relay_toggle(self);
        break;

    default:
        if (self->logger)
        {
            Logger_Log(self->logger, LOG_LEVEL_WARN, "SIDE_BTN", __FILE__, __LINE__,
                       "Unknown pattern: %u clicks (ignored)", clicks);
        }
        break;
    }
}

static void execute_relay_toggle(SideButtonService_t *self)
{
    RelayStatus_t status;
    Result_t res;

    res = RelayController_GetStatus(self->relay, &status);
    if (res != ERR_OK)
    {
        return;
    }

    if (status.contact_state == RELAY_CONTACT_CLOSED)
    {
        (void)RelayController_SetState(self->relay, RELAY_CONTACT_OPEN);
        (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_PRESS);
    }
    else
    {
        (void)RelayController_SetState(self->relay, RELAY_CONTACT_CLOSED);
        (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_PRESS);
    }
}

static void execute_gps_reset(SideButtonService_t *self)
{
    (void)GPS_Control_Reset(self->gps);
    (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_PRESS);
}

static void execute_emergency_relay_toggle(SideButtonService_t *self)
{
    RelayStatus_t status;
    Result_t res;

    res = RelayController_GetStatus(self->relay, &status);
    if (res != ERR_OK)
    {
        return;
    }

    if ((status.contact_state == RELAY_CONTACT_CLOSED) || (status.internal_state == RELAY_STATE_ACTIVE))
    {
        (void)RelayController_SetAlarmState(self->relay, true);
        (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_LONG_PRESS);
    }
    else
    {
        (void)RelayController_SetAlarmState(self->relay, false);
        (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_LONG_PRESS);
    }
}

static void execute_wifi_toggle(SideButtonService_t *self)
{
    if (WifiModule_IsEnabled(self->wifi_module))
    {
        (void)WifiModule_Disable(self->wifi_module);
    }
    else
    {
        (void)WifiModule_Enable(self->wifi_module);
    }
}

static void execute_factory_reset(SideButtonService_t *self)
{
    (void)ConfigStorage_ResetToDefaults(self->config);
    (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_CONFIG_SAVED);
}

static void execute_status_report(SideButtonService_t *self)
{
    (void)BuzzerNotificationService_NotifyEvent(self->buzzer, BUZZER_EVENT_BUTTON_PRESS);
}