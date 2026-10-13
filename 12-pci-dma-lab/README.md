# PCI／DMA Driver Lab

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先讓 guest 的 lspci 看見 EDU，再依序完成 probe/remove、MMIO、IRQ、DMA，每個階段分開驗證。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

狀態：待實作。目標是理解硬體資料如何進入記憶體，作為 PCIe／網卡／儲存／加速器 driver 的共通基礎。
前置：[05 的 FIFO／生命週期](../05-linux-kernel-driver/README.md)、[09 的 mutex／同步](../09-concurrency-memory-model/README.md)。環境：Linux／WSL2 上的獨立 QEMU guest；固定 kernel、QEMU、machine 與 toolchain。guest 必須支援 PCI 與 EDU；可用 x86_64 q35，與 04 的 ARM boot 實驗分開保存設定。

## 只做一條資料路徑

userspace 測試 → driver → BAR MMIO 啟動 EDU → DMA → IRQ 完成通知 → driver → userspace 比對。

| 階段 | 要做什麼 | 完成標準 |
| --- | --- | --- |
| 1：MMIO | 在既有 guest 啟動參數加入 -device edu；辨識 1234:11e8，寫 pci_driver、probe/remove、取得 BAR0，用 readl/writel 存取 liveness register | 反相回讀符合規格；probe 失敗釋放已取得資源 |
| 2：IRQ | 先 INTx，再選做 MSI；request handler、辨識並 acknowledge 中斷、喚醒等待者 | 多次操作完成；等待有 timeout；不在 IRQ 中睡眠 |
| 3：DMA | 設定符合 EDU 的 DMA mask，配置 coherent buffer；主記憶體 → 裝置 buffer → 主記憶體，逐 byte 比對 | 合法長度資料一致；拒絕越界；完成後才交回 CPU／使用者 |
| 4：可靠性 | 序列化傳輸；處理等待者取消、timeout、remove、配置失敗；用 runner 保存結果 | 未確認 DMA 停止前不釋放 buffer；失敗有明確狀態且不會永久卡住 |

第一步只驗證 lspci 看得到 EDU，保存可重跑的 QEMU 命令；此文件不是已驗證的建置腳本。
EDU 預設 DMA mask 為 28 bit，裝置 buffer 為 4096 bytes，IRQ 支援 INTx／MSI；沒有 MSI-X，不把它列為本專案必做。
EDU 不提供任意中止 DMA 的通用保證：timeout 只表示等待超時。需依規格等待 in-flight 傳輸結束／確認 quiesce 後才清理；無法確認時保留資源並報錯，由專用 guest 重啟復原。

## 要真正理解的五件事

- CPU virtual、physical、dma_addr_t 的差別；不把 C pointer 直接填進 DMA register。
- coherent allocation 與 streaming mapping 的差別；coherent 不表示可以忽略存取順序。
- buffer 何時由 CPU／device 擁有，何時可讀、修改、釋放。
- interrupt 是完成通知；spinlock、mutex 與 memory barrier 各解決什麼問題。
- probe／remove 的資源生命週期；先停止來源與工作，再釋放仍可能被使用的資源。

## 最小驗收

- [ ] 按 register 規格驗證 MMIO 存取寬度、liveness、IRQ acknowledge。
- [ ] DMA round-trip 測 1、64、4096 bytes；API 拒絕 0 與 4097 bytes；實際邊界依 EDU 規格處理。
- [ ] 使用不同固定 pattern 重跑 1000 次，記錄 mismatch、timeout、完成數；數量是練習預算。
- [ ] 測等待期間取消與 unbind，確認仍有 DMA 時 buffer 不會提早釋放；KASAN／lockdep 依可用環境檢查。
- [ ] runner 有退出碼、timeout、原始 log、CSV；量不同 payload 的 guest 延遲，清楚標示模擬條件。

## 必要閱讀（依序）

1. [QEMU EDU 規格](https://www.qemu.org/docs/master/specs/edu.html)：只先讀 PCI specs、MMIO、IRQ、DMA；這就是練習的 datasheet。
2. [Linux PCI driver](https://docs.kernel.org/PCI/pci.html)：先讀初始化、資源與移除流程；API 以鎖定 kernel 的原始碼為準。
3. [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)：先讀位址、DMA mask、coherent 與 streaming mapping。

## 之後如何接高速介面工作

本專案證明 PCI／DMA 軟體流程，不證明實體 PCIe 速率、link training、AER、PHY 或訊號品質。
下一步依職缺選一種實體介面：PCIe 看 BAR／MSI／DMA／reset；USB 看 endpoint／URB／拔除；Ethernet 看 TX/RX ring／NAPI／MAC-PHY。
不要一次開三條線。PHY／SerDes／Retimer 韌體另需 link 狀態、調校與量測；QEMU 不替代這些經驗。

[回主路線](../README.md)
