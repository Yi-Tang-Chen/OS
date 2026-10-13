# 02 教學：讓資料在 Linux 程式之間可靠流動

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚無服務實作。先完成本教學的 3-byte 傳輸練習，再把 01 的 parser 接進來。下列檔名是之後動手時才建立的建議，不是已存在的程式。

## 1. 用途與具體例子

感測器每秒送 100 筆資料，儲存端有時停頓。你的服務必須持續接收、辨識完整訊息、暫存資料，再交給儲存端。資料來太快時，要有能解釋的處理方式。

先用軟體模擬：producer 產生帶序號的資料，service 接收並解析，sink 把結果印出或存檔。最小資料路徑是 `producer → socket → parser → sink`；queue 是處理速度不同後才加入的部分。

例子：producer 送出 sequence 1、2、3，接收端印出三筆相同序號。即使某筆被拆成三次傳送，也只能被交付一次。這與 01 parser 的分段測試直接相接。

## 2. 先學六個名詞

| 名詞 | 先用這個意思理解 | 這裡會用在哪裡 |
| --- | --- | --- |
| process | 正在執行的程式實例 | producer 和 service 日後可分別執行 |
| fd | 程式用來指向已開啟資源的整數代號 | read/write/close 的參數 |
| socketpair | 建立一對已連通的 socket | 不用設定 IP，就能練傳輸 |
| byte stream | 一串有順序的 bytes，沒有每次 write 的訊息邊界 | 收到的 chunk 不等於一個 frame |
| blocking | 暫時不能進行時，呼叫可能等待 | 第一個練習先用它 |
| nonblocking | 暫時不能進行時，回傳錯誤狀態讓程式處理其他工作 | 之後搭配 poll |

先讀 [socketpair(2)](https://man7.org/linux/man-pages/man2/socketpair.2.html) 的 SYNOPSIS、RETURN VALUE；找出「兩個 fd 從哪裡回來」與「失敗回傳什麼」。再讀 [read(2)](https://man7.org/linux/man-pages/man2/read.2.html)、[write(2)](https://man7.org/linux/man-pages/man2/write.2.html) 的 RETURN VALUE。重點是回傳的 byte 數可能小於你要求的數量。

## 3. 第一個程式：同一個 process 傳送三個 bytes

只先新增 `examples/socketpair_demo.c`，不必先拆多個 process 或建立服務框架。

中文流程：

1. 宣告 `int sv[2]`；用 `socketpair(AF_UNIX, SOCK_STREAM, 0, sv)` 建立兩端，失敗就印錯誤並回傳非零。
2. 準備 `uint8_t` 陣列 `{0x10, 0x20, 0x30}`，從 sv[0] 寫入。用 offset 記住已寫入幾個 bytes，直到這個小陣列送完。
3. 對 sv[0] 做 `shutdown(sv[0], SHUT_WR)`，表示這一端不再送資料，仍保留 fd 供最後統一 close。
4. 從 sv[1] 反覆讀，直到回傳 0。每次只印出真正讀到的 bytes，印十六進位；不要把 binary buffer 當作以 `\0` 結尾的字串。
5. 比較累積資料是否剛好是 `10 20 30`，close 兩端；完全符合才回傳 0。

步驟 3 的行為查 [shutdown(2)](https://man7.org/linux/man-pages/man2/shutdown.2.html) 的 SHUT_WR。如此接收端知道資料已結束，不會在收到三個 bytes 之後永久等下一筆。

在你寫好檔案後，從此專案目錄執行：

```sh
mkdir -p build
cc -std=c11 -Wall -Wextra -Werror -g examples/socketpair_demo.c -o build/socketpair_demo
./build/socketpair_demo
```

完成條件：內容一致、累積恰好 3 bytes、程式自行結束、兩個 fd 都關閉。每次 read 分成幾塊不是驗收條件。同一 process 先寫後讀只適用這個很小的示範；大量資料可能把 socket buffer 塞滿，接著就需要分 process 或事件迴圈。

## 4. 把「我以為」改成清楚的 I/O 規則

使用 `ssize_t` 接 read/write 回傳值，因為它需要表示 -1。先判斷是否大於 0，再拿它當成有效長度。

| 結果 | 程式應做的事 |
| --- | --- |
| read 回傳正數 | 處理這次真正取得的 bytes |
| 對 stream 做非零長度 read 回傳 0 | 到達 EOF，依合約清理連線 |
| 回傳 -1 且 errno 是 EINTR | 檢查停止要求，再決定重試 |
| 回傳 -1 且 errno 是 EAGAIN/EWOULDBLOCK | 非阻塞時現在無法進行，回事件迴圈等待 |
| write 回傳正數但小於剩餘長度 | 前面已送出，只能重送尚未送出的後段 |
| 其他錯誤或非零長度 write 無進展 | 清楚回報／終止該次操作，不做無限重試 |

第一個 blocking 練習先處理成功、EOF、EINTR 與其他錯誤；nonblocking 時才會刻意測 EAGAIN。errno 只在呼叫表示失敗後查看。

練習把接收 buffer 大小改成 1，再改成 2。每次分塊不同，最後資料仍一致，才表示你沒有依賴「一次 read 就讀完」。記錄每次回傳值幫助理解，但正式壓測不能讓大量 printf 成為瓶頸。

## 5. 接上 01 parser，再拆 producer 與 service

1. 先把 3 個 bytes 換成 01 定義的一個合法 frame，在接收端把每次 read 的有效區段餵給 parser。
2. callback 先只保存或印出 sequence 和 payload。半包不能呼叫 callback，完整包只呼叫一次。
3. 再送兩個 frames，依序用 1-byte chunks 與整段傳送，確認結果相同。
4. 新增 CMakeLists，把 01 作為同一份 library 連結；可由此專案用 `add_subdirectory(../01-embedded-c-toolkit ...binary-dir...)` 收入，別複製 parser 原始碼後分開維護。語法查 [CMake add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html)。
5. 再學 `fork` 或具路徑的 Unix domain socket。若用 fork，每個 process 關閉不屬於自己的端點，畫一張 fd ownership 表。

建議此時才整理為 `src/producer.c`、`src/service.c`、`tests/`。producer 只管產生資料；service 處理連線與 parser；sink 初期可只是 service 中的一個函式。

Reference：[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)，讀 fd 繼承；[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)，讀 pathname sockets。讀完要能解釋「為何另一個 process 還拿著寫端時，接收方可能等不到 EOF」。

## 6. 加入 poll 與有界佇列

事件迴圈的概念是「等到某個 fd 可讀／可寫，再做一小段工作」。先用 poll；懂了以後才換 epoll。用 `fcntl` 讀取既有 flags 後加上 O_NONBLOCK，不要不小心覆蓋其他 flags。

1. 先只監聽一個讀端，poll 到可讀後讀資料，暫時無資料就回到等待。
2. 加入寫端的 pending buffer，保存 offset；只有真的有待送資料時才監聽可寫，避免 idle 時不停醒來。
3. 加入容量 8 筆的 frame queue。queue 是傳完 parser 後的完整 frame，不等於 01 存 bytes 的 ring buffer。
4. 先選「滿時丟棄新 frame」這一種策略，增加 drop counter；之後再比較暫停讀取的背壓策略。這是教學起點，須寫入服務合約。
5. 把 sink 改成每次 poll 迴圈依 deadline 消費，或用獨立慢 consumer；不要在唯一事件迴圈裡長時間 sleep，否則所有來源一起停住。

Reference：[poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html)，先讀 events/revents、POLLIN、POLLOUT、POLLHUP；[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)，等 poll 正確後再讀 level/edge-triggered。readiness 是此刻可能有進展，不保證能一次讀寫完整 frame。

## 7. 最後加入故障與量測

逐一新增，前一項通過再做下一項：

- 半包斷線再重連：新 session 重設 parser，舊尾巴不能接上新資料。每次重連記錄 fd 數，不能持續增加。
- 寫入端遇到 peer 關閉：處理 EPIPE 和 SIGPIPE；socket 可研究 `send` 的 MSG_NOSIGNAL，不能讓測試程序意外被信號終止。參考 [send(2)](https://man7.org/linux/man-pages/man2/send.2.html)。
- 多來源：每次給每個來源固定工作預算，防止一個持續可讀的 fd 占滿迴圈。
- SIGTERM：定義剩餘資料 drain 或丟棄的期限，記錄結果；不要在 signal handler 裡直接做一般清理與 printf。
- TCP：保留 parser，只更換連線方式；完成後才比較 UDP 的 datagram、遺失和亂序。

量測先保存「產生、解析成功、丟棄、待處理」的數量與測試區間，核對資料去向。吞吐是單位時間完成幾筆；延遲要先定義從哪個時間點算到哪個時間點。再加 CPU、RSS、fd、p50/p95/p99，避免一開始同時追所有指標。

## 8. 驗證與卡關提示

| 現象 | 最小重現／先查哪裡 |
| --- | --- |
| 收完資料仍卡住 | 寫端是否 shutdown/close？fork 後是否還有人持有寫端？ |
| 資料重複 | 短寫後是否又從 offset 0 重送？ |
| parser 收到奇怪尾巴 | 有沒有把 buffer 容量當成 read 回傳長度？ |
| idle CPU 很高 | 是否無條件監聽 POLLOUT、或遇到 EAGAIN 立刻忙迴圈重試？ |
| 慢 consumer 導致記憶體一直增加 | 所有 pending buffers 與 queues 是否都有上限？ |

完成第一輪後回到 README 的 IO-01、REC-01、LOAD-01、STOP-01，逐項建立可重跑的測試。今天的第一個成果仍然只是第 3 節的 3-byte 程式。
