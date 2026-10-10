# 可重現的除錯、效能與核心OS實驗

狀態：規劃中，目前只有規格。
環境：Linux/WSL2；perf或kernel功能受限時記錄限制，可用独立VM補測。
前置：02有可跑的服務；09有同步基礎。

## 要交付什麼

把自己專案中的真bug整理成debug dossier：重現、觀察、假設、證據、修正、回歸。補足process/thread、virtual memory、檔案持久性和cache行為。

## 分層實作

- L1：GDB breakpoint/watchpoint/backtrace/core dump、ASan/UBSan、strace，為三個不同故障建立最小reproducer。
- L2：用clock_gettime與可用時的perf量baseline，定位copy、allocation、syscall、cache成本；每次只改一個因素。
- L3：fork/exec/fd繼承、mmap/page fault、記憶體生命週期、fsync/rename/crash recovery實驗；結果與假設不符也需記錄。

## 效能與資源

每份報告包含原始log、二進位/commit、編譯旗標、CPU與輸入，拒絕只放截圖或一句『快了很多』。

## 第一個動作

從自己已修好的bug挑一個，重建最小錯誤版本與回歸測試，練習直接讀 backtrace。

## 把驗證做成工具

用 Python／shell 為 02、04 或 12 寫一個 runner：設定測試參數、啟動程序、timeout、保存 stdout／stderr 與退出碼、輸出 CSV。一次只選一個專案整合。

搭配實驗補電腦組成：virtual／physical address、page fault、cache line、alignment、endianness；DMA address／ownership 接 12。能解釋觀察結果與限制，比多列工具名稱有用。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| DBG-01 | 越界、use-after-free、死鎖各一個可控reproducer | 工具輸出能定位到來源與原因；UB負例在隔離測試process執行並有timeout。 | 待實作 |
| VM-01 | 同一映射冷／熱讀取，記錄page fault與時間 | 能區分page fault、cache miss與磁碟I/O，不用單一指標混稱。 | 待實作 |
| FD-01 | fork/exec前後檢查FD_CLOEXEC、pipe EOF | 只關一端不一定EOF；子process不意外持有fd。 | 待實作 |
| PERF-01 | 前後版本用相同資料各跑至少10次 | 報median/p95及環境，修正需保留同等正確性測試。 | 待實作 |

## 必要閱讀

- [GDB：先讀 backtrace、watchpoint](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)。
- [strace：看 syscall 與錯誤碼](https://man7.org/linux/man-pages/man1/strace.1.html)。

[回主路線](../README.md)
