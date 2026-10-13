# 08 教學：從讀懂一個外部測試，到做出可審查的修改

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚未選定上游專案或缺陷。此階段的產出是本地 reproducer、修正、測試和 patch；是否對外提交，等內容完成後由你決定。

## 1. 用途與例子

前面的作品多由你定義規則。參與現有專案時，需要先理解別人的介面合約、風格和測試，再提出有依據的修改。

假設一個你使用的 parser 在分段輸入下漏掉一筆 frame。你的工作是找出最小輸入，確認這是否違反文件，再修正並留下 regression test，讓未來修改不會重新引入相同問題。

這只是題型例子，不代表已在任何上游發現此問題。不要先假定外部程式有 bug，再修改測試去證明自己的猜測。

## 2. 先認識流程中的詞

| 名詞 | 意思 | 本專案要留下的內容 |
| --- | --- | --- |
| upstream | 原專案的主要來源 | repository URL 與基準 commit |
| reproducer | 最小重現問題的輸入與步驟 | 一條命令或小測試，附預期和實際 |
| regression test | 防止已修問題再次出現的測試 | 原版失敗、修正版通過 |
| patch／diff | 兩個版本之間的修改 | 只包含和問題相關的內容 |
| review | 他人檢查理由、行為與風險 | 每項問題的回應與修改 |
| rebase | 把自己的修改重新接到新的基準 | 更新後仍需驗證，不能只解完衝突就算完成 |

前置：能用 Git 看 status/diff，能自己編譯與執行一個已做過的專案。先選前面實際用過、能在本機跑測試的 library 或工具；不要第一題就跨進完全不熟悉的 subsystem。

## 3. 第一個成果：跑通上游原本的測試

1. 找到原始 repository，讀 README、LICENSE、CONTRIBUTING 和相關目錄的規則。
2. 建立獨立 checkout，記錄精確 commit；別直接把整個上游 source 塞進此作品集。
3. 按其文件完成原版 build/test，保存命令與完整結果。
4. 選一個和你已學功能相近的小測試，找到「輸入、呼叫的函式、預期結果」三處。
5. 用自己的話描述這個測試。第一天做到這裡就有進展，還不必有 patch。

之後在本專案新增 `notes/upstream.md`，記錄 URL、commit、license、建置要求、測試命令、CONTRIBUTING 位置。測試本來就失敗時，先判斷是否環境缺件，不把所有失敗直接歸因上游缺陷。

Reference：[git diff](https://git-scm.com/docs/git-diff)，先讀 working tree、index、commit 比較；[git worktree](https://git-scm.com/docs/git-worktree)，需要同時保留原版與修正版時再讀 add/list。worktree 是方便隔離兩個版本的工具，不是開始學習的必要門檻。

## 4. 把一個疑問變成最小 reproducer

先從你真的遇到的錯誤或上游 issue 開始。若暫時找不到，先練讀碼與補足對既有行為的測試，不要捏造已發現的缺陷。

reproducer 應回答五件事：

1. 正確行為由哪份文件、測試或 API 合約支持？
2. 哪一組最小輸入觸發問題？
3. 在哪個 commit 和環境可以重現？
4. 實際輸出、退出碼或工具診斷是什麼？
5. 如何用一條命令重跑？

例如若只有 10000 筆資料會失敗，先減半資料找出仍失敗的子集，再逐步刪掉不必要操作。最小重現能降低理解成本，也能避免把症狀與真正原因混在一起。

學習時可以在自己的玩具程式製造一個錯誤再修復，但它只能標成練習，不能標成上游缺陷或真實貢獻。

## 5. 修正之前先建立能失敗的測試

在上游既有測試架構內寫 regression test，使用原本的命名和 assertions。先只加入測試，不加入修正，執行並確認它真的因目標問題失敗。

然後修改最小範圍的 production code，重跑相同測試與鄰近測試。若「修好」的方法只是放寬 expected result、忽略錯誤或降低測試覆蓋，要重新檢查是否仍滿足原合約。

保存這张證據表：

| 版本 | 原有相關測試 | 新 regression test | 如何重跑 |
| --- | --- | --- | --- |
| 原版 + 新測試 | 實際結果 | 應因目標缺陷失敗 | 精確命令 |
| 修正版 + 同一測試 | 實際結果 | 應通過 | 同一命令 |

表格填真實輸出，不先填「通過」。如果原版的新測試本來就通過，表示目前測試沒有證明該修正必要。

## 6. 整理成 reviewer 看得懂的 patch

修改說明依序回答：什麼輸入觸發問題、原本會怎樣、原因是什麼、修改後會怎樣、怎麼驗證。不要只寫「fix bug」或逐行重述 diff。

在 upstream checkout 檢查：

```sh
git status --short
git diff --check
git diff
```

確認沒有 build output、編輯器設定、無關格式化或其他實驗。要形成 commit/patch 時依該專案規則操作；[git format-patch](https://git-scm.com/docs/git-format-patch) 說明如何把已完成的 commits 匯出成可審查郵件格式。

Reference：[Linux Submitting Patches](https://docs.kernel.org/process/submitting-patches.html)，先讀 Describe your changes、Separate your changes。它提供修改說明的例子，但 DCO、寄信流程、測試要求是否適用，要以你實際選的 upstream 規則為準；不要把 kernel 流程套給所有專案。

## 7. 練習回應 review

請 reviewer 檢查邊界、合約、測試是否會誤通過、修正是否影響其他 caller。逐項記錄「接受並修改、補證據、說明不採用理由」，不只記一句已處理。

若要跟進新的 upstream commit，先保存目前狀態，再 rebase 或重新套用 patch；衝突處理後重新跑相關測試。程式合併成功不等於行為仍然正確。

## 8. 交付物與卡關處理

本專案之後可以保存 `notes/upstream.md`、`docs/problem.md`、`patches/` 與測試 log 索引。記錄修改狀態為本地練習、尚未提交、討論中或已合併；每一種都依事實描述。

| 卡住的地方 | 下一個小動作 |
| --- | --- |
| 專案太大，不知道讀哪裡 | 從一個已能執行的測試追到被呼叫函式 |
| issue 說有 bug，本機重現不了 | 比對版本、平台、feature flags 和輸入，不先亂改程式 |
| patch 變得很大 | 列出解決目標問題必需的檔案，抽離無關重構 |
| 想不到測試 | 從 reproducer 中移除環境依賴，保留最小輸入與可觀察結果 |

完成後回 README 的 REPRO-01、SCOPE-01、REVIEW-01、STATUS-01。這個專案的價值是證明你能讀別人的規則、重現問題並提供可驗證的修正。
