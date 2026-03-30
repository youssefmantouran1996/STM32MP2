cmake_minimum_required(VERSION 3.25)
project(stm32mp257fdk_workspace LANGUAGES NONE)

# ─── Top-level orchestrator ───────────────────────────────────────────────────
# Build each target independently with its own toolchain.
# Use scripts/build_all.sh or invoke cmake manually for each sub-project.

message(STATUS "STM32MP257FDK Workspace")
message(STATUS "  Cortex-M33 firmware : cortex-m33/  (use toolchain-m33.cmake)")
message(STATUS "  Cortex-A35 app      : cortex-a35/  (use toolchain-aarch64.cmake)")
message(STATUS "")
message(STATUS "Run: ./scripts/build_all.sh   to build both targets.")
