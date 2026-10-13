# Embedded Systems Projects

C and Linux systems projects, with RTOS and device-driver work planned alongside them.

Project 01 contains a ring buffer implementation and unit tests. Projects 02–12 currently contain specifications and learning guides; source code and results will be added as they are implemented.

| Project | Focus | Current contents |
| --- | --- | --- |
| [01 · C Toolkit](01-embedded-c-toolkit/README.md) | Ring buffer, CRC, streaming parser | Ring buffer, tests and learning guide; CRC/parser planned |
| [02 · Linux Service](02-linux-sensor-simulator/README.md) | Nonblocking I/O, bounded queues, reconnects | Specification and learning guide |
| [03 · Zephyr](03-zephyr-native-sim/README.md) | Threads, queues, telemetry | Specification and learning guide |
| [04 · Buildroot](04-buildroot-qemu-image/README.md) | Linux image, U-Boot, startup | Specification and learning guide |
| [05 · Kernel Driver](05-linux-kernel-driver/README.md) | FIFO device, locking, lifecycle | Specification and learning guide |
| [06 · Yocto](06-yocto-layer/README.md) | Layer, recipes, image, SDK | Specification and learning guide |
| [07 · A/B Update](07-ota-update-rollback/README.md) | Signed updates and rollback | Specification and learning guide |
| [08 · Open-source](08-open-source-contribution/README.md) | Reproducer, patch, regression | Specification and learning guide |
| [09 · Concurrency](09-concurrency-memory-model/README.md) | Bounded queue, pthreads, atomics | Specification and learning guide |
| [10 · Bare-metal](10-baremetal-qemu-bringup/README.md) | Startup, linker, interrupts | Specification and learning guide |
| [11 · Debugging](11-debug-performance-lab/README.md) | GDB, sanitizers, system experiments | Specification and learning guide |
| [12 · PCI/DMA](12-pci-dma-lab/README.md) | QEMU EDU, MMIO, IRQ, DMA | Specification and learning guide |

## 從哪裡開始學

請先讀 [學習入口與 01～12 教學索引](LEARNING.md)。每個專案都有 `LEARNING.md`，依用途、具體例子、必要概念、小實作、驗證與排錯帶你前進，並附官方參考資料的閱讀指引。

目前從 [01 教學：看懂 assert 與第一個測試](01-embedded-c-toolkit/LEARNING.md) 開始即可。各 README 是完整規格；教學中的待建檔案和預期結果不代表程式已實作或驗收通過。

## Build and test

Requires CMake and a C compiler. Run from the repository root:

```sh
cmake -S 01-embedded-c-toolkit -B 01-embedded-c-toolkit/build -DCMAKE_BUILD_TYPE=Debug
cmake --build 01-embedded-c-toolkit/build --config Debug
ctest --test-dir 01-embedded-c-toolkit/build -C Debug --output-on-failure
```

Use a separate directory such as `build-wsl` when switching between Windows and WSL.

Next: CRC/parser in 01, then the Linux service in 02. Each project README describes its scope and acceptance criteria.
