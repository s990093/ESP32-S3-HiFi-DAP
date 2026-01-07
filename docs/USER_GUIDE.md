# ESP32 Production-Grade WAV Player - 使用者指南

## 📖 目錄

- [快速開始](#快速開始)
- [硬體連接](#硬體連接)
- [Serial 指令參考](#serial-指令參考)
- [按鈕控制](#按鈕控制)
- [進階功能](#進階功能)
- [故障排除](#故障排除)

---

## 🚀 快速開始

### 第一次使用

1. **準備 SD 卡**

   - 格式化為 FAT32
   - 放入 WAV 檔案（44.1kHz, 16-bit, Stereo）
   - 使用 `scripts/audio_converter.py` 轉換其他格式

2. **上傳韌體**

   ```bash
   cd ESP32-S3-HiFi-DAP
   python3 scripts/upload.py src/WavPlayer --board esp32
   ```

3. **開始播放**
   - 插入 SD 卡
   - 上電啟動
   - 按 **PAUSE** 按鈕開始播放

### 開機順序

```
ESP32 啟動
    ↓
載入 NVS 儲存狀態
    ↓
掃描 SD 卡 WAV 檔案
    ↓
初始化 I2S (APLL)
    ↓
恢復上次播放位置
    ↓
等待按鈕輸入
```

---

## 🔌 硬體連接

### SD 卡模組 (SPI)

| 功能 | ESP32 GPIO | 說明         |
| ---- | ---------- | ------------ |
| MISO | GPIO 19    | 數據輸出     |
| MOSI | GPIO 23    | 數據輸入     |
| SCK  | GPIO 18    | 時鐘 (20MHz) |
| CS   | GPIO 5     | 片選         |
| VCC  | 3.3V       | 電源         |
| GND  | GND        | 接地         |

### I2S DAC

| 功能      | ESP32 GPIO | 說明         |
| --------- | ---------- | ------------ |
| BCK       | GPIO 4     | 位元時鐘     |
| WS (LRCK) | GPIO 15    | 左右聲道選擇 |
| DATA      | GPIO 2     | 串列數據     |
| VIN       | 3.3V or 5V | 依 DAC 規格  |
| GND       | GND        | 接地         |

### 控制按鈕

| 功能  | ESP32 GPIO | 接法              |
| ----- | ---------- | ----------------- |
| VOL+  | GPIO 12    | 按鈕 → GPIO, 上拉 |
| VOL-  | GPIO 13    | 按鈕 → GPIO, 上拉 |
| PREV  | GPIO 14    | 按鈕 → GPIO, 上拉 |
| NEXT  | GPIO 27    | 按鈕 → GPIO, 上拉 |
| PAUSE | GPIO 26    | 按鈕 → GPIO, 上拉 |

> **Note**: 使用 `INPUT_PULLDOWN` 模式，按鈕接 3.3V

---

## 💻 Serial 指令參考

連接 Serial Monitor (460800 baud) 並輸入以下指令：

### 1. `mem` / `memory`

顯示記憶體使用狀態（含視覺化圖表）

```
mem

╔════════════════════════════════════════╗
║         Memory Status                  ║
╚════════════════════════════════════════╝
HEAP Memory:
  Total:      361400 bytes
  Used:       127416 bytes (35.3%)
  Free:       233984 bytes (64.7%)
  Min Free:   233000 bytes
  Usage: [███████░░░░░░░░░░░░░] 35.3%

PSRAM: Not available
```

**用途**: 檢查記憶體洩漏、確認系統穩定性

---

### 2. `status` / `s`

顯示播放器當前狀態

```
status

╔════════════════════════════════════════╗
║         Player Status                  ║
╚════════════════════════════════════════╝
State:   ▶️  Playing
Track:   3/8
File:    /music/song3.wav
Volume:  30%
Uptime:  1234 sec
```

**用途**: 快速查看播放資訊

---

### 3. `settings` / `config`

顯示系統設定與硬體配置

```
settings

╔════════════════════════════════════════╗
║         System Settings                ║
╚════════════════════════════════════════╝

📟 Hardware Configuration:
  I2S BCK:       GPIO 4
  I2S WS:        GPIO 15
  I2S DATA:      GPIO 2
  SD MISO:       GPIO 19
  SD MOSI:       GPIO 23
  SD SCK:        GPIO 18
  SD CS:         GPIO 5

🎮 Button Mapping:
  VOL+:          GPIO 12
  VOL-:          GPIO 13
  PREV:          GPIO 14
  NEXT:          GPIO 27
  PAUSE:         GPIO 26

🔧 System Parameters:
  Buffer Size:   8192 bytes
  Max Tracks:    32
  Sample Rate:   44100 Hz
  Bit Depth:     16-bit
  Channels:      Stereo (2)
  APLL:          Enabled
  SPI Speed:     20 MHz
  DMA Buffers:   8 x 1024

⏱️  Timing Settings:
  Debounce:      200 ms
  Long Press:    500 ms
  Double Click:  400 ms
  Fade Samples:  2048 (~46.4 ms)

🎵 Audio Features:
  ✓ Chunk-based WAV parsing
  ✓ Logarithmic volume curve
  ✓ Fade in/out transitions
  ✓ DMA buffer flush (anti-pop)
  ✓ NVS playback resume
  ✓ Hidden file filtering
```

**用途**: 確認硬體接線、查看系統參數

---

### 4. `cpu` / `tasks`

顯示 FreeRTOS 任務狀態與 CPU 負載

```
cpu

╔════════════════════════════════════════╗
║         Task Status (FreeRTOS)         ║
╚════════════════════════════════════════╝

Name          State   Prio    Stack   ID
──────────────────────────────────────────
AudioTask      B       2       1540    4
ButtonTask     B       1       1024    5
IDLE0          R       0       120     1
IDLE1          R       0       110     2
loopTask       X       1       2500    3
──────────────────────────────────────────

📊 State Legend:
  X: Running   (目前正在執行)
  B: Blocked   (等待中/閒置 - CPU 有空)
  R: Ready     (準備執行)
  S: Suspended (暫停)
  D: Deleted   (刪除中)

⚠️  Stack: 剩餘記憶體 (bytes)
  • <100  = 危險！可能 Stack Overflow
  • >500  = 安全
  • >2000 = 分配太多，可減少
```

**如何解讀**:

- `AudioTask` 和 `ButtonTask` 大部分時間應該是 `B` (Blocked) - 表示 CPU 有空閒
- `IDLE` 任務出現表示系統健康
- `Stack` 接近 0 表示危險，需增加任務堆疊大小

**用途**: 檢測 CPU 負載、除錯任務卡死問題

---

### 5. `nvs` / `read`

顯示 NVS Flash 儲存的狀態

```
nvs

╔════════════════════════════════════════╗
║         NVS Storage (Flash)            ║
╚════════════════════════════════════════╝

💾 Stored Preferences:
  Track Index:   2
  Track File:    /song3.wav
  Volume:        30%
  Position:      45.3s
  Was Playing:   No

📋 Current Runtime State:
  Track Index:   2
  Track File:    /song3.wav
  Volume:        30%
  Position:      45.3s
  State:         Paused

⚙️  NVS Operations:
  Auto-save triggers:
    - Track change
    - Volume change
    - Pause/Play toggle
    - Every 10 seconds (background)
  Manual commands:
    - 'save'   - Force save current state
    - 'clear'  - Clear NVS saved state
    - 'resume' - Reload saved state
```

**用途**:

- 確認斷點續播位置
- 檢查 NVS 是否正常儲存
- 對比 Flash 與 RAM 狀態

---

### 6. `tree` / `ls`

列出 SD 卡所有檔案

```
tree

📁 SD Card Structure:
═══════════════════════════════════════
📄 test.wav                       12.5 MB
📄 song2.wav                       8.3 MB
📄 music.wav                      15.2 MB
🔒 ._hidden.wav                   2.1 KB (hidden)
═══════════════════════════════════════
Total: 3 files, 0 dirs, 36.0 MB
```

**用途**: 快速檢視 SD 卡內容、確認檔案可見性

---

### 7. `cat <filename>`

顯示檔案資訊與內容

```
cat test.wav

📄 File: /test.wav
═══════════════════════════════════════
Size: 13107128 bytes

WAV Header:
  RIFF: RIFF
  File Size: 13107120
  WAVE: WAVE

Chunks:
  [0] fmt  - 16 bytes
      Format: 1 (1=PCM)
      Channels: 2
      Sample Rate: 44100 Hz
      Bit Depth: 16 bits
  [1] data - 13107084 bytes
```

**用途**:

- 檢查 WAV 檔案格式
- 除錯播放問題
- 查看 chunk 結構

---

### 8. `save`

手動儲存當前播放狀態到 NVS

```
save

✅ Playback state saved
```

**用途**:

- 在重要時刻手動備份狀態
- 測試 NVS 功能

---

### 9. `clear` / `reset`

清除 NVS 中儲存的所有資料

```
clear

🗑️  NVS cleared - all saved state deleted
```

**用途**:

- 重置播放狀態
- 解決 NVS 資料錯亂問題

---

### 10. `resume`

從 NVS 重新載入播放狀態

```
resume

🔄 Resuming from last session
   Track: 2, Volume: 30%, Position: 45.3s
✅ Playback state restored
```

**用途**:

- 恢復到上次儲存的狀態
- 測試斷點續播功能

---

---

### 11. `help` / `h` / `?`

顯示所有可用指令

```
help

╔════════════════════════════════════════╗
║         Available Commands             ║
╚════════════════════════════════════════╝
  mem, memory  - Show memory status
  status, s    - Show player status
  settings     - Show system configuration
  cpu, tasks   - Show FreeRTOS task status
  nvs, read    - Show NVS stored state
  tree, ls     - List SD card files
  cat <file>   - Show file info/content
  save         - Save playback state
  resume       - Restore playback state
  help, h, ?   - Show this help
```

---

## 🎮 按鈕控制

### VOL+ (GPIO 12)

- **單按**: 音量 +5%
- **長按**: 音量 +1% (持續，每 50ms)

### VOL- (GPIO 13)

- **單按**: 音量 -5%
- **長按**: 音量 -1% (持續，每 50ms)

### PREV (GPIO 14)

- **單按**: 上一首（重置播放位置）

### NEXT (GPIO 27)

- **單按**: 下一首（重置播放位置）

### PAUSE (GPIO 26)

- **單按**: 暫停/播放切換
- **雙擊** (< 400ms): 下一首

### PREV + NEXT (組合鍵)

- **長按** (0.5 秒): 切換循環模式 (Loop Single / Loop All)

---

## 🌟 進階功能

### 斷點續播

**功能**: 記住上次播放到哪個位置

**運作方式**:

1. 每 10 秒自動儲存播放位置到 NVS
2. 換曲、調音量、暫停時也會儲存
3. 斷電後重啟，自動恢復到上次位置

**示範**:

```
播放 song.wav 到 45.3 秒
    ↓
(突然斷電)
    ↓
重新開機
    ↓
🔄 Resuming from last session
   Track: 1, Volume: 30%, Position: 45.3s
    ↓
⏩ Resuming from 45.3s
    ↓
繼續播放！
```

**手動測試**:

```bash
# 1. 播放一段時間
(等待 20 秒)

# 2. 查看狀態
nvs
# 應該顯示 Position: ~20s

# 3. 重啟 ESP32
(拔電源再插回)

# 4. 確認恢復
# 應該看到自動載入訊息
```

---

### CPU 負載監控

**目的**: 實時監控 FreeRTOS 任務狀態

**健康指標**:

```
✅ 健康系統:
- AudioTask:  B (Blocked) - 大部分時間在等待
- ButtonTask: B (Blocked) - 大部分時間待命
- IDLE0/1:    R (Ready)   - 有機會執行

❌ 異常系統:
- AudioTask:  X (Running) - 持續滿載
- IDLE 消失 - CPU 100% 無空閒
- Stack < 100 - 即將崩潰
```

**除錯案例**:

```
問題: 切歌時卡頓

步驟 1: 檢查 CPU
cpu

發現: AudioTask Stack = 50 (太低!)

解決: 增加任務堆疊
xTaskCreate(..., 8192, ...) → xTaskCreate(..., 12288, ...)
```

---

### 記憶體監控

**目的**: 檢測記憶體洩漏

**正常模式**:

```
開機:  Free Heap: 250000 bytes
1小時: Free Heap: 235000 bytes (正常下降)
2小時: Free Heap: 233000 bytes (穩定)
3小時: Free Heap: 233000 bytes (穩定)
```

**異常模式** (記憶體洩漏):

```
開機:  Free Heap: 250000 bytes
1小時: Free Heap: 200000 bytes
2小時: Free Heap: 150000 bytes (持續下降!)
3小時: Free Heap:  50000 bytes (危險!)
```

**檢查方式**:

```bash
# 每小時執行一次
mem

# 記錄 Min Free Heap
# 如果持續下降 → 有記憶體洩漏
```

---

## 🐛 故障排除

### SD 卡無法讀取

**症狀**:

```
❌ SD Card failed!
```

**檢查清單**:

1. SD 卡是 FAT32 格式？
2. 接線正確？(MISO=19, MOSI=23, SCK=18, CS=5)
3. 電源足夠？(建議 5V/1A)
4. SD 卡速度？(建議 Class 10+)

**解決方式**:

```cpp
// 降低 SPI 速度測試
SD.begin(SD_CS, SPI, 10000000);  // 從 20MHz 降到 10MHz
```

---

### 沒有聲音

**檢查清單**:

1. DAC 接線正確？
2. DAC 有供電？
3. 音量是否 > 0？(`status` 查看)
4. 是否在暫停狀態？

**測試**:

```bash
status
# 確認:
# - State: Playing
# - Volume: > 0%

# 如果是 Paused
# 按 PAUSE 按鈕或重啟
```

---

### 播放卡頓

**可能原因**:

1. SD 卡太慢
2. WAV 檔案損壞
3. CPU 滿載

**診斷**:

```bash
# 1. 檢查 CPU
cpu
# AudioTask 應該大部分是 B (Blocked)

# 2. 檢查 WAV 檔案
cat song.wav
# 確認格式是 PCM, 44.1kHz, 16-bit

# 3. 使用更快的 SD 卡
# Class 10 或 UHS-I
```

---

### 換曲有爆音

**正常情況**: 應該已透過 DMA flush 消除

**如果還有爆音**:

```cpp
// 檢查 FADE_SAMPLES 設定
#define FADE_SAMPLES 2048  // 增加到 4096 試試

// 或降低 SPI 速度
SD.begin(SD_CS, SPI, 10000000);
```

---

### NVS 不儲存

**檢查**:

```bash
# 1. 測試儲存
save
✅ Playback state saved  # 應該看到這個

# 2. 查看內容
nvs
# 應該顯示儲存的數值

# 3. 重啟測試
(重啟 ESP32)
# 應該自動載入
```

**如果失敗**: Flash 可能損壞,重刷韌體試試

---

## 📚 參考資料

- [README.md](../README.md) - 專案概覽
- [NVS_EXPLAINED.md](NVS_EXPLAINED.md) - NVS 儲存機制
- [PRODUCTION_SUMMARY.md](PRODUCTION_SUMMARY.md) - 產品級功能總結
- [audio_converter_guide.md](audio_converter_guide.md) - 音訊轉換工具

---

## 🛠️ Serial File Manager (UART)

New in v3.2, you can manage files on the SD card directly over the USB Serial connection without removing the card.

### 1. Connectivity Test

- **Command**: `ping`
- **Response**: `pong`
- **Command**: `test_write`
- **Description**: Creates a test file `/test_serial.txt` to verify SD write permissions.

### 2. File Upload

- **Command**: `upload <remote_path> <size>`
- **Tools**: Use the provided python script for reliable transfer.
  ```bash
  python3 scripts/serial_upload.py /dev/cu.usbserial-XXXX local_file.mp3 /song.mp3
  ```
- **Note**: This uses a binary protocol and requires the dedicated script. Do not type this command manually.

---

---

## 🎓 進階閱讀

### FreeRTOS 任務設計

```cpp
// Core 0: 控制類任務
xTaskCreatePinnedToCore(
  buttonHandlerTask,  // 函數
  "ButtonTask",       // 名稱
  2048,               // Stack 大小
  NULL,               // 參數
  1,                  // 優先級 (低)
  &buttonTaskHandle,  // Handle
  0                   // Core 0
);

// Core 1: 音訊任務
xTaskCreatePinnedToCore(
  audioPlaybackTask,
  "AudioTask",
  8192,               // 需要更大 Stack
  NULL,
  2,                  // 優先級 (高)
  &audioTaskHandle,
  1                   // Core 1
);
```

### I2S APLL 配置

```cpp
i2s_config_t i2s_config = {
  .use_apll = true,        // 啟用 Audio PLL
  .sample_rate = 44100,    // 精確 44.1kHz
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .dma_buf_count = 8,      // 緩衝數量
  .dma_buf_len = 1024,     // 每個緩衝大小
};
```

### 音量曲線

```
線性 (舊):   0 - 10 - 20 - 30 - ... - 100
立方 (新):   Vol^3 曲線 (10% 音量時僅為原本的 0.1%)

人耳感知: 立方曲線在低音量下提供更精細的控制，適合高靈敏度耳機。
```

---

**版本**: v2.0.1 Production  
**最後更新**: 2026-01-01  
**狀態**: ✅ 穩定運行
