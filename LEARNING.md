# 從哪裡開始：嵌入式與系統程式學習入口

這份 repository 是逐步完成的求職作品集。目前 01 有 ring buffer 與測試；02～12 有規格和教學，程式與實驗結果仍待你實作。文件裡的預期結果不是已通過的報告。

## 如何讀每一個專案

先讀該專案的 `LEARNING.md`，照「用途 → 具體例子 → 必要概念 → 小實作 → 驗證」往下走。`README.md` 保存完整目標、進階範圍和驗收條件；初學時不用一次讀懂所有名詞。

每次只完成一個能觀察結果的小步驟。開始寫之前，用自己的話寫四句：

1. 輸入是什麼？例如一個 byte、三個 bytes、一次讀取請求。
2. 程式這一步要做什麼？例如保存資料、算 CRC、喚醒等待者。
3. 正確時應看到什麼？例如取到 42、印出 READY、收到一次 callback。
4. 如何知道錯了？例如值不對、超時、錯誤碼、assert 失敗。

答不出來時，先把步驟縮小，或先讀該節 reference。可以先寫中文流程，再寫函式宣告，最後才補實作。學習用的小程式可以逐步整理，不必第一天就建立完整架構。

## 依序開始的入口

| 專案 | 教學入口 | 第一個可完成的小成果 | 完整實作前會用到的前置 |
| --- | --- | --- | --- |
| 01 | [C 工具與測試](01-embedded-c-toolkit/LEARNING.md) | 看懂一個 assert，跑出一次成功／失敗 | 變數、函式、陣列與指標；教學從這裡補 |
| 02 | [Linux 資料服務](02-linux-sensor-simulator/LEARNING.md) | socketpair 傳過去 3 個 bytes | 01 parser 到整合階段才需要 |
| 03 | [RTOS telemetry](03-zephyr-native-sim/LEARNING.md) | native_sim 印出 Hello World | C 函式與結構；本教學補 thread/queue |
| 04 | [Linux 映像](04-buildroot-qemu-image/LEARNING.md) | 官方 QEMU 組態開機到 shell | Linux 命令；之後整合 02 |
| 05 | [Character driver](05-linux-kernel-driver/LEARNING.md) | guest 內載入／卸載最小 module | 02 read/write、04 guest；同步先讀 09 入門 |
| 06 | [Yocto layer](06-yocto-layer/LEARNING.md) | 官方 image 開機，再安裝一個小程式 | 04 host/target/package 概念；之後整合 02 |
| 07 | [A/B 更新](07-ota-update-rollback/LEARNING.md) | 寫出 A/B 狀態表並取得兩個可辨識版本 | 04 或 06 可重建 image；02 健康檢查 |
| 08 | [開源修改](08-open-source-contribution/LEARNING.md) | 在固定 upstream commit 跑通原有測試 | 已實際用過一個相關專案、基本 Git |
| 09 | [並行與同步](09-concurrency-memory-model/LEARNING.md) | 建立一個 thread，等待它結束 | 01 queue、02 單執行緒資料流程 |
| 10 | [Bare-metal 啟動](10-baremetal-qemu-bringup/LEARNING.md) | GDB 停在自己的 reset handler/main | C 指標、位元操作；教學補 ELF 與 linker |
| 11 | [除錯與效能](11-debug-performance-lab/LEARNING.md) | 用 GDB 觀察一次 ring buffer 狀態變化 | 入門可接 01；完整實驗用 02、09 |
| 12 | [PCI 與 DMA](12-pci-dma-lab/LEARNING.md) | guest 的 lspci 看見 EDU | 05 資源生命週期、09 同步、可用的 PCI guest |

可以照 01～12 的順序建立作品，但編號不表示所有知識都嚴格依序：05 需要提前讀 09 的 mutex／等待入門；03 的優先序實驗也需要同步概念。提前補一小節不等於同時開做整個 09。11 的 GDB 入門可以在 01 卡住時使用。

## 如何使用 reference

每份教學會指出「什麼時候讀、先看哪段、要回答哪個問題」。英文文件先找函式名稱、參數、回傳值、例子，再讀錯誤情境。man page 裡的 `(2)` 通常指 system call，`(3)` 指 library function，`(7)` 常是概念說明；本機若有手冊，可用 `man 2 read` 這種格式開啟。

文件優先引用專案官方手冊、Linux man-pages、標準草案與工具作者文件。`latest`、`master` 和無版本的網址可能更新；開始 03～12 的實作時，先固定使用的 release/commit，並切到相符版本的文件，記錄在自己的環境筆記。教學不預先宣稱某組尚未建置的版本已相容。

建議每個專案開始時才新增 `notes/learning-log.md`，記錄環境、實際命令、輸入、預期、結果與一個疑問。原始 log 和量測數據保留另外檔案。不要把例子中的預期數字抄成自己的實測。

## 今天就從這裡開始

打開 [01 教學](01-embedded-c-toolkit/LEARNING.md)，先讀第 1～3 節。看懂 `test_empty_buffer` 每個 assert 後，再照第 4 節親自跑一次。今天不必先做 CRC，也不必先安裝後面十一個專案的工具。
