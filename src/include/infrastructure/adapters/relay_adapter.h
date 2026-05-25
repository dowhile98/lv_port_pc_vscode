/**
 * @file relay_adapter.h
 * @brief Adaptador de relé - Implementación concreta de IRelayController.
 *
 * Infrastructure Layer - Orquesta:
 * · Domain Services (TimeWindowValidator, CycleScheduler)
 * · HAL Interfaces (GPIO, Timer)
 * · Time Synchronization (TimeAdapter via callback PPS)
 * · State Machine Management
 *
 * El RelayAdapter NO contiene lógica de negocio; solo delegación.
 * Toda la lógica está en los Domain Services inyectados.
 *
 * Sincronización:
 * · ISR (PPS): Muy rápido, solo postea evento a Active Object
 * · Worker Thread: Ejecuta Update(), procesa eventos, modifica GPIO
 * · No usa mutex en ruta crítica (ISR → postEvent es lock-free en ThreadX)
 */

#ifndef RELAY_ADAPTER_H
#define RELAY_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

#include "hal_types.h"
#include "interfaces/i_gpio.h"
#include "interfaces/i_timer.h"
#include "interfaces/i_relay_controller.h"
#include "interfaces/i_time_source.h"
#include "interfaces/i_time_sync_control.h"
#include "interfaces/i_config_storage.h"
#include "domain/services/time_window_validator.h"
#include "domain/services/cycle_scheduler.h"
#include "common/relay_types.h"

#define RELAY_ADAPTER_MAX_CALLBACKS 4 /**< Máximo número de callbacks registrados para cambios de estado. */
/**
 * @brief Estados de la máquina de estados del RelayAdapter.
 *
 * Define transiciones válidas y comportamientos por estado.
 * Previene estados inválidos y race conditions.
 */
typedef enum
{
	RELAY_FSM_INIT,			  /**< Estado inicial, no listo. */
	RELAY_FSM_WAIT_TIME_SYNC, /**< Esperando sincronización de tiempo. */
	RELAY_FSM_WAIT_WINDOW,	  /**< Tiempo sincronizado, esperando ventana. */
	RELAY_FSM_ACTIVE_CYCLING, /**< Dentro de ventana, relé en ciclo. */
	RELAY_FSM_FORCED_OPEN,	  /**< Forzado abierto (alarma alta temperatura). */
	RELAY_FSM_ERROR			  /**< Estado de error. */
} RelayFSM_State_t;

/**
 * @brief Configuración del RelayAdapter.
 *
 * @note CORRECCIÓN BLOCKER-003 (ISP): Se removió sync_control.
 *       RelayAdapter NO implementa ITimeSyncControl.
 *       Solo CONSUME ITimeSource para leer tiempo sincronizado.
 */
typedef struct
{
	/* Dependencias inyectadas - MÍNIMAS (ISP) */
	ITimeSource *time_source;	   /**< Lectura de tiempo (requerida). */
	I_GPIO *gpio;				   /**< Interfaz GPIO para control del relé. */
	I_TIMER *timer;				   /**< Interfaz Timer para cambios precisos. */
	I_TIMER_Handle_t timer_handle; /**< Handle del timer específico. */

	CycleSchedulerService_t *cycle_scheduler; /**< Servicio de ciclos (inyectado, no NULL). */

	/* Configuración de hardware */
	GPIO_Port_t relay_gpio_port; /**< Puerto GPIO del relé. */
	GPIO_Pin_t relay_gpio_pin;	 /**< Pin GPIO del relé. */

	// DB4
	GPIO_Port_t db4_gpio_port;
	GPIO_Pin_t db4_gpio_pin;
	// DB5
	GPIO_Port_t db5_gpio_port;
	GPIO_Pin_t db5_gpio_pin;
	/* Configuración del relé */
	TimeWindowConfig_t window_config; /**< Ventana de tiempo permitida. */

	/* Período de actualización periódica (ej: 100ms). */
	uint32_t update_interval_ms; /**< Intervalo recomendado para llamar Update(). */
} RelayAdapterConfig_t;

/**
 * @brief Adaptador de relé (Private).
 *
 * Estructura interna. Los clientes usan interfaces IRelayController.
 *
 * @note CORRECCIÓN BLOCKER-002: Máquina de estados explícita.
 *       Los estados booleanos (is_time_synchronized, is_in_window) ahora
 *       se reemplazan por fsm_state, que valida transiciones permitidas.
 *
 * @note CORRECCIÓN BLOCKER-003: Se removió sync_control.
 *       RelayAdapter ahora es consumidor puro de ITimeSource.
 */
typedef struct
{
	/* Interfaz exportada */
	IRelayController iface;

	/* Indicador de inicialización (BLOCKER HIGH-002) */
	bool is_initialized; /**< True si Init() fue ejecutado exitosamente. */

	/* Dependencias inyectadas - MÍNIMAS (ISP) */
	ITimeSource *time_source;
	I_GPIO *gpio;
	I_TIMER *timer;
	I_TIMER_Handle_t timer_handle;

	/* Configuración de hardware */
	// CTRL
	GPIO_Port_t relay_gpio_port;
	GPIO_Pin_t relay_gpio_pin;
	// DB4
	GPIO_Port_t db4_gpio_port;
	GPIO_Pin_t db4_gpio_pin;
	// DB5
	GPIO_Port_t db5_gpio_port;
	GPIO_Pin_t db5_gpio_pin;
	/* Domain Services */
	TimeWindowConfig_t window_config;		  /**< Config de ventana (STATELESS). */
	CycleSchedulerService_t *cycle_scheduler; /**< Servicio inyectado (STATEFUL). */

	/* BLOCKER-002: Máquina de estados explícita */
	RelayFSM_State_t fsm_state; /**< Estado actual de la máquina de estados. */

	/* Estado actual */
	RelayContactState_t current_state; /**< Estado físico actual (OPEN/CLOSED). */

	/* Métricas */
	uint32_t total_on_time_ms;			   /**< Tiempo acumulado con relé ON (ms). */
	uint32_t last_state_change_time_ms;	   /**< Timestamp del último cambio (ms). */
	DateTime_t last_state_change_datetime; /**< Hora del último cambio. */

	/* Callbacks */
	RelayStateChangeCallback_t state_change_callback[RELAY_ADAPTER_MAX_CALLBACKS];
	void *state_change_context[RELAY_ADAPTER_MAX_CALLBACKS];
	uint8_t registered_callbacks; /**< Bitmap de callbacks registrados (máx RELAY_ADAPTER_MAX_CALLBACKS). */
	/* Control de temporización (LÓGICA ORIGINAL v4.0.0) */
	uint32_t last_Tr_seconds;	 /**< Último Tr procesado (segundos desde medianoche). */
	uint32_t current_period_ms;	 /**< Periodo actual T = Ton + Toff (ms). */
	uint8_t current_cycle_index; /**< Índice de ciclo activo (multicycle). */
	bool timer_is_configured;	 /**< true si timer OC ya fue configurado. */

	/* Callbacks Timer OC (para cambios precisos de GPIO en ISR) */
	I_TIMER_OC_Callback_t oc_callback; /**< Callback registrado para OC events. */
	void *oc_callback_context;		   /**< Contexto para callback OC. */

	/* Alarm Management (ACTION-013) */
	bool alarm_active; /**< true if high-temperature alarm is active. */
} RelayAdapter_t;

/**
 * @brief Inicializa el RelayAdapter.
 *
 * Realiza:
 * · Inyección de dependencias
 * · Inicialización de Domain Services
 * · Configuración de GPIO y Timer
 * · Registro de callback PPS con TimeAdapter
 * · Establecimiento de estado inicial
 *
 * @param[out] self    Instancia a inicializar (no NULL).
 * @param[in]  config  Configuración (no NULL).
 *
 * @return ERR_OK si éxito,
 *         ERR_NULL_POINTER si parámetros NULL,
 *         ERR_INVALID_PARAM si config es inválida.
 *
 * @note Post-inicialización:
 *   · El relé está en estado RELAY_STATE_IDLE (OFF, esperando sincronización).
 *   · Debe llamarse Update() periódicamente (ej: cada 100ms) desde main loop.
 *   · El callback PPS se registra automáticamente con TimeAdapter.
 */
Result_t RelayAdapter_Init(RelayAdapter_t *self,
						   const RelayAdapterConfig_t *config);
/**
 * @brief De-inicializa RelayAdapter y libera recursos
 * @param[in] self Puntero al adapter
 * @return ERR_OK si exitoso
 * @note Idempotente: puede llamarse múltiples veces sin error
 */
Result_t RelayAdapter_Deinit(RelayAdapter_t *self);
/**
 * @brief Actualiza la máquina de estados del relé.
 *
 * Debe llamarse periódicamente (ej: cada 100ms) desde el main loop o
 * un thread de baja prioridad. Realiza:
 * · Evaluación de tiempo actual
 * · Validación de ventana temporal
 * · Cálculo de estado ON/OFF según ciclos
 * · Modificación de GPIO si es necesario
 * · Ejecución de callbacks
 *
 * @param[in] self  Instancia (no NULL).
 *
 * @return ERR_OK si actualización exitosa,
 *         ERR_NULL_POINTER si self es NULL,
 *         ERR_INVALID_STATE si hay error crítico.
 *
 * @note Performance:
 *   · Tiempo típico: <1ms en modo normal.
 *   · Sine determinística, no bloquea.
 *   · Puede ejecutar callbacks (rápidos).
 */
Result_t RelayAdapter_Update(RelayAdapter_t *self);

/**
 * @brief Maneja callback de sincronización PPS (desde TimeAdapter).
 *
 * Se llama automáticamente cuando GPS emite PPS válido.
 * En este adapter, solo marca que debe evaluarse el estado en próximo Update().
 *
 * @param[in] self     Instancia (no NULL, típicamente casteada desde ITimeSyncControl).
 * @param[in] gps_time Hora UTC del GPS (no NULL).
 *
 * @return ERR_OK siempre (no falla).
 *
 * @note Contexto: ISR (muy rápido). NO bloquea, NO usa mutex.
 */
Result_t RelayAdapter_OnPPS(RelayAdapter_t *self,
							const DateTime_t *gps_time);

/**
 * @brief Obtiene la interfaz IRelayController para inyección en Application.
 *
 * @param[in] self  Instancia inicializada (no NULL).
 *
 * @return Puntero a IRelayController que puede ser pasado a Use Cases.
 *
 * Ejemplo:
 *   RelayAdapter_t adapter = {0};
 *   RelayAdapter_Init(&adapter, &config);
 *   IRelayController *relay = RelayAdapter_GetInterface(&adapter);
 *   // Inyectar 'relay' en Application Layer
 */
IRelayController *RelayAdapter_GetInterface(RelayAdapter_t *self);

/**
 * @brief Actualiza estado de alarma del relay (thread-safe).
 *
 * Cuando alarma activa:
 * - Fuerza apertura de contacto (safety-critical)
 * - Transiciona FSM a RELAY_FSM_FORCED_OPEN
 * - Activa GPIO indicador alarma (DB5) si configurado
 *
 * Cuando alarma liberada:
 * - Limpia flag de alarma
 * - Retorna FSM a WAIT_TIME_SYNC para reanudar ciclo normal
 *
 * @param[in] self          Instancia del adaptador
 * @param[in] alarm_active  true si alarma activa, false si liberada
 *
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si self es NULL
 * @return ERR_INVALID_STATE si no inicializado
 *
 * @note Llamado desde RelayAO al recibir eventos ALARM_OVERTEMP/ALARM_CLEARED
 * @note Thread-safe: ejecuta en contexto RelayAO thread
 * @note Pattern: Similar a TimeAdapter alarm callback (ACTION-007)
 */
Result_t RelayAdapter_SetAlarmState(RelayAdapter_t *self, bool alarm_active);

#endif /* RELAY_ADAPTER_H */
