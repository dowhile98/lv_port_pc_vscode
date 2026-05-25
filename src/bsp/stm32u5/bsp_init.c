/**
 * @file bsp_init.c
 * @brief Implementación del inicializador del BSP STM32U5
 */

#include "bsp_init.h"
#include "bsp_stm32u5_gpio.h"
#include "bsp_stm32u5_uart.h"
#include "bsp_stm32u5_timer.h"
#include "bsp_stm32u5_spi.h"
#include "bsp_stm32u5_i2c.h"
#include "bsp_stm32u5_exti.h"
#include "bsp_stm32u5_octospi.h"
#include "bsp_stm32u5_rtc.h"
#include "bsp_stm32u5_adc_temp.h"
#include "infrastructure/adapters/crypto/cyclone_crypto_verifier_adapter.h"
#include "infrastructure/adapters/cyclone_boot_ops_adapter.h"
#include "infrastructure/adapters/cyclone_boot_ext_flash_adapter.h"
#include "Core/Inc/res_sign_pub_key.h" /* pemResSignPublicKey[] */
#include <stdbool.h>
/* Includes de inicialización de hardware (CubeMX) */
#include "gpio.h"
#include "usart.h"
#include "tim.h"
#include "spi.h"
#include "i2c.h"
#include "octospi.h"
#include "rtc.h"
#include "adc.h"

/* Contenedor estático de interfaces */
static BSP_Interfaces_t s_bsp_interfaces = {0};

/* Adapters estáticos para OTA (CycloneCRYPTO + CycloneBOOT) */
static CycloneCryptoVerifierAdapter_t s_crypto_verifier_adapter;
static CycloneBootOpsAdapter_t s_cyclone_boot_ops_adapter;

Result_t BSP_Init(void)
{
    if (s_bsp_interfaces.is_initialized)
    {
        return ERR_OK; /* Ya inicializado */
    }

    const BoardProfile_t *board = BSP_GetBoardProfile();
    /* 1. Inicializar hardware de bajo nivel (generado por CubeMX) */
    MX_USART2_UART_Init();

    if (board->display.enabled == true)
    {
        MX_SPI1_Init();
    }

    if (board->battery.enabled == true)
    {
        MX_I2C2_Init();
    }
    MX_I2C1_Init();
    MX_OCTOSPI1_Init();
    MX_RTC_Init();
    MX_TIM2_Init();
    MX_SPI3_Init();
    MX_ADC1_Init();
    (void)BspAdcTemp_Init(&hadc1); /* Temperature sensor: calibrate + configure TEMPSENSOR channel */


#if 0
    MX_USART1_UART_Init();
    MX_TIM6_Init();
    MX_TIM7_Init();

#endif
    /* 2. Obtener interfaces de los drivers BSP (singleton pattern) */
    s_bsp_interfaces.gpio = Bsp_Stm32U5_Gpio_GetInterface();
    s_bsp_interfaces.uart = Bsp_Stm32U5_Uart_GetInterface();
    s_bsp_interfaces.timer = Bsp_Stm32U5_Timer_GetInterface();
    s_bsp_interfaces.spi = Bsp_Stm32U5_Spi_GetInterface();
    s_bsp_interfaces.i2c = Bsp_Stm32U5_I2c_GetInterface();
    s_bsp_interfaces.exti = Bsp_Stm32U5_Exti_GetInterface();
    s_bsp_interfaces.ext_flash = Bsp_Stm32U5_Octospi_GetInterface();
    s_bsp_interfaces.rtc = Bsp_Stm32U5_Rtc_GetInterface();
    s_bsp_interfaces.temperature_sensor = BspAdcTemp_GetInterface(); /* NULL if ADC init failed */

    GPIO_WritePin(s_bsp_interfaces.gpio , GPIO_RESET_GPIO_Port, GPIO_RESET_Pin, 0);
    /* 2b. Inicializar adapters OTA (CycloneCRYPTO + CycloneBOOT) */
    /* Logger no disponible aún en BSP init → pasar NULL */
    /* CRITICAL: Use RESOURCES public key for ExternalLoaderStrategy (resources.img verification) */
    Result_t crypto_res = CycloneCryptoVerifierAdapter_Init(&s_crypto_verifier_adapter,
                                                             NULL,  /* logger - set later */
                                                             pemResSignPublicKey,
                                                             pemResSignPublicKey_len);
    if (crypto_res != ERR_OK)
    {
        return crypto_res;
    }
    s_bsp_interfaces.i_crypto_verifier = CycloneCryptoVerifierAdapter_GetInterface(&s_crypto_verifier_adapter);

    /* 2c. Bind external flash interface al CycloneBOOT ext flash adapter */
    /* CRITICAL: Debe hacerse ANTES de CycloneBootOpsAdapter_Init() */
    void *octospi_handle = BSP_GetOctoSPIHandle(0);
    Result_t bind_res = CycloneBootExtFlashAdapter_BindInterface(s_bsp_interfaces.ext_flash, octospi_handle);
    if (bind_res != ERR_OK)
    {
        return bind_res;
    }
    /* Logger se inyectará más tarde en DI_InitUpdateSubsystem() */

    Result_t cboot_res = CycloneBootOpsAdapter_Init(&s_cyclone_boot_ops_adapter);
    if (cboot_res != ERR_OK)
    {
        return cboot_res;
    }
    s_bsp_interfaces.i_cyclone_boot_ops = CycloneBootOpsAdapter_GetInterface(&s_cyclone_boot_ops_adapter);

    /* 3. Validar que todas las interfaces son válidas */
    if (!s_bsp_interfaces.gpio ||
        !s_bsp_interfaces.uart ||
        !s_bsp_interfaces.timer ||
        !s_bsp_interfaces.spi ||
        !s_bsp_interfaces.i2c ||
        !s_bsp_interfaces.exti ||
        !s_bsp_interfaces.ext_flash ||
        !s_bsp_interfaces.rtc)
    {
        return ERR_ERROR; /* Alguna interfaz falló */
    }

    s_bsp_interfaces.is_initialized = true;
    return ERR_OK;
}

const BSP_Interfaces_t *BSP_GetInterfaces(void)
{
    return &s_bsp_interfaces;
}

bool BSP_IsInitialized(void)
{
    return s_bsp_interfaces.is_initialized;
}

/*============================================================================*
 * PERIPHERAL HANDLE LOOKUP IMPLEMENTATION
 *============================================================================*/

/**
 * @brief Obtiene handle UART por índice.
 * @note Array estático con todos los UART disponibles en este hardware.
 * @note Si se agregan más UARTs en CubeMX, actualizar este array.
 * @note Retorna void* para mantener handles opacos (cast en adapters)
 */
void *BSP_GetUARTHandle(uint8_t index)
{
    /* Tabla estática de handles UART disponibles */
    static void *const uart_map[] = {
        &huart2, /* Index 0: UART2 (GPS en CICX1-V5) */
        /* Agregar más UARTs aquí cuando se habiliten en CubeMX:
         * &huart1,  // Index 1: UART1 (Debug)
         * &huart3,  // Index 2: UART3 (WiFi)
         */
    };

    if (index >= (sizeof(uart_map) / sizeof(uart_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return uart_map[index];
}

/**
 * @brief Obtiene handle I2C por índice.
 * @note Array estático con todos los I2C disponibles en este hardware.
 * @note Retorna void* para mantener handles opacos
 */
void *BSP_GetI2CHandle(uint8_t index)
{
    /* Tabla estática de handles I2C disponibles */
    static void *const i2c_map[] = {
        &hi2c1, /* Index 0: I2C1 (EEPROM M24M01E en CICX1-V5) */
        &hi2c2,
        /* Agregar más I2Cs aquí cuando se habiliten en CubeMX:
         * &hi2c2,  // Index 1: I2C2 (Sensores)
         * &hi2c3,  // Index 2: I2C3 (Display)
         */
    };

    if (index >= (sizeof(i2c_map) / sizeof(i2c_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return i2c_map[index];
}

/**
 * @brief Obtiene handle SPI por índice.
 * @note Array estático con todos los SPI disponibles en este hardware.
 * @note Retorna void* para mantener handles opacos
 */
void *BSP_GetSPIHandle(uint8_t index)
{
    /* Tabla estática de handles SPI disponibles */
    static void *const spi_map[] = {
        &hspi1, /* Index 0: SPI1 (Display ST7789 en CICX1-V5) */
        NULL,
        &hspi3, /* Index 1: SPI3 (Reservado) */
        /* Agregar más SPIs aquí cuando se habiliten en CubeMX:
         * &hspi2,  // Index 2: SPI2 (WiFi)
         */
    };

    if (index >= (sizeof(spi_map) / sizeof(spi_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return spi_map[index];
}

/**
 * @brief Obtiene handle Timer por índice.
 * @note Array estático con todos los Timers disponibles en este hardware.
 * @note Retorna void* para mantener handles opacos
 */
void *BSP_GetTimerHandle(uint8_t index)
{
    /* Tabla estática de handles Timer disponibles */
    static void *const timer_map[] = {
        &htim2, /* Index 0: TIM2 (Relay cycle timing en CICX1-V5) */
        /* Agregar más Timers aquí cuando se habiliten en CubeMX:
         * &htim6,  // Index 1: TIM6 (General purpose)
         * &htim7,  // Index 2: TIM7 (General purpose)
         */
    };

    if (index >= (sizeof(timer_map) / sizeof(timer_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return timer_map[index];
}

/**
 * @brief Obtiene handle RTC por índice.
 * @note Solo hay un RTC en STM32, pero mantenemos índice por consistencia.
 * @note Retorna void* para mantener handles opacos
 */
void *BSP_GetRTCHandle(uint8_t index)
{
    /* Tabla estática de handles RTC disponibles (usualmente solo 1) */
    static void *const rtc_map[] = {
        &hrtc, /* Index 0: RTC (único RTC del sistema) */
    };

    if (index >= (sizeof(rtc_map) / sizeof(rtc_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return rtc_map[index];
}

/**
 * @brief Obtiene handle OctoSPI por índice.
 * @note OctoSPI se usa para external flash de alta velocidad (MX25LM51245G).
 * @note Retorna void* para mantener handles opacos
 */
void *BSP_GetOctoSPIHandle(uint8_t index)
{
    /* Tabla estática de handles OctoSPI disponibles */
    static void *const octospi_map[] = {
        &hospi1, /* Index 0: OCTOSPI1 (External Flash MX25LM51245G en CICX1-V5) */
        /* Agregar más OctoSPI aquí cuando se habiliten en CubeMX:
         * &hospi2,  // Index 1: OCTOSPI2 (si existe en el MCU)
         */
    };

    if (index >= (sizeof(octospi_map) / sizeof(octospi_map[0])))
    {
        return NULL; /* Índice fuera de rango */
    }

    return octospi_map[index];
}
