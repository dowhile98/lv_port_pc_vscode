/**
 * @file elog_adapter.c
 * @brief Implementación del adapter ILogger usando lwprintf.
 * @version 2.0.0
 */

#include "infrastructure/adapters/elog_adapter.h"
#include "lwprintf/lwprintf.h"
#include <string.h>
#include <stdio.h>

/* ===== Color Codes (ANSI) ===== */

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_WHITE "\033[0;37m"

/* ===== Private Helpers ===== */

/**
 * @brief Mapea LogLevel_t a string de nivel.
 */
static const char *get_level_string(LogLevel_t level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return "E";
    case LOG_LEVEL_WARN:
        return "W";
    case LOG_LEVEL_INFO:
        return "I";
    case LOG_LEVEL_DEBUG:
        return "D";
    case LOG_LEVEL_VERBOSE:
        return "V";
    default:
        return "?";
    }
}

/**
 * @brief Mapea LogLevel_t a código de color ANSI.
 */
static const char *get_level_color(LogLevel_t level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return COLOR_RED;
    case LOG_LEVEL_WARN:
        return COLOR_YELLOW;
    case LOG_LEVEL_INFO:
        return COLOR_GREEN;
    case LOG_LEVEL_DEBUG:
        return COLOR_CYAN;
    case LOG_LEVEL_VERBOSE:
        return COLOR_WHITE;
    default:
        return COLOR_WHITE;
    }
}

/**
 * @brief Wrapper para output callback de lwprintf.
 * @note lwprintf espera retorno int, output_fn del adapter también.
 */
static int lwprintf_output_wrapper(int ch, lwprintf_t *lwobj)
{
    ElogAdapter_t *self = (ElogAdapter_t *)lwprintf_get_arg(lwobj);
    if (self && self->config.output_fn)
    {
        return self->config.output_fn(ch, self->config.output_arg);
    }
    return 0; /* Terminar si no hay callback */
}

/* ===== V-Table Implementation ===== */

/**
 * @brief Implementación de ILogger::Log.
 * @note Inspirado en elog: usa patrón de concatenación de strings sin buffer temporal.
 * @note Formato: LEVEL (timestamp) [TAG] file:line: mensaje\r\n
 * @note lwprintf tiene mutex interno (ThreadX) → thread-safe por diseño.
 */
static void elog_adapter_log_impl(void *impl,
                                  LogLevel_t level,
                                  const char *tag,
                                  const char *file,
                                  uint32_t line,
                                  const char *fmt,
                                  va_list args)
{
    if (impl == NULL || tag == NULL || fmt == NULL)
    {
        return;
    }

    ElogAdapter_t *self = (ElogAdapter_t *)impl;

    /* Filtro de nivel mínimo */
    if (level > self->config.min_level)
    {
        return;
    }

    /* Obtener timestamp */
    uint32_t timestamp = 0;
    if (self->config.get_tick_fn)
    {
        timestamp = self->config.get_tick_fn();
    }

    const char *level_str = get_level_string(level);

    /* Extraer basename del archivo si está habilitado */
    const char *filename = file;
    if (self->config.show_file_line && file != NULL)
    {
        const char *slash = file;
        while (*slash)
        {
            if (*slash == '/' || *slash == '\\')
            {
                filename = slash + 1;
            }
            slash++;
        }
    }

    /* Adquirir mutex manualmente para múltiples escrituras atómicas */
    if (!lwprintf_protect_ex(&self->lwprintf_instance))
    {
        return; /* No se pudo adquirir mutex, abortar */
    }

    /* Escribir color si está habilitado */
    if (self->config.enable_colors)
    {
        const char *color = get_level_color(level);
        lwprintf_printf_ex(&self->lwprintf_instance, "%s", color);
    }

    /* 1. Prefijo: "LEVEL (timestamp) [TAG]" */
    lwprintf_printf_ex(&self->lwprintf_instance, "%s (%lu) [%s]",
                       level_str, timestamp, tag);

    /* 2. Agregar file:line si está habilitado: " file.c:123" */
    if (self->config.show_file_line && file != NULL)
    {
        lwprintf_printf_ex(&self->lwprintf_instance, " %s:%lu", filename, line);
    }

    /* 3. Separador antes del mensaje: ": " */
    lwprintf_printf_ex(&self->lwprintf_instance, ": ");

    /* 4. Mensaje del usuario con formato variable */

    lwprintf_vprintf_ex(&self->lwprintf_instance, fmt, args);


    /* Escribir reset color si está habilitado */
    if (self->config.enable_colors)
    {
        lwprintf_printf_ex(&self->lwprintf_instance, "%s", COLOR_RESET);
    }

    /* 5. Newline final */
    lwprintf_printf_ex(&self->lwprintf_instance, "\r\n");

    /* Liberar mutex manualmente */
    lwprintf_unprotect_ex(&self->lwprintf_instance);
}

/* ===== V-Table estática ===== */

static const ILogger_Vtable s_elog_vtable = {
    .Log = elog_adapter_log_impl};

/* ===== Public API ===== */

Result_t ElogAdapter_Init(ElogAdapter_t *self, const ElogAdapterConfig_t *config)
{
    if (self == NULL || config == NULL)
    {
        return ERR_NULL_POINTER;
    }

    if (config->output_fn == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Limpiar estructura */
    memset(self, 0, sizeof(ElogAdapter_t));

    /* Copiar configuración */
    self->config = *config;

    /* Inicializar lwprintf con ThreadX mutex */
    uint8_t result = lwprintf_init_ex(&self->lwprintf_instance, lwprintf_output_wrapper);
    if (result == 0)
    {
        return ERR_ERROR;
    }

    /* Configurar argumento de usuario (self) para callback */
    lwprintf_set_arg(&self->lwprintf_instance, self);

    /* Setup V-Table */
    self->interface.vtable = &s_elog_vtable;
    self->interface.impl = self;
    self->is_initialized = true;

    return ERR_OK;
}

ILogger *ElogAdapter_GetInterface(ElogAdapter_t *self)
{
    if (self == NULL || !self->is_initialized)
    {
        return NULL;
    }
    return &self->interface;
}

Result_t ElogAdapter_Deinit(ElogAdapter_t *self)
{
    if (self == NULL)
    {
        return ERR_NULL_POINTER;
    }

    /* Limpiar estructura (lwprintf no requiere deinit explícito) */
    self->is_initialized = false;
    self->interface.vtable = NULL;
    self->interface.impl = NULL;

    return ERR_OK;
}
