/**
 * @file eeprom_at24cxx.c
 * @brief Implementación del driver AT24CXX usando I_I2C.
 *
 * @note Soporta múltiples variantes (AT24C02 a AT24C512).
 *       Dirección de memoria: 8-bit para <=2KB, 16-bit para >2KB.
 */

#include "eeprom_at24cxx.h"
#include <string.h>

/* ===== Configuraciones por variante ===== */
typedef struct
{
    uint16_t page_size;
    bool uses_16bit_addr;
} AT24CXX_Params_t;

static const AT24CXX_Params_t s_at24_params[] = {
    [AT24C02] = {.page_size = 8, .uses_16bit_addr = false},
    [AT24C04] = {.page_size = 16, .uses_16bit_addr = false},
    [AT24C08] = {.page_size = 16, .uses_16bit_addr = false},
    [AT24C16] = {.page_size = 16, .uses_16bit_addr = false},
    [AT24C32] = {.page_size = 32, .uses_16bit_addr = true},
    [AT24C64] = {.page_size = 32, .uses_16bit_addr = true},
    [AT24C128] = {.page_size = 64, .uses_16bit_addr = true},
    [AT24C256] = {.page_size = 64, .uses_16bit_addr = true},
    [AT24C512] = {.page_size = 128, .uses_16bit_addr = true}};

/* ===== Prototipos internos ===== */
static Result_t at24_init(void *self, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config);
static Result_t at24_deinit(void *self, I_EXT_EEPROM_Handle_t handle);
static Result_t at24_read_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data);
static Result_t at24_read_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data, uint16_t size);
static Result_t at24_write_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t data);
static Result_t at24_write_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, const uint8_t *data, uint16_t size);
static Result_t at24_is_device_ready(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms);
static Result_t at24_get_info(void *self, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info);

/* ===== V-Table ===== */
static const I_EXT_EEPROM_Vtable s_at24_vtable = {
    .Init = at24_init,
    .DeInit = at24_deinit,
    .ReadByte = at24_read_byte,
    .ReadData = at24_read_data,
    .WriteByte = at24_write_byte,
    .WriteData = at24_write_data,
    .IsDeviceReady = at24_is_device_ready,
    .GetInfo = at24_get_info};

/* ===== Singleton ===== */
static AT24CXX_Context_t s_at24_context;
static I_EXT_EEPROM s_at24_iface = {
    .vtable = &s_at24_vtable,
    .impl = &s_at24_context};

I_EXT_EEPROM *AT24CXX_Create(I_I2C *i2c_iface, I_I2C_Handle_t i2c_handle, AT24CXX_Variant_t variant)
{
    if (i2c_iface == NULL || i2c_handle == NULL)
        return NULL;

    /* Validar variante */
    if (variant < AT24C02 || variant > AT24C512)
        return NULL;

    s_at24_context.i2c_iface = i2c_iface;
    s_at24_context.i2c_handle = i2c_handle;
    s_at24_context.variant = variant;

    /* Encontrar parámetros de la variante */
    const AT24CXX_Params_t *params = NULL;
    for (uint8_t i = 0; i < sizeof(s_at24_params) / sizeof(AT24CXX_Params_t); i++)
    {
        if ((uint32_t)variant == (uint32_t)(1U << (i + 1)) * 128U)
        {
            params = &s_at24_params[i];
            break;
        }
    }

    if (params == NULL)
    {
        /* Búsqueda directa por valor */
        switch (variant)
        {
        case AT24C02:
            params = &s_at24_params[0];
            break;
        case AT24C04:
            params = &s_at24_params[1];
            break;
        case AT24C08:
            params = &s_at24_params[2];
            break;
        case AT24C16:
            params = &s_at24_params[3];
            break;
        case AT24C32:
            params = &s_at24_params[4];
            break;
        case AT24C64:
            params = &s_at24_params[5];
            break;
        case AT24C128:
            params = &s_at24_params[6];
            break;
        case AT24C256:
            params = &s_at24_params[7];
            break;
        case AT24C512:
            params = &s_at24_params[8];
            break;
        default:
            return NULL;
        }
    }

    /* Configuración por defecto */
    s_at24_context.config.total_size = (uint32_t)variant;
    s_at24_context.config.page_size = params->page_size;
    s_at24_context.config.device_address = 0xA0U; /* Dirección por defecto */
    s_at24_context.config.write_delay_ms = 5U;    /* tW típico: 5ms */

    return &s_at24_iface;
}

/* ===== Implementación V-Table ===== */

static Result_t at24_init(void *self, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL || config == NULL)
        return ERR_NULL_POINTER;

    /* Actualizar configuración */
    ctx->config = *config;

    /* Verificar que el dispositivo responde */
    return at24_is_device_ready(self, handle, 100);
}

static Result_t at24_deinit(void *self, I_EXT_EEPROM_Handle_t handle)
{
    (void)self;
    (void)handle;
    return ERR_OK;
}

static Result_t at24_read_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address >= ctx->config.total_size)
        return ERR_INVALID_PARAM;

    /* Determinar tamaño de dirección */
    const AT24CXX_Params_t *params = &s_at24_params[0];
    for (uint8_t i = 0; i < 9; i++)
    {
        if ((uint32_t)ctx->variant == (uint32_t)(1U << (i + 1)) * 128U)
        {
            params = &s_at24_params[i];
            break;
        }
    }

    I_I2C_MemAddrSize_t addr_size = params->uses_16bit_addr ? I_I2C_MEMADDR_SIZE_16BIT : I_I2C_MEMADDR_SIZE_8BIT;

    return I2C_Mem_Read(ctx->i2c_iface, ctx->i2c_handle,
                        ctx->config.device_address,
                        (uint16_t)address,
                        addr_size,
                        data, 1, 100);
}

static Result_t at24_read_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data, uint16_t size)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address + size > ctx->config.total_size)
        return ERR_INVALID_PARAM;

    /* Determinar tamaño de dirección */
    bool uses_16bit = (ctx->variant >= AT24C32);
    I_I2C_MemAddrSize_t addr_size = uses_16bit ? I_I2C_MEMADDR_SIZE_16BIT : I_I2C_MEMADDR_SIZE_8BIT;

    /* Lectura secuencial */
    return I2C_Mem_Read(ctx->i2c_iface, ctx->i2c_handle,
                        ctx->config.device_address,
                        (uint16_t)address,
                        addr_size,
                        data, size, 500);
}

static Result_t at24_write_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t data)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL)
        return ERR_NULL_POINTER;
    if (address >= ctx->config.total_size)
        return ERR_INVALID_PARAM;

    bool uses_16bit = (ctx->variant >= AT24C32);
    I_I2C_MemAddrSize_t addr_size = uses_16bit ? I_I2C_MEMADDR_SIZE_16BIT : I_I2C_MEMADDR_SIZE_8BIT;

    Result_t result = I2C_Mem_Write(ctx->i2c_iface, ctx->i2c_handle,
                                    ctx->config.device_address,
                                    (uint16_t)address,
                                    addr_size,
                                    &data, 1, 100);

    if (result != ERR_OK)
        return result;

    /* Esperar tiempo de escritura */
    return at24_is_device_ready(self, handle, ctx->config.write_delay_ms + 10);
}

static Result_t at24_write_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, const uint8_t *data, uint16_t size)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address + size > ctx->config.total_size)
        return ERR_INVALID_PARAM;
    if (size == 0)
        return ERR_OK;

    bool uses_16bit = (ctx->variant >= AT24C32);
    I_I2C_MemAddrSize_t addr_size = uses_16bit ? I_I2C_MEMADDR_SIZE_16BIT : I_I2C_MEMADDR_SIZE_8BIT;

    uint32_t current_addr = address;
    uint16_t remaining = size;
    const uint8_t *current_data = data;

    /* Escribir página por página */
    while (remaining > 0)
    {
        uint32_t page_boundary = (current_addr / ctx->config.page_size + 1) * ctx->config.page_size;
        uint16_t bytes_to_write = (uint16_t)((page_boundary - current_addr) < remaining ? (page_boundary - current_addr) : remaining);

        Result_t result = I2C_Mem_Write(ctx->i2c_iface, ctx->i2c_handle,
                                        ctx->config.device_address,
                                        (uint16_t)current_addr,
                                        addr_size,
                                        (uint8_t *)current_data, bytes_to_write, 100);

        if (result != ERR_OK)
            return result;

        /* Esperar que complete */
        result = at24_is_device_ready(self, handle, ctx->config.write_delay_ms + 10);
        if (result != ERR_OK)
            return result;

        current_addr += bytes_to_write;
        current_data += bytes_to_write;
        remaining -= bytes_to_write;
    }

    return ERR_OK;
}

static Result_t at24_is_device_ready(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (ctx == NULL)
        return ERR_NULL_POINTER;

    return I2C_IsDeviceReady(ctx->i2c_iface, ctx->i2c_handle,
                             ctx->config.device_address,
                             10, timeout_ms);
}

static Result_t at24_get_info(void *self, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info)
{
    (void)handle;
    AT24CXX_Context_t *ctx = (AT24CXX_Context_t *)self;

    if (info == NULL || ctx == NULL)
        return ERR_NULL_POINTER;

    info->manufacturer = "Microchip/Atmel";

    /* Determinar nombre de la variante */
    switch (ctx->variant)
    {
    case AT24C02:
        info->part_number = "AT24C02";
        break;
    case AT24C04:
        info->part_number = "AT24C04";
        break;
    case AT24C08:
        info->part_number = "AT24C08";
        break;
    case AT24C16:
        info->part_number = "AT24C16";
        break;
    case AT24C32:
        info->part_number = "AT24C32";
        break;
    case AT24C64:
        info->part_number = "AT24C64";
        break;
    case AT24C128:
        info->part_number = "AT24C128";
        break;
    case AT24C256:
        info->part_number = "AT24C256";
        break;
    case AT24C512:
        info->part_number = "AT24C512";
        break;
    default:
        info->part_number = "AT24CXX_Unknown";
        break;
    }

    info->capacity_bytes = (uint32_t)ctx->variant;

    return ERR_OK;
}
