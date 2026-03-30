#!/usr/bin/env bash
# flash_m33.sh — Flash Cortex-M33 firmware using STM32CubeProgrammer (CLI).
# Usage: ./scripts/flash_m33.sh [path/to/firmware.elf]
set -euo pipefail

ELF="${1:-build/m33/stm32mp257fdk_m33.elf}"
PROGRAMMER="STM32_Programmer_CLI"  # must be on PATH

if ! command -v "$PROGRAMMER" &>/dev/null; then
    echo "ERROR: $PROGRAMMER not found. Install STM32CubeProgrammer and add it to PATH."
    exit 1
fi

if [[ ! -f "$ELF" ]]; then
    echo "ERROR: ELF not found: $ELF"
    echo "Run ./scripts/build_all.sh first."
    exit 1
fi

echo "Flashing $ELF via ST-LINK..."
"$PROGRAMMER" \
    --connect port=SWD freq=8000 reset=HWrst \
    --download "$ELF" \
    --verify \
    --start

echo "Flash complete."
