/**
 * @file time_adapter.h
 * @brief Adaptador de tiempo con fallback automático (GPS <-> RTC).
 *
 * Infrastructure Layer - Implementa ITimeSource e ITimeSyncControl.
 */

#ifndef TIME_ADAPTER_H
#define TIME_ADAPTER_H

#include "interfaces/i_time_source.h"
#include "interfaces/i_time_sync_control.h"
#include "interfaces/i_gps_source.h"
#include "interfaces/i_rtc.h"
#include "interfaces/i_event_notifier.h"
#include "domain/services/pps_dispatcher.h"

/**
 * @brief Estados de sincronización del tiempo.
 */
typedef enum
{
    TIME_SYNC_STATE_INIT,         /**< Inicializando el sistema. */
    TIME_SYNC_STATE_WAIT_GPS_FIX, /**< Esperando señal válida de GPS. */
    TIME_SYNC_STATE_GPS_ACTIVE,   /**< Sincronizado con GPS (PPS activo). */
    TIME_SYNC_STATE_RTC_ONLY,     /**< Fallback: Usando RTC interno por pérdida de GPS. */
    TIME_SYNC_STATE_ERROR         /**< Error crítico en el subsistema de tiempo. */
} TimeSyncState_t;

/**
 * @brief Origen del PPS (para diagnóstico).
 */
typedef enum
{
    PPS_SOURCE_NONE = 0, /**< Sin PPS activo. */
    PPS_SOURCE_GPS = 1,  /**< PPS real de GPS. */
    PPS_SOURCE_RTC = 2   /**< PPS sintético de alarma RTC. */
} PPSSource_t;

/**
 * @brief Configuración del TimeAdapter.
 */
typedef struct
{
    I_RTC *rtc;                      /**< Interfaz HAL del RTC (Dependencia). */
    RTC_Handle_t rtc_handle;         /**< Handle del periférico RTC. */
    IGPSSource *gps;                 /**< Interfaz de lectura GPS (Dependencia). */
    PPSDispatcher_t *pps_dispatcher; /**< Dispatcher para notificar PPS a subscribers (ACTION-007). */
    uint32_t pps_timeout_ms;         /**< Timeout para detectar pérdida de PPS (ej: 2000ms). */
    uint32_t rtc_write_interval_ms;  /**< Intervalo mínimo entre escrituras RTC (default 24h = 86400000ms). */
    IEventNotifier *event_notifier;  /**< Event notifier para GPS_LOCK/TIME_SYNC events (opcional). */
} TimeAdapterConfig_t;

/**
 * @brief Estructura del TimeAdapter.
 */
typedef struct
{
    ITimeSource source_iface;    /**< Interfaz de lectura (Domain). */
    ITimeSyncControl sync_iface; /**< Interfaz de sincronización (Infrastructure). */

    I_RTC *rtc;
    RTC_Handle_t rtc_handle;
    IGPSSource *gps;
    PPSDispatcher_t *pps_dispatcher; /**< Para notificar PPS a subscribers (ACTION-007). */

    TimeSyncState_t state;
    uint32_t last_pps_timestamp;
    uint32_t pps_timeout_ms;
    bool is_synchronized;

    /* ACTION-007: RTC write throttling */
    uint32_t rtc_write_interval_ms;    /**< Intervalo mínimo entre escrituras RTC. */
    uint32_t last_rtc_write_timestamp; /**< Timestamp de última escritura RTC. */

    /* ACTION-007: RTC alarm fallback */
    bool rtc_alarm_active;     /**< True cuando usa alarma RTC para PPS. */
    RTC_Alarm_t alarm_id;      /**< ID de alarma (I_RTC_ALARM_A). */
    void *alarm_event_context; /**< Contexto para posting eventos PPS (ej: RelayAO queue). */

    /* Estado de inicialización */
    bool is_initialized; /**< Flag de inicialización del adapter. */

    IEventNotifier *event_notifier; /**< Event notifier para GPS_LOCK/TIME_SYNC events. */
} TimeAdapter_t;

/**
 * @brief Inicializa el TimeAdapter.
 */
Result_t TimeAdapter_Init(TimeAdapter_t *self, const TimeAdapterConfig_t *config);

/**
 * @brief De-inicializa TimeAdapter y libera recursos
 * @param[in] self Puntero al adapter
 * @return ERR_OK si exitoso
 */
Result_t TimeAdapter_Deinit(TimeAdapter_t *self);

/**
 * @brief Actualiza la máquina de estados (detección de timeouts).
 *        Debe llamarse periódicamente (ej: cada 100ms).
 */
Result_t TimeAdapter_Update(TimeAdapter_t *self);

/**
 * @brief Obtiene la interfaz de lectura (ITimeSource).
 */
ITimeSource *TimeAdapter_GetTimeSourceInterface(TimeAdapter_t *self);

/**
 * @brief Obtiene la interfaz de sincronización (ITimeSyncControl).
 */
ITimeSyncControl *TimeAdapter_GetSyncControlInterface(TimeAdapter_t *self);

/**
 * @brief Configura alarma RTC para generar PPS fallback (ACTION-007).
 * @param self Puntero a TimeAdapter.
 * @param seconds_offset Offset en segundos desde hora actual (típicamente 1s).
 * @return ERR_OK si éxito, error code si falla.
 */
Result_t TimeAdapter_ConfigureRTCAlarm(TimeAdapter_t *self, uint32_t seconds_offset);

/**
 * @brief Desactiva alarma RTC (ACTION-007).
 * @param self Puntero a TimeAdapter.
 * @return ERR_OK si éxito, error code si falla.
 */
Result_t TimeAdapter_DeactivateRTCAlarm(TimeAdapter_t *self);

/**
 * @brief Registra contexto para posting eventos PPS desde alarma RTC (ACTION-007).
 * @param self Puntero a TimeAdapter.
 * @param context Contexto de evento (ej: puntero a RelayAO queue).
 */
void TimeAdapter_RegisterAlarmEventContext(TimeAdapter_t *self, void *context);

/**
 * @brief Obtiene el estado actual de sincronización (ACTION-007).
 * @param self Puntero a TimeAdapter.
 * @return Estado de sincronización (WAIT_GPS_FIX, GPS_ACTIVE, RTC_ONLY, etc.).
 */
TimeSyncState_t TimeAdapter_GetSyncState(const TimeAdapter_t *self);

/**
 * @brief Obtiene el origen actual del PPS (GPS vs RTC) (ACTION-007).
 * @param self Puntero a TimeAdapter.
 * @return PPS_SOURCE_GPS si GPS activo, PPS_SOURCE_RTC si alarma RTC, PPS_SOURCE_NONE si sin señal.
 */
PPSSource_t TimeAdapter_GetPPSSource(const TimeAdapter_t *self);

#endif /* TIME_ADAPTER_H */
