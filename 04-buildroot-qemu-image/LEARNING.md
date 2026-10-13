# 04 教學：做出能啟動自己程式的 Linux 映像

[回規格與驗收](README.md) · [學習入口](../LEARNING.md)

本專案尚未選定版本或產出 image。先用官方組態開機，再逐步加入自己的 package；以下為實作路徑。

## 1. 用途與例子

02 是在你的電腦上執行資料服務。04 要回答：「換到一個乾淨的裝置環境，怎樣讓 Linux 啟動後自動執行同一個服務？」

Buildroot 根據設定組合 toolchain、kernel、root filesystem 和套件；QEMU 提供可重跑的模擬機器。最小成果是 QEMU 裡的 Linux 開機到 shell；下一步才是自動印出 `SENSOR_SERVICE_READY`。

這個 ready marker 要在服務已完成必要初始化後印出，不能一進 main 就印，否則 runner 會把尚未可用的服務判成成功。

## 2. 先分清楚六個東西

| 名詞 | 意思 | 例子 |
| --- | --- | --- |
| host | 執行建置工作的電腦 | 你的 Ubuntu |
| target／guest | 執行產物的目標系統 | QEMU 裡的 ARM64 Linux |
| toolchain | 編譯、連結及相關工具組 | 產生 target 可用的 ELF |
| kernel | 管理程序、記憶體與裝置的核心 | Linux Image |
| rootfs | 系統看到的根目錄內容 | `/bin`、`/etc`、library、你的程式 |
| bootloader／init | 前者載入 kernel，後者在 kernel 啟動 userspace 後啟動服務 | U-Boot／BusyBox init 等 |

Linux image 不等於一個 `.c` 編成的執行檔。rootfs 可能是 initramfs 或磁碟檔案系統；第一輪沿用官方 defconfig 的選擇。先讀 [Buildroot Manual](https://buildroot.org/downloads/manual/manual.html) 的 Getting started、System requirements、Buildroot quick start，認識設定與輸出目錄即可。

## 3. 第一個成果：使用官方 QEMU defconfig 開機

1. 選一個 Buildroot release，下載來源並記錄 checksum 或 commit。建置來源與 output 放工作目錄，作品集保存設定和腳本。
2. 查看該版本 `configs/` 的 QEMU defconfigs 與 `board/qemu/` 對應 readme。可從 ARM64 virt 方向選起，之後接 U-Boot；選用名稱必須在該 checkout 存在。
3. 讀該組態要求的 host packages、QEMU machine 和啟動方式，再開始 build。
4. 用該版本板級說明或產生的啟動腳本啟動，保存完整 console。
5. 登入 guest，執行 `uname -m`、查看根目錄，確認你已在 guest，再從 guest 正常關機。

建置命令的形式如下，這是需代入實際值的模板，不能原樣複製尖括號：

```text
make O=<獨立輸出目錄的絕對路徑> <該版本存在的defconfig名稱>
make O=<同一輸出目錄的絕對路徑>
```

完成條件：你能指出 host、guest 各在哪裡，知道用了哪些 image 路徑，且能用記錄的命令再次開機。第一次不用自己算 kernel 載入位址，也不急著整合 02。

## 4. 第一個自訂內容：讓 rootfs 多一個可辨認檔案

在本專案逐步新增：

```text
external/
  external.desc         外部樹名稱與描述
  external.mk           收入自己的 package makefiles
  Config.in             package 設定入口
  configs/              自己保存的 defconfig
  board/demo/rootfs-overlay/etc/project-version
scripts/                日後保存建置／啟動命令
notes/environment.md    版本、machine、RAM、console、磁碟介面
```

依 [Buildroot Manual](https://buildroot.org/downloads/manual/manual.html) 的 Keeping customizations outside of Buildroot、Root filesystem overlays 建立 BR2_EXTERNAL。把 `project-version` 設為容易辨認的值，透過 overlay 進入 image。

驗證：重新 build、重開 guest，在 guest `cat /etc/project-version`。若看不到，先查啟動腳本是否仍指向舊 output；不要只在 host 看到 overlay 檔案就當整合完成。

## 5. 加一個 package，再換成 02 服務

先用只印出版本的小 C 程式練 package，依 generic-package 或 cmake-package 的文件流程，交給 target toolchain 編譯。此步驟學四件事：來源從哪裡來、如何編譯、安裝到 target 哪裡、runtime 需要什麼。

1. 在 external/package 新增小程式的 package 定義，接到 Config.in 與 external.mk。
2. 讓它安裝到 guest 的 `/usr/bin`，重新 build。
3. 在 host 用 `file`／`readelf -h` 看 ELF 架構，再進 guest 執行；架構正確不代表動態 loader/library 就一定齊全。
4. 換成 02 的固定 revision，保留同一份程式碼來源與相依資訊。
5. 按 image 實際使用的 init 系統加入啟動設定，不要假設所有 image 都用 systemd。
6. 啟動後完成必要初始化再印 ready，檢查失敗時應有非零退出碼與原因。

Reference：[Buildroot Manual](https://buildroot.org/downloads/manual/manual.html) 的 Adding new packages、Infrastructure for CMake-based packages；讀完要能解釋「安裝到 host 與安裝到 target 有何不同」。ELF 欄位可查 [GNU readelf](https://sourceware.org/binutils/docs/binutils/readelf.html)。

## 6. 再接 U-Boot：把開機路徑看完整

第一輪 QEMU 可以直接載入 kernel。跑通後才擴展成 `U-Boot → kernel + DTB → rootfs → init → service`。

DTB 是編譯後的裝置樹，告訴 kernel 裝置與資源配置。先沿用平台提供的 DTB，讀一個節點的 compatible、reg、interrupts，再試一個可控的小修改。

動手前做一張表：kernel、DTB、initrd（若有）放在哪裡、最大大小、載入位址。從所選平台文件和實際 image size 決定，不把網路上另一塊板的地址照抄過來。

Reference：[U-Boot QEMU ARM](https://docs.u-boot.org/en/latest/board/emulation/qemu-arm.html) 的 Build、Running；[booti](https://docs.u-boot.org/en/latest/usage/cmd/booti.html) 的參數；[Linux Devicetree Usage](https://docs.kernel.org/devicetree/usage-model.html) 的平台與裝置辨識。讀完再刻意改錯一次 `root=`，從 console 判斷在哪個階段失敗。

## 7. 把開機變成能判斷成功或失敗的實驗

新增 runner 前先手動開成功一次。runner 的最小責任是啟動 QEMU、保存 stdout/stderr、等待 ready marker、到期限就結束該 guest 並回傳非零。

接著重跑 20 次，每次分開保存 log。再測 rootfs 唯讀、data 分區滿、服務設定缺失。每個案例都要知道「預期故障在哪一層」，不是只搜尋有沒有 ERROR 字樣。

可重建表示有足夠輸入和步驟能重做；bit-for-bit reproducible 還需要比較兩次乾淨建置的 hashes。若不同，查 timestamp、路徑或未固定來源，記錄原因再修正，不能從兩次都能開機推論二進位完全一致。

## 8. 卡住時依開機階段查

| 現象 | 第一個檢查點 |
| --- | --- |
| 沒有 console | machine、serial 設定、kernel console 參數是否一致 |
| U-Boot 有輸出，kernel 沒輸出 | image 類型、架構、DTB、載入地址 |
| kernel 無法 mount rootfs | root=、磁碟介面、filesystem driver 是否編入 |
| 有 shell，沒有自己的程式 | package 是否啟用、安裝位置、是否啟動舊 image |
| 檔案存在但無法執行 | 架構、執行權限、動態 loader、shared libraries |
| 程式可執行但不自動啟動 | init 種類、啟動順序、權限、設定和資料目錄 |

回 README 對照 BOOT-01、FS-01、BUILD-01、PKG-01。完成後應能交付 external tree、固定輸入、build/run 命令和 console 證據；QEMU 之外的實板 clock、pinmux、DDR 與電氣行為留待硬體階段。
