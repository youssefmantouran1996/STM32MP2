# STM32MP257FDK — C/C++ Development Project

A dual-target C/C++ project template for the **STM32MP257F-DK Discovery Kit**, targeting both processing units of the STM32MP257 MPU.

## Architecture Overview

```
┌──────────────────────────────────────────────────────┐
│              STM32MP257FAK3 MPU                      │
│                                                      │
│  ┌─────────────────────┐   ┌──────────────────────┐ │
│  │  Cortex-A35 (×2)    │   │    Cortex-M33        │ │
│  │  1.5 GHz, 64-bit    │◄──►│  400 MHz, 32-bit    │ │
│  │  OpenSTLinux        │   │  STM32CubeMP2        │ │
│  │  C/C++ userspace    │   │  Bare-metal / RTOS   │ │
│  │  (cortex-a35/)      │   │  (cortex-m33/)       │ │
│  └─────────────────────┘   └──────────────────────┘ │
│         OpenAMP / RPMsg IPC  (shared/ipc/)           │
└──────────────────────────────────────────────────────┘
```

## Repository Structure

```
stm32mp257fdk-project/
├── cortex-a35/               # Linux userspace C++ application (Cortex-A35)
│   ├── app/
│   │   ├── src/
│   │   │   ├── main.cpp      # Application entry point
│   │   │   └── ipc_rpmsg.cpp # RPMsg IPC client (talks to M33)
│   │   ├── include/
│   │   │   └── ipc_rpmsg.h
│   │   └── tests/            # Unit tests
│   ├── cmake/
│   │   └── toolchain-aarch64.cmake
│   └── CMakeLists.txt
│
├── cortex-m33/               # Bare-metal firmware (Cortex-M33)
│   ├── Core/
│   │   ├── Src/
│   │   │   ├── main.c                # Firmware entry point
│   │   │   ├── stm32mp2xx_hal_msp.c  # HAL MSP init
│   │   │   └── stm32mp2xx_it.c       # IRQ handlers
│   │   └── Inc/
│   │       ├── main.h
│   │       ├── stm32mp2xx_hal_conf.h
│   │       └── stm32mp2xx_it.h
│   ├── Drivers/              # STM32CubeMP2 HAL (add via CubeMX or manually)
│   │   ├── STM32MP2xx_HAL_Driver/
│   │   └── CMSIS/
│   ├── Middlewares/
│   │   └── OpenAMP/          # OpenAMP middleware for IPC
│   ├── cmake/
│   │   └── toolchain-m33.cmake
│   ├── STM32MP257FAIX_FLASH.ld  # Linker script
│   └── CMakeLists.txt
│
├── shared/                   # Shared definitions (IPC channels, resource table)
│   └── ipc/
│       ├── resource_table.h  # OpenAMP resource table
│       └── virt_uart.h       # Virtual UART channel definitions
│
├── scripts/
│   ├── build_all.sh
│   ├── flash_m33.sh
│   └── start_m33_remoteproc.sh
│
├── .github/workflows/ci.yml  # GitHub Actions CI pipeline
├── Dockerfile                # Cross-compilation environment
├── CMakeLists.txt            # Top-level CMake
├── .clang-format
└── .gitignore
```

## Prerequisites

| Tool | Purpose | Install |
|------|---------|---------|
| `arm-none-eabi-gcc` ≥ 12 | Cortex-M33 cross-compiler | `sudo apt install gcc-arm-none-eabi` |
| `aarch64-linux-gnu-gcc` ≥ 12 | Cortex-A35 cross-compiler | `sudo apt install gcc-aarch64-linux-gnu` |
| `cmake` ≥ 3.25 | Build system | `sudo apt install cmake` |
| `ninja-build` | Fast build backend | `sudo apt install ninja-build` |
| STM32CubeIDE | IDE + CubeMX | [st.com](https://www.st.com/en/development-tools/stm32cubeide.html) |
| STM32CubeProgrammer | Flashing | [st.com](https://www.st.com/en/development-tools/stm32cubeprog.html) |
| OpenOCD (ST fork) | Debugging | [device-stm-openocd](https://github.com/STMicroelectronics/OpenOCD) |

> **STM32CubeMP2 Drivers**: Download from [st.com](https://www.st.com/en/embedded-software/stm32cubemp2.html) and copy HAL drivers into `cortex-m33/Drivers/`.

## Quick Start

```bash
git clone https://github.com/YOUR_ORG/stm32mp257fdk-project.git
cd stm32mp257fdk-project
./scripts/build_all.sh
```

Or build individually:

```bash
# Cortex-M33 firmware
cmake -B build/m33 -S cortex-m33 \
  -DCMAKE_TOOLCHAIN_FILE=cortex-m33/cmake/toolchain-m33.cmake -G Ninja
cmake --build build/m33

# Cortex-A35 Linux app (cross-compiled for aarch64)
cmake -B build/a35 -S cortex-a35 \
  -DCMAKE_TOOLCHAIN_FILE=cortex-a35/cmake/toolchain-aarch64.cmake -G Ninja
cmake --build build/a35
```

## Deploying to the Board

### Flash / load the M33 firmware
```bash
# Via STM32CubeProgrammer (board in DFU / JTAG)
./scripts/flash_m33.sh build/m33/stm32mp257fdk_m33.elf

# Or load at runtime via Linux remoteproc (board must be running OpenSTLinux)
./scripts/start_m33_remoteproc.sh build/m33/stm32mp257fdk_m33.elf
```

### Deploy the A35 Linux application
```bash
scp build/a35/stm32mp257fdk_app root@<BOARD_IP>:/usr/local/bin/
ssh root@<BOARD_IP> /usr/local/bin/stm32mp257fdk_app
```

## Debugging the Cortex-M33

1. Start the firmware via remoteproc on the board
2. Launch OpenOCD: `openocd -f board/stm32mp257f-dk.cfg`
3. Attach GDB:
```bash
arm-none-eabi-gdb build/m33/stm32mp257fdk_m33.elf \
  -ex "target remote :3333"
```

## IPC: Cortex-A35 ↔ Cortex-M33

Both cores communicate via **OpenAMP / RPMsg** over a shared DDR region defined in `shared/ipc/resource_table.h`. On the Linux side the virtual UART appears as `/dev/ttyRPMSGx`. See `cortex-a35/app/src/ipc_rpmsg.cpp` and `cortex-m33/Middlewares/OpenAMP/` for implementation details.

## Using Docker

```bash
docker build -t stm32mp257-builder .
docker run --rm -v $(pwd):/workspace stm32mp257-builder ./scripts/build_all.sh
```

## References

- [STM32MP257F-DK Product Page](https://www.st.com/en/evaluation-tools/stm32mp257f-dk.html)
- [STM32MPU Wiki](https://wiki.st.com/stm32mpu)
- [STM32CubeMP2 Firmware Package](https://www.st.com/en/embedded-software/stm32cubemp2.html)
- [OpenSTLinux Distribution](https://www.st.com/en/embedded-software/openstlinux-distribution.html)
- [OpenAMP Project](https://github.com/OpenAMP/open-amp)
- [Zephyr on STM32MP257F-DK](https://docs.zephyrproject.org/latest/boards/st/stm32mp257f_dk/doc/index.html)

## License

MIT License — see [LICENSE](LICENSE).
