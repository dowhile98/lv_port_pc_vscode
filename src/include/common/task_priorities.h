/**
 * @file task_priorities.h
 * @brief Mapa centralizado de prioridades ThreadX para todos los hilos de
 *        aplicación del firmware TCS CICX1.
 *
 * ## Regla ThreadX
 * Valor **menor** = prioridad **más alta** (0 es la máxima).
 * La distancia entre prioridades es IRRELEVANTE: prioridad 3 preempta a 4
 * de forma idéntica a como preempta a 99. No existen gaps con beneficio.
 *
 * ## Slots reservados por LVGL (lv_threadx_osal.c — prio_map[])
 *
 * | TX Prio | LVGL enum              | Uso                               |
 * |---------|------------------------|-----------------------------------|
 * |    2    | LV_THREAD_PRIO_HIGHEST | Futuro uso interno LVGL           |
 * |    6    | LV_THREAD_PRIO_HIGH    | Draw thread (rasterizador)        |
 * |   10    | LV_THREAD_PRIO_MID     | LV_THREADX_DEFAULT_PRIORITY       |
 * |   16    | LV_THREAD_PRIO_LOW     | Reservado                         |
 * |   20    | LV_THREAD_PRIO_LOWEST  | Reservado                         |
 *
 * INVARIANTE: ningún AO de aplicación puede usar valores 2, 6, 10, 16 ó 20.
 *
 * ## Tabla de prioridades de aplicación
 *
 * | TX Prio | Constante               | Tarea                                     |
 * |---------|-------------------------|-------------------------------------------|
 * |    3    | TASK_PRIO_RELAY         | RelayAO — safety, preempta draw thread    |
 * |    4    | TASK_PRIO_NETWORK       | NetworkAO (WiFi/HTTP)                     |
 * |    4    | TASK_PRIO_CONTROL       | ControlTask                               |
 * |    5    | TASK_PRIO_GPS           | GPSAo                                     |
 * |   (6)   | (LVGL draw thread)      | Reservado — NO usar                       |
 * |    7    | TASK_PRIO_STORAGE       | StorageCoordinatorAO                      |
 * |    8    | TASK_PRIO_DIGITAL_INPUT | DigitalInputAO (botones)                  |
 * |    8    | TASK_PRIO_BUZZER        | BuzzerAO                                  |
 * |    9    | TASK_PRIO_UI            | UiAO — kick lv_task_handler               |
 *
 * ## Flujo UI/LVGL
 * UiAO (prio 9) llama lv_task_handler(), que señaliza al draw thread (prio 6).
 * El draw thread preempta a UiAO inmediatamente, rasteriza, señaliza al DMA y
 * se bloquea. UiAO se reanuda para el siguiente ciclo.
 * UiAO DEBE tener prioridad menor (número mayor) que el draw thread.
 *
 * @note Para cambiar cualquier prioridad edite SOLO este archivo.
 *       Los módulos usan estas constantes — NO literales numéricos.
 *
 * @author Tecna Smart Lab
 * @date   2026-03-02
 */
#ifndef TASK_PRIORITIES_H
#define TASK_PRIORITIES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* =========================================================================
 * SLOTS RESERVADOS POR LVGL — documentados para visibilidad, nunca usar
 * ======================================================================== */
/* #define _LVGL_PRIO_HIGHEST  2U  -- LV_THREAD_PRIO_HIGHEST                */
/* #define _LVGL_DRAW_THREAD   6U  -- LV_DRAW_THREAD_PRIO (THREAD_PRIO_HIGH) */
/* #define _LVGL_PRIO_MID     10U  -- LV_THREAD_PRIO_MID (default)          */
/* #define _LVGL_PRIO_LOW     16U  -- LV_THREAD_PRIO_LOW                    */
/* #define _LVGL_PRIO_LOWEST  20U  -- LV_THREAD_PRIO_LOWEST                 */

/* =========================================================================
 * TIER 1 — Safety / Realtime
 * ======================================================================== */

/**
 * @brief RelayAO — control de corte de corriente.
 *
 * Mayor prioridad de aplicación. Preempta cualquier hilo incluyendo el draw
 * thread de LVGL (prio 6) para garantizar apertura determinista del relay.
 */
#define TASK_PRIO_RELAY (3U)

/* =========================================================================
 * TIER 2 — Comunicaciones y control
 * ======================================================================== */

/**
 * @brief NetworkAO — pila WiFi/HTTP (esp-hosted).
 *
 * Por encima del draw thread (6): los eventos de red no deben ser retrasados
 * por el rasterizador.
 */
#define TASK_PRIO_NETWORK (4U)

/**
 * @brief ControlTask — bucle de control principal.
 */
#define TASK_PRIO_CONTROL (4U)

/**
 * @brief GPSAo — procesamiento NMEA y sincronización de tiempo.
 */
#define TASK_PRIO_GPS (5U)

/* =========================================================================
 * TIER 3 — Almacenamiento  (por debajo del draw thread — 6 reservado LVGL)
 * ======================================================================== */

/**
 * @brief StorageCoordinatorAO — escrituras diferidas en EEPROM.
 */
#define TASK_PRIO_STORAGE (7U)

/* =========================================================================
 * TIER 4 — Periféricos de usuario
 * ======================================================================== */

/**
 * @brief DigitalInputAO — polling de botones físicos (período 20 ms).
 *
 * Velocidad humana (>50 ms por pulsación): no requiere alta prioridad.
 */
#define TASK_PRIO_DIGITAL_INPUT (8U)

/**
 * @brief BuzzerAO — feedback auditivo asíncrono.
 */
#define TASK_PRIO_BUZZER (8U)

/* =========================================================================
 * TIER 5 — Presentación  (siempre debajo del draw thread de LVGL)
 * ======================================================================== */

/**
 * @brief UiAO — bucle LVGL (lv_task_handler + ui_tick).
 *
 * @note DEBE ser prioridad MENOR (número mayor) que el draw thread (6).
 *       UiAO hace el "kick"; el draw thread preempta inmediatamente y
 *       rasteriza. Si UiAO fuera más prioritario, el rasterizador nunca
 *       correría hasta que UiAO se bloqueara en os_thread_sleep().
 */
#define TASK_PRIO_UI (9U)

#ifdef __cplusplus
}
#endif

#endif /* TASK_PRIORITIES_H */
