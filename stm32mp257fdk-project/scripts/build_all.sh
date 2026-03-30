#!/usr/bin/env bash
# build_all.sh — Build both the Cortex-M33 firmware and the Cortex-A35 app.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"

echo "══════════════════════════════════════════════════"
echo "  STM32MP257FDK — Build All Targets"
echo "══════════════════════════════════════════════════"

# ── Cortex-M33 firmware ───────────────────────────────
echo ""
echo "▶ Building Cortex-M33 firmware..."
cmake -B "$BUILD/m33" -S "$ROOT/cortex-m33" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cortex-m33/cmake/toolchain-m33.cmake" \
    -DCMAKE_BUILD_TYPE=Debug \
    -G Ninja --fresh
cmake --build "$BUILD/m33" --parallel

echo "  ✓ M33 firmware: $BUILD/m33/stm32mp257fdk_m33.elf"

# ── Cortex-A35 Linux app ──────────────────────────────
echo ""
echo "▶ Building Cortex-A35 Linux application..."
cmake -B "$BUILD/a35" -S "$ROOT/cortex-a35" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cortex-a35/cmake/toolchain-aarch64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja --fresh
cmake --build "$BUILD/a35" --parallel

echo "  ✓ A35 app: $BUILD/a35/stm32mp257fdk_app"

echo ""
echo "══════════════════════════════════════════════════"
echo "  Build complete!"
echo "══════════════════════════════════════════════════"
