# BSP (Board Support Package)

## Purpose

Contains **concrete implementations** of HAL interfaces for specific hardware platforms.

## Implementations

### `stm32u5/`

Production implementation for STM32U575VGTx microcontroller.

#### Status: ✅ 100% Complete

**Implemented Drivers:**

- ✅ `bsp_stm32u5_gpio.c/.h` - GPIO control with tests
- ✅ `bsp_stm32u5_uart.c/.h` - UART communication with tests
- ✅ `bsp_stm32u5_timer.c/.h` - Timers and PWM with tests
- ✅ `bsp_stm32u5_exti.c/.h` - External interrupts with tests
- ✅ `bsp_stm32u5_spi.c/.h` - SPI bus
- ✅ `bsp_stm32u5_i2c.c/.h` - I2C bus
- ✅ `bsp_stm32u5_octospi.c/.h` - External flash (OctoSPI)
- ✅ `bsp_stm32u5_rtc.c/.h` - Real-Time Clock

**Infrastructure:**

- ✅ `bsp_init.c/.h` - Centralized BSP initialization
- ✅ `bsp_config.h` - Pin mapping and hardware configuration

**Features:**

- Uses STM32 HAL drivers
- Maps to actual hardware peripherals
- Factory pattern for interface creation
- Configured via `bsp_config.h` with pin mappings

### `mock/`

❌ **TODO**: Mock implementation for **PC-based unit testing**.

- Simulates hardware behavior
- No dependencies on STM32 HAL
- Enables testing without physical hardware

### `esp32/`

❌ **TODO**: Placeholder for future ESP32 port (demonstrates portability).

## Rules

- ✅ **ONLY BSP files** may include vendor-specific headers (`stm32u5xx_hal.h`)
- ✅ Implements HAL interfaces defined in `hal/interfaces/`
- ✅ Uses **static allocation only** (no malloc)
- ❌ BSP code must NOT contain business logic

## Key Files

### Per Platform (STM32U5)

```
stm32u5/
├── bsp_init.c/h              # ✅ Initializes all BSP drivers
├── bsp_config.h              # ✅ Pin mapping and hardware configuration
├── bsp_stm32u5_gpio.c/h      # ✅ GPIO implementation
├── bsp_stm32u5_uart.c/h      # ✅ UART implementation
├── bsp_stm32u5_timer.c/h     # ✅ Timer implementation
├── bsp_stm32u5_exti.c/h      # ✅ External interrupts
├── bsp_stm32u5_spi.c/h       # ✅ SPI implementation
├── bsp_stm32u5_i2c.c/h       # ✅ I2C implementation
├── bsp_stm32u5_octospi.c/h   # ✅ OctoSPI for external flash
└── bsp_stm32u5_rtc.c/h       # ✅ RTC implementation
```

## Usage Example

```c
#include "bsp/stm32u5/bsp_init.h"

int main(void)
{
    /* Initialize HAL */
    HAL_Init();
    SystemClock_Config();

    /* Initialize BSP (hardware + interfaces) */
    if (BSP_Init() != ERR_OK) {
        Error_Handler();
    }

    /* Get BSP interfaces */
    const BSP_Interfaces_t* bsp = BSP_GetInterfaces();

    /* Use interfaces */
    RTC_Time_t time;
    RTC_GetTime(bsp->rtc, BSP_RTC_MAIN, &time);

    /* ... */
}
```

## Build Selection

BSP is selected at compile time via CMake:

```cmake
set(BSP_TARGET "stm32u5")  # or "mock" for tests
```

## Testing

Unit tests for STM32U5 BSP are located in:

```
tests/unit/bsp/stm32u5/
├── test_bsp_stm32u5_gpio.c
├── test_bsp_stm32u5_uart.c
├── test_bsp_stm32u5_timer.c
└── test_bsp_stm32u5_exti.c
```

Run tests with:

```bash
ceedling test:all
```

## Hardware Dependencies

The STM32U5 BSP depends on:

- STM32CubeMX generated code in `Core/`
- STM32U5 HAL drivers in `Drivers/STM32U5xx_HAL_Driver/`
- CMSIS headers in `Drivers/CMSIS/`

## Next Steps

1. ❌ Implement BSP Mock for PC testing
2. ❌ Add unit tests for RTC, SPI, I2C drivers
3. ❌ Document pin mappings in `bsp_config.h`
4. ❌ Create DMA wrappers if needed
