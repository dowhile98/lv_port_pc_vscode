/**
 * @file  i_update_manager.h
 * @brief Interfaz polimórfica para el gestor de actualizaciones OTA.
 *
 * Contrato de la capa de dominio para las operaciones de actualización.
 * Oculta completamente la estrategia concreta (FirmwareUpdateStrategy vía
 * CycloneBOOT, o ExternalLoaderStrategy vía I_EXT_FLASH) al llamador.
 *
 * @par Flujo de uso (Strategy Pattern)
 * @code
 *   // 1. Begin fija la estrategia — única llamada con discriminación de tipo.
 *   IUpdateManager_Begin(&mgr, UPDATE_TYPE_FIRMWARE);
 *
 *   // 2. Write y End son agnósticos al destino.
 *   while (chunk_available)
 *       IUpdateManager_Write(&mgr, buf, len);
 *
 *   IUpdateManager_End(&mgr);          // verifica firma ECDSA internamente
 *   IUpdateManager_Reboot(&mgr);       // solo para UPDATE_TYPE_FIRMWARE
 * @endcode
 *
 * @par Thread-safety
 * Las implementaciones concretas (UpdateManager_t) protegen el acceso con un
 * TX_MUTEX de ThreadX. Un segundo Begin() mientras hay update activo devuelve
 * ERR_BUSY.
 *
 * @par Prohibiciones de capa
 * Este header NO incluye ningún header de CycloneBOOT, HAL, ni ThreadX.
 * Solo depende de <stdint.h>, <stddef.h> y "hal_types.h".
 */

#ifndef I_UPDATE_MANAGER_H
#define I_UPDATE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_types.h" /* Result_t */

#ifdef __cplusplus
extern "C"
{
#endif

    /* ═══════════════════════════════════════════════════════════════════════════
     * Types
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Selecciona la estrategia concreta en Begin().
     *
     * @note Write / End / Abort / Reboot NO reciben este tipo — son agnósticos
     *       al destino. La estrategia seleccionada en Begin encapsula todo.
     */
    typedef enum UpdateType
    {
        UPDATE_TYPE_FIRMWARE = 0U,        /**< OTA: CycloneBOOT → Update Slot en ext-flash. */
        UPDATE_TYPE_EXTERNAL_LOADER = 1U, /**< Recursos: escritura directa a región resources. */
    } UpdateType_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * V-Table
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief V-Table para operaciones de actualización.
     *
     * Todos los punteros deben ser no-NULL en una instancia válida.
     */
    typedef struct IUpdateManager_Vtable
    {
        /**
         * @brief  Inicia una sesión de actualización y fija la estrategia activa.
         *
         * @note   Para UPDATE_TYPE_FIRMWARE: desactiva XIP (Memory-Mapped Mode) e
         *         inicializa CycloneBOOT (updateInit). Para UPDATE_TYPE_EXTERNAL_LOADER:
         *         prepara la máquina de estados del parser de formato .img.
         * @note   NOT ISR-safe. Llamar solo desde contexto de tarea.
         *
         * @param[in] self  Instancia concreta (no NULL).
         * @param[in] type  Estrategia a activar.
         *
         * @return ERR_OK          Sesión iniciada.
         * @return ERR_BUSY        Ya hay un update en progreso.
         * @return ERR_NULL_POINTER self es NULL.
         * @return ERR_ERROR       Fallo de inicialización interna (CycloneBOOT / flash).
         */
        Result_t (*Begin)(void *self, UpdateType_t type);

        /**
         * @brief  Envía un bloque de datos al destino activo.
         *
         * @note   Internamente llama updateProcess() o EXT_FLASH_Write() según la
         *         estrategia fijada en Begin. El handler HTTP no necesita saber cuál.
         * @note   NOT ISR-safe.
         *
         * @param[in] self    Instancia concreta (no NULL).
         * @param[in] buffer  Datos recibidos (no NULL).
         * @param[in] len     Número de bytes en buffer (> 0).
         *
         * @return ERR_OK           Bloque procesado correctamente.
         * @return ERR_NULL_POINTER self o buffer son NULL.
         * @return ERR_INVALID_PARAM len == 0.
         * @return ERR_ERROR        Fallo de escritura en flash.
         * @return ERR_INVALID_PARAM No se llamó Begin() previamente.
         */
        Result_t (*Write)(void *self, const uint8_t *buffer, size_t len);

        /**
         * @brief  Finaliza la sesión: verifica firma ECDSA-SHA256 y cierra la sesión.
         *
         * @note   Para UPDATE_TYPE_FIRMWARE: llama updateFinalize() (CycloneBOOT
         *         verifica la firma internamente con pemUpdtSignPublicKey).
         *         Para UPDATE_TYPE_EXTERNAL_LOADER: sha256Final() →
         *         ecdsaVerifySignature(pemResSignPublicKey); si falla, ejecuta rollback
         *         (erase de la región escrita) y retorna ERR_INVALID_SIGNATURE.
         *         En ambos casos re-activa XIP si estaba desactivado.
         * @note   NOT ISR-safe.
         *
         * @param[in] self  Instancia concreta (no NULL).
         *
         * @return ERR_OK               Firma válida; actualización guardada en flash.
         * @return ERR_INVALID_SIGNATURE Firma inválida; rollback ejecutado.
         * @return ERR_NULL_POINTER      self es NULL.
         * @return ERR_ERROR             Fallo de hardware durante finalización.
         */
        Result_t (*End)(void *self);

        /**
         * @brief  Cancela la sesión activa y restaura el estado del sistema.
         *
         * @note   Garantiza re-activación de XIP independientemente del estado
         *         interno de la estrategia. Safe to call en cualquier momento
         *         después de Begin(), incluso si Write() retornó error.
         * @note   NOT ISR-safe.
         *
         * @param[in] self  Instancia concreta (no NULL).
         *
         * @return ERR_OK           Sesión cancelada; sistema en estado seguro.
         * @return ERR_NULL_POINTER self es NULL.
         */
        Result_t (*Abort)(void *self);

        /**
         * @brief  Reinicia el MCU para activar el nuevo firmware.
         *
         * @note   Solo tiene efecto útil para UPDATE_TYPE_FIRMWARE. La implementación
         *         hace osDelay(5000) para dar tiempo al handler HTTP de enviar HTTP 200
         *         antes del reset. Para UPDATE_TYPE_EXTERNAL_LOADER no requiere reboot
         *         (los recursos están disponibles en XIP inmediatamente tras End()).
         * @note   NOT ISR-safe. Esta llamada no retorna si tiene éxito.
         *
         * @param[in] self  Instancia concreta (no NULL).
         *
         * @return ERR_NULL_POINTER self es NULL.
         * @return ERR_ERROR        Fallo en updateReboot() (CycloneBOOT).
         *         (Si tiene éxito, la función no retorna.)
         */
        Result_t (*Reboot)(void *self);

    } IUpdateManager_Vtable_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Interface instance
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Handle polimórfico para el gestor de actualizaciones.
     *
     * Los llamadores (HTTP handlers, DI container) mantienen un puntero a esta
     * struct y despachan a través de los inline wrappers de abajo.
     * Las implementaciones concretas (UpdateManager_t) embeben esta struct como
     * campo y rellenan .vtable + .impl en su función Init().
     */
    typedef struct IUpdateManager
    {
        const IUpdateManager_Vtable_t *vtable; /**< V-Table de la implementación (no NULL). */
        void *impl;                            /**< Puntero a la instancia concreta (no NULL). */
    } IUpdateManager_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Validity helper
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Comprueba que el handle y todos los punteros de la vtable son no-NULL.
     * @return true si el handle está completamente inicializado.
     */
    static inline bool IUpdateManager_IsValid(const IUpdateManager_t *iface)
    {
        return (iface != NULL) && (iface->vtable != NULL) && (iface->vtable->Begin != NULL) && (iface->vtable->Write != NULL) && (iface->vtable->End != NULL) && (iface->vtable->Abort != NULL) && (iface->vtable->Reboot != NULL) && (iface->impl != NULL);
    }

    /* ═══════════════════════════════════════════════════════════════════════════
     * Inline wrappers
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief  Inicia una sesión de actualización (ver vtable).
     */
    static inline Result_t IUpdateManager_Begin(IUpdateManager_t *iface, UpdateType_t type)
    {
        if (!IUpdateManager_IsValid(iface))
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Begin(iface->impl, type);
    }

    /**
     * @brief  Envía un bloque de datos al destino activo (ver vtable).
     */
    static inline Result_t IUpdateManager_Write(IUpdateManager_t *iface,
                                                const uint8_t *buffer,
                                                size_t len)
    {
        if (!IUpdateManager_IsValid(iface) || buffer == NULL)
        {
            return ERR_NULL_POINTER;
        }
        if (len == 0U)
        {
            return ERR_INVALID_PARAM;
        }
        return iface->vtable->Write(iface->impl, buffer, len);
    }

    /**
     * @brief  Finaliza la sesión y verifica firma ECDSA-SHA256 (ver vtable).
     */
    static inline Result_t IUpdateManager_End(IUpdateManager_t *iface)
    {
        if (!IUpdateManager_IsValid(iface))
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->End(iface->impl);
    }

    /**
     * @brief  Cancela la sesión activa (ver vtable).
     */
    static inline Result_t IUpdateManager_Abort(IUpdateManager_t *iface)
    {
        if (!IUpdateManager_IsValid(iface))
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Abort(iface->impl);
    }

    /**
     * @brief  Reinicia el MCU para activar el nuevo firmware (ver vtable).
     */
    static inline Result_t IUpdateManager_Reboot(IUpdateManager_t *iface)
    {
        if (!IUpdateManager_IsValid(iface))
        {
            return ERR_NULL_POINTER;
        }
        return iface->vtable->Reboot(iface->impl);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_UPDATE_MANAGER_H */
