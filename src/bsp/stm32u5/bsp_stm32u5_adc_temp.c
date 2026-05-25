/**
 * @file   bsp_stm32u5_adc_temp.c
 * @brief  STM32U575 internal temperature sensor driver — ADC1, polling mode.
 *
 * @details
 * Implements ITemperatureSensor_t backed by ADC1_IN_TEMPSENSOR (channel 19).
 * Uses HAL blocking API. Channel is configured once in BspAdcTemp_Init().
 * Each read averages ADCTEMP_AVG_SAMPLES consecutive conversions to reduce
 * the inherent ±4-6 °C single-sample noise of the internal sensor.
 *
 * Formula (RM0456 § ADC — "Reading the temperature"):
 *
 *   TS_DATA_NORM = TS_RAW × VDDA_MV / CAL_VREF_MV          (Vref+ normalisation)
 *
 *   T(°C) = (CAL2_TEMP - CAL1_TEMP) × (TS_DATA_NORM - CAL1)
 *           ──────────────────────────────────────────────────  + CAL1_TEMP
 *                        (CAL2 - CAL1)
 *
 * Calibration constants (factory-programmed, STM32U575, RM0456 Table 52):
 *   CAL1 = *((uint16_t*)0x0BFA0710)  — raw ADC at  30 °C, Vref+ = 3.0 V
 *   CAL2 = *((uint16_t*)0x0BFA0742)  — raw ADC at 110 °C, Vref+ = 3.0 V
 *
 * @note   TEMPSENSOR_CAL2_TEMP from stm32u5xx_ll_adc.h is 130 — this is a
 *         copy-paste error from STM32F4 headers. RM0456 Table 52 confirms
 *         TS_CAL2 on STM32U575 was acquired at 110 °C. We override it below.
 *
 * @note   ADCTEMP_VDDA_MV must match the actual VDDA supply of the board.
 *         Default 3300 mV (typical STM32 board). Set to 3000 if board runs
 *         at 3.0 V to eliminate the Vref+ correction.
 */

#include "bsp/stm32u5/bsp_stm32u5_adc_temp.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_ll_adc.h" /* TEMPSENSOR_CAL*_ADDR, TEMPSENSOR_CAL*_TEMP */
#include <stddef.h>

/* ── Board VDDA assumption ────────────────────────────────────────────────── */
/**
 * @def   ADCTEMP_VDDA_MV
 * @brief Board VDDA supply voltage in millivolts.
 *
 * Used to normalise the raw ADC reading to the calibration reference (3000 mV).
 * Override at build time: -DADCTEMP_VDDA_MV=3000 for a 3.0 V board.
 */
#ifndef ADCTEMP_VDDA_MV
#define ADCTEMP_VDDA_MV 3300UL
#endif

/**
 * @def   ADCTEMP_CAL2_TEMP_DEGC
 * @brief Correct TS_CAL2 temperature for STM32U575 per RM0456 Table 52.
 *
 * The LL header macro TEMPSENSOR_CAL2_TEMP is 130 — a copy-paste error from
 * STM32F4. On STM32U575, factory calibration point 2 is at 110 °C.
 * Using 130 would stretch the interpolation slope by ×(100/80), causing a
 * ~5 °C error at 50 °C and growing linearly with temperature.
 */
#define ADCTEMP_CAL2_TEMP_DEGC 110L

/**
 * @def   ADCTEMP_AVG_SAMPLES
 * @brief Number of consecutive conversions to average per read call.
 *
 * The STM32U575 internal temperature sensor has ±4-6 °C peak-to-peak
 * single-sample noise. Averaging 8 samples reduces it to ~±1.5 °C.
 * Increase to 16 for ±0.8 °C (adds ~0.4 ms extra per read at 16 MHz ADC).
 */
#ifndef ADCTEMP_AVG_SAMPLES
#define ADCTEMP_AVG_SAMPLES 8U
#endif

/* ── Timeout ──────────────────────────────────────────────────────────────── */
/** Maximum wait for ADC conversion complete (ms). */
#define ADCTEMP_CONVERSION_TIMEOUT_MS 10UL

/* ── Private context ──────────────────────────────────────────────────────── */
typedef struct
{
    ADC_HandleTypeDef *hadc; /**< ADC1 handle (not owned — provided by caller). */
    bool is_init;            /**< True after successful BspAdcTemp_Init(). */
} AdcTempCtx_t;

static AdcTempCtx_t s_ctx = {.hadc = NULL, .is_init = false};

/* ── Forward declarations ─────────────────────────────────────────────────── */
static Result_t read_temperature_celsius(void *self, float *out_celsius);

/* ── Static vtable (one instance shared across all callers) ───────────────── */
static const ITemperatureSensor_Vtable s_vtable = {
    .ReadTemperatureCelsius = read_temperature_celsius,
};

/* ── Static interface handle ──────────────────────────────────────────────── */
static ITemperatureSensor_t s_iface = {
    .vtable = &s_vtable,
    .impl = &s_ctx,
};

/* ============================================================================
 * PUBLIC API
 * ========================================================================= */

Result_t BspAdcTemp_Init(ADC_HandleTypeDef *hadc)
{
    if (hadc == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Reject if not ADC1 — TEMPSENSOR channel is only on ADC1/ADC2 */
    if (hadc->Instance != ADC1)
    {
        return ERR_INVALID_PARAM;
    }

    /* Run ADC self-calibration (single-ended, no differential input) */
    if (HAL_ADCEx_Calibration_Start(hadc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
    {
        return ERR_ERROR;
    }

    /* Configure TEMPSENSOR channel once — no need to repeat on every read */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES; /* 391/16MHz ≈ 24 µs > tSTART_min 10 µs */
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0UL;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK)
    {
        return ERR_ERROR;
    }

    s_ctx.hadc = hadc;
    s_ctx.is_init = true;

    return ERR_OK;
}

ITemperatureSensor_t *BspAdcTemp_GetInterface(void)
{
    if (!s_ctx.is_init)
    {
        return NULL;
    }
    return &s_iface;
}

/* ============================================================================
 * PRIVATE IMPLEMENTATION
 * ========================================================================= */

/**
 * @brief  Convert a raw 14-bit ADC value to degrees Celsius.
 *
 * Applies VDDA normalisation then the factory-calibration formula.
 * Uses ADCTEMP_CAL2_TEMP_DEGC (110 °C) per RM0456 Table 52 — NOT the
 * erroneous TEMPSENSOR_CAL2_TEMP (130) from the LL header.
 *
 * @param[in]  ts_raw         Raw 14-bit ADC reading of the temperature sensor.
 * @param[out] out_celsius    Computed temperature in °C.
 */
static void convert_raw_to_celsius(uint32_t ts_raw, float *out_celsius)
{
    /* --- 1. Normalise for actual VDDA vs calibration reference (3000 mV) --- */
    /* TS_DATA_NORM = ts_raw × VDDA_MV / CAL_VREF_MV                         */
    /* At 3.3 V: NORM = ts_raw × 3300/3000 ≈ ts_raw × 1.1                    */
    uint32_t ts_norm = (uint32_t)(ts_raw * ADCTEMP_VDDA_MV / TEMPSENSOR_CAL_VREFANALOG);

    /* --- 2. Read factory calibration data from system flash --- */
    int32_t cal1 = (int32_t)(*TEMPSENSOR_CAL1_ADDR);     /* raw ADC at CAL1_TEMP  */
    int32_t cal2 = (int32_t)(*TEMPSENSOR_CAL2_ADDR);     /* raw ADC at CAL2_TEMP  */
    int32_t cal1_temp = (int32_t)TEMPSENSOR_CAL1_TEMP;   /* 30 °C                 */
    int32_t cal2_temp = (int32_t)ADCTEMP_CAL2_TEMP_DEGC; /* 110 °C (RM0456 §22)  */

    /* --- 3. Apply calibration formula (float arithmetic) --- */
    /*
     * T = (CAL2_TEMP - CAL1_TEMP) × (TS_NORM - CAL1)
     *     ─────────────────────────────────────────────  + CAL1_TEMP
     *                  (CAL2 - CAL1)
     */
    float numerator = (float)(cal2_temp - cal1_temp) * (float)((int32_t)ts_norm - cal1);
    float denominator = (float)(cal2 - cal1);

    *out_celsius = (numerator / denominator) + (float)cal1_temp;
}

/**
 * @brief  vtable implementation — perform averaged ADC read and convert to °C.
 *
 * Performs ADCTEMP_AVG_SAMPLES consecutive conversions and averages the raw
 * values before applying the calibration formula. This reduces the ±4-6 °C
 * peak-to-peak single-sample noise of the internal sensor to ~±1.5 °C.
 *
 * Channel was already configured in BspAdcTemp_Init(); no reconfiguration
 * needed here.
 */
static Result_t read_temperature_celsius(void *self, float *out_celsius)
{
    AdcTempCtx_t *ctx = (AdcTempCtx_t *)self;

    if (ctx == NULL || out_celsius == NULL)
    {
        return ERR_NULL_POINTER;
    }
    if (!ctx->is_init || ctx->hadc == NULL)
    {
        return ERR_ERROR;
    }

    /* --- Average ADCTEMP_AVG_SAMPLES conversions -------------------------  */
    uint32_t accumulator = 0UL;

    for (uint32_t i = 0UL; i < ADCTEMP_AVG_SAMPLES; i++)
    {
        if (HAL_ADC_Start(ctx->hadc) != HAL_OK)
        {
            return ERR_ERROR;
        }

        if (HAL_ADC_PollForConversion(ctx->hadc, ADCTEMP_CONVERSION_TIMEOUT_MS) != HAL_OK)
        {
            (void)HAL_ADC_Stop(ctx->hadc);
            return ERR_TIMEOUT;
        }

        accumulator += HAL_ADC_GetValue(ctx->hadc);
        (void)HAL_ADC_Stop(ctx->hadc);
    }

    uint32_t ts_raw = accumulator / ADCTEMP_AVG_SAMPLES;

    /* --- Convert averaged raw value to °C --------------------------------- */
    convert_raw_to_celsius(ts_raw, out_celsius);

    return ERR_OK;
}
