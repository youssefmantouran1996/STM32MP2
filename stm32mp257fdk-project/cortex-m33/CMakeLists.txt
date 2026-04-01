cmake_minimum_required(VERSION 3.25)
project(stm32mp257fdk_m33 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Compiler flags ──────────────────────────────────────────────────────────
set(CPU_FLAGS
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
)

add_compile_options(
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Release>:-O2>
)

add_link_options(
    ${CPU_FLAGS}
    -T${CMAKE_SOURCE_DIR}/STM32MP257FAIX_FLASH.ld
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    --specs=nosys.specs
    --specs=nano.specs
)

# ─── Source files ────────────────────────────────────────────────────────────
set(SOURCES
    # Startup
    Core/Startup/startup_stm32mp257fxx_m33.s

    # Application
    Core/Src/main.c
    Core/Src/stm32mp2xx_hal_msp.c
    Core/Src/stm32mp2xx_it.c
    Core/Src/system_stm32mp2xx.c
    Core/Src/uart_log.c
    Core/Src/ipc_handler.c
    Core/Src/syscalls.c

    # OpenAMP resource table (placed in .resource_table ELF section)
    ../shared/ipc/resource_table.c

    # STM32MP2xx HAL drivers
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_cortex.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_gpio.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_rcc.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_rcc_ex.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_pwr.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_pwr_ex.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_uart.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_uart_ex.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_ipcc.c
    Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_exti.c
)

set(INCLUDES
    Core/Inc
    ../shared/ipc
    Drivers/STM32MP2xx_HAL_Driver/Inc
    Drivers/CMSIS/Device/ST/STM32MP2xx/Include
    Drivers/CMSIS/Include
)

# ─── Target definition ───────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

# Inject Unix build timestamp so the firmware can report it via IPC_CMD_GET_VERSION
string(TIMESTAMP FW_BUILD_TIMESTAMP "%s" UTC)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32MP257Fxx
    USE_HAL_DRIVER
    CORE_CM33
    FW_BUILD_TIMESTAMP=${FW_BUILD_TIMESTAMP}U
)

# ─── Post-build: generate .bin and print size ─────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating binary and printing size"
)
