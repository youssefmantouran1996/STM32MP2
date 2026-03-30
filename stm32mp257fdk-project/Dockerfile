# Cross-compilation environment for STM32MP257FDK
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    # Cortex-M33 toolchain
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    # Cortex-A35 / aarch64 toolchain
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    # Build tools
    cmake \
    ninja-build \
    make \
    # Code quality
    clang-format \
    # Utilities
    git \
    curl \
    wget \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
