# Include - Public Headers

## Purpose

Contains all **public header files** for the application following Clean Architecture layers.

## Structure

### `interfaces/`

**High-level domain interfaces** (not to be confused with HAL interfaces).

- `IGPSSource.h` - GPS data abstraction
- `IRelayControl.h` - Relay control abstraction
- `ITimeSource.h` - Time source abstraction
- `IConfigStorage.h` - Configuration storage abstraction
- `IBatteryMonitor.h` - Battery monitoring abstraction
- etc.

### `domain/`

**Core business logic** - 100% portable, no hardware dependencies.

#### `entities/`

Business entities and value objects:

- `InterruptionCycle.h`
- `TimeWindow.h`
- `SystemState.h`
- `ContactState.h`

#### `services/`

Domain services (pure logic):

- `CycleSchedulerService.h`
- `TimeValidatorService.h`
- `PPSManagerService.h`

### `application/`

**Use cases and application orchestration**.

#### `usecases/`

- `InterruptionUseCase.h`
- `ConfigurationUseCase.h`
- `MonitoringUseCase.h`

#### `activeobjects/`

ThreadX Active Objects for concurrency:

- `ActiveObject.h`
- `InterruptionAO.h`
- `EventManagerAO.h`

### `infrastructure/`

**Adapters connecting domain to hardware**.

#### `adapters/`

- `GPSUartAdapter.h` - GPS UART adapter (uses HAL interfaces internally)
- `RelayTimerAdapter.h` - Relay control adapter (uses HAL interfaces internally)
- `RTCAdapter.h` - RTC adapter
- `EEPROMAdapter.h` - EEPROM storage adapter
- etc.

#### `di/`

Dependency Injection container:

- `DependencyContainer.h`

### `presentation/`

**User Interface layer**.

- `UIController.h`
- `screens/` - LVGL screen controllers
- `observers/` - UI observers for state updates

### `common/`

**Shared types and utilities**.

- `types.h` - Common type definitions
- `error_codes.h` - Error code enumerations
- `config.h` - Configuration constants

## Dependency Rules

```
Presentation → Application → Domain
     ↓              ↓
Infrastructure ← (no dependencies to upper layers)
     ↓
    HAL (interfaces only)
     ↓
    BSP (selected at compile time)
```

**Critical**: Domain and Application layers must NEVER include:

- `stm32*.h`
- `bsp_*.h`
- Any hardware-specific headers
