# 可維護的 BSP layer 與產品建置

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先跑通官方 image，再讓自己的 layer 安裝一個小程式，最後才整合 02 服務、SDK 和產品設定。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

狀態：規劃中，已有規格與逐步教學，程式尚待實作。
環境：Linux/WSL2；固定 Yocto release、layer revision 與足夠磁碟空間。
前置：04 的 image/package 概念；02 daemon。此路線與04部分重疊，依職缺選擇深度。

## 要交付什麼

以自己的 meta layer 整合既有 daemon、service、image 與 SDK；證明可在乾淨環境交付，而不只是單機 bitbake 成功。

## 分層實作

- L1：recipe、SRC_URI/SRCREV、LICENSE/LIC_FILES_CHKSUM、DEPENDS/RDEPENDS 與 systemd service。
- L2：bbappend 與自訂 machine/distro 的責任分工；package QA、ptest、SDK consumer 範例。
- L3：兩次 clean build 比較、sstate/cache 範圍、來源鏡像與離線重建策略、SPDX/SBOM 與 license artifacts。

## 效能與資源

clean/incremental build時間、image大小、package count、cache使用資訊；把環境因素與修改效果分開。

## 第一個動作

先做一個獨立layer與daemon recipe，保留bitbake log，不急著打造完整BSP。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| RECIPE-01 | 更換 daemon revision、移除一個runtime dependency | 版本可追溯；QA或runtime smoke test 能抓出缺件。 | 待實作 |
| SDK-01 | 乾淨目錄用產出的 SDK 編譯一個 consumer | 產物架構正確，能在同一image啟動。 | 待實作 |
| REBUILD-01 | 第二個乾淨環境依manifest建置 | 版本與產物可比較，cache命中率不是可重現的替代指標。 | 待實作 |
| SERVICE-01 | 首次啟動、重啟、升級後設定檔保留 | 權限、資料路徑、service依賴符合文件。 | 待實作 |

## 必要閱讀

- [Yocto 開發手冊：layer、recipe、SDK 按任務查](https://docs.yoctoproject.org/dev-manual/index.html)。

[回主路線](../README.md)
