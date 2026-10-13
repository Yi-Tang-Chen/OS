# Embedded C Toolkit

## 學習從這裡開始

先讀 [逐步教學 LEARNING.md](LEARNING.md)：用途與例子 → 必要概念 → 第一個小實作 → 驗證與排錯。重要概念附 reference、閱讀範圍與學習重點。

先讀教學第 1～3 節，理解 ring buffer 的用途、檔案分工與 `assert`，再做第 4 節的成功／失敗測試練習。

下方保留完整專案規格；初學時先按教學的小步驟前進，再回來對照驗收。

呼叫端提供儲存空間的 byte ring buffer，不在 library 內配置記憶體。目前已有 ring buffer 與單元測試；CRC 和串流 parser 尚待實作。

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

Windows 與 WSL 使用不同 build 目錄。[tests/test_ring_buffer.c](tests/test_ring_buffer.c) 涵蓋初始化參數、FIFO、空／滿、wraparound 與 peek。

## 下一步

1. 補容量 1、重複 wraparound、失敗時狀態與輸出保留的測試。
2. 加 CRC-8，固定 polynomial、initial value、reflection 與 xorout。
3. 定義 magic、version、length、sequence、payload、CRC，明訂端序與最大長度。
4. 寫不依賴 OS 的 streaming parser，支援逐 byte、多封包及壞資料後恢復。
5. 在 [02 Linux Service](../02-linux-sensor-simulator/README.md) 使用同一份 parser。

parser 驗收：一次、逐 byte、每個切點分兩段輸入應得到相同 frames；過長 length 與壞 CRC 不越界，後續合法 frame 可恢復。

[專案列表](../README.md)
