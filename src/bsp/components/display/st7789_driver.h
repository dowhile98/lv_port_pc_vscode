/**
 * @file st7789_driver.h
 * @brief ST7789 TFT LCD Display Driver - Clean Architecture Implementation
 * @version 2.1.0
 */

#ifndef BSP_COMPONENTS_DISPLAY_ST7789_DRIVER_H_
#define BSP_COMPONENTS_DISPLAY_ST7789_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "include/interfaces/i_display.h"
#include "hal/interfaces/i_gpio.h"
#include "hal/interfaces/i_spi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ========================================================================
     * ST7789 COMMAND DEFINITIONS
     * ======================================================================== */

#define ST7789_CMD_NOP 0x00U
#define ST7789_CMD_SWRESET 0x01U
#define ST7789_CMD_RDDID 0x04U
#define ST7789_CMD_RDDST 0x09U
#define ST7789_CMD_SLPIN 0x10U
#define ST7789_CMD_SLPOUT 0x11U
#define ST7789_CMD_PTLON 0x12U
#define ST7789_CMD_NORON 0x13U
#define ST7789_CMD_INVOFF 0x20U
#define ST7789_CMD_INVON 0x21U
#define ST7789_CMD_DISPOFF 0x28U
#define ST7789_CMD_DISPON 0x29U
#define ST7789_CMD_CASET 0x2AU
#define ST7789_CMD_RASET 0x2BU
#define ST7789_CMD_RAMWR 0x2CU
#define ST7789_CMD_RAMRD 0x2EU
#define ST7789_CMD_PTLAR 0x30U
#define ST7789_CMD_MADCTL 0x36U
#define ST7789_CMD_COLMOD 0x3AU

/* Level-2 extended registers (required for full init sequence) */
#define ST7789_CMD_RAM_CTRL 0xB0U        /**< RAM Control */
#define ST7789_CMD_GATE_CTRL 0xB7U       /**< Gate Control */
#define ST7789_CMD_VCOM_SET 0xBBU        /**< VCOM Setting */
#define ST7789_CMD_LCM_CTRL 0xC0U        /**< LCM Control */
#define ST7789_CMD_VDV_VRH_EN 0xC2U      /**< VDV and VRH Command Enable */
#define ST7789_CMD_VDV_SET 0xC4U         /**< VDV Set */
#define ST7789_CMD_FRAME_RATE_CTRL 0xC6U /**< Frame Rate Control (normal mode) */
#define ST7789_CMD_POWER_CTRL1 0xD0U     /**< Power Control 1 */
#define ST7789_CMD_PV_GAMMA 0xE0U        /**< Positive Voltage Gamma Control */
#define ST7789_CMD_NV_GAMMA 0xE1U        /**< Negative Voltage Gamma Control */

#define ST7789_MADCTL_MY 0x80U
#define ST7789_MADCTL_MX 0x40U
#define ST7789_MADCTL_MV 0x20U
#define ST7789_MADCTL_ML 0x10U
#define ST7789_MADCTL_RGB 0x00U
#define ST7789_MADCTL_BGR 0x08U
#define ST7789_MADCTL_MH 0x04U

#define ST7789_COLMOD_12BIT 0x03U
#define ST7789_COLMOD_16BIT 0x05U
#define ST7789_COLMOD_18BIT 0x06U

#define ST7789_DEFAULT_WIDTH 240U
#define ST7789_DEFAULT_HEIGHT 320U

    typedef enum
    {
        ST7789_COLOR_FORMAT_RGB565 = ST7789_COLMOD_16BIT,
        ST7789_COLOR_FORMAT_RGB666 = ST7789_COLMOD_18BIT
    } ST7789_ColorFormat_t;

    typedef struct
    {
        uint16_t width;
        uint16_t height;
        ST7789_ColorFormat_t color_format;
        Display_Orientation_t orientation;
        bool invert_colors;
        uint8_t initial_brightness;
    } ST7789_Config_t;

    /**
     * @brief ST7789 Concrete Display Driver.
     */
    typedef struct
    {
        I_Display base;

        /* Hardware dependencies */
        I_SPI *spi;
        void *spi_handle;
        I_GPIO *gpio_rst;
        I_GPIO *gpio_dc;
        I_GPIO *gpio_cs; /**< Chip Select pin (NUEVO) */
        I_GPIO *gpio_backlight;

        /* Driver state */
        uint16_t width;
        uint16_t height;
        ST7789_ColorFormat_t color_format;
        Display_Orientation_t orientation;
        bool invert_colors;
        bool is_initialized;
        uint8_t brightness;

        /* DMA state */
        volatile bool dma_busy;
        Display_TransferCompleteCallback_t dma_callback;

    } ST7789_Driver_t;

    /**
     * @brief Initialize ST7789 driver object.
     * @param cs Chip Select GPIO (NUEVO).
     */
    Result_t ST7789_Init(ST7789_Driver_t *self,
                         I_SPI *spi,
                         void *handle,
                         I_GPIO *rst,
                         I_GPIO *dc,
                         I_GPIO *cs,
                         I_GPIO *backlight,
                         const ST7789_Config_t *config);

    void ST7789_SignalTransferComplete(ST7789_Driver_t *self);
    bool ST7789_IsBusy(const ST7789_Driver_t *self);
    Result_t ST7789_SetInversion(ST7789_Driver_t *self, bool invert);
    Result_t ST7789_FillScreen(ST7789_Driver_t *self, uint16_t color);

#ifdef __cplusplus
}
#endif

#endif /* BSP_COMPONENTS_DISPLAY_ST7789_DRIVER_H_ */