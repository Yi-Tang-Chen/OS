# 10 教學：理解 CPU 如何從 reset 走到自己的 main

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前沒有 startup、linker script 或 firmware。先選定一個 QEMU Cortex-M machine，只追一條最小啟動路徑。

## 1. 用途與例子

在 Linux 執行 C 程式時，很多啟動工作已由 OS 和 runtime 處理。bare-metal 程式要明確安排 CPU 從哪裡取入口、stack 放哪裡、global variables 如何得到初始值，最後才到 main。

第一個成果可以是 GDB 停在自己的 `Reset_Handler`，再走到 main。接著才讓 UART 印出一個字元。沒有終端輸出時，能用 debugger 證明 CPU 走到哪裡很重要。

## 2. 先認識記憶體與啟動名詞

| 名詞 | 白話意思 | 本專案的問題 |
| --- | --- | --- |
| memory map | CPU 位址對應到 ROM、RAM、周邊的配置 | 哪段可放程式、哪段可寫資料？ |
| vector table | Cortex-M 的初始 stack 與例外處理入口表 | reset 後如何找到 handler？ |
| linker script | 指定程式各部分放在哪裡 | .text/.data/.bss 是否超出範圍？ |
| .text／.rodata | 指令與唯讀資料 | 通常放在程式儲存區 |
| .data | 有非零等初始內容的可寫靜態資料 | 初始值從載入位置複製到 RAM |
| .bss | 啟動時需要清零的靜態資料 | startup 負責清零範圍 |
| MMIO | 用位址存取裝置 register | 有存取寬度、副作用與順序規則 |
| ELF／map | 前者含程式與符號資訊，後者顯示連結配置 | debugger、位址與大小的證據 |

前置：能看懂 C 指標、十六進位、位移，區分區域變數與 static/global 的生命週期。先以表格理解位址，不必第一天讀完整份 ARM 架構手冊。

## 3. 第一個動作：選 machine，做一張資料表

可以以 QEMU 的 `lm3s6965evb`（Cortex-M3）作為學習候選；開始前確認本機 `qemu-system-arm -machine help` 列出它，固定 QEMU 和 arm-none-eabi toolchain 版本。

先讀 [QEMU Stellaris boards](https://www.qemu.org/docs/master/system/arm/stellaris.html) 的模擬硬體清單，再讀 [TI LM3S6965 datasheet](https://www.ti.com/lit/ds/symlink/lm3s6965.pdf) 的 Memory Map、UART 與相關 reset/clock 章節。實體資料表和 QEMU 實作支援範圍要交叉比對，不假設所有周邊細節都被模擬。

在之後新增的 `docs/memory-map.md` 記錄：ROM/flash 起點與大小、SRAM 起點與大小、vector 起點、stack 上界與對齊、UART base、要用的 status/data registers、存取寬度。每個值附來源章節；這張表填完才寫常數。

這就是第一個可交付的小成果。選其他 Cortex-M machine 時，重新查它的 memory map，不能沿用 Stellaris 的地址。

## 4. 建立能連結的最小 firmware

逐步新增：

```text
src/startup.c       vector table、Reset_Handler、default handler
src/main.c          第一版在可觀察位置停住
linker.ld           MEMORY 與 SECTIONS
Makefile            或 CMake，明確指定 cross compiler 與 CPU flags
scripts/run-qemu.sh 使用固定 machine 啟動
docs/memory-map.md
```

這是預定結構；startup 也可以用少量 assembly，但必須知道 C 函式開始前 stack 已如何建立。

1. 先寫 linker MEMORY：依查到的 map 定義 ROM 與 RAM；再決定 vector、text、data、bss、stack 的位置。
2. 保留 vector table section，避免 linker garbage collection 丟掉它。
3. vector 的第一項放初始 stack pointer，第二項放 reset handler，其他必要例外入口先進 default handler。Cortex-M handler 位址與 Thumb 狀態要依工具鏈／架構規則處理。
4. Reset_Handler 初始化必要 runtime 狀態，再呼叫 main。若 main 返回，進明確的停住路徑。
5. 編譯為 Cortex-M3/Thumb ELF，輸出 map。使用自訂 startup 時明訂 runtime／標準庫連結方式；第一版避免 printf/malloc 帶入不明的 syscall 或半套 runtime 需求。

References：[GNU ld Scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)，沿目錄讀 MEMORY、SECTIONS、Input Section and Garbage Collection；[CMSIS startup](https://arm-software.github.io/CMSIS_6/main/Core/startup_c_pg.html)，看 vector 和 Reset_Handler 的責任。CMSIS 範例可能把初始化交給其他 runtime 函式，不能只複製呼叫名稱卻沒連結其實作。

## 5. 第一個執行成果：GDB 停在 reset，再進 main

按選定 machine 的方式把 ELF 載入 QEMU。加入 `-S` 讓 CPU 啟動後先暫停，使用只綁定 loopback 的 GDB server，例如 `-gdb tcp:127.0.0.1:1234`。保存完整 QEMU 命令；machine、console 和 image 路徑都要實際填好。

用支援 ARM 的 GDB 開啟同一份 ELF，再操作：

```text
target remote 127.0.0.1:1234
break Reset_Handler
break main
continue
info registers
```

在停下時核對 PC、SP 與 map。若 CPU 已位於 reset 入口而 breakpoint 未立即觸發，先查看當前 PC 再決定 step/continue；不要把「未印 UART」等同「沒有進 main」。

Reference：[GDB Manual](https://sourceware.org/gdb/current/onlinedocs/gdb.html/) 的 Remote Debugging、Breakpoints、Registers。完成條件：能證明 reset/main 的實際入口與 stack 落在你規劃的範圍內。

## 6. 驗證 .data／.bss 初始化

在 main.c 放一個初值為 7 的 global、一個未明確初始化的 global、一個有初值的 static；進 main 時應分別符合你定義的值與零初始化規則。

linker 提供 data 的 ROM 載入範圍、RAM 執行範圍，以及 bss 的開始／結束符號。startup 複製 data、清零 bss；不要越過 section 邊界，或把 stack/其他保留區當成 bss 清掉。

用 debugger 在初始化前觀察或改成辨認用 pattern，再單步經過初始化，確認是自己的程式完成了 copy/clear。QEMU 的初始 RAM 或 ELF loader 可能剛好讓值看起來正確，單看 main 裡等於 0 不一定證明 startup 正確。

完成後保存 ELF/map 和觀察步驟，再加 UART 輸出驗證值。

## 7. UART、timer IRQ 和主迴圈交接

第一個 UART 動作只送一個字元：按資料表檢查周邊準備、設定與 TX readiness，再以指定寬度寫 data register。先用 polling，確定 console 連到正確 UART 後再做 interrupt。

接著加 timer IRQ，只做短小工作，例如更新計數或投遞事件。主迴圈負責後續處理；ISR 不應做會阻塞的傳送。

register 的 RO、RW、W1C 含義不同：W1C 是寫 1 清除指定狀態，不能無條件套用普通變數的 read-modify-write。volatile 可以用於表達特定 MMIO 存取需求，但不自動提供 atomic、thread/ISR 同步或所有硬體 memory barrier。

共享資料先採用容易證明正確的短 critical section，並依平台規則保留／恢復原本的中斷狀態。之後再考慮原子操作，確認目標工具鏈與硬體支援。

## 8. 最後做 fault 定位與驗收

在獨立測試 firmware 觸發一個可控 fault，保存必要的 stacked PC/LR 和 fault registers，用 ELF symbol/disassembly 定位來源。具體可用的 fault registers 依 Cortex-M 型號查手冊，不假設每種 M 系列都完全相同。

| 現象 | 先查 |
| --- | --- |
| 一開始就 fault | vector 位置、初始 SP、handler 位址與 Thumb、linker 配置 |
| main 的 global 值不對 | data 複製來源／目的、bss 邊界、startup 是否真的執行 |
| 有 main，UART 無輸出 | machine 對應 UART、clock/reset、register offset、console 參數 |
| 加 IRQ 才壞 | vector 編號、acknowledge、stack、共享資料與重入 |

最後對照 README 的 START-01、IRQ-01、FAULT-01。BOARD-01 需要實板、datasheet 與量測工具；QEMU 成果可以證明啟動和軟體資料流程，實體時序和電氣驗證仍須另外完成。
