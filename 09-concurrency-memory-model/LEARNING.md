# 09 教學：先讓兩個工作正確合作，再理解 atomics

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚無並行 queue。03／05 需要同步觀念時，可先讀第 2、4、5 節，不必立即做完 atomic ring。

## 1. 用途與具體例子

一條 thread 生產資料，另一條負責處理。它們共用容量 4 的 queue：空時 consumer 應等待，滿時 producer 應等待，關閉時所有等待者都能離開。

01 的 ring buffer 假設操作依序發生。多條 thread 同時改 head、tail、count 時，單純沿用原程式可能造成 data race。這個專案先用 mutex 保護共同狀態，再用 condition variable 讓等待不必持續占 CPU。

## 2. 先備概念

| 概念 | 意思 | 最小問題 |
| --- | --- | --- |
| thread | 同一 process 內可獨立排程的執行流程 | main 結束前是否等 worker 完成？ |
| shared state | 多個 thread 都可能存取的資料 | queue 的 count 是否同時被修改？ |
| data race | 沒有足夠同步的衝突存取，至少一方寫入且涉及非原子操作 | 不能只用「跑很多次沒錯」證明不存在 |
| mutex | 讓受保護區段一次由一個持有者進入 | 哪些欄位必須一起保持一致？ |
| condition variable | 配合 mutex 等待狀態改變的機制 | 醒來後條件真的成立了嗎？ |
| join | 等指定 thread 結束並取得完成狀態 | shared data 是否還有人在用？ |

先在紙上把 `counter++` 分成「讀舊值、計算、寫新值」，交錯兩個人的步驟，理解為什麼可能遺失更新。這只是說明問題的模型；不要用一個真的有 data race 的程式跑出的數字當成 C 保證的行為。

References：[pthread_create(3)](https://man7.org/linux/man-pages/man3/pthread_create.3.html) 讀 start routine、參數、回傳值；[pthread_join(3)](https://man7.org/linux/man-pages/man3/pthread_join.3.html) 讀等待與資源回收。pthread 函式通常直接回傳錯誤碼，不能照 read/write 的 -1/errno 模式一律處理。

## 3. 第一個程式：建立 worker 並等它完成

只先新增 `examples/thread_hello.c`：main 準備一份參數，建立一條 worker；worker 計算結果並返回；main join 成功後才讀取結果並印出。

參數物件要活到 worker 用完。先用 main 作用域內的物件，main 在離開前 join；不要把暫時函式的區域變數地址交給稍後才執行的 worker。

檔案寫好後，從本專案目錄執行：

```sh
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -g -pthread examples/thread_hello.c -o build/thread_hello
./build/thread_hello
```

完成條件：檢查 create/join 的回傳值；main 一定在 worker 結束後讀取結果。不要把哪一行 printf 先出現當作排程保證。

## 4. 第二個練習：用 mutex 保護一個 counter

先用靜態 initializer 或 init API 初始化 mutex，兩條 worker 各增加 counter 固定次數。每次增量放在同一把 mutex 的 lock/unlock 之間；最後 main join 兩者，再檢查總和，確認無人使用後才 destroy。尚未初始化的 mutex 不能直接 lock。

在筆記寫出鎖保護的資料：這一步只有 counter；到 queue 會變成 storage、head、tail、count、closed 之間的共同規則。讀取共同狀態也要遵守同一同步設計，不是只有寫入才加鎖。

先查 [pthread_mutex_lock](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html) 的正常 mutex 行為與回傳值。mutex 不能替你決定什麼時候 queue 應該算已關閉，這要先定義合約。

## 5. 建立有界 queue，先寫 close 合約

建議之後新增 `include/work_queue.h`、`src/work_queue.c`、`tests/test_work_queue.c`。初版採用下面的練習合約：

- open 且未滿：push 成功；滿了則等待。
- open 且非空：pop 成功；空了則等待。
- close 之後：所有新 push 失敗；已入隊資料仍可被 pop。
- closed 且 empty：pop 回傳明確的結束狀態。
- close 喚醒等資料與等空間的所有等待者；所有使用者退出後才能 destroy/free。

condition variable 不保存「這次通知屬於你」的訊息。consumer 的概念流程是：

```text
取得 mutex
while queue 為空 且 尚未 closed:
    cond_wait(not_empty, mutex)
若 queue 為空 且 已 closed:
    解鎖，回傳結束
取出一筆，通知 not_full
解鎖，回傳資料
```

`cond_wait` 會在等待時釋放 mutex，返回前重新取得它。用 while 而非 if，是因為可能有 spurious wakeup，且其他 consumer 可能先拿走資料。呼叫 wait 前，呼叫端必須已持有相應 mutex。

Reference：[POSIX condition wait](https://man7.org/linux/man-pages/man3/pthread_cond_wait.3p.html)，先讀 DESCRIPTION、Condition Wait Semantics；讀完要能指出「實際判斷的是 count/closed，而不是有沒有收到通知」。

動手順序：先單 thread 測 queue 基本 FIFO；再一個 producer/consumer；再測 close；最後才增加多個 producer/consumer。shutdown 初期使用 closed flag 協作退出，`pthread_cancel` 和 cleanup handlers 留到基本生命週期正確之後。

## 6. 用可控制的事件驗證等待與關閉

需要測的三個情境：consumer 等空 queue 時 close；producer 等滿 queue 時 close；queue 還有資料時 close 並 drain。

每個測試先安排容量和資料，再透過測試同步點／barrier／受鎖保護的狀態確認 worker 已進入預定階段。只 sleep 100ms 然後假設 worker 在等，可能讓錯誤測試偶爾通過。

所有 worker 必須在測試 deadline 內結束。Linux 測試 runner 可以在整個子程序外設 timeout，避免失敗把整套測試卡住。核對每筆 sequence 只出現一次，不能只看總數相同。

## 7. mutex 版本正確後，再寫 SPSC atomic ring

SPSC 表示恰好一個 producer、一個 consumer；MPSC／MPMC 是不同約束。不要把 01 共用 count 加上 atomic 就當作完整設計。

先畫資料發布關係：

1. producer 確認有可用空間後寫 payload。
2. producer 用 release store 發布新的寫入位置。
3. consumer 用 acquire load 看到該進度後才讀相應 payload。
4. consumer 完成讀取，以 release 發布可回收的位置。
5. producer acquire 觀察回收位置後才覆寫那個空間。

這是設計方向，不是完整算法；還要決定空／滿判斷、索引回繞、哪些欄位由誰寫，以及關閉流程。先沿用熟悉的保守 memory order，建立正確性論證後才討論放寬。

Reference：[C11 草案 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) §5.1.2.4 與 §7.17，查 data race、synchronizes-with、acquire/release。標準文件是精確查閱用，先用上面兩條方向的交接關係理解。`volatile` 不提供這種 thread 同步，atomic 型別也不自動保證整個 queue 正確或在每個平台 lock-free。

## 8. Sanitizer 和效能比較

先用 [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) 的 Usage 建立獨立 build，測 mutex 與 atomic 正式版本。它沒有報錯不等於邏輯沒有遺失／重複；仍需 sequence/checksum 測試。ASan 和 TSan 分開建置，不在同一 binary 混用。

效能測量另用適當最佳化且未開 sanitizer 的 build，與正確性測試分開記錄。固定資料量、payload 大小、容量、CPU 條件、等待策略；比較吞吐、p99 延遲與 CPU 時間。若 atomic 版本一直 busy-spin，必須把額外 CPU 消耗一起報告。

容量先測 1（依介面允許情況）、2、3、257，揭露只在 2 的冪次大小才碰巧正確的索引假設；若某設計保留空位，分清「storage slots」與「可用容量」。

## 9. 常見卡點與完成條件

| 現象 | 先查 |
| --- | --- |
| close 後卡住 | 是否喚醒兩種等待者？while 條件是否含 closed？ |
| 偶爾多／少一筆 | queue 修改是否都在鎖內？atomic 發布時機是否正確？ |
| deadlock | 是否持 mutex 呼叫了會再鎖同一把鎖的函式？等待時是否釋放？ |
| 測試偶爾通過 | 是否只靠 sleep 猜交錯？是否只驗總數沒驗 sequence？ |
| TSan 安靜但資料錯 | 檢查合約與演算法，data race 不是所有 bug 的總稱 |

第一輪完成 mutex queue 和 close 測試就有明確成果。再依 README 的 WAKE-01、RACE-01、ORDER-01、LAT-01 完成 SPSC、壓力與量測部分。
