#include "infrastructure/adapters/relay_adapter.h"
#include "infrastructure/osal/osal.h"
#include <string.h>

/* Forward declarations */
static Result_t RelayAdapter_SetContactState(void *_self, RelayContactState_t new_state);

/* Forward declaration de las vtables */
static Result_t Relay_GetState(void *impl, RelayContactState_t *out_is_closed);
static Result_t Relay_GetStatus(void *impl, RelayStatus_t *out_status);
static Result_t Relay_UpdateConfig(void *impl, const RelayConfig_t *config);
static Result_t Relay_RegisterStateChangeCallback(void *self, RelayStateChangeCallback_t cb, void *context);
static Result_t Relay_UnregisterStateChangeCallback(void *impl);
/* NEW: Active Object Integration Methods */
static Result_t Relay_OnPPS(void *impl, const DateTime_t *gps_time);
static Result_t Relay_Update(void *impl);
static Result_t Relay_SetAlarmState(void *impl, bool alarm_active);

static const IRelayController_Vtable relay_controller_vtable = {
	.SetState = RelayAdapter_SetContactState,
	.GetState = Relay_GetState,
	.GetStatus = Relay_GetStatus,
	.UpdateConfig = Relay_UpdateConfig,
	.RegisterStateChangeCallback = Relay_RegisterStateChangeCallback,
	.UnregisterStateChangeCallback = Relay_UnregisterStateChangeCallback,
	.OnPPS = Relay_OnPPS,				 /* ✅ Active Object Integration */
	.Update = Relay_Update,				 /* ✅ Active Object Integration */
	.SetAlarmState = Relay_SetAlarmState /* ✅ ACTION-013: Alarm Management */
};

/* ===== Máquina de Estados (BLOCKER-002) ===== */

/**
 * @brief Tabla de transiciones válidas de la máquina de estados.
 *
 * Formato: valid_transitions[FROM_STATE][TO_STATE]
 * Previene estados inválidos y facilita debug.
 *
 * ✅ CORRECCIÓN CRÍTICA (ACTION-013 + BUG FIX):
 * - FORCED_OPEN ahora alcanzable desde TODOS los estados operacionales (incluido INIT)
 * - Alarma es SAFETY-CRITICAL → debe interrumpir sistema en cualquier momento
 * - FORCED_OPEN permite reentrada idempotente (alarmas múltiples no causan ERROR)
 * - Solo ERROR es terminal (requiere reset para salir)
 *
 * Escenarios críticos cubiertos:
 * 1. Alarma activa antes de time sync → INIT → FORCED ✅
 * 2. Alarma durante operación normal → ANY → FORCED ✅
 * 3. Alarmas repetidas/múltiples → FORCED → FORCED ✅
 * 4. Recuperación de alarma → FORCED → WAIT_SYNC ✅
 */
static const bool valid_transitions[RELAY_FSM_ERROR + 1][RELAY_FSM_ERROR + 1] = {
	/* FROM\TO:       INIT   WAIT_SYNC   WAIT_WIN   CYCLING   FORCED   ERROR */
	/* INIT */ {false, true, false, false, true, true},		 /* ✅ INIT → FORCED (alarma temprana) */
	/* WAIT_SYNC */ {false, false, true, false, true, true}, /* ✅ → FORCED_OPEN */
	/* WAIT_WIN */ {false, true, false, true, true, true},	 /* ✅ → FORCED_OPEN */
	/* CYCLING */ {false, true, true, false, true, true},	 /* ✅ → FORCED_OPEN */
	/* FORCED */ {false, true, false, true, true, true},	 /* ✅ → WAIT_SYNC | reentrada idempotente */
	/* ERROR */ {true, false, false, false, false, false}};	 /* Solo INIT puede salir de ERROR */

/**
 * @brief Valida si una transición de estado es permitida.
 *
 * @param from   Estado actual
 * @param to     Estado destino
 *
 * @return true si transición es válida, false si es inválida
 */
static bool RelayFSM_CanTransition(RelayFSM_State_t from, RelayFSM_State_t to)
{
	if (from > RELAY_FSM_ERROR || to > RELAY_FSM_ERROR)
		return false;
	return valid_transitions[from][to];
}

/**
 * @brief Ejecuta la transición de estado de la máquina.
 *
 * @param self      Instancia del adaptador
 * @param new_state Estado destino
 *
 * @return ERR_OK si transición exitosa
 * @return ERR_INVALID_STATE si transición no permitida
 *
 * @note Transiciones idempotentes (mismo estado) siempre permitidas (early return).
 */
static Result_t RelayFSM_Transition(RelayAdapter_t *self, RelayFSM_State_t new_state)
{
	/* ✅ FIX: Transiciones idempotentes siempre permitidas (no causan ERROR)
	 * Ejemplo: FORCED → FORCED cuando alarma múltiple
	 * Rationale: Evita ERROR state en actualizaciones redundantes */
	if (self->fsm_state == new_state)
	{
		return ERR_OK; /* Ya en el estado correcto, nada que hacer */
	}

	if (!RelayFSM_CanTransition(self->fsm_state, new_state))
	{
		self->fsm_state = RELAY_FSM_ERROR;
		return ERR_INVALID_STATE;
	}

	/* Transición de estado */
	self->fsm_state = new_state;

	/* Log transición (puede extenderse con callbacks OnEntry/OnExit) */
	// TODO: Registrar transición en event log

	return ERR_OK;
}

/**
 * @brief Callback Timer OC - Ejecuta en contexto ISR para cambio preciso de GPIO.
 *
 * Este callback se ejecuta cuando el timer alcanza los valores OC1 o OC2.
 * Cambia el estado físico del relé con precisión de 1μs.
 *
 * LóGICA ORIGINAL v4.0.0:
 * - CH1: Primer cambio de estado (ON→OFF o OFF→ON según startWith)
 * - CH2: Segundo cambio de estado (completa el ciclo)
 *
 * @param[in] context  Puntero a RelayAdapter_t
 * @param[in] channel  Canal que disparó (CH1 o CH2)
 */
static void RelayAdapter_TimerOC_Callback(void *context, I_TIMER_Channel_t channel)
{
	RelayAdapter_t *self = (RelayAdapter_t *)context;

	if (self == NULL || !self->is_initialized)
	{
		return;
	}

	/* Obtener configuración del ciclo actual para saber cómo mapear canales */
	RelayConfig_t config;
	if (CycleScheduler_GetCurrentConfig(self->cycle_scheduler, &config) != ERR_OK)
	{
		return;
	}

	RelayContactState_t new_relay_state;

	if (channel == I_TIMER_CHANNEL_1)
	{
		/* CH1: Primer evento del ciclo */
		if (config.start_with_on)
		{
			/* Si inicia en ON, CH1 cierra relé */

			new_relay_state = RELAY_CONTACT_CLOSED;
		}
		else
		{
			/* Si inicia en OFF, CH1 abre relé */
			new_relay_state = RELAY_CONTACT_OPEN;
		}
	}
	else if (channel == I_TIMER_CHANNEL_2)
	{
		/* CH2: Segundo evento del ciclo (complementario a CH1) */
		if (config.start_with_on)
		{
			/* Si inicia en ON, CH2 abre relé */
			new_relay_state = RELAY_CONTACT_OPEN;
		}
		else
		{
			/* Si inicia en OFF, CH2 cierra relé */
			new_relay_state = RELAY_CONTACT_CLOSED;
		}
	}
	else
	{
		return; /* Canal desconocido */
	}

	/* Actualizar estado interno */
	RelayAdapter_SetContactState(self, new_relay_state);
}

Result_t RelayAdapter_Init(RelayAdapter_t *self, const RelayAdapterConfig_t *config)
{
	if (self == NULL || config == NULL)
		return ERR_NULL_POINTER;

	if (config->cycle_scheduler == NULL)
		return ERR_NULL_POINTER; /* cycle_scheduler DEBE estar inyectado */

	/* BLOCKER HIGH-002: Validar dependencias requeridas */
	if (config->time_source == NULL || config->gpio == NULL || config->timer == NULL)
		return ERR_NULL_POINTER;

	memset(self, 0, sizeof(RelayAdapter_t));

	/* Inyección de dependencias - MÍNIMAS (BLOCKER-003: ISP) */
	self->time_source = config->time_source;
	/* NOTA: sync_control REMOVIDO - RelayAdapter no implementa ITimeSyncControl */
	self->gpio = config->gpio;
	self->timer = config->timer;
	self->timer_handle = config->timer_handle;
	self->relay_gpio_port = config->relay_gpio_port;
	self->relay_gpio_pin = config->relay_gpio_pin;
	self->db4_gpio_port = config->db4_gpio_port;
	self->db4_gpio_pin = config->db4_gpio_pin;
	self->db5_gpio_port = config->db5_gpio_port;
	self->db5_gpio_pin = config->db5_gpio_pin;
	/* Inicializar interfaces exportadas */
	self->iface.vtable = &relay_controller_vtable;
	self->iface.impl = self;

	/* Validar configuración de ventana */
	Result_t res = TimeWindowValidator_ValidateConfig(&config->window_config);
	if (res != ERR_OK)
		return res;

	/* Inyectar Domain Service (STATEFUL) */
	self->cycle_scheduler = config->cycle_scheduler;

	/* Copiar configuración de ventana (STATELESS, datos puros) */
	self->window_config = config->window_config;

	/* BLOCKER-002: Máquina de estados explícita */
	self->fsm_state = RELAY_FSM_INIT;
	self->current_state = RELAY_CONTACT_OPEN;

	/* Inicializar tracking de Tr y timer OC */
	self->last_Tr_seconds = 0xFFFFFFFF; /* Valor inválido para forzar primera configuración */
	self->current_period_ms = 0;
	self->current_cycle_index = 0;
	self->timer_is_configured = false;
	self->oc_callback = NULL;
	self->oc_callback_context = NULL;

	/* Registrar callback Timer OC para cambios precisos de GPIO */
	Result_t timer_cb_res = Timer_RegisterOCCallback(self->timer, self->timer_handle, RelayAdapter_TimerOC_Callback, self);

	if (timer_cb_res != ERR_OK)
	{
		return timer_cb_res;
	}

	/* BLOCKER HIGH-002: Marcar como inicializado */
	self->is_initialized = true;

	return ERR_OK;
}

/**
 * @brief Procesa evento PPS - CRÍTICO: Ejecutar SOLO cuando llega señal PPS.
 *
 * RESPONSABILIDADES (Time-Critical Operations):
 * 1. Obtener Tr (tiempo relativo desde medianoche en segundos)
 * 2. SOLO reconfigurar timer OC si:
 *    - Tr cambió desde última vez Y
 *    - Tr es múltiplo exacto del periodo (Tr % T == 0) O Tr == 0
 * 3. Configurar Timer OC con precisión de 1μs:
 *    - OC1: 10μs + compensación
 *    - OC2: Ton/Toff + compensación
 *    - Period: T * 1000 μs
 *
 * NO RESPONSABLE DE (delegadas a Update()):
 * - ❌ Transiciones FSM (manejadas por Update())
 * - ❌ Validación ventana de tiempo (manejada por Update())
 * - ❌ Alarmas (manejadas por SetAlarmState())
 *
 * @param[in] self      Instancia del adaptador
 * @param[in] gps_time  Hora actual del GPS (opcional, puede ser NULL)
 *
 * @return ERR_OK si procesado correctamente
 * @note Ejecuta en contexto RelayAO thread (NO ISR)
 * @note Patrón similar: GPSAdapter_OnPPS() solo procesa NMEA, no gestiona FSM
 */
Result_t RelayAdapter_OnPPS(RelayAdapter_t *self, const DateTime_t *gps_time)
{
	if (self == NULL)
		return ERR_NULL_POINTER;

	/* BLOCKER HIGH-002: Validar inicialización */
	if (!self->is_initialized)
		return ERR_INVALID_STATE;

	DateTime_t now;
	Result_t time_result = TimeSource_GetTime(self->time_source, &now);
	bool is_time_synced = (time_result == ERR_OK);

	/* BLOCKER-002: Máquina de estados con transiciones explícitas */
	RelayFSM_State_t next_state = self->fsm_state;

	/* Máquina de estados basada en condiciones */
	switch (self->fsm_state)
	{
	case RELAY_FSM_INIT:
		/* INIT → WAIT_TIME_SYNC */
		next_state = RELAY_FSM_WAIT_TIME_SYNC;
		break;

	case RELAY_FSM_WAIT_TIME_SYNC:
		if (is_time_synced)
		{
			/* Tiempo disponible, ir a verificar ventana */
			next_state = RELAY_FSM_WAIT_WINDOW;
		}
		/* Si no está sincronizado, permanecer en WAIT_TIME_SYNC */
		break;

	case RELAY_FSM_WAIT_WINDOW:
	{
		bool in_window = false;
		TimeWindowValidator_IsActive(&self->window_config, &now, &in_window);

		if (!is_time_synced)
		{
			/* Pérdida de sincronización, volver a esperar */
			next_state = RELAY_FSM_WAIT_TIME_SYNC;
			RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
		}
		else if (in_window)
		{
			/* Dentro de ventana, activar ciclo */
			next_state = RELAY_FSM_ACTIVE_CYCLING;
		}
		/* Si no en ventana y sincronizado, permanecer en WAIT_WINDOW */
		break;
	}

	case RELAY_FSM_ACTIVE_CYCLING:
	{
		bool in_window = false;
		TimeWindowValidator_IsActive(&self->window_config, &now, &in_window);

		if (!is_time_synced)
		{
			/* Pérdida de sincronización, abortar ciclo y DETENER timer */
			next_state = RELAY_FSM_WAIT_TIME_SYNC;
			Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
			Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);
			self->timer_is_configured = false;
			RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
		}
		else if (!in_window)
		{
			/* Salió de ventana, parar ciclo y DETENER timer */
			next_state = RELAY_FSM_WAIT_WINDOW;
			Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
			Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);
			self->timer_is_configured = false;
			RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
		}
		else
		{
			/* tiempo relativo */
			uint32_t relative_sec = 0;

			TimeWindowValidator_GetRelativeTime(&self->window_config, &now, &relative_sec);

			/* Obtener configuración del ciclo actual */
			RelayConfig_t relay_config = {0};
			Result_t config_res = CycleScheduler_GetCurrentConfig(self->cycle_scheduler, &relay_config);
			if (config_res != ERR_OK)
			{
				break;
			}

			// reconfigure timer
			if (relative_sec != self->last_Tr_seconds && (relay_config.enabled == true))
			{
				uint32_t Ton_ms = 0;
				uint32_t Toff_ms = 0;
				if (relay_config.multicycle.enabled)
				{
					uint8_t cycle_index = 0;
					CycleScheduler_GetMulticycleIndex(self->cycle_scheduler, &cycle_index);
					Ton_ms = relay_config.multicycle.ton[cycle_index];
					Toff_ms = relay_config.multicycle.toff[cycle_index];
				}
				else
				{
					Ton_ms = relay_config.simple_cycle.ton;
					Toff_ms = relay_config.simple_cycle.toff;
				}

				uint32_t period_ms = Ton_ms + Toff_ms;

				if (period_ms > 0 && (((relative_sec * 1000) % period_ms == 0) || (relative_sec == 0)))
				{
					self->last_Tr_seconds = relative_sec;

					/* Detener timer si ya estaba corriendo */
					if (self->timer_is_configured)
					{
						Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
						Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);
						Timer_StopUpdate_IT(self->timer, self->timer_handle);
					}

					/* Configurar Timer OC con precisión de 1μs:
					 * Timer corre a 1MHz → cada tick = 1μs
					 * OC1: 10μs + compensación (primer evento)
					 * OC2: Ton/Toff_ms * 1000 + compensación (segundo evento)
					 * Period: T * 1000 - 1 (auto-reload) */

					uint32_t comp_on_to_off_us = relay_config.ton_margin_ms * 1000;
					uint32_t comp_off_to_on_us = relay_config.toff_margin_ms * 1000;

					uint32_t oc1_value, oc2_value;

					// configure period
					if (relay_config.start_with_on)
					{
						/* Inicia en ON:
						 * OC1 → Relé cierra (después de 10μs + comp)
						 * OC2 → Relé abre (después de Ton + comp) */
						oc1_value = 10 + comp_off_to_on_us;
						oc2_value = (Ton_ms * 1000) + comp_on_to_off_us;
					}
					else
					{
						/* Inicia en OFF:
						 * OC1 → Relé abre (después de 10μs + comp)
						 * OC2 → Relé cierra (después de Toff + comp) */
						oc1_value = 10 + comp_on_to_off_us;
						oc2_value = (Toff_ms * 1000) + comp_off_to_on_us;
						/* Set initial state for start with OFF */
						RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
					}

					/* Configurar comparadores */
					I_TIMER_OC_Config_t oc1_cfg = {
						.channel = I_TIMER_CHANNEL_1,
						.compare_value = oc1_value,
						.preload_enable = false};

					I_TIMER_OC_Config_t oc2_cfg = {
						.channel = I_TIMER_CHANNEL_2,
						.compare_value = oc2_value,
						.preload_enable = false};

					Timer_OC_ConfigChannel(self->timer, self->timer_handle, &oc1_cfg);
					Timer_OC_ConfigChannel(self->timer, self->timer_handle, &oc2_cfg);

					/* Configurar periodo (auto-reload) */
					uint32_t arr_value = (period_ms * 1000) - 1;
					Timer_SetAutoReload(self->timer, self->timer_handle, arr_value);

					/* Iniciar interrupciones OC */
					Timer_OC_Start_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
					Timer_OC_Start_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);

					Timer_StartUpdate_IT(self->timer, self->timer_handle);

					self->timer_is_configured = true;
					self->current_period_ms = period_ms;
				}
			}
		}
		/* Si tiempo sincronizado y en ventana, continuar en ACTIVE_CYCLING */
		break;
	}

	case RELAY_FSM_FORCED_OPEN:
		/* En estado forzado abierto, esperar a que se resuelva la alarma */
		RelayAdapter_SetContactState(self, RELAY_CONTACT_OPEN);
		/* El cambio de estado ocurre externamente vía RelayAdapter_SetHighTempAlarm */
		break;

	case RELAY_FSM_ERROR:
		/* En error, dejar relé abierto */
		RelayAdapter_SetContactState(self, RELAY_CONTACT_OPEN);
		break;

	default:
		return ERR_INVALID_PARAM;
	}

	/* Ejecutar transición de estado */
	if (next_state != self->fsm_state)
	{
		Result_t transition_result = RelayFSM_Transition(self, next_state);
		if (transition_result != ERR_OK)
			return transition_result;
	}

	return ERR_OK;
}

/**
 * @brief Actualización periódica - Gestión de FSM y validación de condiciones.
 *
 * RESPONSABILIDADES (State Management):
 * 1. Validar ventana de tiempo activa
 * 2. Validar días de semana activos
 * 3. Ejecutar transiciones FSM:
 *    - INIT → WAIT_TIME_SYNC (cuando inicializado)
 *    - WAIT_TIME_SYNC → WAIT_WINDOW (cuando GPS válido)
 *    - WAIT_WINDOW → ACTIVE_CYCLING (dentro de ventana)
 *    - ACTIVE_CYCLING → WAIT_WINDOW (fuera de ventana)
 *    - ANY_STATE → ERROR (en caso de fallo crítico)
 * 4. Verificar timeout de sincronización temporal
 *
 * NO RESPONSABLE DE (delegadas a otras funciones):
 * - ❌ Reconfiguración timer OC (manejada por OnPPS())
 * - ❌ Cálculo de Tr (manejado por OnPPS())
 * - ❌ Escritura directa GPIO (manejada por SetContactState())
 *
 * @param[in] self Instancia del adaptador
 * @return ERR_OK si actualización exitosa
 * @note Ejecuta en todos los eventos: PPS, TIMER, CONFIG, TIMEOUT, ALARM_CLEARED
 * @note Patrón similar: TimeAdapter_Update() solo gestiona FSM sync state
 */
Result_t RelayAdapter_Update(RelayAdapter_t *self)
{
	if (self == NULL)
		return ERR_NULL_POINTER;

	if (!self->is_initialized)
		return ERR_INVALID_STATE;

	/* ✅ FIX: Verificar pérdida de sincronización GPS y config.enabled */
	DateTime_t now;
	Result_t time_result = TimeSource_GetTime(self->time_source, &now);
	bool is_time_synced = (time_result == ERR_OK);

	/* Obtener configuración actual */
	RelayConfig_t config;
	Result_t config_res = CycleScheduler_GetCurrentConfig(self->cycle_scheduler, &config);
	if (config_res != ERR_OK)
		return config_res;

	/* ✅ FIX Problema 1: Si GPS se desconectó y relay está ciclando → DETENER */
	if (self->fsm_state == RELAY_FSM_ACTIVE_CYCLING)
	{
		if (!is_time_synced)
		{
			/* Pérdida de GPS sync → Transicionar a WAIT_TIME_SYNC */
			Result_t fsm_res = RelayFSM_Transition(self, RELAY_FSM_WAIT_TIME_SYNC);
			if (fsm_res != ERR_OK)
				return fsm_res;

			/* DETENER timer OC */
			if (self->timer_is_configured)
			{
				Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
				Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);
				Timer_StopUpdate_IT(self->timer, self->timer_handle);
				self->timer_is_configured = false;
			}

			/* Cerrar contacto */
			RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
		}
		/* ✅ FIX Problema 2: Si config.enabled cambió a false → DETENER */
		else if (!config.enabled)
		{
			/* Relay deshabilitado → Transicionar a WAIT_WINDOW */
			Result_t fsm_res = RelayFSM_Transition(self, RELAY_FSM_WAIT_WINDOW);
			if (fsm_res != ERR_OK)
				return fsm_res;

			/* DETENER timer OC */
			if (self->timer_is_configured)
			{
				Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_1);
				Timer_OC_Stop_IT(self->timer, self->timer_handle, I_TIMER_CHANNEL_2);
				Timer_StopUpdate_IT(self->timer, self->timer_handle);
				self->timer_is_configured = false;
			}

			/* Cerrar contacto */
			RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
		}
	}

	return ERR_OK;
}

/**
 * @brief Establece estado físico del contacto (privado).
 *
 * Lógica (ACTION-013 Refactored):
 * 1. Early return si estado sin cambios
 * 2. Obtener configuración actual (tipo contacto)
 * 3. ✅ Usar self->alarm_active (ya no hardcoded false)
 * 4. Si alarma activa → forzar OPEN (safety override)
 * 5. ✅ Mapeo simplificado: estado lógico → nivel GPIO (un solo switch)
 * 6. Escribir GPIO contacto (DB4)
 * 7. Actualizar estado interno
 * 8. Invocar callback si registrado
 *
 * @param self      Instancia del adaptador
 * @param new_state Nuevo estado (OPEN/CLOSED)
 *
 * @return ERR_OK si exitoso
 * @return ERR_INVALID_PARAM si new_state inválido
 *
 * @note CRITICAL PATH: Ejecutado en OnPPS y Update
 * @note SAFETY-CRITICAL: Alarma fuerza apertura independiente de new_state
 */
static Result_t RelayAdapter_SetContactState(void *_self, RelayContactState_t new_state)
{
	RelayAdapter_t *self = (RelayAdapter_t *)_self;
	/* 1. Early return si sin cambios */
	if (self->current_state == new_state)
		return ERR_OK;

	/* 2. Obtener configuración actual del ciclo */
	RelayConfig_t config;
	Result_t config_res = CycleScheduler_GetCurrentConfig(self->cycle_scheduler, &config);
	if (config_res != ERR_OK)
	{
		return config_res;
	}

	/* 3. ✅ Usar flag almacenado (ACTION-013: ya no hardcoded false) */
	bool alarm_high_temp = self->alarm_active;

	/* 4. Safety override: forzar apertura si alarma activa + escribir DB5 indicador */
	GPIO_State_t db5_state = alarm_high_temp ? I_GPIO_STATE_HIGH : I_GPIO_STATE_LOW;
	GPIO_WritePin(self->gpio, self->db5_gpio_port, self->db5_gpio_pin, db5_state);

	if (alarm_high_temp)
	{
		new_state = RELAY_CONTACT_OPEN;
	}

	/* 5. Calcular estado para DB4 (indicador de estado lógico) */
	GPIO_State_t db4_state = I_GPIO_STATE_LOW;
	if (!alarm_high_temp)
	{
		/* Solo actualizar DB4 si NO hay alarma (cuando hay alarma, DB5 indica el problema) */
		switch (config.contact_type)
		{
		case RELAY_TYPE_NC:
			db4_state = (new_state ^ 0x1) & 0x1; /* NC invierte lógica */
			break;
		case RELAY_TYPE_NO:
			db4_state = new_state & 0x1; /* NO directo */
			break;
		}
	}
	GPIO_WritePin(self->gpio, self->db4_gpio_port, self->db4_gpio_pin, db4_state);

	/* 6. Calcular estado para relay_gpio_pin (control físico del relé) */
	GPIO_State_t relay_state;
	switch (new_state)
	{
	case RELAY_CONTACT_CLOSED:
		/* NC cerrado=HIGH, NO cerrado=LOW */
		relay_state = (config.contact_type == RELAY_TYPE_NO) ? I_GPIO_STATE_LOW : I_GPIO_STATE_HIGH;
		break;

	case RELAY_CONTACT_OPEN:
		/* NC abierto=LOW, NO abierto=HIGH */
		relay_state = (config.contact_type == RELAY_TYPE_NO) ? I_GPIO_STATE_HIGH : I_GPIO_STATE_LOW;
		break;

	default:
		return ERR_INVALID_PARAM;
	}

	/* 7. Escribir GPIO contacto (ctrl pin) */
	Result_t gpio_result = GPIO_WritePin(self->gpio,
										 self->relay_gpio_port,
										 self->relay_gpio_pin,
										 relay_state);

	if (gpio_result != ERR_OK)
		return gpio_result;

	/* 7. Actualizar estado interno */
	RelayContactState_t old_state = self->current_state;
	self->current_state = new_state;
	self->last_state_change_time_ms = os_ticks_get();

	/* 8. Invocar callback si registrado */
	DateTime_t now;
	if (TimeSource_GetTime(self->time_source, &now) == ERR_OK)
	{
		self->last_state_change_datetime = now;
	}

	for (uint8_t i = 0; i < self->registered_callbacks; i++)
	{
		if (self->state_change_callback[i])
		{
			self->state_change_callback[i](self->state_change_context[i], old_state, new_state, os_ticks_get());
		}
	}

	return ERR_OK;
}

IRelayController *RelayAdapter_GetInterface(RelayAdapter_t *self)
{
	if (self == NULL)
		return NULL;
	return &self->iface;
}

static Result_t Relay_GetState(void *impl, RelayContactState_t *out_is_closed)
{
	RelayAdapter_t *self = (RelayAdapter_t *)impl;
	if (out_is_closed == NULL)
		return ERR_NULL_POINTER;
	*out_is_closed = (self->current_state == RELAY_CONTACT_CLOSED);
	return ERR_OK;
}

/**
 * @brief Obtiene el estado observable completo del relé.
 */
static Result_t Relay_GetStatus(void *impl, RelayStatus_t *out_status)
{
	RelayAdapter_t *self = (RelayAdapter_t *)impl;

	if (self == NULL || out_status == NULL)
		return ERR_NULL_POINTER;

	/* Obtener tiempo actual para evaluar sincronización y ventana */
	DateTime_t now;
	Result_t time_res = TimeSource_GetTime(self->time_source, &now);
	bool is_synced = (time_res == ERR_OK);

	/* Evaluar si está en ventana */
	bool in_window = false;

	if (is_synced)
	{
		TimeWindowValidator_IsActive(&self->window_config, &now, &in_window);
	}

	/* Map RelayFSM_State_t (6 values) → RelayInternalState_t (4 values) */
	RelayInternalState_t internal_state;
	switch (self->fsm_state)
	{
	case RELAY_FSM_INIT:
		internal_state = RELAY_STATE_IDLE;
		break;
	case RELAY_FSM_WAIT_TIME_SYNC:
	case RELAY_FSM_WAIT_WINDOW:
		internal_state = RELAY_STATE_WAITING;
		break;
	case RELAY_FSM_ACTIVE_CYCLING:
		internal_state = RELAY_STATE_ACTIVE;
		break;
	case RELAY_FSM_FORCED_OPEN:
		internal_state = RELAY_STATE_IDLE; /* alarm_active field signals the reason */
		break;
	case RELAY_FSM_ERROR:
	default:
		internal_state = RELAY_STATE_ERROR;
		break;
	}

	/* Llenar estructura de estado */
	out_status->contact_state = self->current_state;
	out_status->internal_state = internal_state;
	out_status->is_synchronized = is_synced;
	out_status->is_in_window = in_window;
	out_status->alarm_active = self->alarm_active; /* ✅ Exponer estado alarma overtemp (RELAY_EVENT_ALARM_OVERTEMP) */
	out_status->uptime_ms = self->total_on_time_ms;
	out_status->last_state_change = self->last_state_change_datetime;
	out_status->next_transition_time = 0; /* TODO: Calcular tiempo siguiente transición */

	return ERR_OK;
}

/**
 * @brief Actualiza la configuración del relé en runtime.
 */
static Result_t Relay_UpdateConfig(void *impl, const RelayConfig_t *config)
{
	RelayAdapter_t *self = (RelayAdapter_t *)impl;

	if (self == NULL || config == NULL)
		return ERR_NULL_POINTER;

	/* Validar nueva configuración */
	Result_t val_res = TimeWindowValidator_ValidateConfig(&config->time_window);
	if (val_res != ERR_OK)
		return val_res;

	/* Actualizar configuración de ventana */
	self->window_config = config->time_window;

	/* Inyectar configuración en CycleScheduler si es necesario */
	if (self->cycle_scheduler != NULL)
	{
		Result_t sched_res = CycleScheduler_UpdateConfig(self->cycle_scheduler, config);
		if (sched_res != ERR_OK)
			return sched_res;

		/*apply config immediately if in active cycling */
		if (self->fsm_state != RELAY_FSM_ACTIVE_CYCLING)
		{
			self->current_state = !self->current_state; /* Force state change on update */
		}
	}

	return ERR_OK;
}

static Result_t Relay_RegisterStateChangeCallback(void *impl, RelayStateChangeCallback_t cb, void *context)
{
	RelayAdapter_t *self = (RelayAdapter_t *)impl;
	if (self == NULL)
		return ERR_NULL_POINTER;

	if (self->registered_callbacks >= RELAY_ADAPTER_MAX_CALLBACKS)
		return ERR_NO_MEMORY; /* Límite de callbacks alcanzado */

	self->state_change_callback[self->registered_callbacks] = cb;
	self->state_change_context[self->registered_callbacks] = context;

	self->registered_callbacks++;

	return ERR_OK;
}

/**
 * @brief Desregistra callback de cambio de estado.
 */
static Result_t Relay_UnregisterStateChangeCallback(void *impl)
{
	RelayAdapter_t *self = (RelayAdapter_t *)impl;

	// Siempre se puede desregistrar, incluso si no hay callback}
	for (uint8_t i = 0; i < self->registered_callbacks; i++)
	{
		self->state_change_callback[i] = NULL;
		self->state_change_context[i] = NULL;
	}
	return ERR_OK;
}

/* ===== NEW: Active Object Integration V-Table Wrappers ===== */

/**
 * @brief IRelayController.OnPPS implementation.
 * @note Wrapper that forwards to existing RelayAdapter_OnPPS() function.
 *
 * This enables RelayAO to call via interface without knowing concrete type.
 */
static Result_t Relay_OnPPS(void *impl, const DateTime_t *gps_time)
{
	return RelayAdapter_OnPPS((RelayAdapter_t *)impl, gps_time);
}

/**
 * @brief IRelayController.Update implementation.
 * @note Wrapper that forwards to existing RelayAdapter_Update() function.
 *
 * This enables RelayAO to call via interface without knowing concrete type.
 */
static Result_t Relay_Update(void *impl)
{
	return RelayAdapter_Update((RelayAdapter_t *)impl);
}

/**
 * @brief IRelayController.SetAlarmState implementation (vtable wrapper).
 * @note Wrapper that forwards to RelayAdapter_SetAlarmState().
 *
 * This enables RelayAO to call via interface without downcast.
 *
 * @param[in] impl          Implementación (RelayAdapter_t*)
 * @param[in] alarm_active  true si alarma activa, false si liberada
 * @return Result from RelayAdapter_SetAlarmState()
 */
static Result_t Relay_SetAlarmState(void *impl, bool alarm_active)
{
	return RelayAdapter_SetAlarmState((RelayAdapter_t *)impl, alarm_active);
}

/**
 * @brief Actualiza estado de alarma del relay (thread-safe) - PUBLIC API.
 *
 * Implementación:
 * 1. Validar parámetros (NULL, inicialización)
 * 2. Actualizar flag interno alarm_active
 * 3. Si alarma activa:
 *    - Transicionar FSM a RELAY_FSM_FORCED_OPEN
 *    - Forzar apertura inmediata de contacto (safety-critical)
 * 4. Si alarma liberada:
 *    - Transicionar FSM a RELAY_FSM_WAIT_TIME_SYNC
 *    - Dejar que Update() gestione reanudación de ciclo normal
 *
 * @param[in] self          Instancia del adaptador
 * @param[in] alarm_active  true si alarma detectada, false si liberada
 *
 * @return ERR_OK si exitoso
 * @return ERR_NULL_POINTER si self es NULL
 * @return ERR_INVALID_STATE si no inicializado o transición FSM inválida
 *
 * @note SAFETY-CRITICAL: Alarma activa → apertura contacto en <100ms
 * @note Thread-safe: ejecuta en contexto RelayAO thread (no ISR)
 * @note Pattern: Similar a TimeAdapter_OnPPS() alarm handling (ACTION-007)
 */
Result_t RelayAdapter_SetAlarmState(RelayAdapter_t *self, bool alarm_active)
{

	/* 1. Validación de parámetros */
	if (self == NULL)
		return ERR_NULL_POINTER;

	if (!self->is_initialized)
		return ERR_INVALID_STATE;

	/* ✅ IDEMPOTENCIA: Si ya está en el estado solicitado, retornar OK inmediatamente
	 * Rationale:
	 * - Alarmas múltiples/repetidas no causan transiciones innecesarias
	 * - Application Layer (RelayAO) no necesita verificar estado antes de llamar
	 * - Reduce acoplamiento: RelayAO solo enruta eventos, no tiene lógica de dominio */
	if (self->alarm_active == alarm_active)
	{
		return ERR_OK; /* Ya en el estado correcto, nada que hacer */
	}

	/* 2. Actualizar flag de alarma */
	self->alarm_active = alarm_active;

	if (alarm_active)
	{
		/* 3. Alarma activa: forzar apertura inmediata */

		/* Transicionar FSM a FORCED_OPEN */
		Result_t fsm_res = RelayFSM_Transition(self, RELAY_FSM_FORCED_OPEN);
		if (fsm_res != ERR_OK)
			return fsm_res;

		/* Forzar apertura de contacto (safety-critical path) */
		return RelayAdapter_SetContactState(self, RELAY_CONTACT_OPEN);
	}
	else
	{
		/* 4. Alarma liberada: retornar a operación normal */

		/* Transicionar FSM a WAIT_TIME_SYNC para reanudar desde inicio */
		Result_t fsm_res = RelayFSM_Transition(self, RELAY_FSM_WAIT_TIME_SYNC);
		if (fsm_res != ERR_OK)
			return fsm_res;

		/* ✅ Cerrar contacto inmediatamente (reanudación rápida sin esperar PPS)
		 * Rationale:
		 * - Evita latencia de hasta 1s (esperar próximo PPS)
		 * - Comportamiento simétrico con alarma activa (apertura inmediata)
		 * - OnPPS() gestionará transiciones posteriores (WAIT_SYNC → WAIT_WIN → CYCLING)
		 */
		return RelayAdapter_SetContactState(self, RELAY_CONTACT_CLOSED);
	}
}

Result_t RelayAdapter_Deinit(RelayAdapter_t *self)
{
	if (self == NULL)
		return ERR_NULL_POINTER;
	if (!self->is_initialized)
		return ERR_OK; /* Idempotente */

	/* No hay recursos dinámicos que liberar (no mutex, no timer propio) */
	/* Solo resetear estado */
	self->is_initialized = false;
	return ERR_OK;
}
