# 12 教學：把裝置 register、中斷與 DMA 串成一條資料路徑

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚無 PCI driver 或 EDU guest 設定。先完成 05 的資源生命週期與 09 的同步入門，使用支援 PCI 的獨立 QEMU guest。

## 1. 用途與具體例子

CPU 可以向裝置 register 寫入命令，讓裝置把一段資料搬到記憶體，完成後用中斷通知 driver。這條路徑會出現在網卡、儲存與其他裝置驅動中。

這裡用 QEMU 的 EDU 教學裝置：userspace 準備固定 pattern，driver 把它交給裝置，再從裝置搬回，最後逐 byte 比較。第一次只證明 guest 能看見 EDU，接著依序 MMIO → IRQ → DMA。

## 2. 先分清名詞與位址

| 名詞 | 意思 | 先避免的錯誤 |
| --- | --- | --- |
| PCI ID | 識別裝置 vendor/device 的欄位 | 名字相似不代表 ID 相同 |
| BAR | PCI 裝置公開的資源區域描述 | BAR 不是可以直接當普通 C pointer 的數字 |
| MMIO | 映射後存取裝置 registers 的方式 | 使用指定 accessor、offset、寬度 |
| IRQ | 裝置通知 CPU 的中斷 | 中斷到了還要查來源並 acknowledge |
| DMA | 裝置直接存取記憶體的資料傳輸 | 不是 CPU 呼叫 memcpy 搬完就算 DMA |
| CPU virtual address | CPU 程式用來存取 buffer 的位址 | 不能直接填進 DMA register |
| dma_addr_t | DMA API 提供給裝置使用的位址 | 可能經 IOMMU 映射，不等於一般 pointer |
| ownership | 此刻誰可修改／讀取／回收 buffer | timeout 不表示裝置已停止存取 |

先讀 [QEMU EDU 規格](https://www.qemu.org/docs/master/specs/edu.html) 的 PCI specs、MMIO area、IRQ controller、DMA controller。它是這個練習的裝置手冊；實作時切到所選 QEMU 版本的同份文件。

## 3. 第一個成果：lspci 看見裝置

1. 使用固定 Linux/QEMU 版本的 PCI guest，例如 x86_64 q35，記錄 kernel config、RAM、磁碟、console。它可以與 04 的 ARM guest 分開。
2. 在已能正常開機的命令中加入 `-device edu`，先確認該 QEMU build 支援 EDU。
3. 進 guest 執行 `lspci -nn`，找 `1234:11e8`；若沒有 lspci，先在 guest image 加入對應 pciutils 工具。
4. 保存 EDU 的 bus/device/function 與詳細資源資訊，正常關機再跑一次。

完成條件：可用同一條 QEMU 命令再次看到該 ID。此時還沒必要寫 driver。看不到時先查 QEMU command、machine 是否有 PCI、guest 的 PCI 支援，不先改 kernel module。

之後本專案才逐步新增 `scripts/run-qemu.sh`、`driver/edu_demo.c`、`driver/Makefile`、`tests/roundtrip.c`、`docs/registers.md`、`notes/environment.md`。

## 4. 建立 PCI driver，只做 probe/remove

先讀 [How To Write Linux PCI Drivers](https://docs.kernel.org/PCI/pci.html) 的結構、device ID table、probe、資源取得與移除。API 以你鎖定的 kernel source/headers 為準。

1. 宣告匹配 EDU ID 的 pci_driver，讓 probe 印出辨認到的裝置。
2. 按 PCI API 啟用裝置，取得所需 BAR 資源，映射供 MMIO 使用。
3. 把 device state 保存到每個裝置自己的結構，而不是假定全系統永遠只有一個 instance。
4. 建立失敗 unwind：某一步失敗，只回收前面已成功取得的資源，順序與依賴對應。
5. 此階段不啟動 DMA，先驗證 bind/unbind 與 module 載入／卸載。

完成條件：probe/remove 能重複且資源正常回收，沒有同一 BAR 被自己重複占用。若使用 managed resources，仍須理解釋放時機；它不會替你停止硬體正在做的工作。

## 5. 第一個 MMIO 操作：liveness register

在 `docs/registers.md` 抄下要用的 offset、寬度、讀寫權限與副作用，附規格來源。先只使用 EDU 的 liveness 檢查：寫入固定 pattern，回讀規格定義的反相值。

動手順序：先確認 BAR 大小，再使用 `readl/writel` 等相符的 accessor；把 expected value 限制在該 register 的位寬，避免 host 整數提升造成比較錯誤。換幾組 pattern 重跑。

完成條件：讀回值符合規格，錯誤時回報實際值、預期值與 offset。不能只用「有 dmesg」作為成功依據，也不要用任意位址掃描猜 register。

## 6. IRQ：先獨立測通知，不混入 DMA

先用 EDU 提供的 interrupt raise/ack registers 做一次可控通知。初版選 INTx，了解共享中斷的情況，再選做 MSI；EDU 的此規格不支援 MSI-X。

1. 設定相符的 IRQ 資源與 handler；handler 查裝置 status，確定是否自己裝置的事件。
2. 先建立「尚未完成」狀態與等待物件，再觸發命令，避免中斷先到而事件遺失。
3. handler 按規格 acknowledge，保存結果並通知等待者；需要睡眠的工作留在 process context 或適合的延後工作。
4. userspace 等待設 timeout，檢查每次只回報對應的那次命令，不把上次完成旗標拿來重用。

Reference：同一份 [EDU IRQ controller](https://www.qemu.org/docs/master/specs/edu.html) 與 [Linux Completions](https://docs.kernel.org/scheduler/completion.html)。先理解 completion 的初始化、等待、完成及生命週期；handler 可能到來前，相關狀態必須仍然有效。

## 7. DMA：先做 1 byte round-trip

先讀 [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html) 的 CPU and DMA addresses、DMA mask、consistent/coherent mappings。EDU 預設 DMA mask 是 28 bit，裝置端 DMA buffer 是 4096 bytes；以所選版本規格和啟動配置核對，不把這些限制推廣到所有 PCI 裝置。

1. 設定符合裝置能力的 DMA mask 並檢查結果，再取得 coherent buffer。保存 CPU pointer 和 DMA address 兩個不同值。
2. 確認裝置 bus mastering 與所需 IRQ 已準備好，初版同時只允許一個傳輸。
3. CPU 在擁有 buffer 時填入一個 byte，例如 `0x5A`。
4. 依規格設定 host → device 的 source/destination/count，使用的是 DMA address 與裝置定義的 buffer address，不能混用 BAR offset 與 CPU pointer。
5. 發布資料與寫 command 的順序要依 DMA／MMIO API 規則處理；coherent 不代表所有 ordering 自動正確。
6. 確認傳輸完成，才安排 device → host。接收位置先放與預期不同的 pattern，或使用獨立的接收 buffer；再次確認完成後由 CPU 讀取比較。若回傳位置原本就留著送出的正確值，即使回程沒有搬資料也可能誤判成功。
7. 成功後擴大為 64、4096 bytes，保存 mismatch 的第一個位置與實際值。

先說得清楚每一步 ownership，再寫 register 操作。若未來換成 streaming mappings，還要依 DMA API 管理 map/unmap 與同步方向；不是把 coherent 分配函式換掉就完成。

## 8. 定義 userspace 合約與失敗路徑

在原有 05 的介面經驗上，定義一次 round-trip 請求：合法長度 1～4096，拒絕 0 與 4097；輸入資料如何複製、同步返回還是等待、錯誤碼、取消和 timeout 的含義都要固定。

timeout 只代表等待超時，不能推論 DMA 已停止。EDU 沒有能讓你假定「任意時刻中止 DMA 並立即安全 free」的通用保證。

處理步驟：停止接收新請求，查實際 in-flight 狀態，按規格確認完成／靜止；確認沒有裝置、handler、worker、使用者仍可能存取後才能釋放。無法確認時進入明確 fault 狀態、保留仍可能被使用的資源，停止該輪測試並用專用 guest 重啟復原。不要用 remove 正在執行當作裝置已停止的證據。

先設計 remove/取消與正常完成如何序列化，再測傳輸中 unbind。managed cleanup 也不能繞過這個順序。同步部分回讀 [09 教學](../09-concurrency-memory-model/LEARNING.md)，核心 API 則使用所選 kernel 的機制。

## 9. 驗證順序與卡關線索

依序驗證：辨識 → probe/remove → MMIO → 單次 IRQ → 1-byte DMA → 邊界長度 → 多次傳輸 → timeout/取消/remove。前一階段出錯就先留在該階段。

| 現象 | 先查 |
| --- | --- |
| probe 沒進來 | PCI ID、driver 綁定、是否已有其他 driver 占用 |
| MMIO 回讀錯 | BAR、offset、位寬、register 副作用 |
| 中斷一直觸發 | status/ack 規則、是否確實清掉來源 |
| 等待立刻成功 | completion/state 是否還留上次結果 |
| DMA mismatch | 方向、source/destination、count、ownership、buffer 位址 |
| timeout 後 crash | 是否過早釋放仍由 DMA/IRQ 使用的資源 |

先用固定 pattern 做少量可追蹤測試，再以 runner 重跑 1000 次，保存 payload 大小、完成數、mismatch、timeout、guest 條件與 latency。依 README 的最小驗收逐項填入實測證據；成果是 PCI/DMA 軟體流程，不是實體 PCIe 吞吐、PHY 或 link training 的測量。
