# 可重建的最小 Linux 裝置映像

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先分清 host、guest、kernel 與 rootfs，使用官方 QEMU defconfig 開機到 shell，再加入自己的程式。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

狀態：規劃中，已有規格與逐步教學，程式尚待實作。
環境：WSL2/Linux + QEMU；只使用專案自己的映像檔，不動主機磁碟。
前置：02 的 daemon；可先做04的最小開機再回02整合。

## 要交付什麼

用 BR2_EXTERNAL 保存 package、defconfig、rootfs overlay 與開機設定；从乾淨目錄能重新建置並用序列主控台觀察到服務 ready。

## 分層實作

- L1：固定 Buildroot/QEMU/toolchain 版本，建立 BR2_EXTERNAL，自訂 daemon package 與 init 整合。
- L2：唯讀 rootfs、可寫資料分區、正常關機／非正常關機處理、開機紀錄、非 root 服務權限。
- L3：兩次乾淨建置比較產物與依賴清單、來源 checksum、授權資訊、開機時間分析與最小功能裁剪。

## 效能與資源

kernel/init/rootfs/service 各段開機時間、映像大小、常駐記憶體。序列輸出開始與 ready 的量測起點必須一致。

## 第一個動作

先用一個官方 QEMU defconfig 開機，保存選用版本與console；再建自己的 external package。

## 補強：Linux boot flow／BSP

先用 QEMU 直接載入 kernel 跑通 image，再選 ARM64 virt 與對應 U-Boot，完成 **U-Boot → kernel + DTB → rootfs → init → daemon**。版本、machine、CPU、RAM、console、磁碟介面都固定記錄；不把 QEMU 當成實體 Boot ROM／DDR 初始化驗證。

1. 保存 defconfig、建置與啟動腳本；用 readelf 確認 daemon 架構，區分 host compiler 與 cross toolchain。
2. 在 U-Boot 載入 Image／DTB，設定 console、root= 與 init；確認載入位址不重疊，再啟動。實際位址由所選平台文件與 memory map 決定。
3. 從執行中的 DT 與 boot log 追一個裝置的 compatible、reg、interrupts；接到 [05 driver](../05-linux-kernel-driver/README.md)。
4. 故意改錯 root=，再禁用一個非開機必要裝置的 DT node；分別定位 rootfs mount 與裝置 probe 問題，修復後自動重跑。

驗收：20 次開機均到 ready；每次保存完整 console 和版本；腳本有 timeout、非零失敗退出碼，能判斷停在 bootloader、kernel、rootfs 或 service。真板再補 clock／reset／pinmux／電源與儲存啟動，不要求現在寫完整 bootloader。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| BOOT-01 | 同一映像啟動 20 次，等待具體 ready marker | 每次服務可用；保存 console log 與失敗重現命令。 | 待實作 |
| FS-01 | rootfs 唯讀、data 分區不可寫／已滿 | 服務能回報錯誤而非卡死；沒有偷偷改寫 rootfs。 | 待實作 |
| BUILD-01 | 不同乾淨工作目錄、相同鎖定輸入各建一次 | 比較 hash；若不一致，定位 timestamp/path 等來源並記錄，不直接宣稱可重現。 | 待實作 |
| PKG-01 | daemon package 缺依賴、設定缺失 | build 或启动清楚失敗，不能用主機路徑掩蓋缺件。 | 待實作 |

## 必要閱讀

- [Buildroot manual：搜尋 BR2_EXTERNAL、rootfs overlay](https://buildroot.org/downloads/manual/manual.html)。
- [U-Boot QEMU ARM：平台建置與啟動入口](https://docs.u-boot.org/en/latest/board/emulation/qemu-arm.html)。
- [booti：kernel、initrd、DTB 參數](https://docs.u-boot.org/en/latest/usage/cmd/booti.html)。

[回主路線](../README.md)
