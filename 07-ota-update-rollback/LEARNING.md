# 07 教學：讓更新失敗後仍有可啟動的版本

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚未配置 A/B slots、RAUC 或 bootloader。需要先有 04 或 06 的可重建 image。本專案使用專用 QEMU guest 的 image 檔案，不對主機磁碟套用分區操作。

## 1. 用途與例子

裝置正在跑 v1，你想更新成 v2。若直接覆寫唯一一份系統，寫到一半出錯，可能連原本可用的版本都失去。

A/B 的練習方式是保留兩個系統槽位：正在使用 A 時，先把新版寫進 B；驗證後讓下一次試開 B。B 完成真正的健康檢查才確認成功；若一直不健康，則在有限次嘗試後回到仍可用的 A。

第一個成果不是斷電測試，而是你能畫出這一條正常流程，並在每一步說出「此時哪一份已確認可用」。

## 2. 先學的名詞

| 名詞 | 白話意思 | 要防止的誤解 |
| --- | --- | --- |
| slot | 一份可更新的系統內容位置 | A/B 不一定涵蓋所有可變資料 |
| bundle | 框架封裝的更新內容與 metadata | 不是任意 tar 檔改副檔名 |
| signature | 檢查內容是否由受信任金鑰簽署 | hash 相同本身不能證明來源可信 |
| boot attempt | 對尚未確認版本的試開次數 | 不能無限制重試壞版本 |
| mark-good | 確認目前啟動版本已通過健康檢查 | kernel 有輸出不等於服務健康 |
| rollback | 回到可用版本 | rootfs 回退不會自動回復共用資料庫 |

先讀 [RAUC Basics](https://rauc.readthedocs.io/en/latest/basic.html) 的 slots、bundles 和整體架構，再讀 [Integration](https://rauc.readthedocs.io/en/latest/integration.html) 的 bootloader interface、system configuration。使用既有框架的簽章機制，自己的工作是整合和測試。

## 3. 第一次動手：先寫一張狀態表

新增 `docs/update-flow.md`，先完成以下表格並寫出每步出錯時的去向：

| 階段 | 正在跑 | 已確認可用 | 下一個決策 |
| --- | --- | --- | --- |
| 初始 | A/v1 | A | 保留 A，準備 B |
| 驗證 bundle | A/v1 | A | 不可信則拒絕，不能選 B 開機 |
| 寫 B | A/v1 | A | 寫入完成並驗證後才切換試開目標 |
| 試開 B | B/v2 | A | 檢查服務與資料可用性 |
| B 健康 | B/v2 | A、B（依實際保留狀態） | 明確 mark-good |
| B 不健康 | B/v2 或啟動失敗 | A | 有限次嘗試後回 A |

這是教學上的抽象狀態，不是可直接套用的 RAUC 狀態名稱。bootloader 的持久 metadata 與 RAUC slot 狀態如何對應，要依選定 backend 寫清楚。

接著準備兩份可辨識的 image：v1/v2 的 `/etc/project-version` 或服務版本輸出不同，且各自單獨開機都已驗證。先把「新版本來就不能跑」與「更新整合失敗」分開定位。

## 4. 準備受控的 guest 更新環境

這時才逐步建立：

```text
docs/update-flow.md    狀態與出錯後預期
config/               RAUC system 設定、bootloader 整合設定
scripts/              建立 guest image、啟動、更新與驗證的腳本
tests/                情境與 runner
notes/environment.md  image、RAUC、bootloader 版本與儲存配置
```

1. 選 RAUC 支援且能在所選 QEMU 平台運作的 bootloader backend，固定版本。
2. 畫出專用 guest 磁碟配置，標示 boot、A、B、共用 data；確認 slot 路徑與開機 rootfs 對應。
3. 在 guest 裡部署 RAUC 與系統設定，檢查 status 輸出能正確辨識目前 slot。
4. 配置 boot attempt 和 fallback，先測試 bootloader 自己的選槽邏輯。
5. 使用可重建的初始 image／快照作為每次測試起點；記錄持久 metadata 位於哪裡，重設時不能只還原其中一半。

如果連正常開機的 active slot 都識別錯誤，先停在此步修復，不繼續安裝 bundle。

## 5. 完成一次正常更新

按 [Using RAUC](https://rauc.readthedocs.io/en/latest/using.html) 的 bundle creation、install、status 操作，參數以鎖定版本為準。先用測試金鑰；private key 留在建置端的受控位置，guest 放驗證所需的 trust material。

1. 打包 v2，確認 manifest 的相容識別與 image 類型。
2. 驗證 bundle 資訊與簽章，保存紀錄。
3. 從 A/v1 安裝至 inactive B，確認 A 仍可運行。
4. 按整合方式重啟，檢查目前 slot、rootfs 與服務版本確實是 B/v2。
5. 健康檢查通過後 mark-good，再重啟一次，確認狀態持久化。

健康檢查至少包含必要服務能處理一個實際請求、必要資料可讀寫。先定義 deadline 與失敗條件，避免無限等候，也避免太早確認一個稍後必定崩潰的版本。

## 6. 一次加入一種失敗

| 測試 | 注入方式 | 必須觀察 |
| --- | --- | --- |
| 不受信任 bundle | 使用不受信任的測試簽署者 | 更新被拒絕，原 slot 可用 |
| 內容被改動 | 對已產生的測試 bundle 做受控變造 | 驗證失敗，有明確原因 |
| 寫入失敗 | 在專用 guest 模擬不足空間／儲存錯誤 | 不把半寫入 slot 標成可試開 |
| v2 不健康 | 讓新版服務不回應健康請求 | 未 mark-good，最終回到 A |
| 寫入／metadata 中斷 | 在已命名的 checkpoint 終止或 reset guest | 重啟後對上狀態合約，保留 A 或完整新版 |
| 資料 schema 變更 | 新版修改共用資料後回退 | v1 能讀，或有明確恢復方案 |

先選一個 checkpoint 成功重現，再擴大覆蓋。kill 更新程序、reset QEMU、真實 flash 斷電的故障模型不同，紀錄要精確，不把第一個實驗直接稱為實板斷電驗證。

RAUC 的 API 與狀態欄位查 [Reference](https://rauc.readthedocs.io/en/latest/reference.html)。更新可信來源、資料完整性、產品防降級是不同需求，不能因有簽章就宣稱所有舊版本都會被拒絕。

## 7. runner、排錯與交付

runner 每次從固定初始 image 開始，保存版本、注入點、更新退出碼、boot console、active slot、health 結果、最後執行的版本。所有等待都設定 timeout。

| 現象 | 先查 |
| --- | --- |
| 安裝成功卻仍跑 v1 | bootloader 選槽、rootfs 參數、實際 active slot |
| v2 永遠反覆試開 | attempt counter 是否持久化、fallback 是否配置 |
| 一開機就被確認 | mark-good 是否早於真正健康檢查 |
| rollback 後程式失敗 | 共用 data 是否被新版不相容地修改 |

先完成一次正常更新和一次不健康回退，再對照 README 的 AUTH-01、POWER-01、HEALTH-01、DATA-01 擴展。交付時用實際測試覆蓋表說明測了哪些 checkpoint，尚未做的硬體測試維持待驗證。
