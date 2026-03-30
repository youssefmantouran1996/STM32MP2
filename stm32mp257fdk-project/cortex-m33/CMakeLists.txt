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
    $<$<CONFIG:Debug>:-Og -g3>
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
# NOTE: Copy STM32CubeMP2 HAL sources here after downloading from st.com
set(SOURCES
    Core/Src/main.c
    Core/Src/stm32mp2xx_hal_msp.c
    Core/Src/stm32mp2xx_it.c
    # Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal.c
    # Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_gpio.c
    # Drivers/STM32MP2xx_HAL_Driver/Src/stm32mp2xx_hal_uart.c
    # Add more HAL sources as needed
)

set(INCLUDES
    Core/Inc
    ../shared/ipc
    # Drivers/STM32MP2xx_HAL_Driver/Inc
    # Drivers/CMSIS/Device/ST/STM32MP2xx/Include
    # Drivers/CMSIS/Include
    # Middlewares/OpenAMP
)

# ─── Target definition ───────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    STM32MP257Fxx
    USE_HAL_DRIVER
    CORE_CM33
)

# ─── Post-build: generate .bin and print size ─────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating binary and printing size"
)
