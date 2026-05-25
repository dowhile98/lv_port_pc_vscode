#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>

/**
 * @file hal_types.h
 * @brief Tipos portables para HAL y abstracciones del sistema
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Define tipos fundamentales portables que pueden usarse en TODAS las capas
 * de la arquitectura (Domain, Application, Infrastructure):
 * - Result_t: Códigos de error estándar
 * - Tipos de concurrencia opacos (os_mutex_t, etc.)
 *
 * @note Los tipos de concurrencia son OPACOS (void*) para mantener portabilidad
 * @note La implementación real reside en infrastructure/osal
 * @note Este header NO tiene dependencias de RTOS específicos
 */

/*============================================================================*
 * RESULT CODES
 *============================================================================*/

/**
 * @brief Códigos de resultado para operaciones HAL/System
 */
typedef enum
{
    ERR_OK = 0,                  /**< Operación exitosa */
    ERR_ERROR = -1,              /**< Error genérico */
    ERR_NULL_POINTER = -2,       /**< Puntero NULL inválido */
    ERR_INVALID_PARAM = -3,      /**< Parámetro inválido */
    ERR_BUSY = -4,               /**< Recurso ocupado */
    ERR_TIMEOUT = -5,            /**< Timeout expirado */
    ERR_INVALID_STATE = -6,      /**< Estado inválido */
    ERR_CHECKSUM = -7,           /**< Error de checksum */
    ERR_NOT_FOUND = -8,          /**< Recurso no encontrado */
    ERR_NOT_SUPPORTED = -9,      /**< Operación no soportada */
    ERR_NO_MEM = -10,            /**< No hay memoria disponible */
    ERR_CRC_FAIL = -11,          /**< Error de CRC (checksum no coincide) */
    ERR_NO_MEMORY = -12,         /**< No hay memoria disponible */
    ERR_HW_FAILURE = -13,        /**< Fallo de hardware */
    ERR_INVALID_RESPONSE = -14,  /**< Respuesta inválida de dispositivo */
    ERR_NOT_INIT = -15,          /**< Componente no inicializado */
    ERR_INVALID_ARGS = -16,      /**< Argumentos inválidos */
    ERR_INVALID_SIGNATURE = -17, /**< Firma criptográfica inválida (ECDSA / CRC) */
} Result_t;

/*============================================================================*
 * CONCURRENCY TYPES (Portable, Opaque)
 *============================================================================*/

/**
 * @brief Tipos de concurrencia portables (handles opacos)
 *
 * @details
 * Estos tipos son OPACOS (void*) para desacoplar el código de la aplicación
 * del RTOS específico (FreeRTOS, ThreadX, etc.). La implementación real reside
 * en infrastructure/osal/freertos/osal_freertos.c (PC) o similar.
 *
 * **Uso permitido:**
 * - Domain Layer: ✅ Puede usar os_mutex_t (sincronización thread-safe)
 * - Application Layer: ✅ Puede usar todos los tipos
 * - Infrastructure Layer: ✅ Implementa tipos concretos
 *
 * **Inicialización:**
 * - Todos los handles deben inicializarse en NULL
 * - Creación vía os_*_create() (ver osal.h)
 *
 * @note NO acceder a contenido interno (opaco por diseño)
 * @note Lifetime gestionado por OSAL (no free manual)
 *
 * @see infrastructure/osal/osal.h para API completa
 */
typedef void *os_thread_t;      /**< Handle de thread RTOS (opaco) */
typedef void *os_mutex_t;       /**< Handle de mutex (opaco, thread-safe) */
typedef void *os_semaphore_t;   /**< Handle de semáforo (opaco) */
typedef void *os_queue_t;       /**< Handle de cola de mensajes (opaco) */
typedef void *os_event_flags_t; /**< Handle de event flags (opaco) */
typedef void *os_timer_t;       /**< Handle de timer software (opaco) */

#endif /* HAL_TYPES_H */
