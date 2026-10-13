# 01 教學：從看懂測試，到自己寫 C 資料處理工具

這份教學從目前已存在的 ring buffer 開始。你不需要先懂 RTOS、Linux driver 或通訊協定。第一次只做到第 3 節；CRC 和 parser 是後面的獨立練習。

搭配 [專案規格](README.md) 使用；整體閱讀方式在 [學習入口](../LEARNING.md)。文中的「練習」與預期輸出是你接下來要完成的工作，不代表已經實作。

## 1. 用途：我們究竟在做什麼？

假設感測器送來三筆資料，但處理程式還在忙。你需要一個地方先保存資料，等有空再取出。

ring buffer 是固定容量、先進先出（FIFO）的容器。本專案一次存一個 byte，使用呼叫端提供的陣列。它不會自己向系統申請更多記憶體；滿了就回報失敗。

容量為 3 時，外部可觀察到的行為如下。這裡的「等待資料」是邏輯順序，不一定等於陣列從索引 0 開始的排列。

| 動作 | 等待資料 | 呼叫結果 |
| --- | --- | --- |
| 初始化 | 空 | 成功 |
| push 10 | 10 | 成功 |
| push 20 | 10、20 | 成功 |
| push 30 | 10、20、30 | 成功，已滿 |
| push 40 | 10、20、30 | 失敗，原資料保留 |
| pop | 20、30 | 成功，取到 10 |
| push 40 | 20、30、40 | 成功，使用剛空出的空間 |

01 最後會包含三個不同工具：ring buffer 負責暫存 bytes；CRC 負責計算錯誤檢查值；parser 負責從 bytes 辨認完整訊息。先學會一個，再加入下一個。

## 2. 先認識檔案和 C 語法

| 現有檔案 | 負責什麼 | 第一遍先看哪裡 |
| --- | --- | --- |
| [include/ring_buffer.h](include/ring_buffer.h) | 資料結構與函式宣告，讓使用者知道怎麼呼叫 | `ring_buffer_t`、`init`、`push`、`pop` |
| [src/ring_buffer.c](src/ring_buffer.c) | 函式實作，決定每個操作怎麼改變資料 | `ring_buffer_init` |
| [tests/test_ring_buffer.c](tests/test_ring_buffer.c) | 操作 buffer 並檢查結果 | `main`、`test_empty_buffer` |
| [CMakeLists.txt](CMakeLists.txt) | 告訴建置工具哪些原始碼要編成 library、哪些要編成測試 | `add_library`、`add_executable`、`add_test` |

library 是供其他程式使用的一組函式；本專案的測試程式有 `main()`，會連結這個 library 並呼叫其中的功能。因此「編譯 library 成功」和「執行測試成功」是兩個不同步驟。

先把下面這段當成語法閱讀練習，不必新增檔案：

```c
ring_buffer_t rb;
uint8_t storage[4];
uint8_t value = 0xAA;
bool ok = ring_buffer_init(&rb, storage, sizeof(storage));
```

- `ring_buffer_t rb`：宣告一個保存 buffer 管理資訊的物件；這時還沒初始化，不能先 push/pop。
- `uint8_t storage[4]`：宣告能存 4 個 8-bit 無號整數的陣列；資料真正放在這裡。
- `0xAA`：十六進位，等於十進位 170；測試選它作為容易辨認的原始值。
- `&rb`：取得 rb 的位址，讓函式能修改同一個 rb。`&value` 同理。
- `storage`：在這次函式呼叫中，陣列轉成指向第一個元素的指標。
- `sizeof(storage)`：此處得到整個陣列的大小，為 4 bytes。在接收指標的函式裡，`sizeof(pointer)` 不能用來找回陣列長度。
- `bool ok`：保存函式回傳的成功或失敗；成功是 `true`，失敗是 `false`。

讀實作時還會遇到 `rb->count`：rb 是指標，`->` 用來存取它指向的結構成員。若手上是物件本身，則寫 `rb.count`。`*out_value = ...` 是把資料寫到指標所指的位置，因此呼叫端的 value 會改變。

`==` 是比較，`=` 是指定值；`!` 是邏輯反向；`||` 是「或」且有短路求值，例如 `rb == NULL || rb->count == 0` 在 rb 為 NULL 時不會讀取右邊。`size_t` 是用來表示大小的無號整數型別；`const ring_buffer_t *` 表示不能透過該指標修改結構本身。

先能說出「資料存在 storage，管理資訊存在 rb」即可。入門閱讀使用 [GNU C Language Manual](https://www.gnu.org/software/c-intro-and-ref/manual/html_node/index.html)，依序找 Pointers 的基本宣告與 dereference、Arrays 的索引存取、Structures 的成員存取、Functions 的參數與回傳值。每讀一段，就回來對照上面的四行宣告；GNU 擴充功能暫時略過，本專案使用 C11。

需要查精確語言規則時，再參考 [C11 委員會草案 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) 的 §6.3.2.1（陣列轉換）、§6.5.2.3（成員存取）、§6.5.3.2（位址與間接存取）。這是查閱資料，不要求從頭讀完。

## 3. `assert` 是什麼？測試到底在測什麼？

`assert` 是 `<assert.h>` 提供的巨集。啟用時，括號內的條件成立就繼續；不成立就輸出診斷並終止程式。它不是印出答案的函式。

```c
int value = 3;
assert(value == 3);  /* 成立，繼續 */
assert(value == 5);  /* 不成立，停在這裡 */
```

對照現有 `test_empty_buffer`，先逐行翻成中文：

| 程式 | 想確認什麼 |
| --- | --- |
| `assert(ring_buffer_init(&rb, storage, sizeof(storage)));` | 初始化應該成功 |
| `assert(ring_buffer_is_empty(&rb));` | 剛初始化應該是空的 |
| `assert(!ring_buffer_is_full(&rb));` | 剛初始化不應該是滿的 |
| `assert(ring_buffer_size(&rb) == 0);` | 目前應有 0 筆資料 |
| `assert(ring_buffer_capacity(&rb) == 4);` | 最多可存 4 筆 |
| `assert(!ring_buffer_pop(&rb, &value));` | 從空容器取資料，應回報失敗 |
| `assert(value == 0xAA);` | 失敗時，原本的輸出變數應保持原值 |

測試通常分為三件事：準備初始狀態、做一個操作、檢查結果。現有檔案每個 `static void test_...(void)` 就是一組情境；`static` 在這裡讓函式只在此原始碼檔案內可見，`void` 表示沒有回傳值／沒有參數。`main()` 依序呼叫它們。

**重要限制：定義 `NDEBUG` 時，assert 的條件連執行都會被省略。** 現有測試把 init/push/pop 放在 assert 裡，所以要先用 Debug，且不要自行加入 `-DNDEBUG`。Release 顯示通過，不能當作這份測試真的執行了操作。新增測試可先把操作與檢查分開：

```c
bool ok = ring_buffer_push(&rb, 42);
assert(ok);
```

這能讓操作不因 assert 被關閉而消失，但 assert 關閉時仍沒有驗證；日後支援 Release 測試，需採用不受 NDEBUG 影響的檢查機制。產品程式的外部輸入錯誤則要用正常的 `if` 與錯誤回傳處理。

Reference：[assert(3)](https://man7.org/linux/man-pages/man3/assert.3.html)，先讀 DESCRIPTION、BUGS，回答「為何不能依賴 assert 一定執行有副作用的函式？」。

## 4. 第一次動手：跑出成功，也看一次失敗

以下命令從 repository 根目錄執行：

```sh
cmake -S 01-embedded-c-toolkit -B 01-embedded-c-toolkit/build -DCMAKE_BUILD_TYPE=Debug
cmake --build 01-embedded-c-toolkit/build --config Debug
ctest --test-dir 01-embedded-c-toolkit/build -C Debug --output-on-failure
```

第一行產生建置設定；`-S` 指原始碼目錄、`-B` 指建置產物目錄。第二行編譯並連結。第三行執行 CMake 已登記的測試。

預期 CTest 顯示 `ring_buffer` 通過。目前只有 **1 個 CTest 測試程式，裡面呼叫 6 個測試函式**，不是 CTest 會列出六列。

1. 找到 `test_empty_buffer` 裡「capacity 等於 4」的檢查。
2. 暫時把預期的 4 改為 5。
3. 重新 build，再跑 CTest，預期失敗訊息指出該 assert。
4. 改回 4，重新 build/test，確認恢復通過。

這次練習是確認你知道測試如何判定對錯；最後保留正確版本。若改了程式結果卻沒變，先查是否重新 build、是否用了同一個 build 目錄。

Reference：[CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)，只先看 executable、library、linking 與 Testing/CTest。線上 tutorial 的工具版本可能高於本機；理解概念後以本專案 CMakeLists 與本機 `cmake --help` 為準。

## 5. 看懂 ring buffer 為什麼能繞回去

本專案定義 head 是「下一個寫入位置」，tail 是「下一個讀取位置」，count 是「現在有幾筆」。其他 library 命名可能不同。

容量 3，依序操作：

| 操作後 | head | tail | count | 原因 |
| --- | --- | --- | --- | --- |
| init | 0 | 0 | 0 | 還沒有資料 |
| push 10 | 1 | 0 | 1 | 寫入 storage[0] |
| push 20 | 2 | 0 | 2 | 寫入 storage[1] |
| push 30 | 0 | 0 | 3 | 寫入 storage[2]，下一格繞回 0 |
| pop → 10 | 0 | 1 | 2 | 從 storage[0] 讀取 |
| push 40 | 1 | 1 | 3 | 重新使用 storage[0] |

`(index + 1) % capacity` 就是繞回的方法：`(2 + 1) % 3` 得到 0。head 等於 tail 可能是空，也可能是滿，因此此實作另外保存 count。

練習：不執行程式，先寫出最後三次 pop 的值，再用 `test_wraparound` 對照。預期依序 20、30、40。可以用 GDB 看 head/tail，但先用紙筆即可。

讀完 push/pop 後檢查三個規則：`count <= capacity`；有效索引小於 capacity；失敗操作不改動原有資料與邏輯狀態。前提是使用成功初始化、未被外部破壞的 rb。storage 的生命週期必須涵蓋使用期間；不要回傳指向已離開函式的區域陣列的 buffer。

## 6. 第一個自己寫的功能：補容量 1 的測試

位置：現有 `tests/test_ring_buffer.c`。新增自己的測試函式，然後在 `main()` 呼叫。先用中文寫預期，再翻成呼叫與 assert。

1. 宣告 rb、`storage[1]` 與輸出 value，初始化。
2. 確認 empty、size 0、capacity 1。
3. push 42 成功，確認 full、size 1。
4. push 99 失敗，確認 size 仍為 1。
5. peek 得到 42，確認仍為 full。
6. pop 得到 42，確認 empty。
7. 再 push 7、pop 得到 7，確認同一格能反覆使用。

通過後一次只新增一種情境：重複繞回多圈；空時 peek 不改 value；reset 後可以重新使用；滿時 push 不改 head/tail/count。不要把「取出的值剛好等於預期」當成所有狀態都正確。

reset 只清除邏輯狀態，陣列中舊值仍可能存在，不能要求所有 storage bytes 都變成 0。這是介面合約的選擇。

## 7. 下一個工具：CRC，先只算一串固定 bytes

CRC 是依照固定規則算出的檢查值。傳送方與接收方對同一批資料計算後比較，用來偵測部分資料錯誤；它不提供加密或身分認證。

先補的知識：十六進位、XOR `^`、AND `&`、位移、無號整數。用紙筆算 `0x03 ^ 0x01 == 0x02`，再看多項式算法；不要一開始就做查表最佳化。

這份教學建議固定一個練習用 CRC-8：width 8、polynomial `0x07`、initial `0x00`、輸入輸出均不反射、xorout `0x00`。`0x07` 表示省略最高次項後的係數；完整多項式是 x^8 + x^2 + x + 1。多項式的實際使用例可看 [SMBus 2.0 §5.4.1](https://smbus.org/specs/smbus20.pdf)。此處借用 CRC 規則，封包格式並不是 SMBus。

動手順序：

1. 建議新增 `include/crc8.h`，先定義輸入是 byte pointer 與長度、回傳是 `uint8_t`。明訂長度為 0 的行為與 NULL pointer 合約。
2. 新增 `src/crc8.c`，先寫逐 bit 版本：每個 byte 與目前 CRC XOR，處理 8 次最高位、左移與 polynomial XOR，結果保留 8 bits。
3. 新增 `tests/test_crc8.c`，先測空資料回傳 0，再測 ASCII `123456789`，上述參數應得到 `0xF4`；不要把字串結尾的 `\0` 算進去。
4. 在 CMakeLists 把新 `.c` 加入 library，另外建立並登記 CRC 測試程式。新增檔案不會自動被目前的 CMake 收進去。
5. 加入一個固定 binary input 含 `0x00` 的案例；資料長度由參數傳入，不用 `strlen` 計算任意二進位資料。

完成條件：能解釋每個 CRC 參數、固定向量可重跑，測試期望值不能只是呼叫被測函式產生。尚未需要 socket 或 parser。

## 8. 最後才做 parser：從零碎 bytes 找回一個 frame

frame 是一筆完整訊息。例如「第 7 筆感測資料，payload 為 10、20」。transport 可能先交付前 2 bytes，再交付其餘 bytes，也可能一次送來兩筆。

parser 的工作是記住處理到哪裡，等資料湊齊並驗證後才交付 frame。這裡提出一個**尚待實作的教學格式**，可先寫到你之後建立的 `docs/protocol.md`：

| 欄位 | bytes | 教學約定 |
| --- | --- | --- |
| magic | 2 | 固定 `A5 5A`，辨識可能的開始 |
| version | 1 | 目前只接受 1 |
| length | 1 | payload 長度，允許 0～32 |
| sequence | 2 | 無號 16-bit，大端序：高 byte 在前；65535 後回到 0 |
| payload | length | 原始資料，允許任意 byte，包括 magic |
| CRC | 1 | 第 7 節參數，覆蓋 version 到 payload，不含 magic 和 CRC 自己 |

例如 sequence 7、payload `10 20`（此處為十六進位），待計算 CRC 的封包是 `A5 5A 01 02 00 07 10 20 <CRC>`。最長 frame 為 39 bytes。不要直接把 C struct 的記憶體送出去；padding 與主機端序不構成穩定線路格式。

小步實作：

1. 先用固定 byte array，寫「收到完整 frame 才能解析」的函式，驗證長度、version、CRC，再讀 sequence/payload。測試不足長度必須拒絕。
2. 決定輸出方式。可使用 callback：parser 收到完整 frame 時呼叫使用者提供的函式。先讓 callback 只把結果複製到測試陣列；規定借出的 payload 指標只在 callback 執行期間有效。
3. 建議新增 `include/frame_parser.h`、`src/frame_parser.c`、`tests/test_frame_parser.c`，定義 init/reset/feed。feed 接受 byte pointer 和長度，保留半包狀態，單次可交付多個 frames。
4. 先支援合法資料分段，再支援垃圾前綴、錯誤 version、過長 length 與壞 CRC。先用固定容量的待解析緩衝區；parser 不必強制使用 ring buffer，兩者是不同責任。
5. 壞候選 frame 被拒絕後，從候選起點的下一 byte 重新找 magic，保留已收資料中可能的下一個起點；不要直接清空所有 bytes。補 `A5 A5 5A` 和 payload 內含 magic 的測試。
6. 合法範圍但錯誤的 length 可能讓 parser 等待更多資料。寫明「等待」與「判定錯誤」的界線；stream 中斷時由呼叫端 reset，02 再處理 timeout。有限緩衝區與 CRC 不保證任意毀損後立刻找回所有訊息。

這是本專案設計練習，並沒有一份外部標準替你決定以上格式。語言層面的 byte/shift 規則參考 [N1570 §6.5.7、§7.20](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)；理解 stream 為何需要 framing，可先讀 [tcp(7) DESCRIPTION](https://man7.org/linux/man-pages/man7/tcp.7.html) 的 byte-stream 說明，到 02 再做 socket。

## 9. 如何驗證 parser，做到哪裡才能進 02？

同一段合法資料先整段餵入，保存得到的 frames，接著比較：

- 每次只餵 1 byte。
- 對長度 N 的輸入，逐一在 0～N 的每個位置切成兩段。
- 一次餵兩個完整 frames，以及一個完整 frame 加半個 frame。
- 加入垃圾前綴、壞 CRC、length 33；後面接合法 frame，依第 8 節的恢復合約驗證。
- payload 長度 0、32，內容含 `0x00` 與 magic；sequence 65535 與 0。

先確認輸出的 frame 數量、內容、順序都一致；再檢查錯誤計數與容量不越界。只有「沒 crash」還不夠。需要記憶體檢查工具時，參考 [AddressSanitizer Usage](https://clang.llvm.org/docs/AddressSanitizer.html)，另建 sanitizer build 目錄並同時在編譯／連結加入旗標。

可以進入 02 的條件：你能自己跑測試、解釋 ring buffer 的空／滿與 wraparound、固定 CRC 規則、用純 C 測試證明分段輸入能得到相同 frames。還沒完成 parser 也能先做 02 的 socketpair 入門，但到接收 frame 的階段就需要回來完成。

## 10. 卡住時先查什麼？

| 現象 | 先檢查 |
| --- | --- |
| 看不懂 assert | 把括號內的式子翻成「我預期……」；先算回傳值，再算 `!` 或 `==` |
| 新測試完全沒執行 | 有沒有在 main 呼叫？新測試程式有沒有 add_test？ |
| include 找不到 | 編譯的是哪個 target？是否透過 target_include_directories 傳入 include 路徑？ |
| undefined reference | 宣告存在不代表實作已連結；新 `.c` 有加入 library 嗎？ |
| wraparound 後值錯誤 | 用容量 3 的表格逐次記錄 head/tail/count，不要一次追大量資料 |
| CRC 不同 | 檢查初值、反射、xorout、覆蓋範圍、是否誤算 `\0` |
| parser 分段才失敗 | feed 返回時，是否把尚未完成的狀態清掉了？ |

每次留下「輸入、預期、實際、如何重跑」四項紀錄。這些紀錄會變成你面試時能說明的設計與除錯經驗。
