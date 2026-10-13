# 11 教學：用證據找錯，再做公平的效能比較

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚無除錯報告。第 3 節可直接拿現有 01 練習；完整服務與並行實驗要等 02、09 有可執行版本。

## 1. 用途與例子

程式輸出錯誤時，先問「哪個操作讓狀態和預期不同」，再選工具觀察。假設容量 3 的 ring buffer 第二次繞回後取錯值，你可以在 push/pop 前後比較 head/tail/count，找到第一個不符合規則的位置。

本專案的成果是一份可重現的除錯紀錄：輸入、預期、實際、假設、工具證據、原因、修正與回歸測試。效能部分則回答某個已正確工作的程式，瓶頸在哪、改動後有沒有可量測改善。

## 2. 先知道每個工具看什麼

| 工具／概念 | 能幫忙看什麼 | 不代表什麼 |
| --- | --- | --- |
| GDB | 執行位置、呼叫堆疊、變數變化 | 加 breakpoint 的執行時間不是正式效能 |
| ASan | 部分越界、use-after-free 等記憶體錯誤 | 不代表所有邏輯都正確 |
| UBSan | 部分未定義行為，例如特定整數／對齊問題 | 沒報告不等於完全沒有 UB |
| TSan | data race 等執行緒同步診斷 | 不會替你驗證資料沒有重複 |
| strace | 程式向核心提出的 system calls 與結果 | 不是每個 C 函式都會成為 syscall |
| perf | 在環境允許時觀察 CPU／事件樣本 | 單一 counter 不自動解釋根因 |

先用一個工具回答一個問題。先保存未加大量 log 的可重現輸入，再依需要增加觀察。

## 3. 第一個練習：用 GDB 看 01 的一次 push

先按 [01 教學](../01-embedded-c-toolkit/LEARNING.md) 用 Debug build 跑通測試。以下假設使用 Linux 的單一組態 generator，從 repository 根目錄啟動：

```sh
gdb 01-embedded-c-toolkit/build/test_ring_buffer
```

進 GDB 後：

```text
break ring_buffer_push
run
bt
print *rb
print value
next
```

`break` 設定停點；`run` 開始；`bt` 顯示誰呼叫誰；`print` 讀取變數；`next` 執行下一個來源層級步驟。多次 next，觀察資料寫入、head 移動、count 增加，再用 continue 繼續。一次 next 不一定就完成整個 push。

完成條件：記下一次成功 push 前後的 head/tail/count，並指出這次呼叫來自哪一個測試函式。先預測，再觀察；不要只把 debugger 當成自動跑程式的工具。

Reference：[GDB Manual](https://sourceware.org/gdb/current/onlinedocs/gdb.html/) 的 Breakpoints、Continuing and Stepping、Backtrace、Examining Data。熟悉後再學 watchpoint：它用來抓某個值何時改變，需注意所觀察物件的生命週期。

## 4. 建立第一份除錯報告

之後新增 `reports/001-ring-buffer.md`，記錄：

1. 問題和輸入：最短哪一串操作會出現什麼現象。
2. 環境：commit、compiler、flags、執行命令。
3. 預期：來自哪個介面合約或測試。
4. 假設：例如「head 提早繞回」，以及你要觀察哪個值來驗證。
5. 證據：breakpoint、backtrace、數值或工具報告。
6. 原因與修正：哪個操作違反規則，為何修改能處理。
7. 回歸：原版失敗、修正版通過的測試與結果。

如果目前程式沒有已知 bug，先把第 3 節寫成「觀察練習」，不要捏造缺陷。之後從自己實際遇到的錯誤補正式報告。若刻意建立壞版本學工具，獨立保存並標示為教學負例，不混進正常測試。

## 5. 記憶體錯誤：ASan／UBSan

先讀 [AddressSanitizer Usage](https://clang.llvm.org/docs/AddressSanitizer.html) 和 [UndefinedBehaviorSanitizer Usage](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)。重點是編譯與連結都需相應 flags；留下符號資訊有助於定位。工具和平台支援先確認。

替學習負例建立獨立可執行檔：一次只放越界、use-after-free 等其中一種可控問題，使用 timeout 在子程序跑，預期非零退出／明確報告。觀察報告中的錯誤存取位置與 allocation/free stack，不把最後 crash 的位置直接等同真正原因。

修正後對相同輸入重跑，再把「合法邊界應成功、超過邊界應被拒絕」放進正常 regression test。TSan 的並行檢查接 [09 教學](../09-concurrency-memory-model/LEARNING.md)，與 ASan 分開 build。

## 6. 系統行為：先追一個 fd，再做 mmap 與持久性

有 02 後，先用 strace 追 read/write/poll/close，核對你以為的 I/O 行為。再使用跟蹤子程序的選項觀察 fork 後哪個 process 還持有 fd。

References：[strace(1)](https://man7.org/linux/man-pages/man1/strace.1.html)，讀 trace 過濾、追子程序與輸出選項；[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)、[fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)，查 fd 繼承與 FD_CLOEXEC。

接著各做一個小實驗，不同主題分開記錄：

- fd/EOF：父子 process 持有同一 pipe/socket 的端點，逐一關閉，觀察何時讀到 EOF。
- mmap/page fault：固定大小映射，按頁接觸，量第一次與後續存取；記錄 page faults，不把 page fault、cache miss、磁碟 I/O 視為同一事件。
- 檔案持久性：寫 temporary file、同步、rename、同步目錄；按選定 filesystem 的保證安排 crash 實驗。rename 的原子可見性與資料在 crash 後持久存在是不同問題。

References：[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) 的 MAP_PRIVATE/MAP_SHARED；[fsync(2)](https://man7.org/linux/man-pages/man2/fsync.2.html) 的 directory 同步說明；[rename(2)](https://man7.org/linux/man-pages/man2/rename.2.html) 的替換與原子性。不要在日常資料檔上做故障練習，使用專案測試目錄／guest。

## 7. 效能：先把量測問題寫成一句話

例如：「在固定 64-byte payload、固定 rate 下，02 從接收完整 frame 到 sink 完成的延遲是多少？」先選起點與終點，再選時鐘與收集方式。

參考 [clock_gettime(3)](https://man7.org/linux/man-pages/man3/clock_gettime.3.html)，理解單一主機的 duration 為何選單調時鐘，而不是可能校時的日曆時間。跨機量測還有時鐘同步問題，本階段先做單機。

1. 保存 baseline 的 commit、flags、CPU、輸入資料、queue 容量。
2. 先 warm up 並明訂是否計入，固定測試期間；至少做多次重複。
3. 保存原始 samples，計算 median、p95/p99、吞吐與 CPU，標示各指標的樣本單位與數量。
4. 一次只改一個因素，再用同樣條件重測。
5. 正確性測試仍須通過；不能靠漏處理資料獲得較低延遲。

10 次整體執行時間適合觀察 run-to-run 變化；若要報每筆 frame 的 p99，需要足夠的逐筆樣本，不能把 10 個 run 的數字混稱為單筆延遲分布。正式效能測量與 debugger／sanitizer 診斷分開執行。

## 8. 最後寫一個 runner

先針對 02、04 或 12 其中一個已有命令的案例。之後建立 `tools/run_case.py`，輸入案例名稱／參數，啟動程序，設定 timeout，保存 stdout/stderr、退出碼與 CSV。

Reference：[Python subprocess](https://docs.python.org/3/library/subprocess.html)，讀 run/Popen、timeout、check；程序會生子程序時，再學 process-group 清理，避免 timeout 後背景程序還留著。先手動成功一遍，再自動化同一命令。

卡住時先把「觀察」「假設」「結論」分開：strace 看見很多 read 是觀察；認為 syscall 太多是猜測；相同輸入下減少 syscall 並量到改善才是支持該猜測的證據。最後依 README 的 DBG-01、VM-01、FD-01、PERF-01 完成報告。
