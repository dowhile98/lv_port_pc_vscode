/**
 * @file  i_crypto_verifier.h
 * @brief Interfaz portable para verificación ECDSA-SHA256 de imágenes .img.
 *
 * Abstrae la API de CycloneCRYPTO (`sha256Init/Update/Final` + `ecdsaVerifySignature`)
 * para que la capa de dominio sea testeable en PC sin dependencias de CycloneCRYPTO.
 *
 * @par Flujo de uso
 * @code
 *   ICryptoVerifier_Begin(&verifier);
 *
 *   while (chunk_available)
 *       ICryptoVerifier_Update(&verifier, chunk_ptr, chunk_len);
 *
 *   Result_t res = ICryptoVerifier_Verify(&verifier, footer_sig, 256U);
 *   // ERR_OK → firma válida; ERR_INVALID_SIGNATURE → rechazada
 * @endcode
 *
 * @par Implementaciones
 * - **Concreta**: `src/infrastructure/adapters/crypto/cyclone_crypto_verifier_adapter.c`
 *   wraps `sha256Init/Update/Final` + `ecdsaVerifySignature(pemResSignPublicKey)`.
 * - **Fake**: `tests/fakes/fake_crypto_verifier.c` — configurable para TDD.
 *
 * @par Prohibiciones de capa
 * Este header NO incluye ningún header de CycloneCRYPTO, HAL, ni OS.
 */

#ifndef I_CRYPTO_VERIFIER_H
#define I_CRYPTO_VERIFIER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* ═══════════════════════════════════════════════════════════════════════════
     * V-Table
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief V-Table de operaciones de verificación criptográfica.
     *
     * Mapea SHA256 incremental + ECDSA verify a `Result_t`.
     * El adaptador concreto gestiona internamente `Sha256Context`, `EcPublicKey`, y
     * `EcdsaSignature`; el dominio nunca ve esos tipos.
     */
    typedef struct ICryptoVerifier_Vtable
    {
        /**
         * @brief Resetea el contexto SHA256 para una nueva sesión.
         * @note  Equivale a `sha256Init()`.
         * @param[in] ctx  Contexto opaco del adaptador (no NULL).
         * @return ERR_OK siempre (no falla salvo null).
         */
        Result_t (*Begin)(void *ctx);

        /**
         * @brief Alimenta datos al SHA256 incremental.
         * @note  Equivale a `sha256Update()`. Llamar por cada chunk del BODY.
         * @param[in] ctx   Contexto opaco (no NULL).
         * @param[in] data  Buffer de datos del BODY (no NULL).
         * @param[in] len   Bytes en buffer (> 0).
         * @return ERR_OK on success.
         */
        Result_t (*Update)(void *ctx, const uint8_t *data, size_t len);

        /**
         * @brief Finaliza el SHA256 y verifica la firma ECDSA contra la clave de recursos.
         * @note  Equivale a `sha256Final()` → `ecdsaVerifySignature(pemResSignPublicKey)`.
         * @param[in] ctx      Contexto opaco (no NULL).
         * @param[in] sig      Footer completo del .img (bytes DER de la firma, no NULL).
         * @param[in] sig_len  Longitud del footer (debe ser exactamente 256 bytes).
         * @return ERR_OK                 Firma válida.
         * @return ERR_INVALID_SIGNATURE  Firma inválida.
         * @return ERR_INVALID_PARAM      sig_len != 256.
         */
        Result_t (*Verify)(void *ctx, const uint8_t *sig, size_t sig_len);

    } ICryptoVerifier_Vtable_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Interface instance
     * ═══════════════════════════════════════════════════════════════════════════ */

    /**
     * @brief Handle polimórfico para el verificador criptográfico.
     */
    typedef struct ICryptoVerifier
    {
        const ICryptoVerifier_Vtable_t *vtable; /**< V-Table (no NULL). */
        void *ctx;                              /**< Contexto interno del adaptador (no NULL). */
    } ICryptoVerifier_t;

    /* ═══════════════════════════════════════════════════════════════════════════
     * Inline wrappers
     * ═══════════════════════════════════════════════════════════════════════════ */

    /** @brief Valida que el handle y vtable están completamente inicializados. */
    static inline bool ICryptoVerifier_IsValid(const ICryptoVerifier_t *v)
    {
        return (v != NULL) && (v->vtable != NULL) && (v->vtable->Begin != NULL) && (v->vtable->Update != NULL) && (v->vtable->Verify != NULL) && (v->ctx != NULL);
    }

    static inline Result_t ICryptoVerifier_Begin(ICryptoVerifier_t *v)
    {
        if (!ICryptoVerifier_IsValid(v))
            return ERR_NULL_POINTER;
        return v->vtable->Begin(v->ctx);
    }

    static inline Result_t ICryptoVerifier_Update(ICryptoVerifier_t *v,
                                                  const uint8_t *data, size_t len)
    {
        if (!ICryptoVerifier_IsValid(v) || data == NULL)
            return ERR_NULL_POINTER;
        if (len == 0U)
            return ERR_INVALID_PARAM;
        return v->vtable->Update(v->ctx, data, len);
    }

    static inline Result_t ICryptoVerifier_Verify(ICryptoVerifier_t *v,
                                                  const uint8_t *sig, size_t sig_len)
    {
        if (!ICryptoVerifier_IsValid(v) || sig == NULL)
            return ERR_NULL_POINTER;
        return v->vtable->Verify(v->ctx, sig, sig_len);
    }

#ifdef __cplusplus
}
#endif

#endif /* I_CRYPTO_VERIFIER_H */
