/**
 * @file i_memory_allocator.h
 * @brief Interfaz abstracta para asignación de memoria (portable).
 * @version 1.0.0
 * @date 2026-02-16
 * @author Tecna Smart Lab
 *
 * Define la interfaz de asignación/liberación de memoria que permite
 * inyectar diferentes implementaciones (OSAL, malloc estándar, pools estáticos).
 *
 * @note Esta interfaz sigue el patrón V-Table para permitir Dependency Injection.
 * @note NUNCA usar dynamic allocation directamente en código de negocio,
 *       siempre inyectar esta interfaz.
 */

#ifndef I_MEMORY_ALLOCATOR_H
#define I_MEMORY_ALLOCATOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_types.h"

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Interfaz de asignación de memoria configurable.
     * @note El usuario inyecta la implementación concreta vía DI Container.
     */
    typedef struct IMemoryAllocator_t IMemoryAllocator_t;

    /**
     * @brief Estructura de la interfaz IMemoryAllocator (V-Table pattern).
     */
    struct IMemoryAllocator_t
    {
        void *context; /**< Contexto opaco (puede ser TX_BYTE_POOL*, heap_t*, etc.) */

        /**
         * @brief Asigna un bloque de memoria.
         *
         * Asigna un bloque de memoria de tamaño especificado. La memoria
         * NO está inicializada (contenido indefinido).
         *
         * @param[in] self Instancia de la interfaz (this pointer).
         * @param[in] size Tamaño en bytes a asignar (debe ser > 0).
         * @param[out] out_ptr Puntero al bloque asignado (NULL si falla).
         *
         * @return ERR_OK si éxito, ERR_NO_MEM si no hay memoria disponible,
         *         ERR_NULL_POINTER si out_ptr es NULL,
         *         ERR_INVALID_PARAM si size es 0.
         *
         * @note El bloque retornado está alineado según MEMORY_ALIGNMENT (típicamente 8 bytes).
         * @note Debe ser liberado con free() cuando ya no se necesite.
         */
        Result_t (*allocate)(IMemoryAllocator_t *self, uint32_t size, void **out_ptr);

        /**
         * @brief Libera un bloque de memoria previamente asignado.
         *
         * @param[in] self Instancia de la interfaz (this pointer).
         * @param[in] ptr Puntero al bloque a liberar (puede ser NULL).
         *
         * @return ERR_OK si éxito,
         *         ERR_INVALID_PARAM si el puntero es inválido (heap corrompido).
         *
         * @note free(NULL) es válido y no hace nada (comportamiento estándar C).
         * @note Liberar el mismo puntero dos veces causa comportamiento indefinido.
         */
        Result_t (*free)(IMemoryAllocator_t *self, void *ptr);

        /**
         * @brief Asigna memoria inicializada a cero (calloc).
         *
         * Asigna un bloque para un array de 'count' elementos de 'size' bytes cada uno,
         * inicializando la memoria a cero.
         *
         * @param[in] self Instancia de la interfaz (this pointer).
         * @param[in] count Número de elementos.
         * @param[in] size Tamaño de cada elemento en bytes.
         * @param[out] out_ptr Puntero al bloque asignado (NULL si falla).
         *
         * @return ERR_OK si éxito, ERR_NO_MEM si no hay memoria,
         *         ERR_NULL_POINTER si out_ptr es NULL,
         *         ERR_INVALID_PARAM si count * size causa overflow.
         *
         * @note Si la implementación no provee calloc nativo, se emula con allocate + memset.
         * @note Puede ser NULL si la implementación no soporta calloc.
         */
        Result_t (*calloc)(IMemoryAllocator_t *self, uint32_t count, uint32_t size, void **out_ptr);

        /**
         * @brief Re-asigna memoria (realloc).
         *
         * Cambia el tamaño de un bloque previamente asignado. Si el nuevo tamaño es
         * mayor, el contenido adicional es indefinido. Si es menor, el contenido se trunca.
         *
         * @param[in] self Instancia de la interfaz (this pointer).
         * @param[in] ptr Puntero al bloque original (NULL equivale a malloc).
         * @param[in] new_size Nuevo tamaño en bytes (0 equivale a free).
         * @param[out] out_ptr Puntero al nuevo bloque (puede ser diferente a ptr).
         *
         * @return ERR_OK si éxito, ERR_NO_MEM si no hay memoria,
         *         ERR_NULL_POINTER si out_ptr es NULL,
         *         ERR_NOT_SUPPORTED si la implementación no soporta realloc.
         *
         * @note Si realloc falla, el bloque original NO se libera.
         * @note Si la implementación no provee realloc, se emula con allocate + memcpy + free.
         * @note Puede ser NULL si la implementación no soporta realloc.
         */
        Result_t (*realloc)(IMemoryAllocator_t *self, void *ptr, uint32_t new_size, void **out_ptr);
    };

    /*============================================================================*
     * INLINE HELPERS (Defensive wrappers)
     *============================================================================*/

    /**
     * @brief Verifica si una instancia de IMemoryAllocator es válida.
     */
    static inline bool MemoryAllocator_IsValid(const IMemoryAllocator_t *allocator)
    {
        return (allocator != NULL) &&
               (allocator->allocate != NULL) &&
               (allocator->free != NULL);
    }

    /**
     * @brief Asigna memoria (wrapper defensivo).
     */
    static inline Result_t MemoryAllocator_Allocate(IMemoryAllocator_t *allocator,
                                                    uint32_t size,
                                                    void **out_ptr)
    {
        if (!MemoryAllocator_IsValid(allocator) || out_ptr == NULL)
        {
            return ERR_NULL_POINTER;
        }
        return allocator->allocate(allocator, size, out_ptr);
    }

    /**
     * @brief Libera memoria (wrapper defensivo).
     */
    static inline Result_t MemoryAllocator_Free(IMemoryAllocator_t *allocator, void *ptr)
    {
        if (!MemoryAllocator_IsValid(allocator))
        {
            return ERR_NULL_POINTER;
        }
        return allocator->free(allocator, ptr);
    }

    /**
     * @brief Asigna memoria inicializada a cero (wrapper defensivo).
     */
    static inline Result_t MemoryAllocator_Calloc(IMemoryAllocator_t *allocator,
                                                  uint32_t count,
                                                  uint32_t size,
                                                  void **out_ptr)
    {
        if (!MemoryAllocator_IsValid(allocator) || out_ptr == NULL)
        {
            return ERR_NULL_POINTER;
        }

        if (allocator->calloc != NULL)
        {
            return allocator->calloc(allocator, count, size, out_ptr);
        }

        /* Fallback: emular con allocate + memset */
        if (count != 0 && size > (UINT32_MAX / count))
        {
            return ERR_INVALID_PARAM; /* Overflow detection */
        }

        uint32_t total = count * size;
        Result_t res = allocator->allocate(allocator, total, out_ptr);
        if (res == ERR_OK)
        {
            extern void *memset(void *s, int c, size_t n);
            memset(*out_ptr, 0, total);
        }

        return res;
    }

    /**
     * @brief Re-asigna memoria (wrapper defensivo).
     */
    static inline Result_t MemoryAllocator_Realloc(IMemoryAllocator_t *allocator,
                                                   void *ptr,
                                                   uint32_t new_size,
                                                   void **out_ptr)
    {
        if (!MemoryAllocator_IsValid(allocator) || out_ptr == NULL)
        {
            return ERR_NULL_POINTER;
        }

        if (allocator->realloc != NULL)
        {
            return allocator->realloc(allocator, ptr, new_size, out_ptr);
        }

        /* Si no hay realloc, reportar no soportado */
        return ERR_NOT_SUPPORTED;
    }

#ifdef __cplusplus
}
#endif

#endif /* I_MEMORY_ALLOCATOR_H */
