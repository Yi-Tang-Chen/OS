# 並行、記憶體模型與有界工作佇列

狀態：規劃中，目前只有規格。
環境：Linux/WSL2 userspace，pthread、C11 atomics；TSan 與 ASan 分開建置。
前置：有界佇列、記憶體所有權、partial I/O；先完成 02 的單執行緒版本。

## 要交付什麼

先做mutex+condition variable的正確有界queue，再做SPSC atomic ring，最後比較性能與限制。SPSC不能直接宣稱適用MPSC或MPMC。

## 分層實作

- L1：多producer/consumer、有界queue、while迴圈檢查條件、close廣播喚醒、shutdown與join。
- L2：專用SPSC ring，畫出資料先寫再發布的happens-before關係，使用release/acquire，分析false sharing。
- L3：取消、timed wait、pressure測試、可重現interleaving與benchmark；需MPMC時比較成熟實作，避免無理由自製lock-free。

## 效能與資源

吞吐、p99延遲、CPU時間、context switch、cache miss（工具可用時）、queue occupancy；先固定工作量和CPU配置。

## 第一個動作

先寫queue的close合約和一個等待中被關閉的測試，再選pthread mutex/cond。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| WAKE-01 | consumer等待時close，producer等待滿queue時close | 所有等待者被喚醒；有限時間join，無lost wakeup。 | 待實作 |
| RACE-01 | 多thread總共傳輸一百萬帶序號項目 | 依queue合約無重複／遺失；TSan無診斷但仍需review。 | 待實作 |
| ORDER-01 | SPSC容量2/3/257、資料含序號與checksum | 不可看到尚未寫完的payload；比較relaxed錯誤版本但不執行含data race的負例於正式測試。 | 待實作 |
| LAT-01 | idle、steady、overload與不同batch size | 報吞吐與p99，沒有busy-spin耗盡CPU卻宣稱更快。 | 待實作 |

## 必要閱讀

- [pthread_cond_wait：先理解等待與 mutex](https://man7.org/linux/man-pages/man3/pthread_cond_wait.3.html)。
- [ThreadSanitizer：驗證 userspace data race](https://clang.llvm.org/docs/ThreadSanitizer.html)。

[回主路線](../README.md)
