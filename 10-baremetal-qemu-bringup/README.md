# 從 reset 到 main 的 bare-metal bring-up

狀態：規劃中，目前只有規格。
環境：固定一個QEMU支援的Cortex-M machine和arm-none-eabi toolchain；後續需真板補驗證。
前置：位元操作、整數範圍、計時器；理解 pointer、stack 與儲存期。

## 要交付什麼

在模擬MCU上完成vector table、linker script、startup、.data/.bss初始化、UART輸出與timer IRQ。只實作學習所需啟動碼，不宣稱完整OS或硬體bring-up。

## 分層實作

- L1：查machine記憶體map、linker MEMORY/SECTIONS、reset handler、.data copy、.bss清零，驗證進入main前狀態。
- L2：IRQ與主迴圈交接資料、volatile MMIO存取邊界、timer、UART環形buffer，避免ISR內阻塞。
- L3：fault handler保存stacked PC/LR等context、用elf/map/disassembly定位；真板再補DMA/cache、watchdog與低功耗。

## 效能與資源

image各section大小、stack使用界線、IRQ工作量、boot步驟；真板量latency/power時記錄儀器與頻率。

## 第一個動作

選定QEMU machine和版本，先讀memory map，寫最小startup讓main輸出一行UART。

## 真板時補的最小能力

從 datasheet 的 memory map、clock tree、pinmux 開始；用 UART／I2C／SPI 做一個可量測傳輸。區分 RO、RW、W1C register，說明為何不能一律用 read-modify-write。用 JTAG／SWD 與邏輯分析儀定位一次實際問題。ISR 與主迴圈交接須有同步設計，volatile 不等於 atomic 或 memory barrier。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| START-01 | 初始化global、有初值static、未初始化global及stack sentinel | reset後值符合預期；ELF/map可追到地址。 | 待實作 |
| IRQ-01 | timer遞增與主迴圈消費；故意壓滿UARTbuffer | 不阻塞ISR，丟棄可量測，重入／共享變數策略有說明。 | 待實作 |
| FAULT-01 | 在獨立guest觸發可控制的非法存取 | 保存fault資訊，用GDB定位到來源；不是只印HardFault。 | 待實作 |
| BOARD-01 | 移到實板檢查clock tree、pinmux、I2C/SPI波形、DMA/cache | 此項拿到板子才可勾完成；QEMU不能代替電氣或時序驗證。 | 待實作 |

## 必要閱讀

- [GNU ld：搜尋 MEMORY、SECTIONS](https://sourceware.org/binutils/docs/ld/Scripts.html)。
- [QEMU Arm：確認 machine 支援範圍](https://www.qemu.org/docs/master/system/target-arm.html)。

[回主路線](../README.md)
