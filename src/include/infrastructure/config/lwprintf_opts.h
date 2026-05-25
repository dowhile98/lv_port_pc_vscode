/**
 * @file lwprintf_opts.h
 * @brief LwPRINTF configuration for TCS Current Interrupter (ThreadX + STM32U5)
 * @version 1.0.0
 * @date 2026-02-09
 *
 * @note Este archivo configura lwprintf para usar ThreadX mutex y soporte completo.
 */

#ifndef LWPRINTF_OPTS_HDR_H
#define LWPRINTF_OPTS_HDR_H

/* ===== ThreadX RTOS Support ===== */

/**
 * @brief Habilitar soporte de RTOS (ThreadX)
 */
#define LWPRINTF_CFG_OS 1

/**
 * @brief Tipo de handle de mutex (ThreadX)
 * @note Requiere incluir tx_api.h antes de usar lwprintf
 */
#include "tx_api.h"
#define LWPRINTF_CFG_OS_MUTEX_HANDLE TX_MUTEX

/**
 * @brief Habilitar protección manual de mutex
 * @note IMPORTANTE: Esto permite usar lwprintf_protect_ex/unprotect_ex para
 *       proteger múltiples llamadas con un solo mutex lock, evitando deadlocks
 *       cuando se hacen varias escrituras consecutivas desde la misma tarea.
 */
#define LWPRINTF_CFG_OS_MANUAL_PROTECT 1

/* ===== Feature Support ===== */

/**
 * @brief Soporte para long long int (64-bit)
 */
#define LWPRINTF_CFG_SUPPORT_LONG_LONG 1

/**
 * @brief Soporte para tipos flotantes (float, double)
 * @note Deshabilitado para embedded (ahorro de code size)
 */
#define LWPRINTF_CFG_SUPPORT_TYPE_FLOAT 1

/**
 * @brief Soporte para punteros (%p)
 */
#define LWPRINTF_CFG_SUPPORT_TYPE_POINTER 1

/**
 * @brief Soporte para engineering notation (%k, %K)
 * @note Útil para valores con prefijos SI (k, M, G, etc.)
 */
#define LWPRINTF_CFG_SUPPORT_TYPE_ENGINEERING 0

#endif /* LWPRINTF_OPTS_HDR_H */
