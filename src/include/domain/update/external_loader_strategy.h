/**
 * @file  external_loader_strategy.h
 * @brief Estrategia de actualización de recursos estáticos (External Loader).
 *
 * Implementa `IUpdateManager_t` para actualizar los recursos de la flash externa
 * (assets LVGL, archivos web HTML/JS/CSS) desde un archivo `.img` firmado con
 * ECDSA-SHA256 generado por ImageBuilder.
 *
 * @par Protocolo del archivo `.img`
 * El archivo tiene 3 secciones:
 * ```
 * [ HEADER (64 bytes) | BODY (dataSize bytes) | FOOTER (256 bytes) ]
 * ```
 * - **HEADER**: contiene `dataSize` que indica el tamaño exacto del BODY.
 * - **BODY**: binario puro de recursos — escrito a flash raw offset `0x00000000`.
 * - **FOOTER**: 256 bytes de firma ECDSA-SHA256 del BODY — nunca se escribe a flash.
 *
 * @par Máquina de estados interna
 * ```
 * PARSING_HEADER → STREAMING_BODY (write + hash) → VERIFYING_FOOTER
 *     └─ sha256Final + ecdsaVerify(pemResSignPublicKey)
 *           ├─ OK   → EnableXIP  → ERR_OK
 *           └─ FAIL → erase region → EnableXIP → ERR_INVALID_SIGNATURE
 * ```
 *
 * @par Prohibiciones de capa
 * Este header NO incluye `stm32u5xx_hal.h`, headers de CycloneCRYPTO, ni
 * headers de CycloneBOOT. Toda dependencia criptográfica está en `ICryptoVerifier_t`.
 *
 * @par Uso típico (DI Container)
 * @code
 *   ExternalLoaderStrategy_t loader;
 *   ExternalLoaderStrategyConfig_t cfg = {
 *       .flash_iface  = DI_GetExtFlashInterface(),
 *       .flash_handle = DI_GetExtFlashHandle(),
 *       .crypto       = CycloneCryptoVerifierAdapter_GetInterface(&crypto_adapter),
 *       .sector_size  = 4096U,  // W25Q128: 4 KB sectors
 *       .logger       = DI_GetLogger(),
 *   };
 *   ExternalLoaderStrategy_Init(&loader, &cfg);
 *   IUpdateManager_t *iface = ExternalLoaderStrategy_GetInterface(&loader);
 * @endcode
 */

#ifndef EXTERNAL_LOADER_STRATEGY_H
#define EXTERNAL_LOADER_STRATEGY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"
#include "hal/interfaces/i_ext_flash.h"
#include "interfaces/i_update_manager.h"
#include "interfaces/i_crypto_verifier.h"
#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Constants
 * ═══════════════════════════════════════════════════════════════════════════ */

/** @brief Tamaño del header del archivo .img producido por ImageBuilder (64 bytes). */
#define EXT_LOADER_IMG_HEADER_SIZE (64U)

/** @brief Tamaño del footer ECDSA del archivo .img (64 bytes, firma RAW: 32r + 32s). */
#define EXT_LOADER_IMG_FOOTER_SIZE (64U)

/**
 * @brief Offset del campo `dataSize` dentro del header .img (raw byte offset).
 *
 * Estructura CycloneBOOT ImageHeader (packed):
 *   Offset 0x00-0x03: headVers (uint32_t)
 *   Offset 0x04-0x07: imgIndex (uint32_t)
 *   Offset 0x08:      imgType (uint8_t)
 *   Offset 0x09-0x0C: dataPadding (uint32_t)
 *   Offset 0x0D-0x10: dataSize (uint32_t) ← Campo a leer
 */
#define EXT_LOADER_IMG_DATA_SIZE_OFFSET (13U)

/** @brief Dirección flash raw de inicio para escritura de recursos. */
#define EXT_LOADER_RESOURCES_START_ADDR (0x00000000U)

    /* ═══════════════════════════════════════════════════════════════════════════
     * Configuration (Dependency Injection)
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Configuración de inyección de dependencias para ExternalLoaderStrategy_t.
     */
    typedef struct ExternalLoaderStrategyConfig
    {
        I_EXT_FLASH *flash_iface;          /**< Interfaz flash externa (no NULL). */
        I_EXT_FLASH_Handle_t flash_handle; /**< Handle opaco de flash (puede ser NULL). */
        const ICryptoVerifier_t *crypto;   /**< Verificador SHA256 + ECDSA (no NULL). */
        uint32_t sector_size;              /**< Tamaño de sector para erase (p.ej. 4096U). */
        const ILogger *logger;             /**< Logger opcional (puede ser NULL). */
    } ExternalLoaderStrategyConfig_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * ExternalLoaderStrategy
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Instancia concreta de la estrategia External Loader.
     *
     * @note `iface` DEBE ser el primer campo — permite C99 first-field cast
     *       desde `IUpdateManager_t *` a `ExternalLoaderStrategy_t *`.
     */
    typedef struct ExternalLoaderStrategy
    {
        IUpdateManager_t iface; /**< Interface expuesta — DEBE SER EL PRIMERO. */

        /* Dependencias inyectadas */
        I_EXT_FLASH *flash_iface;
        I_EXT_FLASH_Handle_t flash_handle;
        const ICryptoVerifier_t *crypto;
        const ILogger *logger;
        uint32_t sector_size; /**< Tamaño de sector flash para erase. */

        /* Estado de escritura */
        uint32_t current_addr;         /**< Raw offset actual de escritura. */
        uint32_t next_erase_addr;      /**< Siguiente sector a borrar. */
        uint32_t total_bytes_expected; /**< dataSize extraído del header. */
        uint32_t bytes_body_received;  /**< Bytes del BODY ya escritos. */

        /* Parseo del header .img */
        bool header_parsed;
        uint32_t header_bytes_received; /**< Bytes acumulados del header. */
        uint8_t header_buf[EXT_LOADER_IMG_HEADER_SIZE];

        /* Acumulación del footer (nunca se escribe a flash) */
        uint8_t footer_buf[EXT_LOADER_IMG_FOOTER_SIZE];
        uint32_t footer_bytes;

        bool is_active;
    } ExternalLoaderStrategy_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Public API
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Inicializa la estrategia y conecta las dependencias.
     *
     * @param[out] self    Instancia a inicializar (no NULL).
     * @param[in]  config  Configuración con todas las dependencias (no NULL).
     *
     * @return ERR_OK           Inicialización exitosa.
     * @return ERR_NULL_POINTER self, config, o dependencias obligatorias son NULL.
     * @return ERR_INVALID_PARAM sector_size == 0.
     */
    Result_t ExternalLoaderStrategy_Init(ExternalLoaderStrategy_t *self,
                                         const ExternalLoaderStrategyConfig_t *config);

    /**
     * @brief  Retorna el handle `IUpdateManager_t` para uso polimórfico.
     *
     * @param[in] self  Instancia inicializada (no NULL).
     * @return Puntero a la interfaz embebida, o NULL si self es NULL.
     */
    IUpdateManager_t *ExternalLoaderStrategy_GetInterface(ExternalLoaderStrategy_t *self);

#ifdef __cplusplus
}
#endif

#endif /* EXTERNAL_LOADER_STRATEGY_H */
