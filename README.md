# Embedded Systems Projects

C and Linux systems projects, with RTOS and device-driver work planned alongside them.

Project 01 contains a ring buffer implementation and unit tests. Projects 02–12 currently contain specifications; source code and results will be added as they are implemented.

| Project | Focus | Current contents |
| --- | --- | --- |
| [01 · C Toolkit](01-embedded-c-toolkit/README.md) | Ring buffer, CRC, streaming parser | Ring buffer and tests; CRC/parser planned |
| [02 · Linux Service](02-linux-sensor-simulator/README.md) | Nonblocking I/O, bounded queues, reconnects | Specification |
| [03 · Zephyr](03-zephyr-native-sim/README.md) | Threads, queues, telemetry | Specification |
| [04 · Buildroot](04-buildroot-qemu-image/README.md) | Linux image, U-Boot, startup | Specification |
| [05 · Kernel Driver](05-linux-kernel-driver/README.md) | FIFO device, locking, lifecycle | Specification |
| [06 · Yocto](06-yocto-layer/README.md) | Layer, recipes, image, SDK | Specification |
| [07 · A/B Update](07-ota-update-rollback/README.md) | Signed updates and rollback | Specification |
| [08 · Open-source](08-open-source-contribution/README.md) | Reproducer, patch, regression | Specification |
| [09 · Concurrency](09-concurrency-memory-model/README.md) | Bounded queue, pthreads, atomics | Specification |
| [10 · Bare-metal](10-baremetal-qemu-bringup/README.md) | Startup, linker, interrupts | Specification |
| [11 · Debugging](11-debug-performance-lab/README.md) | GDB, sanitizers, system experiments | Specification |
| [12 · PCI/DMA](12-pci-dma-lab/README.md) | QEMU EDU, MMIO, IRQ, DMA | Specification |

## Build and test

Requires CMake and a C compiler. Run from the repository root:

```sh
cmake -S 01-embedded-c-toolkit -B 01-embedded-c-toolkit/build -DCMAKE_BUILD_TYPE=Debug
cmake --build 01-embedded-c-toolkit/build --config Debug
ctest --test-dir 01-embedded-c-toolkit/build -C Debug --output-on-failure
```

Use a separate directory such as `build-wsl` when switching between Windows and WSL.

Next: CRC/parser in 01, then the Linux service in 02. Each project README describes its scope and acceptance criteria.
