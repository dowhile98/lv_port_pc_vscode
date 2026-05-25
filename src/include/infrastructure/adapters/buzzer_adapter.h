/**
 * @file buzzer_adapter.h
 * @brief Buzzer Adapter - Implementación de IBuzzerControl sobre GPIO
 * @version 2.0.0
 * @date 2026-02-10
 *
 * @details
 * Implementación concreta de IBuzzerControl que utiliza un GPIO simple
 * (on/off) para control de buzzer/beeper. En el futuro puede extenderse
 * para usar PWM para tonos variables.
 *
 * **Arquitectura:**
 * - Usa I_GPIO (HAL interface) para control del pin
 * - Usa polling interno para auto-apagado (sin timer RTOS)
 * - NO accede directamente a STM32 HAL (respeta DIP)
 *
 * **Responsabilidades:**
 * - Traducir comandos de alto nivel (Beep/Stop) a operaciones GPIO
 * - Calcular tick objetivo para auto-apagado (comparación en Update())
 * - Mantener estado de habilitación (mute global)
 *
 * **Polling:**
 * - BuzzerAdapter_Update() debe llamarse periódicamente (~20ms)
 * - BuzzerAO lo ejecuta automáticamente en su loop interno
 * - Usuario NO necesita llamarlo manualmente
 *
 * **Testing:**
 * - Unit tests usan MockGPIO (BSP mock)
 * - Integration tests usan hardware real (BSP STM32)
 *
 * @note Thread-safe: usa mutex interno para proteger estado
 * @note ISR-safe: algunas operaciones permiten llamadas desde ISR
 *
 * @see IBuzzerControl
 * @see I_GPIO
 * @see BuzzerAO
 */

#ifndef BUZZER_ADAPTER_H
#define BUZZER_ADAPTER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "interfaces/i_buzzer_control.h"
#include "interfaces/i_gpio.h"
#include "hal_types.h"
#include "infrastructure/osal/osal.h" /* os_mutex_t */

/*============================================================================*
 * CONSTANTS
 *============================================================================*/

/**
 * @brief Duración mínima de beep (protección anti-ruido)
 * @note Beeps <10ms son ignorados para evitar clicks en el buzzer
 */
#define BUZZER_ADAPTER_MIN_BEEP_MS 10U

/**
 * @brief Duración máxima de beep (protección anti-hang)
 * @note Límite de seguridad para evitar beep continuo infinito
 */
#define BUZZER_ADAPTER_MAX_BEEP_MS 30000U // 30 segundos

    /*============================================================================*
     * TYPES
     *============================================================================*/

    /**
     * @brief Configuración de inicialización del BuzzerAdapter
     */
    typedef struct
    {
        const I_GPIO *gpio_iface; /**< Interfaz GPIO (inyección de dependencia) */
        uint16_t gpio_pin;        /**< Pin GPIO del buzzer */
        GPIO_Port_t gpio_port;    /**< Puerto GPIO del buzzer */

        bool active_high;     /**< true=buzzer activo en HIGH, false=activo en LOW */
        bool default_enabled; /**< Estado inicial de habilitación (true=enabled) */
    } BuzzerAdapterConfig_t;

    /**
     * @brief Estructura de implementación del BuzzerAdapter
     * @note Definición completa movida a header para permitir embedding en DependencyContainer
     */
    typedef struct BuzzerAdapter
    {
        /* Dependencias inyectadas */
        const I_GPIO *gpio_iface;
        GPIO_Port_t gpio_port;
        GPIO_Pin_t gpio_pin;

        /* Configuración */
        bool active_high;
        bool is_enabled;

        /* Estado actual */
        bool is_active;
        bool is_initialized;

        /* Sincronización */
        os_mutex_t mutex;

        /* Auto-stop por polling (sin timer RTOS) */
        uint32_t stop_tick; /**< Tick OSAL en el que debe detenerse (0 = sin auto-stop) */

        /* Interfaz pública */
        IBuzzerControl interface;
    } BuzzerAdapter_t;

    /*============================================================================*
     * PUBLIC API
     *============================================================================*/

    /**
     * @brief Inicializa el BuzzerAdapter
     *
     * @param[in] adapter Puntero a estructura BuzzerAdapter (debe estar pre-alocada)
     * @param[in] config Configuración de inicialización (no NULL)
     *
     * @return ERR_OK si exitoso
     * @return ERR_NULL_POINTER si adapter o config son NULL
     * @return ERR_INVALID_PARAM si gpio_iface es NULL
     *
     * @note Debe llamarse antes de usar el adapter
     * @note Buzzer inicia en estado SILENT
     * @note Thread-safe: puede llamarse una sola vez
     *
     * @pre config->gpio_iface != NULL
     * @post Buzzer configurado y listo para uso
     *
     * @code
     * BuzzerAdapter_t buzzer_adapter;
     * BuzzerAdapterConfig_t config = {
     *     .gpio_iface = &bsp_gpio_interface,
     *     .gpio_pin = 5,
     *     .gpio_port = GPIO_PORT_C,
     *     .active_high = true,
     *     .default_enabled = true
     * };
     * Result_t res = BuzzerAdapter_Init(&buzzer_adapter, &config);
     * @endcode
     */
    Result_t BuzzerAdapter_Init(BuzzerAdapter_t *adapter, const BuzzerAdapterConfig_t *config);

    /**
     * @brief Obtiene interfaz IBuzzerControl del adapter
     *
     * @param[in] adapter Puntero a BuzzerAdapter inicializado
     * @return Puntero a interfaz IBuzzerControl, o NULL si no inicializado
     *
     * @note La interfaz retornada es válida mientras viva el adapter
     * @note Uso típico: inyectar en Active Objects o Use Cases
     *
     * @code
     * IBuzzerControl* buzzer = BuzzerAdapter_GetInterface(&buzzer_adapter);
     * BuzzerControl_Beep(buzzer, BUZZER_TONE_STANDARD, 500);
     * @endcode
     */
    IBuzzerControl *BuzzerAdapter_GetInterface(BuzzerAdapter_t *adapter);

    /**
     * @brief De-inicializa el adapter y libera recursos
     *
     * @param[in] adapter Puntero a BuzzerAdapter
     * @return ERR_OK si exitoso
     *
     * @note Detiene cualquier beep en progreso
     * @note Debe llamarse en shutdown del sistema
     */
    Result_t BuzzerAdapter_Deinit(BuzzerAdapter_t *adapter);

    /**
     * @brief Actualiza el estado del buzzer (maneja auto-stop por polling)
     *
     * @param[in] adapter Puntero a BuzzerAdapter
     * @return ERR_OK si exitoso
     *
     * @note DEBE llamarse periódicamente (cada ~10-50ms) desde un thread
     * @note Maneja el auto-stop del buzzer cuando se cumple duration_ms
     * @note Thread-safe
     *
     * @code
     * // En tu main loop o control thread:
     * while (1) {
     *     BuzzerAdapter_Update(&buzzer_adapter);
     *     os_thread_sleep(20); // 20ms
     * }
     * @endcode
     */
    Result_t BuzzerAdapter_Update(BuzzerAdapter_t *adapter);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_ADAPTER_H */
