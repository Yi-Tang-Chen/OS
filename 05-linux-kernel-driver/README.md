# 生命週期與並行安全的 character driver

狀態：規劃中，目前只有規格。
環境：獨立 QEMU guest 中的可重建 Linux kernel；不直接在工作主機載入練習模組。
前置：02 的 userspace I/O；09 的 locking 與 ownership。

## 要交付什麼

從軟體 FIFO character device 起步，逐步加入 read/write/poll/ioctl 的 UAPI、wait queue、per-open state 與 teardown。核心內部單元測試和 userspace 合约測試分開。

## 分層實作

- L1：最小 cdev/miscdevice、open/release、固定容量 FIFO；錯誤碼、blocking/nonblocking、partial copy 規則先寫入 UAPI。
- L2：多讀者／多寫者、wait_event、poll readiness、signal 中斷、lock ordering、引用計數；不要持 spinlock 做可能睡眠的工作。
- L3：KUnit/kselftest、KASAN/lockdep/KCSAN 按需求分開設定，failslab/fail_usercopy 故障注入；模擬 device removal 並驗證引用生命週期。

## 效能與資源

syscall 延遲、吞吐、等待時間、contention；tracepoint/ftrace 記錄觸發条件。不能只用 dmesg 沒錯就當完成。

## 第一個動作

固定 guest kernel commit，先寫一個單讀者單寫者FIFO與使用者端測試；暫不加入真實硬體介面。

## 補強：Device Tree 與硬體 driver

先做上面的 FIFO，之後完成兩個小階段，避免一開始同時處理硬體與多執行緒。

1. **QEMU matching 練習**：在 ARM guest 的 DT 加入明確標示為教學用的邏輯節點，以 of_match_table 匹配最小 platform driver，驗證 probe／remove。這個節點沒有真硬體，不捏造 MMIO 位址或 IRQ；用 compatible 不匹配、status disabled 檢查不應 probe 的情境。
2. **實板 I2C sensor**：選有公開 datasheet、主線 driver 可讀的感測器，先讀既有 driver 再做自己的最小練習；確認 address、chip ID、register 權限、端序、單位與 timeout。I2C client 使用 i2c_driver，不把它誤寫成 platform driver。依裝置需求接 GPIO IRQ／regulator／reset；讀 clock 與 pinctrl 如何由控制器或平台管理。

驗收：正常讀值、錯誤 ID／位址、傳輸失敗、反覆 bind／unbind；probe 中途失敗沒有資源洩漏，remove 能停止工作並喚醒等待者。硬體未到位時第二階段標「待實板」。後續依裝置類型接 IIO 等標準 subsystem。

MMIO／interrupt／DMA 的可跑模擬練習接 [12 PCI／DMA](../12-pci-dma-lab/README.md)，不在這裡再開一套重複專案。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| ABI-01 | 零長度、O_NONBLOCK、invalid user pointer、signal 中斷 | 回傳值符合文件，partial copy 已移動的資料不可重複或憑空遺失。 | 待實作 |
| RACE-01 | 多 process 同時讀寫並驗證資料序號 | 無遺失／重複（依共享FIFO合約）、無 lockdep/KASAN 診斷。 | 待實作 |
| LIFE-01 | fd 未關閉時嘗試 rmmod；另測可移除device | 模組引用阻止不合法卸載，device teardown 喚醒等待者，最後引用才釋放。 | 待實作 |
| ALLOC-01 | 第 n 次配置失敗、usercopy fault | 已取得資源按所有權釋放；無 use-after-free 或未初始化資料洩露。 | 待實作 |

## 必要閱讀

- [Platform driver：matching、probe 與 resources](https://docs.kernel.org/driver-api/driver-model/platform.html)。
- [Device Tree usage：compatible、reg、interrupts](https://docs.kernel.org/devicetree/usage-model.html)。
- [I2C driver：之後搬到實板時讀](https://docs.kernel.org/i2c/writing-clients.html)。

[回主路線](../README.md)
