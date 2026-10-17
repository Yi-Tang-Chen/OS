# Embedded C Toolkit

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先讀教學第 1～3 節，理解 ring buffer 的用途、檔案分工與 `assert`，再做第 4 節的成功／失敗測試練習。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

目前已有 byte ring buffer、CRC-8，以及支援分批接收的基礎 parser，三者都有測試。library 使用固定容量或呼叫端提供的儲存空間，不動態配置記憶體。

## 01 的整體概念

目標是把「收到的原始 bytes」安全地整理成可以處理的訊息，並把邏輯做成不依賴特定 OS 的 C library，供後續專案使用。

| 工具 | 工作 | 練習到的觀念 |
| --- | --- | --- |
| ring buffer | 依 FIFO 順序暫存、取出 bytes | 陣列、head/tail/count、取餘數、空／滿、固定容量 |
| CRC-8 | 計算檢查值，協助偵測資料錯誤 | 十六進位、XOR、AND、位移、無號整數、固定測試向量 |
| frame parser | 保存半包、按標頭長度收齊、驗證後通知使用者 | 指標、結構、跨呼叫狀態、邊界檢查、callback |
| tests + CMake | 建置 library 和測試程式，檢查預期行為 | .h/.c 分工、連結、assert、CTest、正常與錯誤案例 |

之後可以把它們組合成「資料來源 → ring buffer 暫存 → parser 收齊並呼叫 CRC 驗證 → callback 處理結果」。目前 parser 用自己的線性 buffer，尚未與 ring buffer 串接；這兩種 buffer 各有用途，不必強制合併。

`buffer` 的容量、`used` 的有效資料量、標頭的 payload `length` 是不同概念。`feed()` 接收的是這次到達的 bytes，一次輸入不一定剛好等於一個封包。初始化後才能使用 parser，reset 代表放棄先前未完成的接收進度。

目前 callback 是 `void (*)(void)`，只通知收到有效封包，還沒有輸出 `frame_t` 或 payload。`feed()` 回傳 true 表示本次資料已處理或保存，可能仍在等待半包；遇到錯誤就 reset 並回傳 false，後面的輸入不會自動繼續處理，也沒有回報消耗的 byte 數。NULL callback 可略過通知；NULL data 一律拒絕，即使長度為 0。

## 介面

宣告在 [include/ring_buffer.h](include/ring_buffer.h)，實作在 [src/ring_buffer.c](src/ring_buffer.c)。

- `init`：綁定 storage 與 capacity；容量必須大於 0。
- `push / pop`：依 FIFO 順序存取；滿時不能寫，空時不能讀。
- `peek`：讀取下一個 byte，保留資料。
- `reset`：清空邏輯狀態，不清除 storage 內容。
- `size / capacity / is_empty / is_full`：查詢狀態。

storage 由呼叫端擁有，必須比使用期間活得更久。初始化成功後再操作；目前是單執行緒版本，沒有 ISR／thread 同步。

## 建置與測試

在這個資料夾執行：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Windows 與 WSL 使用不同 build 目錄。CTest 會列出 3 個測試程式，每個程式內還有多個測試函式：

- [test_ring_buffer.c](tests/test_ring_buffer.c)：初始化參數、FIFO、空／滿、wraparound、peek、容量 1。
- [test_crc8.c](tests/test_crc8.c)：空資料、單 byte、固定向量及包含零值的資料。
- [test_frame_parser.c](tests/test_frame_parser.c)：12 個測試函式，涵蓋完整封包、所有切點分兩段、逐 byte、一次五包、完整包後接半包、payload 長度 0/32、payload 內含 magic、錯誤標頭／CRC、reset、NULL 與零長度輸入。

parser 測試的 `on_frame()` 累加計數器，驗證何時通知與通知次數；目前尚不能測試 callback 輸出的欄位內容。固定 CRC 預期值已預先獨立計算。CMake 在測試 target 取消 `NDEBUG`，確保 Release 下的 `assert` 也會執行。

## 下一步

1. 讓 callback 接收解析完成的 `frame_t`，解析 sequence 的大端序，並測試輸出內容及資料有效期限。
2. 加入雜訊前綴、錯誤資料後重新搜尋封包的能力，處理重疊 magic；目前測試只確認遇錯停止及下一次獨立輸入可以重新接收。
3. 補 ring buffer 重複 wraparound、失敗時狀態與輸出保留的測試。
4. 整理協定文件及錯誤／等待行為；合法但錯誤的 length 可能需要等待更多資料或由呼叫端 reset。
5. 在 [02 Linux Service](../02-linux-sensor-simulator/README.md) 使用同一份 parser。

完整 parser 的後續驗收：一次、逐 byte、每個切點分兩段輸入應得到相同 frames 內容；過長 length 與壞 CRC 不越界，能在約定的錯誤恢復規則下重新接收後續合法 frame。

[專案列表](../README.md)
