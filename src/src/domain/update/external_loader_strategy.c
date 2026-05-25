/**
 * @file  external_loader_strategy.c
 * @brief Implementación de ExternalLoaderStrategy — parser de .img con verificación ECDSA.
 *
 * @note  No incluye stm32u5xx_hal.h ni headers de CycloneCRYPTO.
 *        La dependencia criptográfica está encapsulada en ICryptoVerifier_t.
 */

#include "domain/update/external_loader_strategy.h"
#include <string.h>
#include <stddef.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * Private types
 * ═══════════════════════════════════════════════════════════════════════════ */
static const char *TAG = "ext_ldr";
/*
 * Nota: el campo dataSize del header .img está en byte offset 16 del header de 64 bytes.
 * Formato: headVers(4) + imgIndex(4) + imgType(1) + 3B_padding + dataSize(4) = offset 16.
 * Se extrae con memcpy usando EXT_LOADER_IMG_DATA_SIZE_OFFSET para evitar dependencias de
 * alineación del struct compilador.
 */

/* ═══════════════════════════════════════════════════════════════════════════
 * Private helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static ExternalLoaderStrategy_t *cast_self(void *raw)
{
    return (ExternalLoaderStrategy_t *)raw;
}

/**
 * @brief Borra sectores desde 0x00000000 hasta end_addr (rollback tras firma inválida).
 */
static void erase_region(ExternalLoaderStrategy_t *self, uint32_t end_addr)
{
    uint32_t addr = 0x00000000U;
    while (addr < end_addr)
    {
        EXT_FLASH_EraseSector(self->flash_iface, self->flash_handle, addr);
        addr += self->sector_size;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Private vtable implementations
 * ═══════════════════════════════════════════════════════════════════════════ */

static Result_t ext_begin(void *raw, UpdateType_t type)
{
    (void)type;
    ExternalLoaderStrategy_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, TAG, "===== BEGIN External Loader (Resources) =====");
        LOG_DEBUG(self->logger, TAG, "Target addr: 0x%08X", EXT_LOADER_RESOURCES_START_ADDR);
    }

    /* 1. Deshabilitar XIP antes de cualquier escritura */
    Result_t res = EXT_FLASH_DisableMemoryMapped(self->flash_iface, self->flash_handle);
    if (res != ERR_OK)
    {
        if (self->logger != NULL)
        {
            LOG_ERROR(self->logger, TAG, "Failed to disable XIP: %d", res);
        }
        return res;
    }

    if (self->logger != NULL)
    {
        LOG_DEBUG(self->logger, TAG, "XIP disabled");
    }

    /* 2. Inicializar SHA256 */
    res = ICryptoVerifier_Begin((ICryptoVerifier_t *)self->crypto);
    if (res != ERR_OK)
    {
        if (self->logger != NULL)
        {
            LOG_ERROR(self->logger, TAG, "Failed to begin crypto: %d", res);
        }
        EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
        return res;
    }

    if (self->logger != NULL)
    {
        LOG_DEBUG(self->logger, TAG, "SHA256 context initialized");
    }

    /* 3. Resetear máquina de estados */
    self->current_addr = EXT_LOADER_RESOURCES_START_ADDR;
    self->next_erase_addr = EXT_LOADER_RESOURCES_START_ADDR;
    self->total_bytes_expected = 0U;
    self->bytes_body_received = 0U;
    self->header_parsed = false;
    self->header_bytes_received = 0U;
    self->footer_bytes = 0U;
    self->is_active = true;

    return ERR_OK;
}

static Result_t ext_write(void *raw, const uint8_t *buffer, size_t len)
{
    ExternalLoaderStrategy_t *self = cast_self(raw);
    if (self == NULL || buffer == NULL)
        return ERR_NULL_POINTER;
    if (len == 0U)
        return ERR_INVALID_PARAM;
    if (!self->is_active)
        return ERR_INVALID_STATE;

    const uint8_t *ptr = buffer;
    size_t remaining = len;
    Result_t res;

    /* ── Fase 1: Acumular header hasta tener los 64 bytes completos ─────── */
    if (!self->header_parsed)
    {
        uint32_t need = EXT_LOADER_IMG_HEADER_SIZE - self->header_bytes_received;
        uint32_t take = (remaining < need) ? (uint32_t)remaining : need;

        memcpy(&self->header_buf[self->header_bytes_received], ptr, take);
        self->header_bytes_received += take;
        ptr += take;
        remaining -= take;

        if (self->header_bytes_received < EXT_LOADER_IMG_HEADER_SIZE)
        {
            /* Header incompleto — esperamos más chunks */
            return ERR_OK;
        }

        /* Header completo: extraer dataSize mediante offset fijo (LE, offset 16) */
        uint32_t data_size = 0U;
        memcpy(&data_size,
               &self->header_buf[EXT_LOADER_IMG_DATA_SIZE_OFFSET],
               sizeof(uint32_t));
        self->total_bytes_expected = data_size;
        self->header_parsed = true;

        if (self->logger != NULL)
        {
            LOG_INFO(self->logger, TAG, "Header parsed: dataSize = %u bytes", (unsigned int)data_size);
            LOG_DEBUG(self->logger, TAG, "Header bytes: %02X %02X %02X %02X (magic)",
                      self->header_buf[0], self->header_buf[1],
                      self->header_buf[2], self->header_buf[3]);
        }
    }

    /* ── Fase 2: Consumir bytes del body ─────────────────────────────────── */
    while (remaining > 0U && self->bytes_body_received < self->total_bytes_expected)
    {
        uint32_t body_left = self->total_bytes_expected - self->bytes_body_received;
        uint32_t n = ((uint32_t)remaining < body_left)
                         ? (uint32_t)remaining
                         : body_left;

        /* Borrar sector si necesario (progresivo).
         * Si n cruza más de un sector, iterar erase por cada sector a cruzar. */
        uint32_t end_addr = self->current_addr + n;
        while (self->current_addr >= self->next_erase_addr || end_addr > self->next_erase_addr)
        {
            if (self->logger != NULL)
            {
                LOG_DEBUG(self->logger, TAG, "Erasing sector at 0x%08X", self->next_erase_addr);
            }

            res = EXT_FLASH_EraseSector(self->flash_iface, self->flash_handle,
                                        self->next_erase_addr);
            if (res != ERR_OK)
            {
                if (self->logger != NULL)
                {
                    LOG_ERROR(self->logger, TAG, "Erase failed at 0x%08X: %d", self->next_erase_addr, res);
                }
                return res;
            }
            self->next_erase_addr += self->sector_size;
        }

        /* Escribir a flash */
        res = EXT_FLASH_Write(self->flash_iface, self->flash_handle,
                              self->current_addr, ptr, n);
        if (res != ERR_OK)
        {
            if (self->logger != NULL)
            {
                LOG_ERROR(self->logger, TAG, "Write failed at 0x%08X (len=%u): %d",
                          self->current_addr, (unsigned int)n, res);
            }
            return res;
        }

        /* Log solo cada 64KB */
        static uint32_t last_log_kb = 0;
        uint32_t current_kb = self->bytes_body_received / 1024;
        if (self->logger != NULL && (current_kb - last_log_kb >= 64))
        {
            LOG_DEBUG(self->logger, TAG, "Written %u KB @ 0x%08X",
                      current_kb, self->current_addr);
            last_log_kb = current_kb;
        }

        /* Alimentar SHA256 */
        res = ICryptoVerifier_Update((ICryptoVerifier_t *)self->crypto, ptr, n);
        if (res != ERR_OK)
            return res;

        self->bytes_body_received += n;
        self->current_addr += n;
        ptr += n;
        remaining -= n;
    }

    /* ── Fase 3: Bytes restantes → footer (nunca se escriben a flash) ────── */
    while (remaining > 0U && self->footer_bytes < EXT_LOADER_IMG_FOOTER_SIZE)
    {
        uint32_t footer_left = EXT_LOADER_IMG_FOOTER_SIZE - self->footer_bytes;
        uint32_t n = ((uint32_t)remaining < footer_left)
                         ? (uint32_t)remaining
                         : footer_left;

        memcpy(&self->footer_buf[self->footer_bytes], ptr, n);
        self->footer_bytes += n;
        ptr += n;
        remaining -= n;
    }

    return ERR_OK;
}

static Result_t ext_end(void *raw)
{
    ExternalLoaderStrategy_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;
    if (!self->is_active)
        return ERR_INVALID_STATE;

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, TAG, "===== END External Loader (Verification Phase) =====");
        LOG_DEBUG(self->logger, TAG, "Total body bytes written: %u", (unsigned int)self->bytes_body_received);
        LOG_DEBUG(self->logger, TAG, "Footer bytes received: %u / %u",
                  (unsigned int)self->footer_bytes, EXT_LOADER_IMG_FOOTER_SIZE);
    }

    self->is_active = false;

    /* Footer debe estar completo */
    if (self->footer_bytes != EXT_LOADER_IMG_FOOTER_SIZE)
    {
        if (self->logger != NULL)
        {
            LOG_ERROR(self->logger, TAG, "Incomplete footer: %u bytes (expected %u)",
                      (unsigned int)self->footer_bytes, EXT_LOADER_IMG_FOOTER_SIZE);
        }
        EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
        return ERR_INVALID_PARAM;
    }

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, TAG, "Starting ECDSA signature verification (256 bytes footer)");
    }

    /* Verificar firma ECDSA del body (sha256Final + ecdsaVerify dentro del adaptador) */
    Result_t res = ICryptoVerifier_Verify((ICryptoVerifier_t *)self->crypto,
                                          self->footer_buf,
                                          EXT_LOADER_IMG_FOOTER_SIZE);

    if (res != ERR_OK)
    {
        if (self->logger != NULL)
        {
            LOG_ERROR(self->logger, TAG, "Signature verification FAILED: %d", res);
            LOG_ERROR(self->logger, TAG, "Rolling back: erasing written data");
        }
        /* Rollback: borrar todo lo escrito */
        erase_region(self, self->current_addr);
        EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
        return ERR_INVALID_SIGNATURE;
    }

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, TAG, "✓ Signature verification PASSED");
        LOG_INFO(self->logger, TAG, "Re-enabling XIP mode");
    }

    EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);

    if (self->logger != NULL)
    {
        LOG_INFO(self->logger, TAG, "===== External Loader SUCCESS =====");
    }

    return ERR_OK;
}

static Result_t ext_abort(void *raw)
{
    ExternalLoaderStrategy_t *self = cast_self(raw);
    if (self == NULL)
        return ERR_NULL_POINTER;

    /* Re-habilitar XIP incondicionalmente */
    EXT_FLASH_EnableMemoryMapped(self->flash_iface, self->flash_handle);
    self->is_active = false;
    return ERR_OK;
}

static Result_t ext_reboot(void *raw)
{
    /* External Loader no requiere reboot — recursos disponibles en XIP inmediatamente */
    (void)raw;
    return ERR_OK;
}

/* ── Vtable estática ─────────────────────────────────────────────────────── */

static const IUpdateManager_Vtable_t s_ext_vtable = {
    .Begin = ext_begin,
    .Write = ext_write,
    .End = ext_end,
    .Abort = ext_abort,
    .Reboot = ext_reboot,
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════════════ */

Result_t ExternalLoaderStrategy_Init(ExternalLoaderStrategy_t *self,
                                     const ExternalLoaderStrategyConfig_t *config)
{
    if (self == NULL || config == NULL)
        return ERR_NULL_POINTER;
    if (config->flash_iface == NULL)
        return ERR_NULL_POINTER;
    if (config->crypto == NULL)
        return ERR_NULL_POINTER;
    if (config->sector_size == 0U)
        return ERR_INVALID_PARAM;

    self->flash_iface = config->flash_iface;
    self->flash_handle = config->flash_handle;
    self->crypto = config->crypto;
    self->logger = config->logger;
    self->sector_size = config->sector_size;
    self->is_active = false;

    /* Conectar vtable */
    self->iface.vtable = &s_ext_vtable;
    self->iface.impl = self;

    return ERR_OK;
}

IUpdateManager_t *ExternalLoaderStrategy_GetInterface(ExternalLoaderStrategy_t *self)
{
    if (self == NULL)
        return NULL;
    return &self->iface;
}
