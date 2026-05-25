/**
 * @file ssd1306_driver.c
 * @brief SSD1306 OLED Driver with CS and DMA support.
 */

#include "ssd1306_driver.h"
#include <string.h>

/* ===== PRIVATE HELPERS ===== */

static inline void ssd1306_cs_select(SSD1306_Driver_t *self)
{
    if (self->bus_type == SSD1306_BUS_SPI && self->gpio_cs != NULL)
    {
        GPIO_WritePin(self->gpio_cs, NULL, 0, I_GPIO_STATE_LOW);
    }
}

static inline void ssd1306_cs_deselect(SSD1306_Driver_t *self)
{
    if (self->bus_type == SSD1306_BUS_SPI && self->gpio_cs != NULL)
    {
        GPIO_WritePin(self->gpio_cs, NULL, 0, I_GPIO_STATE_HIGH);
    }
}

static Result_t ssd1306_send_command(SSD1306_Driver_t *self, uint8_t cmd)
{
    Result_t res;
    if (self->bus_type == SSD1306_BUS_I2C)
    {
        uint8_t buf[2] = {0x00, cmd};
        res = I2C_Master_Transmit(self->i2c, self->i2c_handle, self->i2c_address, buf, 2, 100);
    }
    else
    {
        ssd1306_cs_select(self);
        GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_LOW);
        res = SPI_Transmit(self->spi, self->spi_handle, &cmd, 1, 100);
        ssd1306_cs_deselect(self);
    }
    return res;
}

/* ===== INTERFACE IMPLEMENTATION ===== */

static Result_t ssd1306_vtable_init(void *self)
{
    SSD1306_Driver_t *driver = (SSD1306_Driver_t *)self;
    if (driver->is_initialized)
        return ERR_OK;

    if (driver->gpio_rst)
    {
        GPIO_WritePin(driver->gpio_rst, NULL, 0, I_GPIO_STATE_LOW);
        for (volatile int i = 0; i < 10000; i++)
            ; /* Delay */
        GPIO_WritePin(driver->gpio_rst, NULL, 0, I_GPIO_STATE_HIGH);
    }

    /* Secuencia de inicialización mínima */
    ssd1306_send_command(driver, 0xAE); /* Off */
    ssd1306_send_command(driver, 0x20); /* Set Memory Mode */
    ssd1306_send_command(driver, 0x00); /* Horizontal */
    ssd1306_send_command(driver, 0xAF); /* On */

    driver->is_initialized = true;
    return ERR_OK;
}

static Result_t ssd1306_vtable_write_area(void *self, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data, Display_TransferCompleteCallback_t cb)
{
    SSD1306_Driver_t *driver = (SSD1306_Driver_t *)self;
    if (!driver->is_initialized)
        return ERR_ERROR;
    if (driver->dma_busy)
        return ERR_BUSY;

    /* Configurar ventana (SSD1306 usa páginas de 8 bits) */
    ssd1306_send_command(driver, 0x21); /* Column range */
    ssd1306_send_command(driver, (uint8_t)x1);
    ssd1306_send_command(driver, (uint8_t)x2);
    ssd1306_send_command(driver, 0x22); /* Page range */
    ssd1306_send_command(driver, (uint8_t)(y1 / 8));
    ssd1306_send_command(driver, (uint8_t)(y2 / 8));

    uint16_t len = (x2 - x1 + 1) * ((y2 - y1 + 1) / 8);

    if (driver->bus_type == SSD1306_BUS_I2C)
    {
        /* I2C implementation (Blocking for simplicity in this OLED) */
        uint8_t ctrl = 0x40;
        I2C_Master_Transmit(driver->i2c, driver->i2c_handle, driver->i2c_address, &ctrl, 1, 10);
        I2C_Master_Transmit(driver->i2c, driver->i2c_handle, driver->i2c_address, data, len, 100);
        if (cb)
            cb();
    }
    else
    {
        ssd1306_cs_select(driver);
        GPIO_WritePin(driver->gpio_dc, NULL, 0, I_GPIO_STATE_HIGH);
        if (cb != NULL)
        {
            driver->dma_callback = cb;
            driver->dma_busy = true;
            SPI_Transmit_Async(driver->spi, driver->spi_handle, data, len);
        }
        else
        {
            SPI_Transmit(driver->spi, driver->spi_handle, data, len, 1000);
            ssd1306_cs_deselect(driver);
        }
    }
    return ERR_OK;
}

static Result_t ssd1306_vtable_get_caps(void *self, Display_Capabilities_t *caps)
{
    SSD1306_Driver_t *driver = (SSD1306_Driver_t *)self;
    caps->width = driver->width;
    caps->height = driver->height;
    caps->color_fmt = I_DISPLAY_COLOR_FORMAT_MONO;
    caps->supports_dma = (driver->bus_type == SSD1306_BUS_SPI);
    caps->supports_backlight = false;
    return ERR_OK;
}

static const I_Display_Vtable ssd1306_vtable = {
    .Init = ssd1306_vtable_init,
    .WriteArea = ssd1306_vtable_write_area,
    .GetCapabilities = ssd1306_vtable_get_caps};

Result_t SSD1306_InitI2C(SSD1306_Driver_t *self, I_I2C *i2c, void *handle, I_GPIO *rst, const void *config)
{
    if (!self || !i2c)
        return ERR_NULL_POINTER;
    memset(self, 0, sizeof(SSD1306_Driver_t));
    self->base.vtable = &ssd1306_vtable;
    self->base.impl = self;
    self->bus_type = SSD1306_BUS_I2C;
    self->i2c = i2c;
    self->i2c_handle = handle;
    self->gpio_rst = rst;
    self->i2c_address = 0x3C << 1;
    self->width = 128;
    self->height = 64;
    return ERR_OK;
}

Result_t SSD1306_InitSPI(SSD1306_Driver_t *self, I_SPI *spi, void *handle, I_GPIO *rst, I_GPIO *dc, I_GPIO *cs, const void *config)
{
    if (!self || !spi || !dc || !cs)
        return ERR_NULL_POINTER;
    memset(self, 0, sizeof(SSD1306_Driver_t));
    self->base.vtable = &ssd1306_vtable;
    self->base.impl = self;
    self->bus_type = SSD1306_BUS_SPI;
    self->spi = spi;
    self->spi_handle = handle;
    self->gpio_rst = rst;
    self->gpio_dc = dc;
    self->gpio_cs = cs;
    self->width = 128;
    self->height = 64;
    return ERR_OK;
}

void SSD1306_SignalTransferComplete(SSD1306_Driver_t *self)
{
    if (self == NULL || !self->dma_busy)
        return;
    ssd1306_cs_deselect(self);

    self->dma_busy = false;
    if (self->dma_callback)
        self->dma_callback();
}

bool SSD1306_IsBusy(const SSD1306_Driver_t *self)
{
    return (self != NULL) ? self->dma_busy : false;
}