/**
 * @file eeprom_m24m01e.c
 * @brief Implementación completa del driver M24M01E (1Mbit).
 *
 * @details Este driver implementa todas las funciones de la V-Table I_EXT_EEPROM
 *          para la EEPROM M24M01E con manejo automático de:
 *          - Fronteras de bloque de 64KB (A16)
 *          - Páginas de escritura de 256 bytes
 *          - Polling para verificación de escritura completada
 *
 * @note Cumple con principios SOLID:
 *       - SRP: Cada función tiene una responsabilidad única
 *       - DIP: Depende de I_I2C (abstracción), no de HAL directamente
 *       - ISP: Implementa solo I_EXT_EEPROM, interfaz segregada
 */

#include "bsp/components/eeprom/eeprom_m24m01e.h"
#include "i_i2c.h"
#include <string.h>

#define M24M01E_CAPACITY_BYTES 131072U /**< 128KB total (1Mbit) */
#define M24M01E_BLOCK_SIZE 65536U      /**< Frontera de 64KB para A16 */
#define M24M01E_PAGE_SIZE 256U         /**< Tamaño de página para escritura */

/* ===== Prototipos internos ===== */
static Result_t m24m01e_init(void *self, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config);
static Result_t m24m01e_deinit(void *self, I_EXT_EEPROM_Handle_t handle);
static Result_t m24m01e_read_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data);
static Result_t m24m01e_read_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data, uint16_t size);
static Result_t m24m01e_write_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t data);
static Result_t m24m01e_write_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, const uint8_t *data, uint16_t size);
static Result_t m24m01e_is_device_ready(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms);
static Result_t m24m01e_get_info(void *self, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info);

/* ===== V-Table ===== */
static const I_EXT_EEPROM_Vtable s_m24m01e_vtable = {
    .Init = m24m01e_init,
    .DeInit = m24m01e_deinit,
    .ReadByte = m24m01e_read_byte,
    .ReadData = m24m01e_read_data,
    .WriteByte = m24m01e_write_byte,
    .WriteData = m24m01e_write_data,
    .IsDeviceReady = m24m01e_is_device_ready,
    .GetInfo = m24m01e_get_info};

/* ===== Singleton ===== */
static M24M01E_Context_t s_m24m01e_context;
static I_EXT_EEPROM s_m24m01e_iface = {
    .vtable = &s_m24m01e_vtable,
    .impl = &s_m24m01e_context};

/**
 * @brief Helper para calcular la dirección I2C efectiva incluyendo el bit A16.
 * @details El M24M01E usa el bit A16 en el Device Select Byte para acceder
 *          a direcciones mayores a 64KB:
 *          - Bloque 0 (0x00000-0x0FFFF): 0xA0
 *          - Bloque 1 (0x10000-0x1FFFF): 0xA2
 *
 * @param[in] base_addr  Dirección I2C base (ej: 0xA0)
 * @param[in] mem_addr   Dirección de memoria absoluta (0-131071)
 * @return uint16_t      Dirección I2C efectiva con A16
 */
static inline uint16_t get_effective_i2c_addr(uint16_t base_addr, uint32_t mem_addr)
{
    /* A16 se envía en el bit 1 del Device Select Byte (asumiendo formato 8-bit de ST) */
    /* 0xA0 -> Block 0 (0x00000-0x0FFFF), 0xA2 -> Block 1 (0x10000-0x1FFFF) */
    return (uint16_t)(base_addr | ((mem_addr >> 15) & 0x02U));
}

/* ===== Función de creación ===== */

I_EXT_EEPROM *M24M01E_Create(I_I2C *i2c_iface, I_I2C_Handle_t i2c_handle,
                             const EepromConfig_t *board_config)
{
    /* Validación de parámetros */
    if (i2c_iface == NULL || i2c_handle == NULL || board_config == NULL)
        return NULL;

    /* CRITICAL: Verificar que el driver solicitado sea M24M01E */
    if (board_config->driver != EEPROM_DRIVER_M24M01E)
    {
        return NULL; /* Config mismatch */
    }

    /* Verificar que la EEPROM esté habilitada */
    if (!board_config->enabled)
    {
        return NULL; /* EEPROM deshabilitada en BoardProfile */
    }

    s_m24m01e_context.i2c_iface = i2c_iface;
    s_m24m01e_context.i2c_handle = i2c_handle;

    /* ✅ Mapear configuración desde BoardProfile → Driver Config */
    s_m24m01e_context.config.total_size = board_config->size_bytes;
    s_m24m01e_context.config.page_size = board_config->page_size;
    s_m24m01e_context.config.device_address = board_config->i2c_address;
    s_m24m01e_context.config.write_delay_ms = 15U; /* tW típico para M24M01E */

    return &s_m24m01e_iface;
}

/* ===== Implementación V-Table ===== */

static Result_t m24m01e_init(void *self, I_EXT_EEPROM_Handle_t handle, const I_EXT_EEPROM_Config_t *config)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;

    if (ctx == NULL || config == NULL)
        return ERR_NULL_POINTER;

    /* Actualizar configuración */
    ctx->config = *config;

    /* Verificar que el dispositivo responde */
    return m24m01e_is_device_ready(self, handle, 100);
}

static Result_t m24m01e_deinit(void *self, I_EXT_EEPROM_Handle_t handle)
{
    (void)self;
    (void)handle;
    return ERR_OK;
}

static Result_t m24m01e_read_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;

    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address >= M24M01E_CAPACITY_BYTES)
        return ERR_INVALID_PARAM;

    return I2C_Mem_Read(ctx->i2c_iface, ctx->i2c_handle,
                        get_effective_i2c_addr(ctx->config.device_address, address),
                        (uint16_t)(address & 0xFFFFU),
                        I_I2C_MEMADDR_SIZE_16BIT,
                        data, 1, 100);
}

static Result_t m24m01e_read_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t *data, uint16_t size)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;
    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address + size > M24M01E_CAPACITY_BYTES)
        return ERR_INVALID_PARAM;

    uint32_t curr_addr = address;
    uint16_t remaining = size;
    uint8_t *curr_data = data;

    while (remaining > 0)
    {
        /* Calcular cuántos bytes podemos leer en el bloque actual (máximo hasta 0x0FFFF o 0x1FFFF) */
        uint32_t block_boundary = (curr_addr < M24M01E_BLOCK_SIZE) ? M24M01E_BLOCK_SIZE : M24M01E_CAPACITY_BYTES;
        uint16_t chunk_size = (uint16_t)((block_boundary - curr_addr) < remaining ? (block_boundary - curr_addr) : remaining);

        Result_t res = I2C_Mem_Read(ctx->i2c_iface, ctx->i2c_handle,
                                    get_effective_i2c_addr(ctx->config.device_address, curr_addr),
                                    (uint16_t)(curr_addr & 0xFFFFU),
                                    I_I2C_MEMADDR_SIZE_16BIT,
                                    curr_data, chunk_size, 500);
        if (res != ERR_OK)
            return res;

        curr_addr += chunk_size;
        curr_data += chunk_size;
        remaining -= chunk_size;
    }
    return ERR_OK;
}

static Result_t m24m01e_write_byte(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, uint8_t data)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;

    if (ctx == NULL)
        return ERR_NULL_POINTER;
    if (address >= M24M01E_CAPACITY_BYTES)
        return ERR_INVALID_PARAM;

    Result_t result = I2C_Mem_Write(ctx->i2c_iface, ctx->i2c_handle,
                                    get_effective_i2c_addr(ctx->config.device_address, address),
                                    (uint16_t)(address & 0xFFFFU),
                                    I_I2C_MEMADDR_SIZE_16BIT,
                                    &data, 1, 100);

    if (result != ERR_OK)
        return result;

    /* Esperar tiempo de escritura */
    return m24m01e_is_device_ready(self, handle, ctx->config.write_delay_ms + 10);
}

static Result_t m24m01e_write_data(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t address, const uint8_t *data, uint16_t size)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;
    if (ctx == NULL || data == NULL)
        return ERR_NULL_POINTER;
    if (address + size > M24M01E_CAPACITY_BYTES)
        return ERR_INVALID_PARAM;
    if (size == 0)
        return ERR_OK;

    uint32_t curr_addr = address;
    uint16_t remaining = size;
    const uint8_t *curr_data = data;

    /* Escribir página por página respetando límites de bloque y página */
    while (remaining > 0)
    {
        /* 1. Límite de bloque 64KB (A16) */
        uint32_t block_boundary = (curr_addr < M24M01E_BLOCK_SIZE) ? M24M01E_BLOCK_SIZE : M24M01E_CAPACITY_BYTES;
        /* 2. Límite de página (256 bytes) */
        uint32_t page_boundary = (curr_addr / M24M01E_PAGE_SIZE + 1) * M24M01E_PAGE_SIZE;

        /* El límite más restrictivo */
        uint32_t boundary = (block_boundary < page_boundary) ? block_boundary : page_boundary;
        uint16_t chunk_size = (uint16_t)((boundary - curr_addr) < remaining ? (boundary - curr_addr) : remaining);

        Result_t res = I2C_Mem_Write(ctx->i2c_iface, ctx->i2c_handle,
                                     get_effective_i2c_addr(ctx->config.device_address, curr_addr),
                                     (uint16_t)(curr_addr & 0xFFFFU),
                                     I_I2C_MEMADDR_SIZE_16BIT,
                                     (uint8_t *)curr_data, chunk_size, 100);
        if (res != ERR_OK)
            return res;

        for (volatile uint32_t delay = 0; delay < 100000; delay++);

        /* Esperar ciclo de escritura interno (tW) */
        res = m24m01e_is_device_ready(self, handle, ctx->config.write_delay_ms + 10);
        if (res != ERR_OK)
            return res;

        curr_addr += chunk_size;
        curr_data += chunk_size;
        remaining -= chunk_size;
    }
    return ERR_OK;
}

static Result_t m24m01e_is_device_ready(void *self, I_EXT_EEPROM_Handle_t handle, uint32_t timeout_ms)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;

    if (ctx == NULL)
        return ERR_NULL_POINTER;

    /* Verificar dispositivo en dirección base (el hardware maneja A16 automáticamente) */
    return I2C_IsDeviceReady(ctx->i2c_iface, ctx->i2c_handle,
                             ctx->config.device_address,
                             10, timeout_ms);
}

static Result_t m24m01e_get_info(void *self, I_EXT_EEPROM_Handle_t handle, I_EXT_EEPROM_Info_t *info)
{
    (void)handle;
    M24M01E_Context_t *ctx = (M24M01E_Context_t *)self;

    if (info == NULL || ctx == NULL)
        return ERR_NULL_POINTER;

    info->manufacturer = "STMicroelectronics";
    info->part_number = "M24M01E";
    info->capacity_bytes = M24M01E_CAPACITY_BYTES;

    return ERR_OK;
}
