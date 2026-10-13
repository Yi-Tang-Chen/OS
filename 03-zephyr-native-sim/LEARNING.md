# 03 教學：用 Zephyr 把取樣、處理與傳送分成工作

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚未建立 Zephyr app。先學會在 native_sim 跑官方範例，再建立自己的應用；此教學的檔案和實驗是待實作項目。

## 1. 用途與例子

想像一個裝置每隔一段時間取樣，整理資料後傳出去。傳送可能比較慢，但你不希望「傳送還沒完成」讓所有工作都無法進行。

RTOS 提供 thread（可被排程的一條執行流程）、queue（工作之間傳資料）和等待機制。第一個成品可以只有兩條 thread：producer 產生序號 0、1、2；consumer 收到後印出相同序號。後面再分成取樣、處理、傳送三段。

native_sim 讓 Zephyr 程式在主機上模擬執行，便於學 API。你在電腦上量到的時間不能直接當作 MCU 的中斷延遲或最壞執行時間。

## 2. 先備概念和環境

先能讀懂 C 函式、struct、pointer，並知道函式返回後區域資料的生命週期。03 初期不要求先實作 09，但到 mutex 和優先序實驗前，要先讀 [09 的同步入門](../09-concurrency-memory-model/LEARNING.md)。

| 名詞 | 白話意思 | 先留意什麼 |
| --- | --- | --- |
| west | 管理 Zephyr workspace 與相關命令的工具 | workspace 和此作品集目錄是兩件事 |
| board | 建置選用的目標平台 | 這裡是 native_sim，不是真實板子 |
| Kconfig／prj.conf | 選擇哪些功能編入應用 | 設定開關不等於程式邏輯 |
| devicetree | 描述硬體與裝置配置 | 第一個 Hello World 通常不需自行改 overlay |
| thread priority | 決定可執行工作誰優先 | 不是每條 thread 平均分時間 |
| message queue | 以固定大小訊息在工作間交接 | 先傳值，減少 pointer ownership 問題 |

先選一個 release，保存 Zephyr commit、west manifest、Python／SDK／compiler 版本和 board 名稱。按所選版本的 [Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) 準備環境；線上 latest 的主機要求可能不同於你的 Ubuntu，不能因 01 能 build 就假設 Zephyr 所需工具也已齊全。

## 3. 第一個成果：官方 Hello World

先在外部 Zephyr workspace 完成官方範例。以下示意從該 workspace 的 `zephyr/` 原始碼目錄執行，前提是已按官方文件安裝依賴，且選定版本提供 `native_sim` board：

```sh
west build -b native_sim samples/hello_world -d build-hello
west build -d build-hello -t run
```

若版本需要特定 board qualifier，以該版本 [native_sim 文件](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html) 的 Building and running 為準，不要從其他版本隨意拼出名稱。

完成條件：看到 Hello World，知道如何停止模擬，並記錄實際命令。先改範例的一段輸出再重建，確認你執行的是剛編好的版本。尚不需要 sensor、queue 或三條 thread。

## 4. 建立自己的最小 app

之後才在本專案新增以下結構；建置輸出留在 build 目錄：

```text
app/
  CMakeLists.txt   告訴 Zephyr 要編譯哪些來源
  prj.conf         此 app 的功能設定
  src/main.c      先只印出自己的啟動訊息
notes/
  environment.md  使用版本與實際命令
```

先照官方範例的最小 CMake 接上 Zephyr，再從已啟用環境的 workspace 用 app 的實際路徑建置。成功後把 main 改成每次印出遞增序號，做固定次數後停止工作或進入可控等待。

學習點：主迴圈不能永遠忙碌地檢查時間；使用 Zephyr sleep 讓出 CPU。此時只證明程式會週期性執行，不宣稱 deadline 已獲得保證。

## 5. 兩條 thread 和一個 queue

先讀 [Message Queues](https://docs.zephyrproject.org/latest/kernel/services/data_passing/message_queues.html) 的 Concepts、Implementation，找出訊息如何複製、容量怎麼設定、滿／空時怎麼處理。再讀 [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) 的 lifecycle、priority。

小步實作：

1. 定義一個小 `sample` 結構，只有 sequence 和 value；value 先是固定公式產生的假資料。
2. 建立容量 4 的 message queue，只傳這個固定大小結構。
3. producer 建立一筆 sample，put 成功才記入 accepted；queue 滿時依你選的 timeout 規則回報並計數。
4. consumer 用 get 等待資料，取出後記錄 sequence。先傳 10 筆，核對完整順序。
5. 初版可讓 producer put 使用 K_NO_WAIT、滿時丟棄新資料；consumer 等待資料。再練限時等待，觀察兩種策略差別。

把一個 struct 放進 queue 不代表其 pointer 指向的資料也被深層複製。因此初版結構只放數值；之後接 payload 時再明訂誰分配、誰可修改、誰釋放。

驗證：先在不過載條件下收齊 10 筆；再把 consumer 變慢，確認 queue 最多 4 筆，且 produced、accepted、dropped、consumed、remaining 能對上。不要在初版用「送出 STOP 訊息一定成功」當作前提，滿 queue 時它也可能被拒絕。

## 6. 加測試，再擴展三段 pipeline

先讀 [Ztest](https://docs.zephyrproject.org/latest/develop/test/ztest.html) 的建立 test suite、assertions。第一個 ztest 可以測一個獨立的 sample 轉換函式：輸入固定 raw 值，輸出明確結果。先學會測試如何被建置與執行，再測 thread 排程。

接著把 consumer 拆成 processor 和 transmitter，形成兩個有界 queues。01 的純 C parser/CRC 若要整合，透過 app 的建置設定編入同一份 source，先跑固定向量；不要把 Linux socket 呼叫帶進純 C library。

測試分開做：資料轉換可用純函式測；queue 過載測容量和計數；排程與 timeout 測事件順序。用大量 log 觀察時，記錄 log 自己也可能改變執行時間。

## 7. 之後才學 mutex、優先序反轉與 watchdog

mutex 是保護共同資料的鎖；semaphore 是計數／通知工具，兩者不能因「都會等待」就隨意互換。參考 [Zephyr Mutexes](https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html)，先讀 ownership、priority inheritance。

優先序反轉練習要安排：低優先序先持鎖，高優先序之後等這把鎖，中優先序再開始工作。使用明確的同步事件控制先後，不要只靠三個隨意的 sleep 猜排程。保存 lock/unlock/block/wakeup trace，再解釋 priority inheritance 的效果與限制。

watchdog 先以軟體狀態表練習：每個重要工作回報最近完成的 sequence；監督者確認「系統仍有進展」才更新健康狀態。單個 sample 無效和整條 pipeline 卡住是不同事件。

## 8. 驗證、排錯與完成證據

| 階段 | 要留的證據 | 卡住先查 |
| --- | --- | --- |
| Hello World | 版本、board、建置與執行命令、輸出 | workspace 是否正確、host 依賴、board 是否存在 |
| 兩 thread | 10 筆序號與 queue 計數 | thread 是否啟動、stack 是否足夠、get/put timeout |
| 過載 | 深度上限、drop、控制訊息仍可處理 | 是否無限配置記憶體、是否把 log 當成傳送工作 |
| 三段流程 | 每段輸入輸出數量與錯誤計數 | 哪一個 queue 停止流動 |
| 優先序 | 可重現事件順序 | priority 類型／數值是否理解正確 |

入門完成後再對照 README 的 Q-01、RT-01、ERR-01、CFG-01。每組 queue depth、stack size、sampling period 分別測試。硬體 timing、ISR latency、功耗留到實板階段；native_sim 的可重現功能測試是此階段成果。
