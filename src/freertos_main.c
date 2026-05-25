/**
 * @file    freertos_main.c
 * @brief   FreeRTOS entry point for the PC simulator.
 *
 * Mirrors the STM32U5 firmware's App_ThreadX_Init() pattern:
 *   System_Init() — DI container creates all AOs (including UiAO which starts
 *                   the LVGL thread via ui_ao_thread_entry).
 *   vTaskStartScheduler() — scheduler takes over; UiAO thread runs.
 *
 * NO manual lv_init() / sdl_hal_init() / ui_init() here.
 * That sequence runs inside ui_ao_thread_entry() exactly as in firmware.
 */

#include "FreeRTOS.h"
#include "task.h"

#if LV_USE_OS == LV_OS_FREERTOS

#include <stdio.h>
#include "infrastructure/startup/system_init.h"

// ........................................................................................................
void vApplicationMallocFailedHook(void)
{
    printf("Malloc failed! Available heap: %ld bytes\n", xPortGetFreeHeapSize());
    for (;;)
        ;
}

void vApplicationIdleHook(void) {}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack overflow in task %s\n", pcTaskName);
    for (;;)
        ;
}

void vApplicationTickHook(void) {}

// ........................................................................................................
/**
 * @brief PC simulator entry point.
 *
 * System_Init() calls DI_InitUISubsystem() which calls UiAO_Init() +
 * UiAO_Start().  UiAO_Start() creates the "ui_ao" FreeRTOS task.
 * vTaskStartScheduler() then runs the task, entering ui_ao_thread_entry()
 * which does: lv_init → UI_Port_Disp_Init (SDL) → UI_Port_Indev_Init →
 * ui_init → loop lv_task_handler().
 */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (System_Init() != ERR_OK)
    {
        printf("System_Init failed\n");
        return 1;
    }

    vTaskStartScheduler();

    /* Never reached */
    return 0;
}

#endif
