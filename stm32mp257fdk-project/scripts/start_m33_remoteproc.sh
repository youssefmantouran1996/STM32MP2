#!/usr/bin/env bash
# start_m33_remoteproc.sh — Load and start M33 firmware via Linux remoteproc.
#
# Run this on the STM32MP257F-DK board (or via SSH) while OpenSTLinux is running.
# Usage: ./scripts/start_m33_remoteproc.sh [path/to/firmware.elf]
set -euo pipefail

ELF="${1:-stm32mp257fdk_m33.elf}"
FIRMWARE_NAME="$(basename "$ELF")"
REMOTEPROC_SYS="/sys/class/remoteproc/remoteproc0"
FIRMWARE_DIR="/lib/firmware"

if [[ ! -d "$REMOTEPROC_SYS" ]]; then
    echo "ERROR: remoteproc sysfs not found. Is OpenSTLinux running?"
    exit 1
fi

# Stop if already running
STATE=$(cat "$REMOTEPROC_SYS/state")
if [[ "$STATE" == "running" ]]; then
    echo "Stopping existing M33 firmware..."
    echo stop > "$REMOTEPROC_SYS/state"
    sleep 1
fi

echo "Copying firmware to $FIRMWARE_DIR/$FIRMWARE_NAME..."
cp "$ELF" "$FIRMWARE_DIR/$FIRMWARE_NAME"

echo "Loading firmware: $FIRMWARE_NAME"
echo "$FIRMWARE_NAME" > "$REMOTEPROC_SYS/firmware"

echo "Starting Cortex-M33..."
echo start > "$REMOTEPROC_SYS/state"
sleep 1

STATE=$(cat "$REMOTEPROC_SYS/state")
echo "M33 state: $STATE"

if [[ "$STATE" == "running" ]]; then
    echo "✓ Cortex-M33 is running."
    echo "  RPMsg device: /dev/ttyRPMSG0"
else
    echo "✗ Failed to start M33. Check dmesg for errors."
    exit 1
fi
