/**
 * @file st7789_driver.c
 * @brief ST7789 TFT LCD Display Driver - Implementation with CS management
 */

#include "st7789_driver.h"
#include <string.h>

/* ========================================================================
 * PRIVATE HELPERS - HARDWARE COMMUNICATION
 * ======================================================================== */

static inline void st7789_cs_select(ST7789_Driver_t *self)
{
    if (self->gpio_cs != NULL)
    {
        GPIO_WritePin(self->gpio_cs, NULL, 0, I_GPIO_STATE_LOW);
    }
}

static inline void st7789_cs_deselect(ST7789_Driver_t *self)
{
    if (self->gpio_cs != NULL)
    {
        GPIO_WritePin(self->gpio_cs, NULL, 0, I_GPIO_STATE_HIGH);
    }
}

static void delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < (ms * 20000U); i++)
        ;
}

static void st7789_hardware_reset(ST7789_Driver_t *self)
{
    if (self->gpio_rst != NULL)
    {
        GPIO_WritePin(self->gpio_rst, NULL, 0, I_GPIO_STATE_HIGH);
        delay_ms(10);
        GPIO_WritePin(self->gpio_rst, NULL, 0, I_GPIO_STATE_LOW);
        delay_ms(10);
        GPIO_WritePin(self->gpio_rst, NULL, 0, I_GPIO_STATE_HIGH);
        delay_ms(120);
    }
}

static Result_t st7789_send_command(ST7789_Driver_t *self, uint8_t cmd)
{
    st7789_cs_select(self);
    GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_LOW);
    Result_t res = SPI_Transmit(self->spi, self->spi_handle, &cmd, 1, 100);
    st7789_cs_deselect(self);
    return res;
}

static Result_t st7789_send_data(ST7789_Driver_t *self, const uint8_t *data, uint16_t len)
{
    st7789_cs_select(self);
    GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_HIGH);
    Result_t res = SPI_Transmit(self->spi, self->spi_handle, data, len, 1000);
    st7789_cs_deselect(self);
    return res;
}

static Result_t st7789_send_command_data(ST7789_Driver_t *self, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    st7789_cs_select(self);
    GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_LOW);
    Result_t result = SPI_Transmit(self->spi, self->spi_handle, &cmd, 1, 100);
    if (result == ERR_OK && len > 0U)
    {
        GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_HIGH);
        result = SPI_Transmit(self->spi, self->spi_handle, data, len, 1000);
    }
    st7789_cs_deselect(self);
    return result;
}

static Result_t st7789_set_window(ST7789_Driver_t *self, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t buffer[4];
    Result_t result;

    buffer[0] = (uint8_t)(x0 >> 8);
    buffer[1] = (uint8_t)(x0 & 0xFF);
    buffer[2] = (uint8_t)(x1 >> 8);
    buffer[3] = (uint8_t)(x1 & 0xFF);
    result = st7789_send_command_data(self, ST7789_CMD_CASET, buffer, 4);
    if (result != ERR_OK)
        return result;

    buffer[0] = (uint8_t)(y0 >> 8);
    buffer[1] = (uint8_t)(y0 & 0xFF);
    buffer[2] = (uint8_t)(y1 >> 8);
    buffer[3] = (uint8_t)(y1 & 0xFF);
    result = st7789_send_command_data(self, ST7789_CMD_RASET, buffer, 4);
    if (result != ERR_OK)
        return result;

    return st7789_send_command(self, ST7789_CMD_RAMWR);
}

static uint8_t get_madctl_orientation(Display_Orientation_t orientation, bool invert)
{
    uint8_t madctl = ST7789_MADCTL_RGB;
    switch (orientation)
    {
    case I_DISPLAY_ORIENTATION_PORTRAIT:
        /* 0x00 — no flip bits; panel probado con esta orientación */
        break;
    case I_DISPLAY_ORIENTATION_LANDSCAPE:
        madctl |= (ST7789_MADCTL_MX | ST7789_MADCTL_MV); /* 0x60 */
        break;
    case I_DISPLAY_ORIENTATION_PORTRAIT_INVERTED:
        madctl |= (ST7789_MADCTL_MY | ST7789_MADCTL_MX); /* 0xC0 */
        break;
    case I_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED:
        madctl |= (ST7789_MADCTL_MY | ST7789_MADCTL_MV); /* 0xA0 */
        break;
    default:
        madctl |= (ST7789_MADCTL_MX | ST7789_MADCTL_MV);
        break;
    }
    (void)invert; /* inversion se maneja vía CMD_INVON al final de configure */
    return madctl;
}

static Result_t st7789_configure(ST7789_Driver_t *self)
{
    Result_t result;
    uint8_t data[14];

    /* 1. Software Reset — estado limpio garantizado ----------------------- */
    result = st7789_send_command(self, ST7789_CMD_SWRESET);
    if (result != ERR_OK)
        return result;
    delay_ms(120);

    /* 2. Sleep Out -------------------------------------------------------- */
    result = st7789_send_command(self, ST7789_CMD_SLPOUT);
    if (result != ERR_OK)
        return result;
    delay_ms(120);

    /* 3. RAM Control (habilita modo 16-bit bus) --------------------------- */
    data[0] = 0x00;
    data[1] = 0xF8;
    result = st7789_send_command_data(self, ST7789_CMD_RAM_CTRL, data, 2);
    if (result != ERR_OK)
        return result;

    /* 4. Pixel Format RGB565 --------------------------------------------- */
    data[0] = (uint8_t)self->color_format;
    result = st7789_send_command_data(self, ST7789_CMD_COLMOD, data, 1);
    if (result != ERR_OK)
        return result;
    delay_ms(10);

    /* 5. Memory Access Control (orientación) ----------------------------- */
    data[0] = get_madctl_orientation(self->orientation, self->invert_colors);
    result = st7789_send_command_data(self, ST7789_CMD_MADCTL, data, 1);
    if (result != ERR_OK)
        return result;

    /* 6. Gate Control ---------------------------------------------------- */
    data[0] = 0x35;
    result = st7789_send_command_data(self, ST7789_CMD_GATE_CTRL, data, 1);
    if (result != ERR_OK)
        return result;

    /* 7. VCOM Setting ---------------------------------------------------- */
    data[0] = 0x1F;
    result = st7789_send_command_data(self, ST7789_CMD_VCOM_SET, data, 1);
    if (result != ERR_OK)
        return result;

    /* 8. LCM Control ----------------------------------------------------- */
    data[0] = 0x2C;
    result = st7789_send_command_data(self, ST7789_CMD_LCM_CTRL, data, 1);
    if (result != ERR_OK)
        return result;

    /* 9. VDV and VRH Enable ---------------------------------------------- */
    data[0] = 0x01;
    data[1] = 0xC3;
    result = st7789_send_command_data(self, ST7789_CMD_VDV_VRH_EN, data, 2);
    if (result != ERR_OK)
        return result;

    /* 10. VDV Set -------------------------------------------------------- */
    data[0] = 0x20;
    result = st7789_send_command_data(self, ST7789_CMD_VDV_SET, data, 1);
    if (result != ERR_OK)
        return result;

    /* 11. Frame Rate Control 60 Hz --------------------------------------- */
    data[0] = 0x0F;
    result = st7789_send_command_data(self, ST7789_CMD_FRAME_RATE_CTRL, data, 1);
    if (result != ERR_OK)
        return result;

    /* 12. Power Control 1 ----------------------------------------------- */
    data[0] = 0xA4;
    data[1] = 0xA1;
    result = st7789_send_command_data(self, ST7789_CMD_POWER_CTRL1, data, 2);
    if (result != ERR_OK)
        return result;

    /* 13. Positive Voltage Gamma ---------------------------------------- */
    data[0] = 0xD0;
    data[1] = 0x08;
    data[2] = 0x11;
    data[3] = 0x08;
    data[4] = 0x0C;
    data[5] = 0x15;
    data[6] = 0x39;
    data[7] = 0x33;
    data[8] = 0x50;
    data[9] = 0x36;
    data[10] = 0x13;
    data[11] = 0x14;
    data[12] = 0x29;
    data[13] = 0x2D;
    result = st7789_send_command_data(self, ST7789_CMD_PV_GAMMA, data, 14);
    if (result != ERR_OK)
        return result;

    /* 14. Negative Voltage Gamma ---------------------------------------- */
    data[0] = 0xD0;
    data[1] = 0x08;
    data[2] = 0x10;
    data[3] = 0x08;
    data[4] = 0x06;
    data[5] = 0x06;
    data[6] = 0x39;
    data[7] = 0x44;
    data[8] = 0x51;
    data[9] = 0x0B;
    data[10] = 0x16;
    data[11] = 0x14;
    data[12] = 0x2F;
    data[13] = 0x31;
    result = st7789_send_command_data(self, ST7789_CMD_NV_GAMMA, data, 14);
    if (result != ERR_OK)
        return result;

    /* 15. Normal Display Mode ------------------------------------------- */
    result = st7789_send_command(self, ST7789_CMD_NORON);
    if (result != ERR_OK)
        return result;
    delay_ms(10);

    /* 16. Inversion OFF luego ON (requerido por este panel para colores correctos) */
    result = st7789_send_command(self, ST7789_CMD_INVOFF);
    if (result != ERR_OK)
        return result;

    /* 17. Display ON ---------------------------------------------------- */
    result = st7789_send_command(self, ST7789_CMD_DISPON);
    if (result != ERR_OK)
        return result;
    delay_ms(120);

    /* 18. Color Inversion ON (panel-specific) --------------------------- */
    result = st7789_send_command(self, ST7789_CMD_INVON);
    if (result != ERR_OK)
        return result;

    return ERR_OK;
}

static Result_t st7789_vtable_init(void *self)
{
    ST7789_Driver_t *driver = (ST7789_Driver_t *)self;
    if (driver->is_initialized)
        return ERR_OK;

    st7789_hardware_reset(driver);
    Result_t result = st7789_configure(driver);
    if (result != ERR_OK)
        return result;

    if (driver->gpio_backlight != NULL)
    {
        GPIO_WritePin(driver->gpio_backlight, NULL, 0, I_GPIO_STATE_HIGH);
    }

    /* Resetear ventana de dirección a pantalla completa (igual que referencia:
     * st7789v_DisplayAdressSet(0, 0, Width-1, Height-1) al final de Init).
     * Garantiza que el primer WriteArea no encuentre un estado indeterminado. */
    (void)st7789_set_window(driver, 0, 0,
                            (uint16_t)(driver->width - 1U),
                            (uint16_t)(driver->height - 1U));

    driver->is_initialized = true;
    return ERR_OK;
}

static Result_t st7789_vtable_write_area(void *self, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data, Display_TransferCompleteCallback_t cb)
{
    ST7789_Driver_t *driver = (ST7789_Driver_t *)self;
    if (!driver->is_initialized)
        return ERR_ERROR;
    if (driver->dma_busy)
        return ERR_BUSY;

    Result_t result = st7789_set_window(driver, x1, y1, x2, y2);
    if (result != ERR_OK)
        return result;

    st7789_cs_select(driver);
    GPIO_WritePin(driver->gpio_dc, NULL, 0, I_GPIO_STATE_HIGH);

    uint32_t data_len = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1) * 2;

    if (cb != NULL)
    {
        driver->dma_callback = cb;
        driver->dma_busy = true;
        result = SPI_Transmit_Async(driver->spi, driver->spi_handle, data, (uint16_t)data_len);
        if (result != ERR_OK)
        {
            driver->dma_busy = false;
            st7789_cs_deselect(driver);
        }
    }
    else
    {
        result = SPI_Transmit(driver->spi, driver->spi_handle, data, (uint16_t)data_len, 5000);
        st7789_cs_deselect(driver);
    }

    return result;
}

static Result_t st7789_vtable_set_brightness(void *self, uint8_t percent)
{
    ST7789_Driver_t *driver = (ST7789_Driver_t *)self;
    driver->brightness = percent;
    if (driver->gpio_backlight != NULL)
    {
        GPIO_WritePin(driver->gpio_backlight, NULL, 0, (percent > 0) ? I_GPIO_STATE_HIGH : I_GPIO_STATE_LOW);
    }
    return ERR_OK;
}

static Result_t st7789_vtable_set_orientation(void *self, Display_Orientation_t ori)
{
    ST7789_Driver_t *driver = (ST7789_Driver_t *)self;
    driver->orientation = ori;

    /* Landscape (bit MV activo) intercambia los ejes físicos del panel.
     * Panel físico: 240 columnas × 320 filas.
     *   Portrait  → width=240 (ST7789_DEFAULT_WIDTH),  height=320 (ST7789_DEFAULT_HEIGHT)
     *   Landscape → width=320 (ST7789_DEFAULT_HEIGHT), height=240 (ST7789_DEFAULT_WIDTH) */
    if (ori == I_DISPLAY_ORIENTATION_LANDSCAPE || ori == I_DISPLAY_ORIENTATION_LANDSCAPE_INVERTED)
    {
        driver->width = ST7789_DEFAULT_HEIGHT; /* 320 px en eje X al rotar 90° */
        driver->height = ST7789_DEFAULT_WIDTH; /* 240 px en eje Y al rotar 90° */
    }
    else /* PORTRAIT / PORTRAIT_INVERTED */
    {
        driver->width = ST7789_DEFAULT_WIDTH;   /* 240 px — ancho natural del panel */
        driver->height = ST7789_DEFAULT_HEIGHT; /* 320 px — alto natural del panel */
    }
    uint8_t madctl = get_madctl_orientation(ori, driver->invert_colors);
    return st7789_send_command_data(driver, ST7789_CMD_MADCTL, &madctl, 1);
}

static Result_t st7789_vtable_on(void *self)
{
    return st7789_send_command((ST7789_Driver_t *)self, ST7789_CMD_DISPON);
}

static Result_t st7789_vtable_off(void *self)
{
    return st7789_send_command((ST7789_Driver_t *)self, ST7789_CMD_DISPOFF);
}

static Result_t st7789_vtable_get_caps(void *self, Display_Capabilities_t *caps)
{
    ST7789_Driver_t *driver = (ST7789_Driver_t *)self;
    caps->width = driver->width;
    caps->height = driver->height;
    caps->color_fmt = I_DISPLAY_COLOR_FORMAT_RGB565;
    caps->supports_dma = true;
    caps->supports_backlight = (driver->gpio_backlight != NULL);
    return ERR_OK;
}

static const I_Display_Vtable st7789_vtable = {
    .Init = st7789_vtable_init,
    .WriteArea = st7789_vtable_write_area,
    .SetBrightness = st7789_vtable_set_brightness,
    .SetOrientation = st7789_vtable_set_orientation,
    .On = st7789_vtable_on,
    .Off = st7789_vtable_off,
    .GetCapabilities = st7789_vtable_get_caps};

Result_t ST7789_Init(ST7789_Driver_t *self, I_SPI *spi, void *handle, I_GPIO *rst, I_GPIO *dc, I_GPIO *cs, I_GPIO *backlight, const ST7789_Config_t *config)
{
    if (self == NULL || spi == NULL || handle == NULL || dc == NULL)
        return ERR_NULL_POINTER;
    memset(self, 0, sizeof(ST7789_Driver_t));
    self->base.vtable = &st7789_vtable;
    self->base.impl = self;
    self->spi = spi;
    self->spi_handle = handle;
    self->gpio_rst = rst;
    self->gpio_dc = dc;
    self->gpio_cs = cs;
    self->gpio_backlight = backlight;

    if (config != NULL)
    {
        self->width = config->width;
        self->height = config->height;
        self->color_format = config->color_format;
        self->orientation = config->orientation;
        self->invert_colors = config->invert_colors;
        self->brightness = config->initial_brightness;
    }
    else
    {
        self->width = ST7789_DEFAULT_WIDTH;
        self->height = ST7789_DEFAULT_HEIGHT;
        self->color_format = ST7789_COLOR_FORMAT_RGB565;
        self->orientation = I_DISPLAY_ORIENTATION_LANDSCAPE;
        self->brightness = 100;
    }
    return ERR_OK;
}

void ST7789_SignalTransferComplete(ST7789_Driver_t *self)
{
    if (self == NULL || !self->dma_busy)
        return;
    st7789_cs_deselect(self);
    self->dma_busy = false;
    if (self->dma_callback != NULL)
        self->dma_callback();
}

bool ST7789_IsBusy(const ST7789_Driver_t *self)
{
    return (self != NULL) ? self->dma_busy : false;
}

Result_t ST7789_SetInversion(ST7789_Driver_t *self, bool invert)
{
    if (self == NULL)
        return ERR_NULL_POINTER;
    self->invert_colors = invert;
    return st7789_send_command(self, invert ? ST7789_CMD_INVON : ST7789_CMD_INVOFF);
}

Result_t ST7789_FillScreen(ST7789_Driver_t *self, uint16_t color)
{
    if (self == NULL || !self->is_initialized)
        return ERR_ERROR;
    Result_t result = st7789_set_window(self, 0, 0, self->width - 1, self->height - 1);
    if (result != ERR_OK)
        return result;
    uint32_t total = (uint32_t)self->width * self->height;
    uint8_t c[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    st7789_cs_select(self);
    GPIO_WritePin(self->gpio_dc, NULL, 0, I_GPIO_STATE_HIGH);
    for (uint32_t i = 0; i < total; i++)
    {
        SPI_Transmit(self->spi, self->spi_handle, c, 2, 10);
    }
    st7789_cs_deselect(self);
    return ERR_OK;
}