# 具資源上限的 RTOS telemetry pipeline

狀態：規劃中，目前只有規格。
環境：Linux/WSL2 + 鎖定版本的 Zephyr native_sim；模擬結果不代表真板 timing。
前置：01 的純 C library、函式指標、環形佇列與 unsigned 計時；同步概念見 09。

## 要交付什麼

週期性取樣、處理、傳送三個 thread，以 message queue 連接；把控制面、資料面、diagnostics 分離。

## 分層實作

- L1：Kconfig、devicetree overlay、thread priorities、queue 與 timer；將 fake sensor 放在 driver interface 後方。
- L2：優先序反轉可重現案例、mutex 與 semaphore 使用理由、stack 使用量、queue high-water mark、watchdog 狀態機。
- L3：ztest/Twister 測試矩陣、測試不同設定／優先序、故障注入、取消與關閉；搬板時再測 ISR latency 和低功耗。

## 效能與資源

模擬時間上的 deadline miss、queue 最大深度、thread stack high-water、資料遺失率。真實抖動與功耗列為 board-only 未完成項。

## 第一個動作

先建好 hello_world 與一個 ztest，鎖定 west manifest，再加兩個 thread 和有界 queue。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| Q-01 | producer 比 consumer 快 10 倍 | queue 不無限增加，丟棄策略有計數，控制命令仍可處理。 | 待實作 |
| RT-01 | 高優先序等待低優先序持有的 mutex，另有中優先序工作 | 用事件 trace 解釋阻塞與 priority inheritance；不把 native_sim host wall-clock 當真實 WCET。 | 待實作 |
| ERR-01 | fake driver 連續 timeout／invalid sample | watchdog 不因單個壞 sample 錯誤重啟；真正無進展可被偵測。 | 待實作 |
| CFG-01 | 至少三組 queue depth／stack size／sampling period | ztest 可重現，記錄不支援的配置和原因。 | 待實作 |

## 必要閱讀

- [Zephyr threads：priority 與 scheduling](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)。
- [native_sim：先了解模擬限制](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html)。

[回主路線](../README.md)
