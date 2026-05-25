/**
 * @file   bsp_stm32u5_adc_temp.h
 * @brief  BSP driver for STM32U575 internal temperature sensor via ADC1.
 *
 * @details
 * Reads the on-chip temperature sensor connected to ADC1 (channel TEMPSENSOR)
 * using blocking HAL polling. Applies the factory calibration formula:
 *
 *   T(°C) = (TS_CAL2_TEMP - TS_CAL1_TEMP) × (TS_DATA* - TS_CAL1)
 *           ────────────────────────────────────────────────────────
 *                         (TS_CAL2 - TS_CAL1)
 *         + TS_CAL1_TEMP
 *
 * Where TS_DATA* is the raw 14-bit ADC value corrected for the actual VDDA
 * voltage relative to the calibration reference (3.0 V).
 *
 * Calibration constants (STM32U575, from system memory):
 *   - TS_CAL1 at 30 °C, Vref+ = 3.0 V  → address 0x0BFA0710
 *   - TS_CAL2 at 130 °C, Vref+ = 3.0 V → address 0x0BFA0742
 *
 * @note   ADC1 must already be initialised by MX_ADC1_Init() before calling
 *         BspAdcTemp_Init().
 * @note   This driver is the ONLY file allowed to include stm32u5xx_hal.h
 *         for ADC access — never include it from application or domain layers.
 */

#ifndef BSP_STM32U5_ADC_TEMP_H
#define BSP_STM32U5_ADC_TEMP_H

#include "interfaces/i_temperature_sensor.h"
#include "hal/hal_types.h"
#include "stm32u5xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  Initialise the ADC temperature driver.
     *
     * Stores the ADC1 handle, performs a one-time ADC calibration if not yet
     * done, and configures the TEMPSENSOR channel with the required sampling
     * time (391 cycles ≈ 24 µs at 16 MHz, ≥ tSTART_min = 10 µs per datasheet).
     *
     * @param[in] hadc  Pointer to an already-initialised ADC1 handle.
     *                  Must not be NULL. Must be ADC1 instance.
     *
     * @return ERR_OK            on success.
     * @return ERR_NULL_POINTER  if hadc is NULL.
     * @return ERR_ERROR         if HAL channel config or calibration fails.
     */
    Result_t BspAdcTemp_Init(ADC_HandleTypeDef *hadc);

    /**
     * @brief  Obtain the ITemperatureSensor interface for this driver.
     *
     * @pre    BspAdcTemp_Init() must have returned ERR_OK.
     *
     * @return Pointer to the static ITemperatureSensor_t instance.
     *         NULL if the driver has not been successfully initialised.
     */
    ITemperatureSensor_t *BspAdcTemp_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_STM32U5_ADC_TEMP_H */
