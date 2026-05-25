#include "infrastructure/adapters/gps_adapter.h"

#include <string.h>

/* ACTION-007 Phase 4: UTC offset support */
#include "domain/time/utc_offsets.h"
#include "common/date_time.h"

#ifndef GPS_ADAPTER_DEFAULT_MIN_PPS
#define GPS_ADAPTER_DEFAULT_MIN_PPS (5U)
#endif

/* Forward declarations de las vtables */
static Result_t gps_adapter_get_position(void *impl, GPSPosition_t *out_position);
static Result_t gps_adapter_get_time(void *impl, DateTime_t *out_time);
static Result_t gps_adapter_get_fix(void *impl, GPSFixStatus_t *out_status);
static Result_t gps_adapter_register_pps(void *impl, PPSCallback_t callback, void *context);
static void gps_adapter_on_exti(void *context, GPIO_Pin_t pin);
static void gps_adapter_on_nmea_statement(GPSAdapter *self);
static Result_t gps_ingestor_process_rx(void *impl, const uint8_t *data, uint16_t len);
static Result_t gps_ingestor_process(void *impl);
static Result_t gps_control_reset(void *impl);
static Result_t gps_control_apply_hw_config(void *impl);

static const IGPSSource_Vtable s_gps_vtable = {
    .GetPosition = gps_adapter_get_position,
    .GetTimeUTC = gps_adapter_get_time,
    .GetFixStatus = gps_adapter_get_fix,
    .RegisterPPSCallback = gps_adapter_register_pps};

static const IGPSIngestor_Vtable s_gps_ingest_vtable = {
    .ProcessRxBuffer = gps_ingestor_process_rx,
    .Process = gps_ingestor_process};

static const IGPSControl_Vtable s_gps_control_vtable = {
    .Reset = gps_control_reset,
    .ApplyHardwareConfig = gps_control_apply_hw_config};

/**
 * @brief Convierte fix_mode de lwGPS a GPSFixStatus_t.
 */
static GPSFixStatus_t convert_lwgps_fix_to_status(uint8_t fix_mode)
{
    switch (fix_mode)
    {
    case 1:
        return GPS_FIX_NONE;
    case 2:
        return GPS_FIX_2D;
    case 3:
        return GPS_FIX_3D;
    default:
        return GPS_FIX_NONE;
    }
}

/**
 * @brief Calcula el día de la semana usando la Congruencia de Zeller (Gregorian).
 * @param dt Puntero a DateTime_t con fecha válida (year >= 2000).
 * @return Día de la semana: 1=Lunes, 2=Martes, ..., 7=Domingo.
 * @note Adaptado de legacy tcs_cicx1_calculateDayOfWeek().
 */
static uint8_t calculate_day_of_week(const DateTime_t *dt)
{
    uint8_t day = dt->day;
    uint8_t month = dt->month;
    uint16_t full_year = dt->year;

    /* Ajustar mes y año para Zeller (Ene y Feb = meses 13 y 14 del año anterior) */
    if (month < 3)
    {
        month += 12;
        full_year -= 1;
    }

    int q = day;             /* Día del mes */
    int m = month;           /* Mes ajustado */
    int K = full_year % 100; /* Año del siglo */
    int J = full_year / 100; /* Siglo */

    /* Congruencia de Zeller (versión Gregorian) */
    int h = (q + (13 * (m + 1)) / 5) + K + (K / 4) + (J / 4) + (5 * J);
    h %= 7;

    /* Ajustar resultado: Lunes=1, Martes=2, ..., Domingo=7 */
    h = (h + 5) % 7 + 1;

    return (uint8_t)h;
}

Result_t GPSAdapter_Init(GPSAdapter *self,
                         I_UART *uart,
                         I_EXTI *exti,
                         GPIO_Pin_t pps_pin,
                         const GPSAdapterConfig_t *config)
{
    if (self == NULL || uart == NULL || exti == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!uart_is_valid(uart) || !exti_is_valid(exti))
    {
        return ERR_INVALID_PARAM;
    }

    self->iface.vtable = &s_gps_vtable;
    self->iface.impl = self;
    self->ingest.vtable = &s_gps_ingest_vtable;
    self->ingest.impl = self;
    self->control.vtable = &s_gps_control_vtable;
    self->control.impl = self;
    self->uart = uart;
    self->exti = exti;
    self->pps_pin = pps_pin;
    self->pps_callback = NULL;
    self->pps_context = NULL;
    self->last_position = (GPSPosition_t){0};
    self->last_time = (DateTime_t){0};
    self->fix_status = GPS_FIX_NONE;
    self->has_time = false;
    self->has_position = false;
    self->pps_valid_count = 0U;
    self->pps_minimum_valid = GPS_ADAPTER_DEFAULT_MIN_PPS;

    /* ACTION-007 Phase 4: Initialize UTC offset configuration */
    self->utc_offset_index = 14; /* Default: UTC+0 (index 14 in UTC_OFFSETS) */
    self->seconds_offset = 0;

    /* Hardware control initialization */
    self->antenna_type = 0; /* Default: internal antenna */
    self->gpio = NULL;

    if (config != NULL && config->pps_minimum_valid > 0U)
    {
        self->pps_minimum_valid = config->pps_minimum_valid;

        /* ACTION-007 Phase 4: Store UTC offset configuration */
        if (config->utc_offset_index >= 0 && config->utc_offset_index < 38)
        {
            self->utc_offset_index = config->utc_offset_index;
        }
        self->seconds_offset = config->seconds_offset;
        /* Hardware control configuration */
        if (config->gpio != NULL)
        {
            self->antenna_type = config->antenna_type;
            self->gpio = config->gpio;
            self->rf_ctrl1_port = config->rf_ctrl1_port;
            self->rf_ctrl1_pin = config->rf_ctrl1_pin;
            self->rf_ctrl2_port = config->rf_ctrl2_port;
            self->rf_ctrl2_pin = config->rf_ctrl2_pin;
            self->gps_rst_port = config->gps_rst_port;
            self->gps_rst_pin = config->gps_rst_pin;
        }
    }

    /* BLOCKER 1: Inicializar lwGPS parser */
    lwgps_init(&self->gps_parser);

    /* BLOCKER 1: Inicializar buffer circular lwRB */
    lwrb_init(&self->rx_ringbuffer, self->rx_buffer_data, sizeof(self->rx_buffer_data));

    Result_t reg_result = EXTI_RegisterCallback(self->exti, self->pps_pin, gps_adapter_on_exti, self);
    if (reg_result != ERR_OK)
    {
        return reg_result;
    }

    Result_t exti_result = EXTI_Enable(self->exti, self->pps_pin);
    if (exti_result != ERR_OK)
    {
        return exti_result;
    }

    /* Apply initial hardware configuration (antenna selection) */
    Result_t hw_result = GPSAdapter_ApplyHardwareConfig(self);
    if (hw_result != ERR_OK)
    {
        return hw_result;
    }

    /* Execute GPS reset sequence (brings GPS to known state) */
    Result_t reset_result = GPSAdapter_Reset(self);
    if (reset_result != ERR_OK)
    {
        return reset_result;
    }

    self->is_initialized = true;
    return ERR_OK;
}

Result_t GPSAdapter_Process(GPSAdapter *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    uint8_t byte;
    bool processed_any = false;

    /* Consumir todo el buffer y alimentar el parser */
    while (lwrb_read(&self->rx_ringbuffer, &byte, 1))
    {
        /* GPSAdapter_ProcessNMEAByte gestiona el parser y s_current_adapter */
        (void)GPSAdapter_ProcessNMEAByte(self, byte);
        processed_any = true;
    }

    return processed_any ? ERR_OK : ERR_BUSY;
}

Result_t GPSAdapter_ProcessNMEAByte(GPSAdapter *self, uint8_t byte)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* BLOCKER 1: Procesar byte con lwGPS */

    lwgps_process(&self->gps_parser, &byte, 1); /* NULL = sin callback de eventos */
    /* Llamar manualmente al handler si no hay callback */
    if (self->gps_parser.p.stat != STAT_UNKNOWN)
    {
        gps_adapter_on_nmea_statement(self);
    }

    return ERR_OK;
}

Result_t GPSAdapter_ProcessRxBuffer(GPSAdapter *self, const uint8_t *data, uint16_t len)
{
    if (self == NULL || data == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* BLOCKER 1: Copiar datos al buffer circular (thread-safe) */
    lwrb_sz_t written = lwrb_write(&self->rx_ringbuffer, data, len);

    return (written == len) ? ERR_OK : ERR_BUSY;
}

/**
 * @brief Callback interno cuando lwGPS completa el parsing de una sentencia NMEA.
 * @note  Esta función se ejecuta en contexto del thread que llama a lwgps_process().
 */
static void gps_adapter_on_nmea_statement(GPSAdapter *self)
{
    if (self == NULL)
    {
        return;
    }
    lwgps_statement_t stat = self->gps_parser.p.stat;
    /* Actualizar datos según el tipo de sentencia */
    switch (stat)
    {
    case STAT_GGA:
#if LWGPS_CFG_STATEMENT_GPGGA
        /* Actualizar posición */
        self->last_position.latitude = (float)self->gps_parser.latitude;
        self->last_position.longitude = (float)self->gps_parser.longitude;
        self->last_position.altitude = (float)self->gps_parser.altitude;
        self->last_position.satellites_used = self->gps_parser.sats_in_use;
        self->last_position.fix_quality = self->gps_parser.fix;
        self->has_position = true;

        /* ✅ FIX: Actualizar SOLO hora si ya tenemos fecha válida (de RMC previo)
         * Si no tenemos fecha, NO marcar has_time como true aún */
        if (self->last_time.year >= 2000) /* Fecha válida ya recibida de RMC */
        {
            self->last_time.hour = self->gps_parser.hours;
            self->last_time.minute = self->gps_parser.minutes;
            self->last_time.second = self->gps_parser.seconds;
            self->has_time = true;
        }
        else
        {
            /* Solo guardar hora, pero NO marcar has_time hasta tener fecha */
            self->last_time.hour = self->gps_parser.hours;
            self->last_time.minute = self->gps_parser.minutes;
            self->last_time.second = self->gps_parser.seconds;
            /* has_time sigue false hasta recibir RMC */
        }
#endif
        break;

    case STAT_RMC:
#if LWGPS_CFG_STATEMENT_GPRMC
        /* ✅ FIX: Actualizar fecha/hora completa de RMC (más confiable) */
        self->last_time.day = self->gps_parser.date;
        self->last_time.month = self->gps_parser.month;
        self->last_time.year = 2000U + self->gps_parser.year; /* lwGPS devuelve año desde 2000 */

        /* También actualizar hora de RMC (puede ser más reciente que GGA) */
        self->last_time.hour = self->gps_parser.hours;
        self->last_time.minute = self->gps_parser.minutes;
        self->last_time.second = self->gps_parser.seconds;

        /* ✅ Marcar has_time solo si RMC es válido */
        self->has_time = (self->gps_parser.is_valid != 0);
#endif
        break;

    case STAT_GSA:
#if LWGPS_CFG_STATEMENT_GPGSA
        /* Actualizar fix status desde GPGSA (más preciso) */
        self->fix_status = convert_lwgps_fix_to_status(self->gps_parser.fix_mode);

        /* Reset PPS count si perdemos FIX 3D */
        if (self->fix_status != GPS_FIX_3D)
        {
            self->pps_valid_count = 0U;
        }
#endif
        break;

    case STAT_CHECKSUM_FAIL:
        /* Log error o incrementar contador de errores */
        break;

    default:
        break;
    }
}

Result_t GPSAdapter_OnPPS(GPSAdapter *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->has_time || self->fix_status != GPS_FIX_3D)
    {
        return ERR_BUSY;
    }

    self->pps_valid_count++;

    if (self->pps_valid_count >= self->pps_minimum_valid && self->pps_callback != NULL)
    {
        self->pps_callback(self->pps_context, &self->last_time);
    }

    return ERR_OK;
}

IGPSSource *GPSAdapter_GetInterface(GPSAdapter *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->iface;
}

IGPSIngestor *GPSAdapter_GetIngestInterface(GPSAdapter *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->ingest;
}

IGPSControl *GPSAdapter_GetControlInterface(GPSAdapter *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return &self->control;
}

static Result_t gps_adapter_get_position(void *impl, GPSPosition_t *out_position)
{
    GPSAdapter *self = (GPSAdapter *)impl;
    if (self == NULL || out_position == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->has_position)
    {
        return ERR_BUSY;
    }

    *out_position = self->last_position;
    return ERR_OK;
}

static Result_t gps_adapter_get_time(void *impl, DateTime_t *out_time)
{
    GPSAdapter *self = (GPSAdapter *)impl;
    if (self == NULL || out_time == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (!self->has_time)
    {
        return ERR_BUSY;
    }

    *out_time = self->last_time;

    /* ACTION-007 Phase 4: Apply UTC offset to GPS time */
    if (self->utc_offset_index >= 0 && self->utc_offset_index < UTC_OFFSET_COUNT)
    {
        /* Convert UTC offset hours to seconds */
        float utc_hours = UTC_OFFSETS[self->utc_offset_index];
        int32_t offset_seconds = (int32_t)(utc_hours * 3600.0f);

        /* Add additional seconds offset (±10s fine-tuning) */
        offset_seconds += self->seconds_offset + 1;

        /* Apply total offset to time */
        DateTime_AddSeconds(out_time, offset_seconds);
    }

    /* Calculate weekday using Zeller's congruence (after UTC offset applied) */
    out_time->weekDay = calculate_day_of_week(out_time);

    return ERR_OK;
}

static Result_t gps_adapter_get_fix(void *impl, GPSFixStatus_t *out_status)
{
    GPSAdapter *self = (GPSAdapter *)impl;
    if (self == NULL || out_status == NULL)
    {
        return ERR_NULL_POINTER;
    }

    *out_status = self->fix_status;
    return ERR_OK;
}

static Result_t gps_adapter_register_pps(void *impl, PPSCallback_t callback, void *context)
{
    GPSAdapter *self = (GPSAdapter *)impl;
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    self->pps_callback = callback;
    self->pps_context = context;
    return ERR_OK;
}

static void gps_adapter_on_exti(void *context, GPIO_Pin_t pin)
{
    GPSAdapter *self = (GPSAdapter *)context;
    if (self == NULL)
    {
        return;
    }

    if (pin != self->pps_pin)
    {
        return;
    }

    (void)GPSAdapter_OnPPS(self);
}

static Result_t gps_ingestor_process_rx(void *impl, const uint8_t *data, uint16_t len)
{
    return GPSAdapter_ProcessRxBuffer((GPSAdapter *)impl, data, len);
}

static Result_t gps_ingestor_process(void *impl)
{
    return GPSAdapter_Process((GPSAdapter *)impl);
}

/* ===== IGPSControl Vtable Implementations ===== */

static Result_t gps_control_reset(void *impl)
{
    return GPSAdapter_Reset((GPSAdapter *)impl);
}

static Result_t gps_control_apply_hw_config(void *impl)
{
    return GPSAdapter_ApplyHardwareConfig((GPSAdapter *)impl);
}

Result_t GPSAdapter_ApplyHardwareConfig(GPSAdapter *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (self->gpio == NULL)
    {
        /* No GPIO configured, skip hardware control */
        return ERR_OK;
    }

    /* Select antenna based on antenna_type (0=internal, 1=external) */
    if (self->antenna_type == 0)
    {
        /* Internal antenna: RF_CTRL1=HIGH, RF_CTRL2=LOW */
        GPIO_WritePin(self->gpio, self->rf_ctrl1_port, self->rf_ctrl1_pin, I_GPIO_STATE_HIGH);
        GPIO_WritePin(self->gpio, self->rf_ctrl2_port, self->rf_ctrl2_pin, I_GPIO_STATE_LOW);
    }
    else
    {
        /* External antenna: RF_CTRL1=LOW, RF_CTRL2=HIGH */
        GPIO_WritePin(self->gpio, self->rf_ctrl1_port, self->rf_ctrl1_pin, I_GPIO_STATE_LOW);
        GPIO_WritePin(self->gpio, self->rf_ctrl2_port, self->rf_ctrl2_pin, I_GPIO_STATE_HIGH);
    }

    /* Note: Reset is handled separately by GPSAdapter_Reset() */
    return ERR_OK;
}

Result_t GPSAdapter_Reset(GPSAdapter *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (self->gpio == NULL)
    {
        /* No GPIO configured, cannot reset */
        return ERR_INVALID_PARAM;
    }

    /* GPS Reset sequence (active LOW):
     * 1. Assert reset (LOW)
     * 2. Hold for minimum 100ms (GPS datasheet requirement)
     * 3. Release reset (HIGH)
     * 4. Wait for GPS boot (typical 1s)
     */

    /* Step 1: Assert reset */
    GPIO_WritePin(self->gpio, self->gps_rst_port, self->gps_rst_pin, I_GPIO_STATE_LOW);

    /* Step 2: Hold reset (100ms minimum per Neo-M9N datasheet) */
    os_thread_sleep(100);

    /* Step 3: Release reset */
    GPIO_WritePin(self->gpio, self->gps_rst_port, self->gps_rst_pin, I_GPIO_STATE_HIGH);

    /* Step 4: Wait for GPS internal boot sequence */
    os_thread_sleep(1000);

    /* Reset internal state (GPS lost all ephemeris data) */
    self->has_time = false;
    self->has_position = false;
    self->fix_status = GPS_FIX_NONE;
    self->pps_valid_count = 0U;

    return ERR_OK;
}

Result_t GPSAdapter_Deinit(GPSAdapter *self)
{
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Limpiar callbacks PPS */
    self->pps_callback = NULL;
    self->pps_context = NULL;

    /* Reset parser lwGPS */
    lwgps_init(&self->gps_parser);

    return ERR_OK;
}
