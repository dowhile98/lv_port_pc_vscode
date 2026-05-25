/*
 * general_types.h
 *
 *  Created on: Feb 10, 2026
 *      Author: tecna-smart-lab
 */

#ifndef COMMON_GENERAL_TYPES_H_
#define COMMON_GENERAL_TYPES_H_

/* C/C++ portable static assertion: _Static_assert is C11, static_assert is C++ */
#if defined(__cplusplus) && !defined(_Static_assert)
#define _Static_assert static_assert
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Configuración general del sistema.
	 */
	typedef struct __attribute__((packed)) GeneralConfig
	{
		uint32_t buzzer_on_time_ms;			   /**< Duración en ms para estado ON del buzzer. */
		uint8_t buzzer_high_temp_alarm;		   /**< Bandera si se va generar el sonido del buzzer o no en caso de alta temperatura */
		uint32_t screen_blacklight_timeout_ms; /**< Tiempo en ms para apagar backlight de pantalla por inactividad. */
	} GeneralConfig_t;

	_Static_assert(sizeof(GeneralConfig_t) == 9, "GeneralConfig_t must be 9 bytes (4+1+4). Update reserved[] in GeneralConfigStorage_t if you change this.");

#ifdef __cplusplus
}
#endif

#endif /* COMMON_GENERAL_TYPES_H_ */
