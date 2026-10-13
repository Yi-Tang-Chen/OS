# 斷電可恢復的簽章 A/B 更新

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先寫出 A/B 更新狀態表，準備兩份可辨識且能開機的 image，再逐步整合 RAUC 與回退測試。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

狀態：規劃中，已有規格與逐步教學，程式尚待實作。
環境：QEMU guest + 專案映像；先軟體故障注入，再考慮真板斷電測試。
前置：04 或 06 的映像、02 的健康檢查、整數範圍與 timeout 處理。

## 要交付什麼

使用 RAUC 或同等成熟框架與其支援的 bootloader 整合。建置簽章 bundle、inactive slot 更新、boot attempt、health confirmation、rollback；自己實作測試與整合，不自製密碼學。

## 分層實作

- L1：先定義slot狀態與持久metadata，使用測試金鑰簽章bundle，正常更新到inactive slot再啟動。
- L2：驗證錯誤簽章、壞hash、儲存不足、寫入中斷、第一次啟動不健康；明訂mark-good時間點。
- L3：多個持久寫入點的kill/reset測試、金鑰輪替方案、版本與防降級需求、資料schema遷移與回退相容性。

## 效能與資源

下載/驗證/寫入/boot各段時間、空間需求、恢復時間與checkpoint覆蓋數；不把kill process直接等同真實flash斷電。

## 第一個動作

先在QEMU中整合成熟更新框架的正常A/B流程，再列checkpoint，不要直接對主機分區操作。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| AUTH-01 | 錯誤金鑰、變造bundle、不支援硬體識別 | 拒絕更新；目前已確認slot保持可用。 | 待實作 |
| POWER-01 | 每個命名持久化checkpoint前後中斷 | 重啟只能進入已確認舊版或完整可驗證新版；不能boot半寫入slot。 | 待實作 |
| HEALTH-01 | 新版可開機但daemon不ready／反覆崩潰 | 未mark-good，有限次重試後回退；保留失敗原因。 | 待實作 |
| DATA-01 | 新版修改資料schema後回退 | 舊版可讀取或有明訂恢復方案；slot回退不等於資料也回退。 | 待實作 |

## 必要閱讀

- [RAUC 整合：先看 bootloader 與 slot](https://rauc.readthedocs.io/en/latest/integration.html)。

[回主路線](../README.md)
