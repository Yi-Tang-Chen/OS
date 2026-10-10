# 有背壓與故障恢復的 Linux 資料服務

狀態：規劃中，目前只有規格。
環境：WSL2/Linux；用 PTY、socketpair 或 Unix domain socket，暫不需要板子。
前置：01 的 parser library，以及檔案 I/O、callback 和 partial write。

## 要交付什麼

producer → 非阻塞 transport → parser → 有界佇列 → sink。把可靠性規則寫清楚：服務不因單一壞來源崩潰；輸入快於輸出時有明確的丟棄／暫停策略。

## 分層實作

- L1：一個 producer 與一個 daemon，使用 poll/epoll，正確處理短讀、短寫、EOF、EINTR、EAGAIN。先用 level-triggered。
- L2：加入多來源、公平處理預算、重連退避、sequence 遺失計數、SIGTERM 可預期關閉；所有 queue 設容量上限。
- L3：比較 level/edge-triggered、實作 read-until-EAGAIN、公平性與背壓；做分離執行緒版本並在09專題分析同步。

## 效能與資源

固定 rate 與 frame size，量 p50/p95/p99 延遲、吞吐、CPU、RSS、fd 數、丟棄數；先收 baseline，再給 10 倍過載，不能只報平均值。

## 第一個動作

用 socketpair/PTY 寫一個會把 frame 切成 1-byte chunks 的 producer，記錄 fd ownership。

## 補強：TCP／UDP 與自動壓測

先完成單機 socketpair，再把同一個 parser 接到 TCP；UDP 是後續比較，不需重寫另一套服務。

- TCP：明訂 length／端序／最大 frame；處理分段與多個 frame 合併到一次 read、短寫、EOF、EINTR／EAGAIN、非阻塞 connect 與 timeout。TCP 不保留訊息邊界。
- UDP：每個 datagram 放一筆含 sequence 的資料；偵測截斷、遺失、重複與亂序，明訂丟棄策略，不假設可靠送達。
- 用 Python 或 shell 控制 producer、斷線及慢 consumer，輸出 CSV：傳送／接收／丟棄數、CPU、queue 上限、p95／p99。先在 loopback 測，不能稱為實體網卡效能。

驗收：TCP 任意切塊仍得到相同 frame；斷線重連不串接舊 session；慢 consumer 時記憶體有界；UDP 用可重現的注入器製造遺失／亂序，計數符合預期。socket 開發不等於 Ethernet driver 經驗。

## 核心驗收

以下均為待驗收情境，並非已通過結果。

| ID | 觸發方式 | 必須驗證 | 結果／證據 |
| --- | --- | --- | --- |
| IO-01 | 每次只傳 1–7 byte，混合短寫與 EAGAIN | frame 不重複、不截斷，剩餘資料保留到下一次 writable。 | 待實作 |
| REC-01 | 10 次斷線／重連，來源在半個 frame 中途消失 | parser 狀態依 session 規則重設；fd 回到基準值。 | 待實作 |
| LOAD-01 | 4 個 producer 其中一個持續灌入，sink 每秒停 100ms | 佇列有界，其餘來源不飢餓；每筆遺失可由計數說明。 | 待實作 |
| STOP-01 | 忙碌中 SIGTERM，10 秒內 sink 無回應 | 2 秒內依文件定義 drain 或丟棄並退出；無永久等待。 | 待實作 |

## 必要閱讀

- [TCP：先讀 DESCRIPTION，理解 byte stream](https://man7.org/linux/man-pages/man7/tcp.7.html)。
- [UDP：datagram 邊界與錯誤](https://man7.org/linux/man-pages/man7/udp.7.html)。
- [epoll：先做 level-triggered](https://man7.org/linux/man-pages/man7/epoll.7.html)。

[回主路線](../README.md)
