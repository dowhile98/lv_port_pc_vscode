# 📚 OSAL — Guía de Implementación

> **⚠️ NOTA PARA PC PORT**: Este proyecto compila con FreeRTOS (POSIX port) en PC.
> La implementación activa es `freertos/osal_freertos.c`. El contenido a continuación
> es referencia histórica para el target firmware STM32U5 con Azure RTOS ThreadX.
> Para el PC port, ignore las referencias a `TX_*` y consulte `osal_freertos.c`.

Guía de referencia para el módulo OSAL (Operating System Abstraction Layer) basado en **Azure RTOS ThreadX**. Sigue el estilo de documentación del `REFACTOR_PLAN.md`: secciones claras, reglas explícitas y énfasis en desacoplar el dominio del RTOS.

## 🎯 Propósito

- Proveer una fachada estable (`osal.h`) que oculte detalles de ThreadX.
- Evitar dependencias de HAL/CMSIS en dominio/aplicación.
- Garantizar memoria **100% estática** (sin `malloc`/`free`).
- Permitir **asignación desde `TX_BYTE_POOL`** cuando el caller no provee storage.
- Facilitar testing en PC mockeando llamadas OSAL.

## 🧭 Alcance

- Implementación concreta en `osal_threadx.c`.
- Tipos opacos y contratos en `include/infrastructure/osal/osal.h`.
- Excluye inicialización del kernel ThreadX (se realiza en BSP/boot).

## 🚦 Reglas Clave

1. **Sin asignación dinámica**: el caller provee almacenamiento para todos los objetos (control blocks, stacks, colas, buffers).
   - **Excepción controlada**: si el OSAL se inicializa con `os_init()` puede reservar desde `TX_BYTE_POOL`.
2. **Handles opacos**: `os_*` son `void*`; nunca exponer tipos ThreadX en capas superiores.
3. **Validación temprana**: todas las APIs devuelven `ERR_NULL_POINTER`/`ERR_INVALID_PARAM` antes de invocar ThreadX.
4. **Timeouts coherentes**: `OS_WAIT_FOREVER` y `OS_NO_WAIT` se traducen a ticks con `os_ms_to_ticks`.
5. **Determinismo**: prohibido bloquear en ISR; ISRs solo deben señalizar (semafóro/event flags).

## 🧱 Mapeo de Objetos OSAL → ThreadX

| OSAL               | ThreadX concreto        | Requisitos de memoria                      |
| ------------------ | ----------------------- | ------------------------------------------ |
| `os_thread_t`      | `TX_THREAD*`            | Caller provee `TX_THREAD` + stack estático |
| `os_mutex_t`       | `TX_MUTEX*`             | Objeto `TX_MUTEX` persistente              |
| `os_semaphore_t`   | `TX_SEMAPHORE*`         | Objeto `TX_SEMAPHORE` persistente          |
| `os_event_flags_t` | `TX_EVENT_FLAGS_GROUP*` | Objeto `TX_EVENT_FLAGS_GROUP` persistente  |
| `os_queue_t`       | `TX_QUEUE*`             | Objeto `TX_QUEUE` + buffer alineado a 32b  |

## 🔨 Creación y Ciclo de Vida

- **Threads**: `os_thread_create` recibe `os_thread_t*` ya apuntando a `TX_THREAD` y `stack_ptr` con tamaño `stack_size`. Usa prioridad fija (preempt-priority) y `auto_start` opcional.
- **Mutex**: `os_mutex_create` asume `TX_MUTEX` ya reservado; no hay pool interno. Usa `TX_NO_INHERIT` (sin priority inheritance por defecto).
- **Semáforos**: contadores binarios/contados; inicialización con `initial_count`.
- **Event Flags**: soporta opciones de ThreadX (`TX_AND`, `TX_OR`, `TX_CLEAR`); tiempos en ms convertidos a ticks.
- **Queues**: tamaño de mensaje en múltiplos de `sizeof(ULONG)`; `buffer_size` debe ser entero de `ULONG` y cubrir al menos 1 elemento.
- **Memoria desde Pool**:
  - `os_init(void *memory_ptr)` registra el `TX_BYTE_POOL`.
  - `os_alloc`/`os_free` exponen asignación/liberación desde el pool.
  - `os_queue_create` permite `*queue == NULL` y/o `config->buffer == NULL` si el pool está inicializado.
  - En modo dinámico, el OSAL reserva control blocks/buffers solo una vez; no hay liberación automática en `os_queue_delete`.

## ⏱️ Conversión de Tiempos

- `os_ms_to_ticks(ms)` usa `TX_TIMER_TICKS_PER_SECOND`; asegurar que esté definido en build (`tx_user.h` o flags del proyecto).
- Para timeouts, el wrapper convierte ms → ticks; `OS_WAIT_FOREVER` mapea a `TX_WAIT_FOREVER`.

## 📑 API y Códigos de Retorno

- Todas las funciones devuelven `Result_t` (ver `hal/hal_types.h`).
- `ERR_OK` éxito; `ERR_NULL_POINTER` cuando cualquier handle/ptr requerido es `NULL`; `ERR_INVALID_PARAM` en tamaños inválidos; `ERR_TIMEOUT` en esperas; `ERR_ERROR` resto.

## 🔐 Concurrencia y Seguridad

- **ISR**: no llame a APIs bloqueantes; use `tx_semaphore_put`/`tx_event_flags_set` en handlers y procese en hilo.
- **Mutex**: evitar `OS_WAIT_FOREVER` en rutas críticas propensas a inversión de prioridad; considere `timeout_ms` finito.
- **Queues**: mensajes se copian; el buffer subyacente debe ser exclusivo del queue.

## 🧪 Testing y Mocks

- Para pruebas en PC, provea una implementación mock de las funciones de `osal.h` que simule semáforos/colas con estructuras triviales.
- Mantenga la misma semántica de códigos de retorno para no romper contratos de dominio/aplicación.

## 🛠️ Ejemplos de Uso

```c
// Reserva estática (BSP o capa de inicio)
static TX_THREAD g_thread_ctrl;
static uint8_t g_thread_stack[1024] __attribute__((aligned(8)));

static TX_MUTEX g_mutex_ctrl;

static TX_QUEUE g_queue_ctrl;
static ULONG g_queue_buffer[16]; // 16 * sizeof(ULONG) bytes

void App_Init(void)
{
    os_thread_t thread = &g_thread_ctrl;
    os_thread_config_t cfg = {
        .name = "worker",
        .entry = worker_entry,
        .arg = NULL,
        .stack_ptr = g_thread_stack,
        .stack_size = sizeof(g_thread_stack),
        .priority = 5U,
        .auto_start = true};

    os_mutex_t mutex = &g_mutex_ctrl;
    os_queue_t queue = &g_queue_ctrl;
    os_queue_config_t qcfg = {
        .name = "q",
        .buffer = g_queue_buffer,
        .buffer_size = sizeof(g_queue_buffer),
        .item_size = sizeof(uint32_t)};

    (void)os_mutex_create(&mutex, "mtx");
    (void)os_queue_create(&queue, &qcfg);
    (void)os_thread_create(&thread, &cfg);
}
```

```c
// Reserva dinámica desde Byte Pool (ThreadX)
void App_ThreadX_Init(VOID *memory_ptr)
{
    TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;
    (void)os_init(byte_pool);

    os_queue_t queue = NULL;
    os_queue_config_t qcfg = {
        .name = "GPS_AO_Q",
        .buffer = NULL,
        .buffer_size = 128U,
        .item_size = sizeof(GPSAoEvent_t)};

    (void)os_queue_create(&queue, &qcfg);
}
```

## ⚠️ Anti-Patrones a Evitar

- Usar `malloc` para control blocks o stacks.
- Pasar `NULL` en `stack_ptr`, buffers o handles esperando que el OSAL los reserve.
- Llamar APIs de bloqueo dentro de ISR.
- Calcular manualmente ticks sin `os_ms_to_ticks` (riesgo de overflow al cambiar `TX_TIMER_TICKS_PER_SECOND`).

## 📂 Archivos Relevantes

- `include/infrastructure/osal/osal.h` — interfaz pública (portátil).
- `src/infrastructure/osal/threadx/osal_threadx.c` — implementación ThreadX.
- `tx_user.h` — configuración de tick rate/stack checking (generado por ThreadX, fuera de OSAL).

## ✅ Checklist de Integración

- [ ] Handles y buffers estáticos reservados en BSP/boot.
- [ ] `TX_TIMER_TICKS_PER_SECOND` configurado y coherente con tiempos requeridos.
- [ ] Sin includes de HAL/CMSIS en capas dominio/aplicación; solo usan `osal.h`.
- [ ] Timeouts revisados para rutas críticas (evitar bloqueos infinitos).
- [ ] Tests (o mocks) mantienen semántica de `Result_t`.

---

**Nota**: El OSAL es la frontera entre el RTOS y el resto del sistema. Mantenga esta capa mínima, determinista y totalmente portable para habilitar pruebas en PC y futuros ports a otros RTOS.
