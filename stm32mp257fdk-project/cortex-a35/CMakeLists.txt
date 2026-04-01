cmake_minimum_required(VERSION 3.25)
project(stm32mp257fdk_app CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O2>
)

set(INCLUDES
    app/include
    ../shared/ipc
)

# ─── Shared IPC + OTA library ────────────────────────────────────────────────
add_library(ipc_ota STATIC
    app/src/ipc_rpmsg.cpp
    app/src/ota_manager.cpp
)
target_include_directories(ipc_ota PUBLIC ${INCLUDES})

# ─── Demo application ────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} app/src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ipc_ota)

# ─── OTA update tool ─────────────────────────────────────────────────────────
add_executable(ota_update app/src/ota_cli.cpp)
target_include_directories(ota_update PRIVATE ${INCLUDES})
target_link_libraries(ota_update PRIVATE ipc_ota)

# ─── Tests ───────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(app/tests)
endif()
