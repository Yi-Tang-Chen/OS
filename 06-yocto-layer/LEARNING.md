# 06 教學：把程式包進可維護的 Yocto layer

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

目前尚無 Yocto layer、recipe 或 image。先了解它與 04 的關係，再按固定 release 的文件完成第一次建置。

## 1. 用途與例子

04 已經練過「把 Linux 與服務組成 image」。06 繼續練產品建置的管理：服務換版本、套件有相依、不同機型有不同設定時，如何把修改保存成可追蹤的 metadata。

例子：同一個 sensor-service 從 v1 換成 v2。你改 recipe 的固定來源版本，重新建置 image，guest 裡執行程式能看到 v2，且知道它來自哪一個 commit。

第一個小成果仍是官方 image 在 QEMU 開機。第二個才是你自己的 layer 安裝一個只印版本的小程式，不必一開始就整合整套 02。

## 2. 必要名詞

| 名詞 | 白話意思 | 先避免的混淆 |
| --- | --- | --- |
| BitBake | 根據 metadata 執行建置任務 | 不只是用來編一個 C 檔的 compiler |
| recipe，`.bb` | 描述某份來源如何取得、建置、安裝與包裝 | recipe 名稱不一定等於所有輸出 package 名稱 |
| layer | 一組相關 recipes 與設定 | 建立資料夾不等於 build 已啟用這個 layer |
| image | 選擇 target 要包含哪些套件及設定 | package build 完不表示已進 image |
| machine／distro | 前者偏硬體目標，後者偏系統政策 | 服務 recipe 不應包辦所有平台政策 |
| SDK | 給另一個開發環境用的 target compiler 與 sysroot | 不是直接拿 host gcc 編 target 程式 |
| sstate | 可重用的建置工作結果 | cache 命中不等於二進位可重現 |

開始前確認你能解釋 04 的 host、target、rootfs、動態 library。依選定 release 的 [Quick Build](https://docs.yoctoproject.org/brief-yoctoprojectqs/index.html) 檢查主機支援、磁碟、RAM 與工具要求。這些數字隨 release 改變，先確認再開始長時間 build。

## 3. 第一個成果：官方 image 可開機

1. 選 release，保存相關 repositories 的 commit 與 branch；layers 要彼此相容。
2. 按該 release 的 Quick Build 建立環境。不同 release 的 setup 工具與環境腳本可能不同，整段流程使用同一版本文件。
3. 選一個官方 QEMU MACHINE，例如該版本支援的 qemux86-64，先 build 最小官方 image。
4. 使用該版本 runqemu 方式開機，登入後讀取版本／架構，正常關閉。
5. 保存「工作目錄、環境初始化命令、build target、啟動命令」到本專案之後新增的 `notes/environment.md`。

常見的 image target 是 `core-image-minimal`，但只有在正確環境初始化後 `bitbake` 才知道去哪裡找 recipe。這一步以官方組態成功為界線，先不動 daemon、service 或自訂 distro。

## 4. 建立自己的 layer：先讓它被看見

之後在本專案新增類似結構：

```text
meta-demo/
  conf/layer.conf
  recipes-demo/hello/hello_1.0.bb
  recipes-demo/hello/files/hello.c
notes/environment.md
```

使用選定版本的 layer 建立工具或官方範本生成最小 layer.conf，再把它加入 build 配置，列出已啟用 layers，確認 meta-demo 出現。

Reference：[Understanding and Creating Layers](https://docs.yoctoproject.org/dev-manual/layers.html)，讀 Creating Your Own Layer、Adding a Layer；回答「layer 在磁碟上與 layer 參與建置差在哪裡？」。`LAYERSERIES_COMPAT` 要如實反映你已驗證的 release，不要只是為了消除警告填滿所有名字。

完成條件：metadata 能被解析，工具能找到 hello recipe。還沒有寫 C 程式前，這就是一個可驗證的小步驟。

## 5. 第一份 recipe：只安裝一個 hello 程式

先寫 `hello.c`，印出 `demo-hello 1.0` 並回傳 0。recipe 一次學一段：

1. 來源：`SRC_URI` 指向本地檔案；查選定 release 的來源解包位置和 `S`／`UNPACKDIR` 規則。
2. 授權：指定你實際採用的 license，附上授權文字與正確 `LIC_FILES_CHKSUM`。checksum 是追蹤授權內容，不是隨便湊一個值。
3. 編譯：使用建置系統提供的 target compiler、CFLAGS、LDFLAGS，不硬寫 `/usr/bin/gcc`。
4. 安裝：放進 recipe 的 staging 安裝樹 `${D}` 與正確 bindir，保留執行權限；這不是直接寫入 host `/usr/bin`。
5. 套件：先 build recipe，再明確把產生的 package 加進 image，重建並進 guest 執行。

Reference：[Writing a New Recipe](https://docs.yoctoproject.org/dev-manual/new-recipe.html)，依序讀 locating source、licensing、compilation、installation；變數意思查 [Variables Glossary](https://docs.yoctoproject.org/ref-manual/variables.html)。做 CMake 專案時再研究 `inherit cmake`，不要先手寫一套交叉編譯流程。

驗證要分三層：recipe task 成功、package 內容正確、image 內可執行。只看到 bitbake 結束，仍不能跳過後两層。

## 6. 換成 02 的服務與啟動設定

1. 用固定 source revision 引入 02，避免自動追蹤最新來源讓舊 image 無法重建。
2. 釐清建置時相依 `DEPENDS` 和執行時 package 相依 `RDEPENDS`：link 要用的開發內容與 target 啟動後需要的程式不是同一件事。
3. 明確選定 image 的 init 系統。若用 systemd，加入 unit、相關 class／設定和必要 feature，再驗證首次啟動與重新啟動。
4. 對設定檔與可寫資料目錄定義 owner、權限、升級保留方式。
5. 改 source revision，驗證 guest 確實跑新版本；保存版本輸出與 service log。

不要把所有修改都塞到 build 目錄的臨時設定而忘記保存。產品化設定逐步移到適合的 layer/image/distro/machine 責任範圍，保留可重建的入口。

## 7. SDK 與可重建實驗

先產出與 image 配套的 SDK。在另一個乾淨工作目錄安裝並 source 它的環境腳本，編譯一個小 consumer，查看 ELF 架構，再放回同一 image 執行。

Reference：[Using the SDK Toolchain Directly](https://docs.yoctoproject.org/sdk-manual/working-projects.html)，只先讀環境設定與小程式範例；回答「不用原本 bitbake 工作目錄，還能否產生可執行 target 程式？」。

接著固定所有輸入，在第二個 build 環境比較產物。分別記錄 clean build、incremental build、sstate 使用範圍；相同 recipe 但來源 branch 移動，不算相同輸入。授權清單與 SBOM 在基本 package 正確後再建立。

## 8. 驗證與卡住時的線索

| 現象 | 先查 |
| --- | --- |
| recipe 找不到 | layer 是否啟用、BBFILES、檔案名稱和工具列出的 recipes |
| fetch 失敗 | SRC_URI、revision 是否存在、來源存取是否成功 |
| compile 失敗 | 對應 task log、target flags、建置時相依 |
| build 成功但 image 沒程式 | 是否加入正確 package、安裝位置和 package file 清單 |
| target 執行時缺 library | runtime 相依、loader 與 image 配置 |
| service 沒啟動 | init 系統、unit 安裝／啟用、權限與設定路徑 |

最終回到 README 的 RECIPE-01、SDK-01、REBUILD-01、SERVICE-01。交付物是自己的 layer、版本 manifest、實際建置步驟、guest 驗證與 SDK consumer，不必把巨大 build output 全部放進 Git。
