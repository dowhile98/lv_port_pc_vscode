/**
 * @file bsp_board_profiles.c
 * @brief Implementación de perfiles de hardware para variantes CICX1
 * @version 1.0.0
 * @date 2026-02-11
 *
 * @note ÚNICO archivo en el sistema que usa #ifdef para selección de variante.
 *       Todo el resto del código es agnóstico a la variante de hardware.
 *
 * @author Tecna Smart Lab
 */

/*============================================================================*
 * INCLUDES
 *============================================================================*/
#include "bsp_board_profile.h"
#include "main.h" /* Para defines de CubeMX: DB0_Pin, SSR_CTRL_Pin, etc. */

/*============================================================================*
 * PROFILE: CICX1-V5 (Full-Featured)
 *============================================================================*/

/**
 * @brief Variante V5: Full-featured
 *
 * Hardware:
 * - Display: ST7789 240x320 TFT SPI
 * - EEPROM: M24M01E 128KB I2C (1Mbit, 256-byte page)
 * - Botones: DB0 (UP), DB1 (ENTER), DB2 (DOWN), PBOUT (ON/OFF)
 * - Control Relé: SY_TXD (GPIOD Pin6)
 * - Alarma: SY_RXD (GPIOD Pin7)
 * - LEDs: LED0, DB4, DB5, SIDE_LED
 * - Batería: Monitoreo habilitado (futuro)
 *
 * @note Este es el profile de la placa original. El comportamiento debe ser
 *       idéntico al código pre-refactor cuando se compila con BOARD_VARIANT_V5.
 */
const BoardProfile_t BOARD_PROFILE_CICX1_V5 = {
    .variant_name = "CICX1-V5-FULL",

    /* ===== Buttons Configuration ===== */
    .buttons = {
        [BUTTON_ONOFF] = {
            .port = PBOUT_GPIO_Port,
            .pin = PBOUT_Pin,
            .active_low = true,
            .long_press_ms = 1500,
            .enabled = true},
        [BUTTON_ENTER] = {.port = DB1_GPIO_Port, .pin = DB1_Pin, .active_low = true, .long_press_ms = 1500, .enabled = true},
        [BUTTON_UP] = {.port = DB0_GPIO_Port, .pin = DB0_Pin, .active_low = true, .long_press_ms = 800, .enabled = true},
        [BUTTON_DOWN] = {.port = DB2_GPIO_Port, .pin = DB2_Pin, .active_low = true, .long_press_ms = 800, .enabled = true},
        [BUTTON_SIDE] = {
            .port = 0, .pin = 0, .active_low = false, .long_press_ms = 0, .enabled = false /* ✅ No existe en V5 */
        }},

    /* ===== Relay Control ===== */
    .relay_control = {.port = SY_TXD_GPIO_Port, .pin = SY_TXD_Pin, .enabled = true},

	.rs485 = {.rs485_rx_en_port = SY_RX_EN_GPIO_Port, .rs485_tx_en_port = SY_TX_EN_GPIO_Port, .rs485_rx_en_pin = SY_RX_EN_Pin, .rs485_tx_en_pin = SY_TX_EN_Pin, .enabled = true},
    /* ===== Alarm Input ===== */
    .alarm_input = {.port = SY_RXD_GPIO_Port, .pin = SY_RXD_Pin, .active_low = true, .long_press_ms = 0, .enabled = false},

    /* ===== Status LEDs ===== */
    .led_status = {[0] = {.port = LED0_GPIO_Port, .pin = LED0_Pin, .enabled = true}, [1] = {.port = DB4_GPIO_Port, .pin = DB4_Pin, .enabled = true}, [2] = {.port = DB5_GPIO_Port, .pin = DB5_Pin, .enabled = true}},

    /* ===== Side LED (General Indicator) ===== */
    .side_led = {.port = SIDE_LED_GPIO_Port, .pin = SIDE_LED_Pin, .enabled = true},

    /* ===== Display ===== */
    .display = {
        .enabled = true,
        .width = 320,                    /* Dimensión lógica X tras rotación 270° (LANDSCAPE_INVERTED) */
        .height = 240,                   /* Dimensión lógica Y tras rotación 270° (LANDSCAPE_INVERTED) */
        .driver = DISPLAY_DRIVER_ST7789, /* ← aquí se selecciona el driver */
        /* Pines CubeMX (Core/Inc/main.h) */
        .rst_port = LCD_RESET_GPIO_Port,
        .rst_pin = LCD_RESET_Pin, /* GPIOB / PIN_12 */
        .dc_port = LCD_DC_GPIO_Port,
        .dc_pin = LCD_DC_Pin, /* GPIOE / PIN_14 */
        .cs_port = LCD_CS_GPIO_Port,
        .cs_pin = LCD_CS_Pin, /* GPIOB / PIN_10 */
        .bl_port = LCD_LED_GPIO_Port,
        .bl_pin = LCD_LED_Pin, /* GPIOE / PIN_0  */
    },

    /* ===== EEPROM ===== */
    .eeprom = {
        .enabled = true, .driver = EEPROM_DRIVER_M24M01E, /* ✅ M24M01E 128KB */
        .size_bytes = 131072,                             /* 128KB */
        .page_size = 256,                                 /* 256-byte page writes */
        .i2c_address = 0xA0                               /* I2C address (7-bit: 0x50, shifted: 0xA0) */
    },

    /* ===== Battery Monitoring (Future) ===== */
    .battery = {.enabled = true},

    /* ===== ESP32 WiFi Co-Processor ===== */
    .esp32_wifi = {
        .enabled = true, .reset_port = GPIO_RESET_GPIO_Port, .reset_pin = GPIO_RESET_Pin, .handshake_port = HANDSHAKE_GPIO_Port, .handshake_pin = HANDSHAKE_Pin, .data_ready_port = GPIO_DATA_READY_GPIO_Port, .data_ready_pin = GPIO_DATA_READY_Pin, .cs_port = SPI_CS_GPIO_Port, .cs_pin = SPI_CS_Pin, .cs_mode = ESP_CS_MODE_AUTO, /* ✅ Modo automático (controlado por hardware SPI) */
        .handshake_exti_line = 8,
        .data_ready_exti_line = 7 /* Reusar EXTI7 para data ready, aunque no se usará en esta variante */
    },

    /* ===== USB RNDIS ===== */
    .usb_rndis = {.enabled = false /* ✅ USB RNDIS gateway habilitado en V5 */},

    /* ===== Additional GPIO Pins ===== */
    .buzzer = {.port = BUZZER_GPIO_Port, .pin = BUZZER_Pin, .enabled = true},
    .gps_reset = {.port = GPS_RST_GPIO_Port, .pin = GPS_RST_Pin, .enabled = false},
    .gps_pps = {.port = GPS_PPS_GPIO_Port, .pin = GPS_PPS_Pin, .active_low = false, .long_press_ms = 0, .enabled = true},
    .rf_ctrl1 = {.port = RF_CTRL1_GPIO_Port, .pin = RF_CTRL1_Pin, .enabled = true},
    .rf_ctrl2 = {.port = RF_CTRL2_GPIO_Port, .pin = RF_CTRL2_Pin, .enabled = true},
    /* ===== Peripheral Handle Mapping ===== */
    .peripheral_map = {
        .gps_uart_index = 0,    /* Index 0 = huart2 (ver bsp_init.c uart_map) */
        .debug_uart_index = 0,  /* Index 0 = huart2 (compartido con GPS, separar cuando huart1 esté habilitado) */
        .wifi_spi_index = 2,    /* Index 0 = hspi3 reservado */
        .eeprom_i2c_index = 0,  /* Index 0 = hi2c1 (M24M01E) */
		.battery_i2c_index = 1,
        .display_spi_index = 0, /* Index 0 = hspi1 (ST7789) */
        .relay_timer_index = 0, /* Index 0 = htim2 (Relay cycle timing) */
        .rtc_index = 0,         /* Index 0 = hrtc (único RTC) */
        .octospi_index = 0      /* Index 0 = hospi1 (External Flash MX25LM51245G 512Mbit) */
    }};

/*============================================================================*
 * PROFILE: SSR3-V1 (Minimal)
 *============================================================================*/

/**
 * @brief Variante SSR3-V1: Minimal (sin display)
 *
 * Hardware:
 * - Display: Ninguno
 * - EEPROM: AT24C256 32KB I2C (256Kbit, 64-byte page, económica)
 * - Botones: Solo SIDE_BUTTON
 * - Control Relé: SSR_CTRL (GPIOB Pin2) en vez de SY_TXD
 * - Alarma: OTP_IN (GPIOB Pin5) en vez de SY_RXD
 * - LEDs: LED0, DB4 (mapeado a LED0), SIDE_LED
 * - Batería: Sin monitoreo
 *
 * @note Esta variante usa diferentes pines para relay y alarma vs V5.
 *       SSR_CTRL en vez de SY_TXD.
 *       OTP_IN en vez de SY_RXD.
 *       EEPROM más pequeña (AT24C256 32KB vs M24M01E 128KB).
 */
const BoardProfile_t BOARD_PROFILE_SSR3_V1 = {
    .variant_name = "SSR3-V1-HW",

    /* ===== Buttons Configuration ===== */
    .buttons = {
        [BUTTON_ONOFF] = {.enabled = false}, /* ✅ Disabled en SSR3-V1 */
        [BUTTON_ENTER] = {.enabled = false},
        [BUTTON_UP] = {.enabled = false},
        [BUTTON_DOWN] = {.enabled = false},
        [BUTTON_SIDE] = {
            .port = SIDE_BUTTON_GPIO_Port,
            .pin = SIDE_BUTTON_Pin,
            .active_low = true,
            .long_press_ms = 500,
            .enabled = true /* ✅ Único botón en SSR3-V1 */
        }},

    /* ===== Relay Control (diferente pin vs V5) ===== */
    .relay_control = {.port = SSR_CTRL_GPIO_Port, /* ✅ SSR_CTRL en vez de SY_TXD */
                      .pin = SSR_CTRL_Pin,
                      .enabled = true},

    /* ===== Alarm Input (diferente pin vs V5) ===== */
    .alarm_input = {.port = OTP_IN_GPIO_Port, /* ✅ OTP_IN en vez de SY_RXD */
                    .pin = OTP_IN_Pin,
                    .active_low = false, /* ✅ Alarma activa en HIGH en SSR3-V1 */
                    .long_press_ms = 0,
                    .enabled = true},

    /* ===== Status LEDs (solo LED0 + DB4) ===== */
    .led_status = {
        [0] = {.port = SIDE_LED_GPIO_Port, .pin = SIDE_LED_Pin, .enabled = true}, [1] = {.port = LED0_GPIO_Port, .pin = LED0_Pin, .enabled = true}, /* ✅ DB4 mapea a LED0 en SSR3-V1 */
        [2] = {.enabled = false}                                                                    /* ✅ DB5 no existe en SSR3-V1 */
    },

    /* ===== Side LED (General Indicator) ===== */
    .side_led = {.port = SIDE_LED_GPIO_Port, .pin = SIDE_LED_Pin, .enabled = true},

    /* ===== Display (disabled) ===== */
    .display = {.enabled = false,
                /* ✅ Sin pantalla en SSR3-V1 */},

    /* ===== EEPROM (Alternative: AT24C256 más económica) ===== */
    .eeprom = {
        .enabled = true, .driver = EEPROM_DRIVER_M24M01E, /* ✅ M24M01E 128KB */
        .size_bytes = 131072,                             /* 128KB */
        .page_size = 256,                                 /* 256-byte page writes */
        .i2c_address = 0xA0                               /* I2C address (7-bit: 0x50, shifted: 0xA0) */
    },

    /* ===== Battery Monitoring (Disabled) ===== */
    .battery = {.enabled = false, /* Sin monitoreo en variante minimal */},

    /* ===== ESP32 WiFi Co-Processor (Disabled in minimal variant) ===== */
    .esp32_wifi = {
        .enabled = true, /* wifi enable */
        .reset_port = GPIO_RESET_GPIO_Port,
        .reset_pin = GPIO_RESET_Pin,
        .handshake_port = HANDSHAKE_GPIO_Port,
        .handshake_pin = HANDSHAKE_Pin,
        .data_ready_port = GPIO_DATA_READY_GPIO_Port,
        .data_ready_pin = GPIO_DATA_READY_Pin,
        .cs_port = SPI_CS_GPIO_Port,
        .cs_pin = SPI_CS_Pin,
        .cs_mode = ESP_CS_MODE_AUTO,
        .handshake_exti_line = 8,
        .data_ready_exti_line = 7 /* Reusar EXTI7 para data ready, aunque no se usará en esta variante */
    },

    /* ===== USB RNDIS ===== */
    .usb_rndis = {.enabled = true /* ✅ USB RNDIS gateway habilitado en SSR3-V1 */},

    /* ===== Additional GPIO Pins ===== */
    .buzzer = {.port = BUZZER_GPIO_Port, .pin = BUZZER_Pin, .enabled = true},
    .gps_reset = {.port = GPS_RST_GPIO_Port, .pin = GPS_RST_Pin, .enabled = true},
    .gps_pps = {.port = GPS_PPS_GPIO_Port, .pin = GPS_PPS_Pin, .active_low = false, .long_press_ms = 0, .enabled = true},
    .rf_ctrl1 = {.port = RF_CTRL1_GPIO_Port, .pin = RF_CTRL1_Pin, .enabled = true},
    .rf_ctrl2 = {.port = RF_CTRL2_GPIO_Port, .pin = RF_CTRL2_Pin, .enabled = true},

    /* ===== Peripheral Handle Mapping ===== */
    .peripheral_map = {
        .gps_uart_index = 0,    /* Index 0 = huart2 (ver bsp_init.c uart_map) */
        .debug_uart_index = 0,  /* Index 0 = huart2 */
        .wifi_spi_index = 2,    /* Index 0 = hspi3 reservado */
        .eeprom_i2c_index = 0,  /* Index 0 = hi2c1 (AT24C256) */
        .display_spi_index = 0, /* No display en SSR3-V1 */
        .relay_timer_index = 0, /* Index 0 = htim2 (mismo timer que V5) */
        .rtc_index = 0,         /* Index 0 = hrtc (único RTC) */
        .octospi_index = 0      /* Index 0 = hospi1 (mismo que V5, hardware idéntico) */
    }};

/*============================================================================*
 * PROFILE: MOCK (For PC Testing)
 *============================================================================*/

/**
 * @brief Perfil MOCK para testing en PC sin hardware real.
 *
 * Usa direcciones ficticias válidas para punteros GPIO.
 * Permite tests unitarios en PC sin STM32 HAL.
 *
 * @note Las direcciones son ficticias pero alineadas correctamente.
 *       BSP Mock debe interceptar estas direcciones y simular comportamiento.
 */
const BoardProfile_t BOARD_PROFILE_MOCK = {
    .variant_name = "MOCK-TEST",

    /* ===== Buttons Configuration ===== */
    .buttons = {
        [BUTTON_ONOFF] = {.enabled = false},
        [BUTTON_ENTER] = {.enabled = false},
        [BUTTON_UP] = {.enabled = false},
        [BUTTON_DOWN] = {.enabled = false},
        [BUTTON_SIDE] = {
            .port = (GPIO_Port_t)0x10000000, /* Dirección ficticia válida */
            .pin = 1,
            .active_low = true,
            .long_press_ms = 100,
            .enabled = true}},

    /* ===== Relay Control ===== */
    .relay_control = {.port = (GPIO_Port_t)0x20000000, .pin = 2, .enabled = true},

    /* ===== Alarm Input ===== */
    .alarm_input = {.port = (GPIO_Port_t)0x30000000, .pin = 3, .active_low = true, .long_press_ms = 0, .enabled = true},

    /* ===== Status LEDs ===== */
    .led_status = {[0] = {.port = (GPIO_Port_t)0x40000000, .pin = 4, .enabled = true}, [1] = {.enabled = false}, [2] = {.enabled = false}},

    /* ===== Side LED ===== */
    .side_led = {.port = (GPIO_Port_t)0x50000000, .pin = 5, .enabled = true},

    /* ===== ESP32 WiFi Co-Processor (Mock for testing) ===== */
    .esp32_wifi = {.enabled = false,                      /* ✅ Disabled for PC testing */
                   .reset_port = (GPIO_Port_t)0xB0000000, /* Mock address */
                   .reset_pin = 11,
                   .handshake_port = (GPIO_Port_t)0xC0000000, /* Mock address */
                   .handshake_pin = 12,
                   .cs_port = (GPIO_Port_t)0xD0000000, /* Mock address */
                   .cs_pin = 13,
                   .cs_mode = ESP_CS_MODE_MANUAL,
                   .handshake_exti_line = 8,
                   .data_ready_exti_line = 7},

    /* ===== Display ===== */
    .display = {.enabled = false, .width = 0, .height = 0, .driver = DISPLAY_DRIVER_NONE},

    /* ===== EEPROM (Simulated) ===== */
    .eeprom = {.enabled = true, .driver = EEPROM_DRIVER_M24M01E, /* Simular chip más grande para tests completos */
               .size_bytes = 131072,
               .page_size = 256,
               .i2c_address = 0xA0},

    /* ===== Battery Monitoring ===== */
    .battery = {.enabled = false},

    /* ===== USB RNDIS ===== */
    .usb_rndis = {.enabled = false /* ✅ Disabled for PC testing */},

    /* ===== Additional GPIO Pins (Mock addresses) ===== */
    .buzzer = {.port = (GPIO_Port_t)0x60000000, .pin = 6, .enabled = true},
    .gps_reset = {.port = (GPIO_Port_t)0x70000000, .pin = 7, .enabled = true},
    .gps_pps = {.port = (GPIO_Port_t)0x80000000, .pin = 8, .active_low = false, .long_press_ms = 0, .enabled = true},
    .rf_ctrl1 = {.port = (GPIO_Port_t)0x90000000, .pin = 9, .enabled = true},
    .rf_ctrl2 = {.port = (GPIO_Port_t)0xA0000000, .pin = 10, .enabled = true},

    /* ===== Peripheral Handle Mapping ===== */
    .peripheral_map = {
        .gps_uart_index = 0,    /* Mock UART 0 */
        .debug_uart_index = 0,  /* Mock UART 0 */
        .wifi_spi_index = 0,    /* Mock SPI 0 */
        .eeprom_i2c_index = 0,  /* Mock I2C 0 */

        .display_spi_index = 0, /* Mock SPI 0 */
        .relay_timer_index = 0, /* Mock Timer 0 */
        .rtc_index = 0,         /* Mock RTC 0 */
        .octospi_index = 0      /* Mock OctoSPI 0 */
    }};

/*============================================================================*
 * PROFILE SELECTOR (ÚNICO #ifdef EN TODO EL SISTEMA)
 *============================================================================*/

/**
 * @brief Retorna el perfil activo según build flags.
 *
 * Esta función es el ÚNICO lugar en el firmware que usa #ifdef para
 * seleccionar variantes de hardware. Todo el código restante es agnóstico.
 *
 * @return Puntero al perfil seleccionado (nunca NULL, const static).
 *
 * @note Thread-safe: retorna puntero a datos const en .rodata
 * @note Si no se define ningún BOARD_VARIANT, genera error de compilación.
 *
 * @example Build flags:
 * ```bash
 * # Compilar para V1
 * make CFLAGS="-DBOARD_VARIANT_CICX1_V5"
 *
 * # Compilar para V2
 * make CFLAGS="-DBOARD_VARIANT_SSR_V1"
 *
 * # Tests en PC
 * cmake -DBOARD_VARIANT=MOCK ..
 * ```
 */
const BoardProfile_t *BSP_GetBoardProfile(void)
{
#if defined(BOARD_VARIANT_CICX1_V5)
    return &BOARD_PROFILE_CICX1_V5;

#elif defined(BOARD_VARIANT_SSR3_V1)
    return &BOARD_PROFILE_SSR3_V1;

#elif defined(BOARD_VARIANT_MOCK)
    return &BOARD_PROFILE_MOCK;

#else
#error "BOARD_VARIANT no definido. Use -DBOARD_VARIANT_V1, -DBOARD_VARIANT_V2 o -DBOARD_VARIANT_MOCK en build flags"
#endif
}
