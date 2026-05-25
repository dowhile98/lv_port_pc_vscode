/**
 * @file elog_adapter.h
 * @brief Adapter que conecta ILogger con lwprintf.
 * @version 2.0.0
 *
 * @note Este adapter usa lwprintf (Third_Party/lwprintf) para logging
 *       thread-safe con ThreadX, permitiendo su uso a través de la
 *       interfaz ILogger sin violar DIP.
 */

#ifndef ELOG_ADAPTER_H
#define ELOG_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "interfaces/i_logger.h"
#include "hal_types.h"
#include "lwprintf/lwprintf.h"

    /**
     * @brief Callback para obtener timestamp (ticks del sistema).
     */
    typedef uint32_t (*ElogAdapter_GetTickFn)(void);

    /**
     * @brief Callback para output de caracteres (UART/SWO/etc).
     * @param[in] ch Carácter a escribir.
     * @param[in] arg Argumento de usuario (puede ser NULL).
     * @return ch en éxito, 0 para terminar.
     */
    typedef int (*ElogAdapter_OutputFn)(int ch, void *arg);

    /**
     * @brief Configuración de inicialización del ElogAdapter.
     */
    typedef struct
    {
        LogLevel_t min_level;              /**< Nivel mínimo de logging */
        bool enable_colors;                /**< Habilitar colores ANSI */
        bool show_file_line;               /**< Mostrar file:line en logs */
        ElogAdapter_GetTickFn get_tick_fn; /**< Callback para timestamp */
        ElogAdapter_OutputFn output_fn;    /**< Callback para escribir caracteres */
        void *output_arg;                  /**< Argumento para output_fn (opcional) */
    } ElogAdapterConfig_t;

    /**
     * @brief Contexto del ElogAdapter.
     */
    typedef struct
    {
        ILogger interface;            /**< Interfaz pública ILogger */
        ElogAdapterConfig_t config;   /**< Configuración almacenada */
        lwprintf_t lwprintf_instance; /**< Instancia de lwprintf */
        bool is_initialized;          /**< Flag de inicialización */
    } ElogAdapter_t;

    /* ===== Public API ===== */

    /**
     * @brief Inicializa el ElogAdapter.
     *
     * @param[in,out] self   Instancia del adapter (debe estar en memoria estática/stack).
     * @param[in]     config Configuración de inicialización.
     *
     * @return ERR_OK en éxito, código de error en fallo.
     *
     * @note Esta función inicializa lwprintf con ThreadX mutex internamente.
     *       Thread-safe por defecto. Solo llamar una vez por instancia.
     */
    Result_t ElogAdapter_Init(ElogAdapter_t *self, const ElogAdapterConfig_t *config);

    /**
     * @brief Obtiene la interfaz ILogger del adapter.
     *
     * @param[in] self Instancia del adapter.
     *
     * @return Puntero a ILogger* (válido mientras self exista), NULL si no inicializado.
     *
     * @note El puntero retornado apunta a memoria dentro de 'self'.
     *       No liberar ni modificar vtable.
     */
    ILogger *ElogAdapter_GetInterface(ElogAdapter_t *self);

    /**
     * @brief Desinicializa el adapter (cleanup).
     *
     * @param[in] self Instancia del adapter.
     *
     * @return ERR_OK en éxito.
     *
     * @note Después de llamar Deinit, GetInterface retornará NULL.
     */
    Result_t ElogAdapter_Deinit(ElogAdapter_t *self);

#ifdef __cplusplus
}
#endif

#endif /* ELOG_ADAPTER_H */
