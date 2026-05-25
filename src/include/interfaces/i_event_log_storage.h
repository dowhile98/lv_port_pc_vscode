/**
 * @file i_event_log_storage.h
 * @brief Interfaz para log circular de eventos en EEPROM.
 *
 * @note Buffer circular con metadatos persistentes.
 *       Máximo 100 eventos con wrap automático.
 */

#ifndef I_EVENT_LOG_STORAGE_H
#define I_EVENT_LOG_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // NULL
#include "hal_types.h"

#define EVENT_LOG_MAX_SIZE 11340 /**< Máximo 11,340 eventos en EEPROM (113.4 KB, 87.5% usage) */

/**
 * @brief Tipos de eventos del sistema.
 *
 * @note Rango de valores asignados por categoría:
 *   0–19  : Eventos de operación (boot, GPS, relay, errores)
 *   20–29 : Alarmas
 *   30–39 : Acciones de sistema (reset, clear)
 *   40–49 : Red / WiFi
 *   50–59 : Licencia
 *   60–69 : Advertencias
 */
typedef enum
{
    /* ── Sistema ──────────────────────────────────────────────────────── */
    EVENT_TYPE_SYSTEM_BOOT = 0,             /**< Arranque del sistema.              info=0, params=fw version */
    EVENT_TYPE_SYSTEM_FACTORY_RESET = 30,   /**< Reset de fábrica.                  info=0 */
    EVENT_TYPE_SYSTEM_EVENTS_CLEARED = 31,  /**< Historial de eventos borrado.      info=0 */
    EVENT_TYPE_SYSTEM_HOURMETER_RESET = 32, /**< Horómetro reseteado.               info=0 */

    /* ── Configuración ────────────────────────────────────────────────── */
    EVENT_TYPE_CONFIG_CHANGED = 1,  /**< Configuración modificada (genérico).  info=ConfigType_t */
    EVENT_TYPE_CONFIG_RELAY = 22,   /**< Configuración de relay guardada.      info=0 */
    EVENT_TYPE_CONFIG_GPS = 23,     /**< Configuración GPS guardada.           info=0 */
    EVENT_TYPE_CONFIG_GENERAL = 24, /**< Configuración general guardada.       info=0 */
    EVENT_TYPE_CONFIG_WIFI = 25,    /**< Configuración WiFi guardada.          info=0 */

    /* ── GPS ──────────────────────────────────────────────────────────── */
    EVENT_TYPE_GPS_LOCK_ACQUIRED = 2,   /**< Fix GPS 3D adquirido.              info=0 */
    EVENT_TYPE_GPS_LOCK_LOST = 3,       /**< Fix GPS perdido.                   info=0 */
    EVENT_TYPE_GPS_TIME_SYNC = 13,      /**< PPS sync activo: RTC sincronizado con GPS. info=0 */
    EVENT_TYPE_GPS_TIME_SYNC_LOST = 14, /**< PPS sync perdido: fallback a RTC solo.     info=timeout_s */

    /* ── Relay / ciclo de interrupción ───────────────────────────────── */
    EVENT_TYPE_RELAY_ENABLED = 5,     /**< Relay habilitado/deshabilitado.    info=0=disabled / 1=enabled */
    EVENT_TYPE_RELAY_SYNC_FAIL = 6,   /**< Sincronización GPS perdida en ciclo activo. info=0 */
    EVENT_TYPE_RELAY_CYCLE_START = 7, /**< Inicio del ciclo de interrupción.  info=0 */
    EVENT_TYPE_RELAY_CYCLE_END = 8,   /**< Fin del ciclo de interrupción.     info=0 */
    EVENT_TYPE_RELAY_FORCED_OPEN = 9, /**< Relay forzado abierto (alarma).    params=temperatura (int16_t °C×10) */
    EVENT_TYPE_RELAY_ERROR = 15,      /**< FSM relé entró en estado de error. info=0 */

    /* ── Alarmas ──────────────────────────────────────────────────────── */
    EVENT_TYPE_ALARM_HIGH_TEMP_ACTIVE = 20,  /**< Alarma alta temperatura activa.    params=temperatura (int16_t °C×10) */
    EVENT_TYPE_ALARM_HIGH_TEMP_CLEARED = 21, /**< Alarma alta temperatura cancelada. params=temperatura (int16_t °C×10) */

    /* ── Errores del sistema ──────────────────────────────────────────── */
    EVENT_TYPE_ERROR_EEPROM = 10,  /**< Error de lectura/escritura EEPROM.    info=Result_t code */
    EVENT_TYPE_ERROR_GPS = 11,     /**< Error de comunicación GPS.            info=Result_t code */
    EVENT_TYPE_ERROR_STORAGE = 12, /**< Error genérico de almacenamiento.     info=ConfigType_t */

    /* ── Red / WiFi ───────────────────────────────────────────────────── */
    EVENT_TYPE_WIFI_RESET = 40,        /**< Módulo WiFi reseteado.             info=0 */
    EVENT_TYPE_WIFI_CONNECTED = 41,    /**< Red WiFi conectada (IP obtenida).  info=0 */
    EVENT_TYPE_WIFI_DISCONNECTED = 42, /**< Red WiFi desconectada.             info=retry_count */

    /* ── Licencia / modo ─────────────────────────────────────────────── */
    EVENT_TYPE_LICENSE_FREE_MODE = 50,  /**< Modo libre activado.              info=0 */
    EVENT_TYPE_LICENSE_RENT_MODE = 51,  /**< Modo renta activado.              params=license_key (uint32_t) */
    EVENT_TYPE_LICENSE_RENT_ENDED = 52, /**< Renta expirada.                   info=0 */

    /* ── Advertencias ────────────────────────────────────────────────── */
    EVENT_TYPE_WARNING_BATTERY_LOW = 60, /**< Batería baja (si aplica HW).     info=0 */
} EventType_t;

/**
 * @brief Severidad del evento.
 */
typedef enum
{
    EVENT_SEVERITY_INFO = 0,
    EVENT_SEVERITY_WARNING = 1,
    EVENT_SEVERITY_ERROR = 2,
    EVENT_SEVERITY_CRITICAL = 3,
} EventSeverity_t;

/**
 * @brief Entrada de log de evento (tamaño fijo: 10 bytes).
 */
typedef struct __attribute__((packed))
{
    uint32_t timestamp; /**< Epoch UTC (4 bytes) */
    uint8_t type;       /**< EventType_t (1 byte) */
    uint8_t severity;   /**< EventSeverity_t (1 byte) */
    uint8_t info;       /**< Byte de información adicional (1 byte) */
    int16_t params;     /**< Parámetros específicos del evento (2 bytes) */
    uint8_t checksum;   /**< CRC8 de este struct (1 byte) */
} EventLogEntry_t;

/**
 * @brief Metadatos del buffer circular.
 */
typedef struct __attribute__((packed))
{
    uint16_t head;     /**< Índice de escritura (0-99) */
    uint16_t count;    /**< Cantidad de eventos válidos (0-100) */
    uint16_t checksum; /**< CRC16 de estos metadatos */
} EventLogMeta_t;

/**
 * @brief V-Table para IEventLogStorage.
 */
typedef struct IEventLogStorage_Vtable
{
    /**
     * @brief Añade un evento al log circular.
     * @note Thread-safe si se usa desde AO.
     */
    Result_t (*Append)(void *self, const EventLogEntry_t *event);

    /**
     * @brief Lee un evento por índice (0 = más antiguo).
     * @return ERR_INVALID_PARAM si index >= count.
     */
    Result_t (*ReadByIndex)(void *self, uint16_t index, EventLogEntry_t *out_event);

    /**
     * @brief Obtiene cantidad de eventos almacenados.
     */
    Result_t (*GetCount)(void *self, uint16_t *out_count);

    /**
     * @brief Lee metadatos del log.
     */
    Result_t (*GetMetadata)(void *self, EventLogMeta_t *out_meta);

    /**
     * @brief Borra todos los eventos (reset).
     */
    Result_t (*Clear)(void *self);

} IEventLogStorage_Vtable;

/**
 * @brief Instancia de interfaz de log de eventos.
 */
typedef struct IEventLogStorage
{
    const IEventLogStorage_Vtable *vtable;
    void *impl;
} IEventLogStorage;

/* ===== Helpers defensivos ===== */

static inline bool event_log_storage_is_valid(const IEventLogStorage *iface)
{
    return (iface != NULL) && (iface->vtable != NULL) && (iface->impl != NULL);
}

static inline Result_t EventLogStorage_Append(IEventLogStorage *iface, const EventLogEntry_t *event)
{
    if (!event_log_storage_is_valid(iface) || event == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->Append(iface->impl, event);
}

static inline Result_t EventLogStorage_ReadByIndex(IEventLogStorage *iface, uint16_t index, EventLogEntry_t *out_event)
{
    if (!event_log_storage_is_valid(iface) || out_event == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->ReadByIndex(iface->impl, index, out_event);
}

static inline Result_t EventLogStorage_GetCount(IEventLogStorage *iface, uint16_t *out_count)
{
    if (!event_log_storage_is_valid(iface) || out_count == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetCount(iface->impl, out_count);
}

static inline Result_t EventLogStorage_GetMetadata(IEventLogStorage *iface, EventLogMeta_t *out_meta)
{
    if (!event_log_storage_is_valid(iface) || out_meta == NULL)
        return ERR_NULL_POINTER;
    return iface->vtable->GetMetadata(iface->impl, out_meta);
}

static inline Result_t EventLogStorage_Clear(IEventLogStorage *iface)
{
    if (!event_log_storage_is_valid(iface))
        return ERR_NULL_POINTER;
    return iface->vtable->Clear(iface->impl);
}

#endif /* I_EVENT_LOG_STORAGE_H */
