/**
 * @file ssd1306_driver.h
 * @brief SSD1306 OLED Display Driver - Clean Architecture Implementation
 * @version 2.1.0
 */

#ifndef BSP_COMPONENTS_DISPLAY_SSD1306_DRIVER_H_
#define BSP_COMPONENTS_DISPLAY_SSD1306_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include "include/interfaces/i_display.h"
#include "hal/interfaces/i_gpio.h"
#include "hal/interfaces/i_spi.h"
#include "hal/interfaces/i_i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Comandos SSD1306 (Resumen) */
#define SSD1306_CMD_DISPLAY_OFF 0xAEU
#define SSD1306_CMD_DISPLAY_ON 0xAFU
#define SSD1306_CMD_SET_CONTRAST 0x81U
#define SSD1306_CMD_DISPLAY_RAM 0xA4U
#define SSD1306_CMD_DISPLAY_NORMAL 0xA6U
#define SSD1306_CMD_DISPLAY_INVERTED 0xA7U
#define SSD1306_CMD_SET_MEMORY_MODE 0x20U
#define SSD1306_CMD_SET_COLUMN_RANGE 0x21U
#define SSD1306_CMD_SET_PAGE_RANGE 0x22U

    typedef enum
    {
        SSD1306_BUS_I2C,
        SSD1306_BUS_SPI
    } SSD1306_BusType_t;

    typedef struct
    {
        I_Display base;

        /* Hardware (Injected) */
        SSD1306_BusType_t bus_type;
        I_I2C *i2c;
        I_SPI *spi;
        void *i2c_handle;
        void *spi_handle;
        I_GPIO *gpio_rst;
        I_GPIO *gpio_dc;
        I_GPIO *gpio_cs; /**< Chip Select (Solo para SPI) */

        /* State */
        uint16_t width;
        uint16_t height;
        uint8_t i2c_address;
        bool is_initialized;
        volatile bool dma_busy;
        Display_TransferCompleteCallback_t dma_callback;

    } SSD1306_Driver_t;

    Result_t SSD1306_InitI2C(SSD1306_Driver_t *self, I_I2C *i2c, void *handle, I_GPIO *rst, const void *config);
    Result_t SSD1306_InitSPI(SSD1306_Driver_t *self, I_SPI *spi, void *handle, I_GPIO *rst, I_GPIO *dc, I_GPIO *cs, const void *config);

    void SSD1306_SignalTransferComplete(SSD1306_Driver_t *self);
    bool SSD1306_IsBusy(const SSD1306_Driver_t *self);

#ifdef __cplusplus
}
#endif

#endif /* BSP_COMPONENTS_DISPLAY_SSD1306_DRIVER_H_ */