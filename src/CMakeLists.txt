# ---------------------------------------------------------------------------
# src/CMakeLists.txt
#
# Defines all application source lists and exports them to the parent scope.
# Add new files/folders here when EEZ Studio regenerates assets or when new
# modules are added under src/.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# HAL & shared assets
# ---------------------------------------------------------------------------
set(_HAL_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mouse_cursor_icon.c
)

# ---------------------------------------------------------------------------
# PC BSP — SDL display adapter + PC BSP_UI_HW_Init
#   Compiled INSTEAD OF bsp_stm32u5_display.c and ui_port_disp.c
# ---------------------------------------------------------------------------
set(_PC_BSP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/bsp_ui_hw_init_pc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/ui_port_disp_pc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/pc_keyboard_input_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/pc_file_config_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/pc_event_log_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/pc_printf_logger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Third_Party/lwbtn/src/lwbtn/lwbtn.c
)

# ---------------------------------------------------------------------------
# Presentation ports (encoder adapter, indev port, UiAO) + full presentation layer
# ---------------------------------------------------------------------------
file(GLOB_RECURSE _PRESENTATION_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/*.c"
)
# Exclude EEZ-generated ui/*.c (handled separately below)
list(FILTER _PRESENTATION_SRCS EXCLUDE REGEX ".*/presentation/ui/.*\\.c$")
# Exclude firmware display port — replaced by ui_port_disp_pc.c
list(FILTER _PRESENTATION_SRCS EXCLUDE REGEX ".*/ui_port/ui_port_disp\\.c$")

set(_PORT_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui_port/lvgl_encoder_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui_port/ui_port_indev.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui_actions.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/activeobjects/ui_ao.c
    ${_PRESENTATION_SRCS}
)

# ---------------------------------------------------------------------------
# EEZ Studio generated UI
#   ui/*.c          — screens, styles, images registry, ui init
#   ui/fonts/*.c    — custom bitmap fonts exported from EEZ
#   ui/images/*.c   — image assets exported from EEZ
# ---------------------------------------------------------------------------
file(GLOB _EEZ_UI_SRCS    "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui/*.c")
file(GLOB _EEZ_FONT_SRCS  "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui/fonts/*.c")
file(GLOB _EEZ_IMAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ui/images/*.c")

# ---------------------------------------------------------------------------
# OSAL – OS Abstraction Layer
#   infrastructure/osal/freertos/  — FreeRTOS backend
# ---------------------------------------------------------------------------
set(_OSAL_FREERTOS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure/osal/freertos/osal_freertos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure/osal/freertos/lv_freertos_osal.c
)

# ---------------------------------------------------------------------------
# PC-specific replacements for STM32U5 firmware modules
#   These provide stub/no-op implementations of HW-dependent subsystems.
# ---------------------------------------------------------------------------
set(_PC_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure/startup/system_init_pc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure/di/dependency_container_pc.c
)

# ---------------------------------------------------------------------------
# PC mock services — lightweight RAM-based replacements for HW interfaces
# ---------------------------------------------------------------------------
set(_PC_MOCK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/mock/mock_logger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/mock/mock_digital_input_source.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/mock/mock_bq27441_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/mock/mock_domain_stubs_pc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/mock/mock_pc_adapters.c
)

# ---------------------------------------------------------------------------
# Export source lists to parent scope
# ---------------------------------------------------------------------------

# Sources compiled in every build (FreeRTOS or bare-metal)
set(APP_SOURCES
    ${_HAL_SRCS}
    ${_PC_BSP_SRCS}
    ${_PORT_SRCS}
    ${_PC_SRCS}
    ${_PC_MOCK_SRCS}
    ${_EEZ_UI_SRCS}
    ${_EEZ_FONT_SRCS}
    ${_EEZ_IMAGE_SRCS}
    PARENT_SCOPE
)

# FreeRTOS application entry point + POSIX glue + OSAL backend
set(APP_FREERTOS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/freertos_posix_port.c
    ${_OSAL_FREERTOS_SRCS}
    PARENT_SCOPE
)

# Bare-metal application entry point
set(APP_BARE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    PARENT_SCOPE
)
