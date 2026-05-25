/**
 * @file bsp_stm32u5_octospi.c
 * @brief Implementación STM32U5 de I_EXT_FLASH vía OCTOSPI (W25Qxx).
 *
 * @note Basado en el código legacy de octospi.c pero refactorizado:
 *       - Sin globales (hospi1 se pasa como handle)
 *       - Manejo de errores con Result_t
 *       - Separación de responsabilidades (init/config/control)
 *       - Compatible con memory-mapped mode para XIP
 */

#include "bsp_stm32u5_octospi.h"
#include "stm32u5xx_hal.h"
#include <stddef.h>

/* ===== Comandos W25Qxx (Winbond QSPI Flash) ===== */
#define W25Q_WRITE_ENABLE_CMD 0x06
#define W25Q_VOLATILE_SR_WRITE_ENABLE 0x50
#define W25Q_READ_STATUS_REG_CMD 0x05
#define W25Q_READ_STATUS_REG2_CMD 0x35
#define W25Q_WRITE_STATUS_REG2_CMD 0x31
#define W25Q_READ_STATUS_REG3_CMD 0x15
#define W25Q_WRITE_STATUS_REG3_CMD 0x11
#define W25Q_CHIP_ERASE_CMD 0xC7
#define W25Q_SECTOR_ERASE_CMD 0x20
#define W25Q_BLOCK_ERASE_CMD 0xD8
#define W25Q_QUAD_PAGE_PROGRAM_CMD 0x32
#define W25Q_QUAD_INOUT_FAST_READ_CMD 0xEB
#define W25Q_RESET_ENABLE_CMD 0x66
#define W25Q_RESET_EXECUTE_CMD 0x99

/* ===== Configuración W25Q16 (puede ajustarse por chip) ===== */
#define W25Q_MEMORY_SIZE 0x1000000 /* 16 MB (128 Mbit) */
#define W25Q_SECTOR_SIZE 0x1000    /* 4 KB */
#define W25Q_PAGE_SIZE 0x100       /* 256 bytes */
#define W25Q_DUMMY_CYCLES_READ 4

/* ===== Helpers internos ===== */
static Result_t ospi_write_enable(OSPI_HandleTypeDef *hospi);
static Result_t ospi_auto_polling_mem_ready(OSPI_HandleTypeDef *hospi);
static Result_t ospi_reset_chip(OSPI_HandleTypeDef *hospi);
static Result_t ospi_configure_quad_mode(OSPI_HandleTypeDef *hospi);

/* ===== Prototipos V-Table ===== */
static Result_t stm32u5_octospi_init(void *self, I_EXT_FLASH_Handle_t handle, const I_EXT_FLASH_Config_t *config);
static Result_t stm32u5_octospi_deinit(void *self, I_EXT_FLASH_Handle_t handle);
static Result_t stm32u5_octospi_read(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, uint8_t *data, uint32_t size);
static Result_t stm32u5_octospi_write(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, const uint8_t *data, uint32_t size);
static Result_t stm32u5_octospi_erase_sector(void *self, I_EXT_FLASH_Handle_t handle, uint32_t sector_address);
static Result_t stm32u5_octospi_erase_chip(void *self, I_EXT_FLASH_Handle_t handle);
static Result_t stm32u5_octospi_enable_mmap(void *self, I_EXT_FLASH_Handle_t handle);
static Result_t stm32u5_octospi_disable_mmap(void *self, I_EXT_FLASH_Handle_t handle);
static Result_t stm32u5_octospi_get_info(void *self, I_EXT_FLASH_Handle_t handle, I_EXT_FLASH_Info_t *info);
static bool stm32u5_octospi_is_busy(void *self, I_EXT_FLASH_Handle_t handle);

/* ===== V-Table ===== */
static const I_EXT_FLASH_Vtable s_stm32u5_octospi_vtable = {
    .Init = stm32u5_octospi_init,
    .DeInit = stm32u5_octospi_deinit,
    .Read = stm32u5_octospi_read,
    .Write = stm32u5_octospi_write,
    .EraseSector = stm32u5_octospi_erase_sector,
    .EraseChip = stm32u5_octospi_erase_chip,
    .EnableMemoryMapped = stm32u5_octospi_enable_mmap,
    .DisableMemoryMapped = stm32u5_octospi_disable_mmap,
    .GetInfo = stm32u5_octospi_get_info,
    .IsBusy = stm32u5_octospi_is_busy};

/* ===== Singleton ===== */
static I_EXT_FLASH s_stm32u5_octospi_iface = {
    .vtable = &s_stm32u5_octospi_vtable,
    .impl =(void *) &s_stm32u5_octospi_vtable};

I_EXT_FLASH *Bsp_Stm32U5_Octospi_GetInterface(void)
{
    return &s_stm32u5_octospi_iface;
}

/* ===== Implementación V-Table ===== */

static Result_t stm32u5_octospi_init(void *self, I_EXT_FLASH_Handle_t handle, const I_EXT_FLASH_Config_t *config)
{
    (void)self;
    if (handle == NULL || config == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;

    /* Reset del chip (soft reset) */
    if (ospi_reset_chip(hospi) != ERR_OK)
        return ERR_ERROR;

    /* Esperar que el chip esté listo */
    if (ospi_auto_polling_mem_ready(hospi) != ERR_OK)
        return ERR_ERROR;

    /* Configurar modo Quad (QE bit en Status Register 2) */
    if (ospi_configure_quad_mode(hospi) != ERR_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_deinit(void *self, I_EXT_FLASH_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;
    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    HAL_OSPI_DeInit(hospi);
    return ERR_OK;
}

static Result_t stm32u5_octospi_read(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, uint8_t *data, uint32_t size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Comando de lectura Quad I/O */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_QUAD_INOUT_FAST_READ_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.Address = address;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytes = 0xFF;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.DataMode = HAL_OSPI_DATA_4_LINES;
    sCommand.NbData = size;
    sCommand.DummyCycles = W25Q_DUMMY_CYCLES_READ;
    sCommand.DQSMode = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_Receive(hospi, data, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_write(void *self, I_EXT_FLASH_Handle_t handle, uint32_t address, const uint8_t *data, uint32_t size)
{
    (void)self;
    if (handle == NULL || data == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint32_t current_addr = address;
    uint32_t end_addr = address + size;
    uint32_t current_size = 0;
    uint32_t page_boundary = 0;

    /* Escribir página por página (respetando límites de 256 bytes) */
    while (current_addr < end_addr)
    {
        /* Calcular tamaño hasta el final de la página actual */
        page_boundary = (current_addr & ~(W25Q_PAGE_SIZE - 1)) + W25Q_PAGE_SIZE;
        current_size = (end_addr < page_boundary) ? (end_addr - current_addr) : (page_boundary - current_addr);

        /* Write Enable */
        if (ospi_write_enable(hospi) != ERR_OK)
            return ERR_ERROR;

        /* Comando de escritura Quad Page Program */
        sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
        sCommand.Instruction = W25Q_QUAD_PAGE_PROGRAM_CMD;
        sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
        sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
        sCommand.Address = current_addr;
        sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        sCommand.DataMode = HAL_OSPI_DATA_4_LINES;
        sCommand.NbData = current_size;
        sCommand.DummyCycles = 0;
        sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

        if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
            return ERR_ERROR;
        if (HAL_OSPI_Transmit(hospi, (uint8_t *)(data + (current_addr - address)), HAL_MAX_DELAY) != HAL_OK)
            return ERR_ERROR;

        /* Esperar a que la operación termine */
        if (ospi_auto_polling_mem_ready(hospi) != ERR_OK)
            return ERR_ERROR;

        current_addr += current_size;
    }

    return ERR_OK;
}

static Result_t stm32u5_octospi_erase_sector(void *self, I_EXT_FLASH_Handle_t handle, uint32_t sector_address)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Alinear dirección al sector */
    sector_address &= ~(W25Q_SECTOR_SIZE - 1);

    /* Write Enable */
    if (ospi_write_enable(hospi) != ERR_OK)
        return ERR_ERROR;

    /* Comando Sector Erase */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_SECTOR_ERASE_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.Address = sector_address;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Esperar que termine */
    if (ospi_auto_polling_mem_ready(hospi) != ERR_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_erase_chip(void *self, I_EXT_FLASH_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Write Enable */
    if (ospi_write_enable(hospi) != ERR_OK)
        return ERR_ERROR;

    /* Comando Chip Erase */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_CHIP_ERASE_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Esperar que termine (puede tardar varios segundos) */
    if (ospi_auto_polling_mem_ready(hospi) != ERR_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_enable_mmap(void *self, I_EXT_FLASH_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_MemoryMappedTypeDef sMemMappedCfg = {0};

    /* Configurar comando de lectura para memory-mapped mode */
    sCommand.FlashId = HAL_OSPI_FLASH_ID_1;
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_QUAD_INOUT_FAST_READ_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
    sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
    sCommand.AlternateBytes = 0xFF;
    sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
    sCommand.DataMode = HAL_OSPI_DATA_4_LINES;
    sCommand.DummyCycles = W25Q_DUMMY_CYCLES_READ;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_MemoryMapped(hospi, &sMemMappedCfg) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_disable_mmap(void *self, I_EXT_FLASH_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return ERR_NULL_POINTER;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;

    /* Abortar el modo memory-mapped */
    if (HAL_OSPI_Abort(hospi) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t stm32u5_octospi_get_info(void *self, I_EXT_FLASH_Handle_t handle, I_EXT_FLASH_Info_t *info)
{
    (void)self;
    (void)handle;
    if (info == NULL)
        return ERR_NULL_POINTER;

    /* Información hardcodeada del W25Q128 (ajustar según chip) */
    info->manufacturer_id = 0xEF; /* Winbond */
    info->device_id = 0x17;       /* W25Q128 (16MB) */
    info->capacity_bytes = W25Q_MEMORY_SIZE;

    return ERR_OK;
}

static bool stm32u5_octospi_is_busy(void *self, I_EXT_FLASH_Handle_t handle)
{
    (void)self;
    if (handle == NULL)
        return false;

    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)handle;
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t status_reg = 0;

    /* Leer Status Register 1 */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_READ_STATUS_REG_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData = 1;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return true; /* Error -> asumimos busy */
    if (HAL_OSPI_Receive(hospi, &status_reg, HAL_MAX_DELAY) != HAL_OK)
        return true;

    return (status_reg & 0x01) != 0; /* Bit 0 = BUSY */
}

/* ===== Helpers Internos ===== */

static Result_t ospi_write_enable(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};

    /* Enviar comando Write Enable */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_WRITE_ENABLE_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Polling para verificar WEL bit (bit 1 del Status Register) */
    sConfig.Match = 0x02;
    sConfig.Mask = 0x02;
    sConfig.MatchMode = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

    sCommand.Instruction = W25Q_READ_STATUS_REG_CMD;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData = 1;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_AutoPolling(hospi, &sConfig, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t ospi_auto_polling_mem_ready(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    OSPI_AutoPollingTypeDef sConfig = {0};

    /* Polling para esperar que BUSY bit (bit 0) = 0 */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_READ_STATUS_REG_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData = 1;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    sConfig.Match = 0x00;
    sConfig.Mask = 0x01;
    sConfig.MatchMode = HAL_OSPI_MATCH_MODE_AND;
    sConfig.Interval = 0x10;
    sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_OSPI_AutoPolling(hospi, &sConfig, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}

static Result_t ospi_reset_chip(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};

    /* Reset Enable */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_RESET_ENABLE_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Reset Execute */
    sCommand.Instruction = W25Q_RESET_EXECUTE_CMD;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Esperar tiempo de reset (típicamente 30us, usamos HAL_Delay seguro) */
    HAL_Delay(1);

    return ERR_OK;
}

static Result_t ospi_configure_quad_mode(OSPI_HandleTypeDef *hospi)
{
    OSPI_RegularCmdTypeDef sCommand = {0};
    uint8_t reg = 0;

    /* Leer Status Register 2 */
    sCommand.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction = W25Q_READ_STATUS_REG2_CMD;
    sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE;
    sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData = 1;
    sCommand.DummyCycles = 0;
    sCommand.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_Receive(hospi, &reg, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Si QE bit ya está activo, retornar OK */
    if (reg & 0x02)
        return ERR_OK;

    /* Activar escritura volátil */
    sCommand.DataMode = HAL_OSPI_DATA_NONE;
    sCommand.Instruction = W25Q_VOLATILE_SR_WRITE_ENABLE;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Escribir Status Register 2 con QE bit activado */
    reg |= 0x02; /* QE = 1 */
    sCommand.DataMode = HAL_OSPI_DATA_1_LINE;
    sCommand.Instruction = W25Q_WRITE_STATUS_REG2_CMD;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_Transmit(hospi, &reg, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    /* Ajustar Status Register 3 (DRV1:2 = 00 para máxima capacidad de corriente) */
    sCommand.Instruction = W25Q_READ_STATUS_REG3_CMD;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_Receive(hospi, &reg, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    reg &= 0x9F; /* DRV1:2 = 00 */
    sCommand.Instruction = W25Q_WRITE_STATUS_REG3_CMD;
    if (HAL_OSPI_Command(hospi, &sCommand, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;
    if (HAL_OSPI_Transmit(hospi, &reg, HAL_MAX_DELAY) != HAL_OK)
        return ERR_ERROR;

    return ERR_OK;
}
