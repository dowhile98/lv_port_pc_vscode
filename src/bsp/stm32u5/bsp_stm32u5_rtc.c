/**
 * @file bsp_stm32u5_rtc.c
 * @brief Implementación de la interfaz I_RTC para el hardware STM32U5.
 */

#include "bsp_stm32u5_rtc.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

/* ===== Prototipos de funciones privadas ===== */
static Result_t stm32u5_rtc_init(void *self, RTC_Handle_t handle);
static Result_t stm32u5_rtc_deinit(void *self, RTC_Handle_t handle);
static Result_t stm32u5_rtc_set_time(void *self, RTC_Handle_t handle, const RTC_Time_t *time);
static Result_t stm32u5_rtc_get_time(void *self, RTC_Handle_t handle, RTC_Time_t *time);
static Result_t stm32u5_rtc_set_date(void *self, RTC_Handle_t handle, const RTC_Date_t *date);
static Result_t stm32u5_rtc_get_date(void *self, RTC_Handle_t handle, RTC_Date_t *date);
static Result_t stm32u5_rtc_set_alarm(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config);
static Result_t stm32u5_rtc_set_alarm_it(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config);
static Result_t stm32u5_rtc_deactivate_alarm(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm);
static void stm32u5_rtc_write_backup_register(void *self, RTC_Handle_t handle, uint32_t regIndex, uint32_t data);
static uint32_t stm32u5_rtc_read_backup_register(void *self, RTC_Handle_t handle, uint32_t regIndex);
static void stm32u5_rtc_erase_backup_registers(void *self, RTC_Handle_t handle);
static void stm32u5_rtc_register_alarm_callback(void *self, RTC_Handle_t handle, RTC_AlarmCallback_t cb, void *context);
static void stm32u5_rtc_register_wakeup_callback(void *self, RTC_Handle_t handle, RTC_WakeupCallback_t cb, void *context);

/* ===== Contexto de callbacks ===== */
typedef struct
{
    RTC_AlarmCallback_t alarm_callback;
    void *alarm_context;
    RTC_WakeupCallback_t wakeup_callback;
    void *wakeup_context;
} STM32U5_RTC_Context_t;

static STM32U5_RTC_Context_t s_rtc_context = {0};

/* ===== Definición de la V-Table para STM32U5 ===== */
static const I_RTC_Vtable s_stm32u5_rtc_vtable = {
    .Init = stm32u5_rtc_init,
    .DeInit = stm32u5_rtc_deinit,
    .SetTime = stm32u5_rtc_set_time,
    .GetTime = stm32u5_rtc_get_time,
    .SetDate = stm32u5_rtc_set_date,
    .GetDate = stm32u5_rtc_get_date,
    .SetAlarm = stm32u5_rtc_set_alarm,
    .SetAlarm_IT = stm32u5_rtc_set_alarm_it,
    .DeactivateAlarm = stm32u5_rtc_deactivate_alarm,
    .WriteBackupRegister = stm32u5_rtc_write_backup_register,
    .ReadBackupRegister = stm32u5_rtc_read_backup_register,
    .EraseBackupRegisters = stm32u5_rtc_erase_backup_registers,
    .RegisterAlarmCallback = stm32u5_rtc_register_alarm_callback,
    .RegisterWakeupCallback = stm32u5_rtc_register_wakeup_callback};

/* ===== Instancia del objeto I_RTC ===== */
static I_RTC s_stm32u5_rtc_iface = {
    .vtable = &s_stm32u5_rtc_vtable,
    .impl = (void *)&s_stm32u5_rtc_vtable /* puntero válido para validación */
};

I_RTC *Bsp_Stm32U5_Rtc_GetInterface(void)
{
    return &s_stm32u5_rtc_iface;
}

/* ===== Implementaciones Privadas ===== */

static Result_t stm32u5_rtc_init(void *self, RTC_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* El RTC ya fue inicializado por MX_RTC_Init() en el BSP_Init */
    /* Esta función solo valida que el handle sea correcto */
    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    if (hrtc->Instance != RTC)
    {
        return ERR_INVALID_PARAM;
    }

    return ERR_OK;
}

static Result_t stm32u5_rtc_deinit(void *self, RTC_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    HAL_StatusTypeDef status = HAL_RTC_DeInit(hrtc);

    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_rtc_set_time(void *self, RTC_Handle_t handle, const RTC_Time_t *time)
{
    (void)self;
    if (handle == NULL || time == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_TimeTypeDef sTime = {0};

    sTime.Hours = time->hours;
    sTime.Minutes = time->minutes;
    sTime.Seconds = time->seconds;
    sTime.SubSeconds = time->subSeconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_StatusTypeDef status = HAL_RTC_SetTime(hrtc, &sTime, RTC_FORMAT_BIN);

    HAL_RTCEx_BKUPWrite(hrtc, RTC_BKP_DR1, 0x32F2); // Marca de backup para indicar que el RTC fue configurado al menos una vez

    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_rtc_get_time(void *self, RTC_Handle_t handle, RTC_Time_t *time)
{
    (void)self;
    if (handle == NULL || time == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_TimeTypeDef sTime = {0};

    HAL_StatusTypeDef status = HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BIN);
    if (status != HAL_OK)
    {
        return ERR_ERROR;
    }

    time->hours = sTime.Hours;
    time->minutes = sTime.Minutes;
    time->seconds = sTime.Seconds;
    time->subSeconds = sTime.SubSeconds;

    return ERR_OK;
}

static Result_t stm32u5_rtc_set_date(void *self, RTC_Handle_t handle, const RTC_Date_t *date)
{
    (void)self;
    if (handle == NULL || date == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_DateTypeDef sDate = {0};

    sDate.WeekDay = date->weekDay;
    sDate.Date = date->day;
    sDate.Month = date->month;
    sDate.Year = date->year;

    HAL_StatusTypeDef status = HAL_RTC_SetDate(hrtc, &sDate, RTC_FORMAT_BIN);
    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_rtc_get_date(void *self, RTC_Handle_t handle, RTC_Date_t *date)
{
    (void)self;
    if (handle == NULL || date == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_DateTypeDef sDate = {0};

    HAL_StatusTypeDef status = HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);
    if (status != HAL_OK)
    {
        return ERR_ERROR;
    }

    date->weekDay = sDate.WeekDay;
    date->day = sDate.Date;
    date->month = sDate.Month;
    date->year = sDate.Year;

    return ERR_OK;
}

static Result_t stm32u5_rtc_set_alarm(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_AlarmTypeDef sAlarm = {0};

    /* Configurar tiempo de alarma */
    sAlarm.AlarmTime.Hours = config->alarmTime.hours;
    sAlarm.AlarmTime.Minutes = config->alarmTime.minutes;
    sAlarm.AlarmTime.Seconds = config->alarmTime.seconds;
    sAlarm.AlarmTime.SubSeconds = config->alarmTime.subSeconds;

    /* Configurar máscara */
    sAlarm.AlarmMask = (uint32_t)config->alarmMask;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    /* Configurar fecha/día de semana */
    sAlarm.AlarmDateWeekDaySel = config->useWeekDay ? RTC_ALARMDATEWEEKDAYSEL_WEEKDAY : RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = config->alarmDateWeekDay;

    /* Seleccionar alarma A o B */
    sAlarm.Alarm = (alarm == I_RTC_ALARM_A) ? RTC_ALARM_A : RTC_ALARM_B;

    HAL_StatusTypeDef status = HAL_RTC_SetAlarm(hrtc, &sAlarm, RTC_FORMAT_BIN);
    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_rtc_set_alarm_it(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm, const RTC_AlarmConfig_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    RTC_AlarmTypeDef sAlarm = {0};

    /* Configurar tiempo de alarma */
    sAlarm.AlarmTime.Hours = config->alarmTime.hours;
    sAlarm.AlarmTime.Minutes = config->alarmTime.minutes;
    sAlarm.AlarmTime.Seconds = config->alarmTime.seconds;
    sAlarm.AlarmTime.SubSeconds = config->alarmTime.subSeconds;

    /* Configurar máscara */
    sAlarm.AlarmMask = (uint32_t)config->alarmMask;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    /* Configurar fecha/día de semana */
    sAlarm.AlarmDateWeekDaySel = config->useWeekDay ? RTC_ALARMDATEWEEKDAYSEL_WEEKDAY : RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = config->alarmDateWeekDay;

    /* Seleccionar alarma A o B */
    sAlarm.Alarm = (alarm == I_RTC_ALARM_A) ? RTC_ALARM_A : RTC_ALARM_B;

    HAL_StatusTypeDef status = HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, RTC_FORMAT_BIN);
    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static Result_t stm32u5_rtc_deactivate_alarm(void *self, RTC_Handle_t handle, RTC_Alarm_t alarm)
{
    (void)self;
    if (handle == NULL)
    {
        return ERR_NULL_POINTER;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;
    uint32_t hal_alarm = (alarm == I_RTC_ALARM_A) ? RTC_ALARM_A : RTC_ALARM_B;

    HAL_StatusTypeDef status = HAL_RTC_DeactivateAlarm(hrtc, hal_alarm);
    return (status == HAL_OK) ? ERR_OK : ERR_ERROR;
}

static void stm32u5_rtc_write_backup_register(void *self, RTC_Handle_t handle, uint32_t regIndex, uint32_t data)
{
    (void)self;
    if (handle == NULL)
    {
        return;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;

    /* STM32U5 tiene 32 backup registers (0-31) */
    if (regIndex >= 32)
    {
        return; /* Índice fuera de rango */
    }

    HAL_RTCEx_BKUPWrite(hrtc, regIndex, data);
}

static uint32_t stm32u5_rtc_read_backup_register(void *self, RTC_Handle_t handle, uint32_t regIndex)
{
    (void)self;
    if (handle == NULL)
    {
        return 0;
    }

    RTC_HandleTypeDef *hrtc = (RTC_HandleTypeDef *)handle;

    /* STM32U5 tiene 32 backup registers (0-31) */
    if (regIndex >= 32)
    {
        return 0; /* Índice fuera de rango */
    }

    return HAL_RTCEx_BKUPRead(hrtc, regIndex);
}

static void stm32u5_rtc_erase_backup_registers(void *self, RTC_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
    {
        return;
    }

    /* Borrar todos los backup registers */
    for (uint32_t i = 0; i < 32; i++)
    {
        stm32u5_rtc_write_backup_register(self, handle, i, 0);
    }
}

static void stm32u5_rtc_register_alarm_callback(void *self, RTC_Handle_t handle, RTC_AlarmCallback_t cb, void *context)
{
    (void)self;
    (void)handle;

    s_rtc_context.alarm_callback = cb;
    s_rtc_context.alarm_context = context;
}

static void stm32u5_rtc_register_wakeup_callback(void *self, RTC_Handle_t handle, RTC_WakeupCallback_t cb, void *context)
{
    (void)self;
    (void)handle;

    s_rtc_context.wakeup_callback = cb;
    s_rtc_context.wakeup_context = context;
}

/* ===== Callbacks HAL (conectan con la capa de aplicación) ===== */

/**
 * @brief Callback de alarma RTC del HAL.
 * @note Debe ser llamado por HAL_RTC_AlarmIRQHandler().
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    if (s_rtc_context.alarm_callback != NULL)
    {
        s_rtc_context.alarm_callback(s_rtc_context.alarm_context, I_RTC_ALARM_A);
    }
}

void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    if (s_rtc_context.alarm_callback != NULL)
    {
        s_rtc_context.alarm_callback(s_rtc_context.alarm_context, I_RTC_ALARM_B);
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    if (s_rtc_context.wakeup_callback != NULL)
    {
        s_rtc_context.wakeup_callback(s_rtc_context.wakeup_context);
    }
}
