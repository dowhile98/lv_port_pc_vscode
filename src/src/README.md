# Source - Implementation Files

## Purpose

Contains all **implementation (.c) files** corresponding to headers in `include/`.

## Structure Mirrors Include

```
src/
├── domain/
│   ├── entities/        # Business entities implementation
│   └── services/        # Domain services implementation
├── application/
│   ├── usecases/        # Use cases implementation
│   └── activeobjects/   # Active Objects implementation
├── infrastructure/
│   ├── adapters/        # Hardware adapters implementation
│   ├── di/              # Dependency Injection implementation
│   └── startup/         # System initialization
├── presentation/
│   ├── screens/         # LVGL screens implementation
│   └── observers/       # UI observers implementation
└── main.c               # Application entry point (eventually migrated here)
```

## Implementation Rules

### Domain Layer (`domain/`)

- ✅ Pure C logic, no hardware dependencies
- ✅ Testable on PC without modifications
- ✅ Uses only `<stdint.h>`, `<stdbool.h>`, `<string.h>`
- ❌ NEVER include hardware headers
- ❌ No global state (use context structs)

### Application Layer (`application/`)

- ✅ Orchestrates domain services
- ✅ Depends on domain interfaces only
- ✅ ThreadX dependencies allowed (Active Objects)
- ❌ No direct hardware access

### Infrastructure Layer (`infrastructure/`)

- ✅ Implements domain interfaces
- ✅ Uses HAL interfaces (`I_GPIO`, `I_UART`, etc.)
- ✅ Connects domain logic to hardware
- ❌ No business logic here (delegate to domain)

### Presentation Layer (`presentation/`)

- ✅ LVGL UI implementation
- ✅ Observes domain state changes
- ✅ Calls application use cases
- ❌ No direct hardware access (use adapters)

## Compilation Units

Each `.c` file is a separate compilation unit. Keep files focused:

- Target: 200-400 lines per file
- Max: 600 lines (if larger, split into multiple files)
- One primary responsibility per file (SRP)
