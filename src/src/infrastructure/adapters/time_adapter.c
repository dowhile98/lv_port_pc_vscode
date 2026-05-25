#include "infrastructure/adapters/time_adapter.h"
#include "infrastructure/osal/osal.h"
#include "common/date_time.h"
#include <stddef.h>

/* ===== Mappings Helpers ===== */

static void datetime_to_rtc(const DateTime_t *dt, RTC_Time_t *rt, RTC_Date_t *rd)
{
    rt->hours = dt->hour;
    rt->minutes = dt->minute;
    rt->seconds = dt->second;
    rt->subSeconds = 0;

    rd->day = dt->day;
    rd->month = dt->month;
    rd->year = (uint8_t)(dt->year % 100);
    rd->weekDay = dt->weekDay;
}

static void rtc_to_datetime(const RTC_Time_t *rt, const RTC_Date_t *rd, DateTime_t *dt)
{
    dt->hour = rt->hours;
    dt->minute = rt->minutes;
    dt->second = rt->seconds;
    dt->day = rd->day;
    dt->month = rd->month;
    dt->year = 2000 + rd->year;
    dt->weekDay = rd->weekDay;
}

/* ===== ITimeSource Implementation ===== */

static Result_t time_adapter_get_time(void *self, DateTime_t *out_time)
{
    TimeAdapter_t *adapter = (TimeAdapter_t *)self;
    if (adapter == NULL || out_time == NULL)
        return ERR_NULL_POINTER;

    /* ACTION-007 Phase 3: GPS-aware time retrieval */
    /* Priority 1: Try GPS time if valid FIX_3D */
    if (adapter->gps != NULL)
    {
        GPSFixStatus_t fix_status;
        Result_t res_fix = GPS_Source_GetFixStatus(adapter->gps, &fix_status);

        if (res_fix == ERR_OK && fix_status == GPS_FIX_3D)
        {
            Result_t res_time = GPS_Source_GetTimeUTC(adapter->gps, out_time);
            if (res_time == ERR_OK)
            {
                return ERR_OK; /* GPS time retrieved successfully (±10ms accuracy) */
            }
        }
    }

    /* Priority 2: Fallback to RTC time */
    RTC_Time_t rt;
    RTC_Date_t rd;

    Result_t res = RTC_GetTime(adapter->rtc, adapter->rtc_handle, &rt);
    if (res != ERR_OK)
        return res;

    res = RTC_GetDate(adapter->rtc, adapter->rtc_handle, &rd);
    if (res != ERR_OK)
        return res;

    rtc_to_datetime(&rt, &rd, out_time);
    return ERR_OK;
}

static Result_t time_adapter_set_time(void *self, const DateTime_t *time)
{
    TimeAdapter_t *adapter = (TimeAdapter_t *)self;
    if (adapter == NULL || time == NULL)
        return ERR_NULL_POINTER;

    /* Compensate for processing delay: add 1 second to prevent RTC lag */
    DateTime_t adjusted_time = *time;
    DateTime_AddSeconds(&adjusted_time, 1);

    RTC_Time_t rt;
    RTC_Date_t rd;
    datetime_to_rtc(&adjusted_time, &rt, &rd);

    /* Both operations MUST succeed for synchronized state */
    Result_t res_time = RTC_SetTime(adapter->rtc, adapter->rtc_handle, &rt);
    if (res_time != ERR_OK)
        return res_time;

    Result_t res_date = RTC_SetDate(adapter->rtc, adapter->rtc_handle, &rd);
    if (res_date != ERR_OK)
        return res_date;

    /* Only mark synchronized if BOTH operations succeeded */
    adapter->is_synchronized = true;
    return ERR_OK;
}

static bool time_adapter_is_synchronized(void *self)
{
    TimeAdapter_t *adapter = (TimeAdapter_t *)self;
    return (adapter != NULL) ? adapter->is_synchronized : false;
}

static const ITimeSource_Vtable source_vtable = {
    .GetTime = time_adapter_get_time,
    .SetTime = time_adapter_set_time,
    .IsSynchronized = time_adapter_is_synchronized};

/* ===== ITimeSyncControl Implementation ===== */

static Result_t time_adapter_on_pps(void *self, const DateTime_t *gps_time)
{
    TimeAdapter_t *adapter = (TimeAdapter_t *)self;
    if (adapter == NULL)
        return ERR_NULL_POINTER;

    /* ACTION-007: Deactivate RTC alarm if recovering from RTC_ONLY */
    if (adapter->state == TIME_SYNC_STATE_GPS_ACTIVE && adapter->rtc_alarm_active)
    {
        (void)TimeAdapter_DeactivateRTCAlarm(adapter);
    }

    adapter->last_pps_timestamp = os_ticks_get();

    DateTime_t fetched_time;
    const DateTime_t *final_time = gps_time;

    /* If no time provided, try to fetch it from GPS source */
    if (final_time == NULL && adapter->gps != NULL)
    {
        if (GPS_Source_GetTimeUTC(adapter->gps, &fetched_time) == ERR_OK)
        {
            final_time = &fetched_time;
        }
    }

    /* ACTION-007: Conditional RTC write (throttling) */
    if (final_time != NULL)
    {
        bool should_write_rtc = false;
        uint32_t now = adapter->last_pps_timestamp;
        DateTime_t gps_fresh_time = {0}; /* Hoisted: used for RTC write AND event timestamps */

        /* CRÍTICO: Solo escribir RTC si el tiempo viene de GPS con FIX válido */
        /* NO escribir si es tiempo del RTC fallback (evita escritura redundante) */
        bool is_gps_time_valid = false;
        if (adapter->gps != NULL)
        {
            GPSFixStatus_t fix_status;
            if (GPS_Source_GetFixStatus(adapter->gps, &fix_status) == ERR_OK)
            {
                is_gps_time_valid = (fix_status == GPS_FIX_3D);
            }
        }

        /* Solo proceder si tenemos tiempo válido del GPS */
        if (is_gps_time_valid)
        {
            /* Obtener tiempo FRESCO del GPS antes de escribir RTC */
            if (GPS_Source_GetTimeUTC(adapter->gps, &gps_fresh_time) != ERR_OK)
            {
                /* Si no se puede obtener tiempo GPS, no escribir RTC */
                is_gps_time_valid = false;
            }
            else
            {
                /* Condition 1: First sync (never written from GPS) */
                if (!adapter->is_synchronized)
                {
                    should_write_rtc = true;
                }
                /* Condition 2: Write interval elapsed (throttling) */
                else if ((now - adapter->last_rtc_write_timestamp) >= adapter->rtc_write_interval_ms)
                {
                    should_write_rtc = true;
                }

                /* Only write RTC if conditions met */
                if (should_write_rtc)
                {
                    /* Usar tiempo GPS fresco, NO el tiempo pasado como parámetro */
                    Result_t res = time_adapter_set_time(adapter, &gps_fresh_time);
                    if (res == ERR_OK)
                    {
                        adapter->last_rtc_write_timestamp = now;
                    }
                }
            }
        }

        /* Transition to GPS_ACTIVE if we were waiting or in fallback */
        if (adapter->state == TIME_SYNC_STATE_WAIT_GPS_FIX ||
            adapter->state == TIME_SYNC_STATE_RTC_ONLY)
        {
            /* ACTION-007: Deactivate RTC alarm when GPS reconnects */
            if (adapter->state == TIME_SYNC_STATE_RTC_ONLY && adapter->rtc_alarm_active)
            {
                (void)TimeAdapter_DeactivateRTCAlarm(adapter);
            }

            /* Solo transicionar a GPS_ACTIVE si GPS tiene FIX válido */
            if (is_gps_time_valid)
            {
                adapter->state = TIME_SYNC_STATE_GPS_ACTIVE;
                if (EventNotifier_IsValid(adapter->event_notifier))
                {
                    /* gps_fresh_time ya fue obtenido y validado arriba */
                    (void)EventNotifier_NotifyEvent(adapter->event_notifier, &gps_fresh_time,
                                                    EVENT_TYPE_GPS_LOCK_ACQUIRED,
                                                    EVENT_SEVERITY_INFO, 0U, 0U);
                    (void)EventNotifier_NotifyEvent(adapter->event_notifier, &gps_fresh_time,
                                                    EVENT_TYPE_GPS_TIME_SYNC,
                                                    EVENT_SEVERITY_INFO, 0U, 0U);
                }
            }
        }
    }

    return ERR_OK;
}

static Result_t time_adapter_sync_time(void *self, const DateTime_t *time)
{
    return time_adapter_set_time(self, time);
}

static const ITimeSyncControl_Vtable sync_vtable = {
    .OnPPS = time_adapter_on_pps,
    .SyncTime = time_adapter_sync_time};

/* ===== Public API ===== */

Result_t TimeAdapter_Init(TimeAdapter_t *self, const TimeAdapterConfig_t *config)
{
    if (self == NULL || config == NULL || config->rtc == NULL)
        return ERR_NULL_POINTER;

    self->rtc = config->rtc;
    self->rtc_handle = config->rtc_handle;
    self->gps = config->gps;
    self->pps_dispatcher = config->pps_dispatcher;
    self->pps_timeout_ms = config->pps_timeout_ms;
    self->last_pps_timestamp = 0;
    self->is_synchronized = false;
    self->event_notifier = config->event_notifier; /* optional, may be NULL */

    /* ACTION-007: Initialize RTC write throttling */
    self->rtc_write_interval_ms = config->rtc_write_interval_ms;
    self->last_rtc_write_timestamp = 0;

    /* ACTION-007: Initialize RTC alarm fields */
    self->rtc_alarm_active = false;
    self->alarm_id = I_RTC_ALARM_A;
    self->alarm_event_context = NULL;

    /**
     * @note Start directly in WAIT_GPS_FIX state (no intermediate INIT state).
     *       TimeAdapter_Update() will manage transitions from here.
     */
    self->state = TIME_SYNC_STATE_WAIT_GPS_FIX;

    self->source_iface.vtable = &source_vtable;
    self->source_iface.impl = self;

    self->sync_iface.vtable = &sync_vtable;
    self->sync_iface.impl = self;

    self->is_initialized = true;
    return ERR_OK;
}

Result_t TimeAdapter_Update(TimeAdapter_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;

    uint32_t now = os_ticks_get();

    switch (self->state)
    {
    case TIME_SYNC_STATE_WAIT_GPS_FIX:
        /* ACTION-007: Si arranca sin GPS, configura alarma RTC como fallback inicial */
        if (!self->rtc_alarm_active)
        {
            /* Solo configurar si han pasado más de pps_timeout_ms sin señal GPS */
            if (self->last_pps_timestamp == 0 || (now - self->last_pps_timestamp) > self->pps_timeout_ms)
            {
                (void)TimeAdapter_ConfigureRTCAlarm(self, 1);
                /* Nota: Permanece en WAIT_GPS_FIX hasta que GPS provea tiempo válido */
            }
        }
        break;

    case TIME_SYNC_STATE_GPS_ACTIVE:
        /**
         * Detect PPS loss: if no OnPPS() called within timeout,
         * fall back to RTC-only mode.
         */
        if ((now - self->last_pps_timestamp) > self->pps_timeout_ms)
        {
            /* ACTION-007: Configure RTC alarm fallback */
            (void)TimeAdapter_ConfigureRTCAlarm(self, 1);
            self->state = TIME_SYNC_STATE_RTC_ONLY;
            if (EventNotifier_IsValid(self->event_notifier))
            {
                DateTime_t now_ts = {0};
                (void)time_adapter_get_time(self, &now_ts); /* Lee hora actual del RTC */
                (void)EventNotifier_NotifyEvent(self->event_notifier, &now_ts,
                                                EVENT_TYPE_GPS_LOCK_LOST,
                                                EVENT_SEVERITY_WARNING, 0U, 0U);
                (void)EventNotifier_NotifyEvent(self->event_notifier, &now_ts,
                                                EVENT_TYPE_GPS_TIME_SYNC_LOST,
                                                EVENT_SEVERITY_WARNING, 0U, 0U);
            }
        }
        break;

    case TIME_SYNC_STATE_RTC_ONLY:
        /**
         * Remain in RTC-only mode until OnPPS() provides valid GPS sync again.
         * OnPPS() will transition back to GPS_ACTIVE.
         */
        break;

    case TIME_SYNC_STATE_INIT:
    case TIME_SYNC_STATE_ERROR:
    default:
        break;
    }

    return ERR_OK;
}

ITimeSource *TimeAdapter_GetTimeSourceInterface(TimeAdapter_t *self)
{
    return (self != NULL) ? &self->source_iface : NULL;
}

ITimeSyncControl *TimeAdapter_GetSyncControlInterface(TimeAdapter_t *self)
{
    return (self != NULL) ? &self->sync_iface : NULL;
}

/* ===== ACTION-007: RTC Alarm Fallback Functions ===== */

/* Forward declaration */
static void time_adapter_rtc_alarm_callback(void *context, RTC_Alarm_t alarm);

Result_t TimeAdapter_ConfigureRTCAlarm(TimeAdapter_t *self, uint32_t seconds_offset)
{
    if (self == NULL || self->rtc == NULL)
        return ERR_NULL_POINTER;

    /* Get current RTC time */
    RTC_Time_t current_time;
    RTC_Date_t current_date;
    Result_t res = RTC_GetTime(self->rtc, self->rtc_handle, &current_time);
    if (res != ERR_OK)
        return res;
    res = RTC_GetDate(self->rtc, self->rtc_handle, &current_date);
    if (res != ERR_OK)
        return res;

    /* Calculate alarm time (+seconds_offset) */
    DateTime_t dt;
    rtc_to_datetime(&current_time, &current_date, &dt);
    DateTime_AddSeconds(&dt, seconds_offset);

    /* Convert back to RTC format */
    RTC_Time_t alarm_time;
    RTC_Date_t alarm_date;
    datetime_to_rtc(&dt, &alarm_time, &alarm_date);

    /* Configure alarm */
    RTC_AlarmConfig_t alarm_cfg = {
        .alarmTime = alarm_time,
        .alarmDateWeekDay = alarm_date.weekDay,
        .alarmMask = I_RTC_ALARMMASK_NONE,
        .useWeekDay = true};

    /* Register callback before enabling alarm */
    RTC_RegisterAlarmCallback(self->rtc, self->rtc_handle, time_adapter_rtc_alarm_callback, self);

    res = RTC_SetAlarm_IT(self->rtc, self->rtc_handle, I_RTC_ALARM_A, &alarm_cfg);
    if (res == ERR_OK)
    {
        self->rtc_alarm_active = true;
        self->alarm_id = I_RTC_ALARM_A;
    }
    return res;
}

Result_t TimeAdapter_DeactivateRTCAlarm(TimeAdapter_t *self)
{
    if (self == NULL || self->rtc == NULL)
        return ERR_NULL_POINTER;
    if (!self->rtc_alarm_active)
        return ERR_OK;

    Result_t res = RTC_DeactivateAlarm(self->rtc, self->rtc_handle, self->alarm_id);
    if (res == ERR_OK)
    {
        self->rtc_alarm_active = false;
    }
    return res;
}

void TimeAdapter_RegisterAlarmEventContext(TimeAdapter_t *self, void *context)
{
    if (self != NULL)
    {
        self->alarm_event_context = context;
    }
}

static void time_adapter_rtc_alarm_callback(void *context, RTC_Alarm_t alarm)
{
    TimeAdapter_t *self = (TimeAdapter_t *)context;
    if (self == NULL || alarm != self->alarm_id)
        return;

    /* Update last PPS timestamp (RTC alarm acts as PPS source) */
    self->last_pps_timestamp = os_ticks_get();

    /* ACTION-007: Notificar a subscribers con PPS sintético (usar tiempo del RTC) */
    /* CRÍTICO: NO llamar time_adapter_on_pps() con tiempo RTC (evita escritura redundante) */
    /* Solo notificar a subscribers que necesitan el tick periódico */

    /* Get current RTC time for subscribers (pero NO para escribir RTC) */
    RTC_Time_t rtc_time;
    RTC_Date_t rtc_date;
    if (RTC_GetTime(self->rtc, self->rtc_handle, &rtc_time) == ERR_OK &&
        RTC_GetDate(self->rtc, self->rtc_handle, &rtc_date) == ERR_OK)
    {
        DateTime_t synthetic_pps_time;
        rtc_to_datetime(&rtc_time, &rtc_date, &synthetic_pps_time);

        /* Notificar SOLO a subscribers externos (RelayAdapter, Logger, etc.) */
        /* NO actualizar estado interno de TimeAdapter (evita escritura RTC) */
        if (self->pps_dispatcher != NULL)
        {
            PPSDispatcher_Dispatch(self->pps_dispatcher, &synthetic_pps_time);
        }
    }

    /* Reconfigure alarm for next second (continuous 1Hz PPS) */
    (void)TimeAdapter_ConfigureRTCAlarm(self, 1);
}

Result_t TimeAdapter_Deinit(TimeAdapter_t *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_initialized)
        return ERR_OK; /* Idempotente */

    /* Desactivar alarma RTC si está activa */
    if (self->rtc_alarm_active)
    {
        (void)TimeAdapter_DeactivateRTCAlarm(self);
    }

    /* No hay recursos dinámicos que liberar */
    /* Solo resetear estado */
    self->is_initialized = false;
    self->state = TIME_SYNC_STATE_WAIT_GPS_FIX;
    return ERR_OK;
}

/* ===== ACTION-007: Getters para diagnóstico ===== */

TimeSyncState_t TimeAdapter_GetSyncState(const TimeAdapter_t *self)
{
    return (self != NULL) ? self->state : TIME_SYNC_STATE_ERROR;
}

PPSSource_t TimeAdapter_GetPPSSource(const TimeAdapter_t *self)
{
    if (self == NULL)
        return PPS_SOURCE_NONE;

    if (self->state == TIME_SYNC_STATE_GPS_ACTIVE)
        return PPS_SOURCE_GPS;
    else if (self->rtc_alarm_active)
        return PPS_SOURCE_RTC;
    else
        return PPS_SOURCE_NONE;
}
