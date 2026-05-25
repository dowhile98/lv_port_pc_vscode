/**
 * @file system_init_pc.c
 * @brief PC implementation of System_Init.
 *
 * Mirrors system_init.c (STM32U5 firmware) without the HW BSP_Init() call.
 * Delegates all wiring to dependency_container_pc.c.
 *
 * Sequence (matches real firmware order):
 *   1. DI_Container_Init            — zero-initialise container + mock services
 *   2. DI_InitLoggingSubsystem      — mock printf logger
 *   3. DI_InitDigitalInputSubsystem — mock event-queue input source
 *   4. DI_InitBatteryMonitorSubsystem — mock BQ27441 (Li-ion 1000mAh simulation)
 *   5. DI_InitUISubsystem           — UiAO_Init + full presenter wiring
 *                                     (InputRouter, EezScreenRouter, all PresentationLayer_Init*)
 *   6. DI_WireComponents            — no-op on PC
 *   7. DI_StartActiveObjects        — UiAO_Start() → creates "ui_ao" FreeRTOS task
 *   8. PresentationLayer_SetSystemReady(true) — enables "System Ready" on Overview screen
 *
 * After System_Init() returns, call vTaskStartScheduler().
 * The "ui_ao" task then runs ui_ao_thread_entry():
 *   lv_init → UI_Port_Disp_Init (SDL window) → UI_Port_Indev_Init → ui_init →
 *   ui_actions_init → loop.
 * ui_actions_init() registers boot-sequence timers:
 *   STARTUP (2500ms) → OVERVIEW → on_overview_show_status_cb (PresentationLayer_IsSystemReady)
 *   → HOME (2000ms) via PresentationLayer_OnBootComplete.
 *
 * System_Update() runs from lvgl_task (~5ms period):
 *   - MockDigitalInputSource_Process() — dispatch queued input events
 *   - MockBQ27441Adapter_Update()     — simulate battery discharge
 *   - lv_timer_handler()              — LVGL rendering/events
 *
 * @author Tecna Smart Lab
 * @date   2026
 */

#include "infrastructure/startup/system_init.h"
#include "infrastructure/presentation/presentation_layer.h"
#include "hal_types.h"
#include "bsp/mock/mock_digital_input_source.h"
#include "bsp/mock/mock_bq27441_adapter.h"
#include "lvgl/lvgl.h"
#include <stddef.h>

/* DI container API — pass NULL to use internal s_container singleton */
extern Result_t DI_Container_Init(void *container, const void *bsp);
extern Result_t DI_InitLoggingSubsystem(void *container);
extern Result_t DI_InitDigitalInputSubsystem(void *container);
extern Result_t DI_InitBatteryMonitorSubsystem(void *container);
extern Result_t DI_InitUISubsystem(void *container);
extern Result_t DI_WireComponents(void *container);
extern Result_t DI_StartActiveObjects(void *container);
extern void *DI_GetBatteryAdapterInstance(void *container);

Result_t System_Init(void)
{
    Result_t res;

    res = DI_Container_Init(NULL, NULL);
    if (res != ERR_OK)
        return res;

    res = DI_InitLoggingSubsystem(NULL);
    if (res != ERR_OK)
        return res;

    res = DI_InitDigitalInputSubsystem(NULL);
    if (res != ERR_OK)
        return res;

    /* Battery monitor — Li-ion discharge simulation (1000mAh, 3.7V) */
    res = DI_InitBatteryMonitorSubsystem(NULL);
    if (res != ERR_OK)
        return res;

    /* Wires all presenters, InputRouter, EezScreenRouter — task NOT started yet */
    res = DI_InitUISubsystem(NULL);
    if (res != ERR_OK)
        return res;

    res = DI_WireComponents(NULL);
    if (res != ERR_OK)
        return res;

    /* Creates UiAO task — runs lv_init/SDL/ui_init after vTaskStartScheduler() */
    res = DI_StartActiveObjects(NULL);
    if (res != ERR_OK)
        return res;

    /* MUST be last — enables "System Ready" on Overview screen.
     * Mirrors real firmware system_init.c line 219. */
    PresentationLayer_SetSystemReady(true);

    return ERR_OK;
}

void System_Update(void)
{
    /* Dispatch queued keyboard/encoder input events to LVGL */
    MockDigitalInputSource_Process();

    /* Update simulated battery (discharge at ~300mA) */
    MockBQ27441Adapter_t *bat = (MockBQ27441Adapter_t *)DI_GetBatteryAdapterInstance(NULL);
    if (bat)
        (void)MockBQ27441Adapter_Update(bat);

    /* LVGL timer handler (rendering, animations, SDL events) */
    lv_timer_handler();
}
