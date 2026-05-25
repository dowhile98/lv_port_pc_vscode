/**
 * @file  firmware_update_strategy.h
 * @brief Estrategia de actualización de firmware vía CycloneBOOT.
 *
 * Implementa `IUpdateManager_t` para actualizaciones OTA de firmware firmadas.
 * Encapsula la secuencia XIP-disable → CycloneBOOT → XIP-enable.
 *
 * @par Dependencias inyectadas (DI Container)
 * - `I_EXT_FLASH *`        — para deshabilitar/habilitar XIP (Memory-Mapped Mode)
 * - `ICycloneBootOps_t *`  — wrapper portable sobre las 4 funciones de CycloneBOOT
 *
 * @par Prohibiciones de capa
 * Este header y su .c NO incluyen `stm32u5xx_hal.h` ni `update/update.h`.
 * Toda dependencia de CycloneBOOT está encapsulada en `ICycloneBootOps_t`.
 *
 * @par Uso típico (DI Container)
 * @code
 *   FirmwareUpdateStrategy_t fw_strategy;
 *   FirmwareUpdateStrategyConfig_t cfg = {
 *       .flash_iface  = DI_GetExtFlashInterface(),
 *       .flash_handle = DI_GetExtFlashHandle(),
 *       .cboot_ops    = CycloneBootOpsAdapter_GetInterface(&cboot_adapter),
 *       .logger       = DI_GetLogger(),
 *   };
 *   FirmwareUpdateStrategy_Init(&fw_strategy, &cfg);
 *
 *   // El caller inyecta IUpdateManager_t * en el UpdateManager.
 *   IUpdateManager_t *iface = FirmwareUpdateStrategy_GetInterface(&fw_strategy);
 * @endcode
 */

#ifndef FIRMWARE_UPDATE_STRATEGY_H
#define FIRMWARE_UPDATE_STRATEGY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"
#include "hal/interfaces/i_ext_flash.h"
#include "interfaces/i_update_manager.h"
#include "interfaces/i_logger.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ═══════════════════════════════════════════════════════════════════════════
     * ICycloneBootOps — abstracción portable sobre CycloneBOOT
     *
     * Las cuatro operaciones de CycloneBOOT mapeadas a Result_t.
     * La implementación concreta (CycloneBootOpsAdapter) vive en
     * src/infrastructure/adapters/cyclone_boot_ops_adapter.c.
     * La implementación falsa para tests vive en tests/fakes/fake_cyclone_boot_ops.c.
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief V-Table para operaciones CycloneBOOT.
     *
     * Cada función toma `void *ctx` — el adaptador concreto provee el `UpdateContext *`
     * interno de CycloneBOOT. El dominio no necesita conocer ese tipo.
     */
    typedef struct ICycloneBootOps_Vtable
    {
        /**
         * @brief Inicializa el contexto de actualización con la configuración de flash.
         * @note  Configura memoria primaria (int flash), secundaria (ext flash),
         *        slots, y la clave pública ECDSA para verificación.
         * @param[in] ctx  Opaque CycloneBOOT context handle (no NULL).
         * @return ERR_OK on success, ERR_ERROR on CycloneBOOT failure.
         */
        Result_t (*Init)(void *ctx);

        /**
         * @brief Procesa un bloque de datos del archivo .img.
         * @note  Llama internamente a `updateProcess()`. Puede ejecutar erase/write.
         *        Llamar IWDG refresh periódicamente a nivel más alto si es necesario.
         * @param[in] ctx   Opaque context (no NULL).
         * @param[in] data  Buffer de datos (no NULL).
         * @param[in] len   Bytes en buffer (> 0).
         * @return ERR_OK on success, ERR_ERROR on write failure.
         */
        Result_t (*Process)(void *ctx, const uint8_t *data, size_t len);

        /**
         * @brief Finaliza la actualización y verifica la firma ECDSA del .img.
         * @note  Llama `updateFinalize()`. CycloneBOOT lee el Update Slot completo,
         *        verifica con `pemUpdtSignPublicKey[]`.
         * @param[in] ctx  Opaque context (no NULL).
         * @return ERR_OK                  Firma válida; imagen lista para swap.
         * @return ERR_INVALID_SIGNATURE   Firma inválida.
         * @return ERR_ERROR               Fallo de hardware.
         */
        Result_t (*Finalize)(void *ctx);

        /**
         * @brief Marca el flag de swap y reinicia el MCU.
         * @note  Llama `updateReboot()`. No retorna si tiene éxito.
         * @param[in] ctx  Opaque context (no NULL).
         * @return ERR_ERROR si `updateReboot()` falla; no retorna en éxito.
         */
        Result_t (*Reboot)(void *ctx);

    } ICycloneBootOps_Vtable_t;

    /**
     * @brief Handle polimórfico para operaciones CycloneBOOT.
     */
    typedef struct ICycloneBootOps
    {
        const ICycloneBootOps_Vtable_t *vtable; /**< V-Table (no NULL). */
        void *ctx;                              /**< UpdateContext opaco (no NULL). */
    } ICycloneBootOps_t;

    /** @brief Inline: verifica que el handle CycloneBOOT está completamente inicializado. */
    static inline bool CycloneBootOps_IsValid(const ICycloneBootOps_t *ops)
    {
        return (ops != NULL) && (ops->vtable != NULL) && (ops->vtable->Init != NULL) && (ops->vtable->Process != NULL) && (ops->vtable->Finalize != NULL) && (ops->vtable->Reboot != NULL) && (ops->ctx != NULL);
    }

    /* ═══════════════════════════════════════════════════════════════════════════
     * FirmwareUpdateStrategy
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Configuración para inicializar FirmwareUpdateStrategy_t (inyección de deps).
     */
    typedef struct FirmwareUpdateStrategyConfig
    {
        I_EXT_FLASH *flash_iface;           /**< Interfaz flash externa (no NULL). */
        I_EXT_FLASH_Handle_t flash_handle;  /**< Handle de flash (puede ser NULL si el driver no lo usa). */
        const ICycloneBootOps_t *cboot_ops; /**< Operaciones CycloneBOOT (no NULL). */
        const ILogger *logger;              /**< Logger opcional (puede ser NULL). */
    } FirmwareUpdateStrategyConfig_t;

    /**
     * @brief Instancia concreta de la estrategia de actualización de firmware.
     *
     * @note El campo `iface` debe ser el primero — el DI Container puede hacer
     *       un cast desde `IUpdateManager_t *` a `FirmwareUpdateStrategy_t *`.
     */
    typedef struct FirmwareUpdateStrategy
    {
        IUpdateManager_t iface; /**< Interface expuesta — DEBE SER EL PRIMERO. */

        I_EXT_FLASH *flash_iface;
        I_EXT_FLASH_Handle_t flash_handle;
        const ICycloneBootOps_t *cboot_ops;
        const ILogger *logger;

        bool is_active; /**< true entre Begin() y End()/Abort(). */
    } FirmwareUpdateStrategy_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Public API
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Inicializa la estrategia de actualización y conecta las dependencias.
     *
     * @param[out] self    Instancia a inicializar (no NULL).
     * @param[in]  config  Configuración con todas las dependencias (no NULL).
     *
     * @return ERR_OK           Inicialización exitosa.
     * @return ERR_NULL_POINTER self, config, o dependencias obligatorias son NULL.
     */
    Result_t FirmwareUpdateStrategy_Init(FirmwareUpdateStrategy_t *self,
                                         const FirmwareUpdateStrategyConfig_t *config);

    /**
     * @brief  Retorna el handle `IUpdateManager_t` para uso polimórfico.
     *
     * @param[in] self  Instancia inicializada (no NULL).
     * @return Puntero a la interfaz embebida, o NULL si self es NULL.
     */
    IUpdateManager_t *FirmwareUpdateStrategy_GetInterface(FirmwareUpdateStrategy_t *self);

#ifdef __cplusplus
}
#endif

#endif /* FIRMWARE_UPDATE_STRATEGY_H */
