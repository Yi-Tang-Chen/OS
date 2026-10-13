# 05 教學：讓 userspace 透過裝置檔案存取核心 FIFO

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前沒有 kernel module。所有載入、卸載和故障實驗都在 04 建立的獨立 QEMU guest 裡進行；先把 guest 的重建與恢復方式跑通。

## 1. 用途與具體例子

02 的服務透過 read/write 存取 socket；driver 則讓 userspace 用類似介面存取核心管理的裝置。

第一個裝置可以完全由軟體模擬：你建立 `/dev/demo_fifo`，程式寫入 `10 20 30`，之後讀取得到相同 bytes。它在核心內暫存資料，並決定空／滿、等待、錯誤與關閉行為。先學會這些，再接真硬體。

一次 write 的邊界不一定保留在下一次 read；初版定義為共享 byte FIFO，多個 reader 會分走資料，不是每個 reader 各收到一份副本。

## 2. 先補的觀念

| 名詞 | 意思 | 在此練習中的作用 |
| --- | --- | --- |
| userspace／kernel space | 應用程式與核心執行的不同環境 | driver 不能直接把 user pointer 當普通核心 pointer 使用 |
| module | 可以載入核心的程式碼 | 建置產物通常是 `.ko` |
| character device | 透過檔案操作提供 byte-oriented 介面的裝置 | userspace 使用 open/read/write |
| file_operations | 一組核心會呼叫的函式指標 | 把 read/write 請求交給你的實作 |
| wait queue | 條件不成立時等待，條件可能成立時喚醒 | FIFO 空時等資料 |
| lifetime／ownership | 資源什麼時候建立、誰還能使用、何時才能釋放 | 避免 remove 後還有人讀到已釋放記憶體 |

先能解釋 02 的 fd、EOF、O_NONBLOCK 和短讀短寫。進第 6 節前，先讀 [09 第 2、4、5 節](../09-concurrency-memory-model/LEARNING.md) 的共同狀態、mutex、條件等待；userspace mutex 與 kernel API 不同，但「鎖保護哪個條件」的問題相通。

## 3. 第一個成果：在 guest 載入和卸載空 module

先讀 [Building External Modules](https://docs.kernel.org/kbuild/modules.html) 的 Introduction、How to build external modules。核心版本、組態和建置輸出必須與 guest 正在跑的核心相符；若啟用 symbol versioning，可能需要完整建置產出的符號資訊，只有一份隨便下載的 headers 不夠。

之後在本專案新增：

```text
module/demo_fifo.c   第一版只有初始化／結束訊息
module/Makefile      使用 kernel kbuild 的 obj-m 設定
tests/              之後放 userspace 測試
docs/abi.md         之後寫 read/write 行為合約
notes/environment.md
```

1. 記錄 guest 的 `uname -r`、架構、對應 kernel commit/config。
2. 寫 module 初始化與結束函式，透過核心日誌印出固定訊息。這裡使用核心 API，不連結 libc，不用 printf/malloc。
3. 按 kbuild 文件建置；可在有完整開發環境的 guest 建，或在 host 用正確 target toolchain 建，再傳進 guest。
4. 在 guest 載入 `.ko`，讀 kernel log；卸載，再看結束訊息。
5. 重複一次，確認你知道操作發生在哪個系統。

完成條件只是載入／卸載與 log 可重現，還不是可讀寫裝置。`Invalid module format` 時先比對 kernel release、架構與符號版本，不要先改 FIFO 邏輯。

## 4. 註冊裝置，先只完成 open/close

使用 [Miscellaneous Devices](https://docs.kernel.org/driver-api/misc_devices.html) 的 miscdevice 作為入門，減少初期管理裝置號的工作。先研究 `misc_register`、`misc_deregister`，以及 file_operations 的 owner。

1. 註冊一個有明確名字的裝置，設定 `.owner = THIS_MODULE` 等必要欄位。
2. 讓 open/release 可以被觀察到，再用小型 userspace 程式開啟與關閉。
3. 若 `/dev` 沒有節點，檢查 guest 的 devtmpfs／裝置管理設定、註冊結果和 major/minor，不要把路徑不存在直接認定為 callback 錯誤。
4. 用非 root 帳號測試前，先明訂裝置節點的群組／權限；不要把「sudo 才能跑」當作介面規則。

完成條件：open 成功、close 後 release 可追蹤；失敗路徑清理已註冊資源。註冊成功前，不應讓使用者看見半初始化的狀態。

## 5. 先寫合約，再寫 FIFO

在 `docs/abi.md` 決定以下行為，先寫單讀者／單寫者的版本：

| 情境 | 入門合約建議 |
| --- | --- |
| count 為 0 的 read/write | 回傳 0，不改變 FIFO |
| FIFO 有資料，read 要得更多 | 回傳實際讀到的 byte 數 |
| FIFO 空，nonblocking read | 回傳 `-EAGAIN`，不把暫時空誤當永久 EOF |
| FIFO 滿，nonblocking write | 回傳 `-EAGAIN` |
| 有部分空間 | 寫入可容納部分，回傳實際 byte 數 |
| usercopy 全部失敗 | 沒有進展時回 `-EFAULT` |
| usercopy 部分成功 | 依實際進展回報，不能重複交付或丟掉已承諾的資料 |

先選核心提供的 kfifo 或一個明確受鎖保護的固定陣列。01 可以幫助理解 FIFO，但核心有自己的型別、配置與同步規則，不是把 userspace library 原封不動放進去就完成。

References：[Kernel API](https://docs.kernel.org/core-api/kernel-api.html) 搜尋 kfifo，讀初始化、容量與 usercopy helpers；[Driver basics](https://docs.kernel.org/driver-api/basics.html) 查使用到的 API。`copy_to_user`／`copy_from_user` 的回傳是未複製數量，不能把它直接當已複製數量或一般 -errno。還要查 helper 適用的鎖和 context。

## 6. 加入阻塞等待與正確關閉

第一次只做 nonblocking 行為並驗證資料。之後才加入「空時睡眠等資料、滿時睡眠等空間」。

流程要圍繞條件設計：檢查 FIFO 狀態；條件不成立就等待；被喚醒後重新檢查；條件成立才移動資料。喚醒不代表資料一定仍在，另一個 reader 可能先取走。

新增等待時，同時定義停止條件。device 進入 removing 狀態後拒絕新操作，讓已等待的呼叫醒來並回傳合約指定的結果，再等仍在使用資源的引用結束。模組引用與可移除 device 的 lifetime 是兩個不同層次。

先讀 [Kernel Locking](https://docs.kernel.org/kernel-hacking/locking.html) 的睡眠、mutex/spinlock 相關段落；不要持 spinlock 呼叫可能睡眠的 usercopy。可用 mutex 序列化初版 process-context 操作，但等待資料時要讓其他操作能取得鎖並改變條件。`wait_event*` 的用法查 [Driver basics](https://docs.kernel.org/driver-api/basics.html)。

接著才加入 poll readiness，讓 02 式事件迴圈可使用此裝置；ioctl 只在確有 read/write 無法清楚表达的控制需求時新增，並定義 UAPI。

## 7. 一步一個 userspace 測試

1. open、寫入 3 bytes、讀回比較；不依賴 echo/cat 對換行與 EOF 的假設。
2. 非阻塞空讀、滿寫，檢查 errno。
3. 小 buffer 分次讀／寫，核對總 byte 數與順序。
4. 一個 reader 等待時，另一個 writer 送資料，驗證能喚醒。
5. 等待中收到 signal，驗證中斷行為與資源回收。
6. 多 process 傳帶序號資料，按共享 FIFO 合約核對；再測 fd 未關閉時卸載的結果、獨立 device removal 流程。

KASAN、lockdep 和 fault injection 是後續檢查工具；先有明確輸入／輸出測試，再啟用相符的 kernel config。每個測試設 deadline，失敗保存 userspace 結果與 kernel log。

## 8. 之後的硬體延伸與排錯

FIFO 正確後，才做 Device Tree matching：教學節點匹配 platform driver，觀察 probe/remove；不要替不存在的硬體編造 MMIO 位址。真正 I2C sensor 使用 i2c_driver，按資料表讀 ID／register，硬體未到位就保留待實板狀態。

References：[Platform driver](https://docs.kernel.org/driver-api/driver-model/platform.html) 讀 matching/probe；[I2C clients](https://docs.kernel.org/i2c/writing-clients.html) 讀 driver 宣告與 probe/remove。PCI/MMIO/DMA 接到 [12 教學](../12-pci-dma-lab/LEARNING.md)。

卡住時先問：是 `.ko` 不能載入、節點不存在、open 失敗、read/write 內容錯誤，還是等待永不結束？用最小一個 process 重現該層，再增加並行。最後回 README 驗收 ABI-01、RACE-01、LIFE-01、ALLOC-01。
