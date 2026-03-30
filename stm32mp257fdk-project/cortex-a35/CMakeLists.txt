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

# ─── Application sources ─────────────────────────────────────────────────────
set(SOURCES
    app/src/main.cpp
    app/src/ipc_rpmsg.cpp
)

set(INCLUDES
    app/include
    ../shared/ipc
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})

# ─── Tests ───────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(app/tests)
endif()
