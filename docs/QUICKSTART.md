# ESP32 WAV Player - 快速入門指南

## 🚀 5 分鐘快速開始

### 步驟 1: 準備硬體 (5 分鐘)

**必備硬體**:

- ✅ ESP32 開發板
- ✅ SD 卡模組
- ✅ I2S DAC (如 PCM5102)
- ✅ microSD 卡 (Class 10+, FAT32)
- ✅ 5 個按鈕 (可選)

**接線** (參考 `docs/USER_GUIDE.md`):

```
SD 卡:
  MISO → GPIO19, MOSI → GPIO23, SCK → GPIO18, CS → GPIO5

I2S DAC:
  BCK → GPIO4, WS → GPIO15, DATA → GPIO2

按鈕 (可選):
  VOL+ → GPIO12, VOL- → GPIO13
  PREV → GPIO14, NEXT → GPIO27, PAUSE → GPIO26
```

---

### 步驟 2: 準備音樂檔案 (3 分鐘)

**轉換音樂**:

```bash
# 安裝 FFmpeg (如果還沒有)
brew install ffmpeg  # macOS
# 或 apt install ffmpeg # Linux

# 安裝 Python 依賴
pip install rich

# 轉換音樂
cd ESP32-S3-HiFi-DAP
python3 scripts/audio_converter.py song.mp3
```

**複製到 SD 卡**:

```bash
# 格式化 SD 卡為 FAT32
# 將轉換好的 .wav 檔複製到 SD 卡根目錄
cp output/*.wav /Volumes/SD_CARD/
```

---

### 步驟 3: 上傳韌體 (2 分鐘)

```bash
# 安裝 Arduino CLI (如果還沒有)
brew install arduino-cli

# 上傳韌體
cd ESP32-S3-HiFi-DAP
python3 scripts/upload.py src/WavPlayer --board esp32
```

等待上傳完成...

---

### 步驟 4: 開始播放！ (30 秒)

1. **插入 SD 卡** 到 SD 卡模組
2. **上電啟動** ESP32
3. **等待初始化** (~3 秒)
4. **按 PAUSE 按鈕** (GPIO26) 開始播放

🎵 **聽到音樂了！**

---

## 🎮 基本操作

### 按鈕控制

| 動作      | 按鈕           | 說明             |
| --------- | -------------- | ---------------- |
| 播放/暫停 | PAUSE (GPIO26) | 單按切換         |
| 下一首    | NEXT (GPIO27)  | 或 PAUSE 雙擊    |
| 上一首    | PREV (GPIO14)  | -                |
| 音量+     | VOL+ (GPIO12)  | 單按+5%, 長按+1% |
| 音量-     | VOL- (GPIO13)  | 單按-5%, 長按-1% |

### Serial 指令

連接 Serial Monitor (460800 baud):

```bash
# 查看狀態
status

# 查看所有指令
help

# 查看記憶體
mem

# 查看CPU
cpu
```

詳細指令請參考 `docs/SERIAL_COMMANDS.md`

---

## ✨ 主要功能

### 1. 🎵 高品質音訊

- 16-bit PCM, 44.1kHz 立體聲
- APLL 精確時鐘
- Fade In/Out (無爆音)
- DMA buffer flush

### 2. 💾 斷點續播

- 自動記錄播放位置
- 斷電後恢復播放
- 每 10 秒自動儲存

### 3. 📊 系統監控

- 10 個 Serial 指令
- FreeRTOS 任務狀態
- 記憶體使用監控
- NVS 儲存查看

### 4. 🛡️ 產品級穩定性

- Robust WAV parsing
- Memory-safe arrays
- Dual-core FreeRTOS
- Error handling

---

## 🐛 常見問題

### Q: SD 卡讀不到？

**A**: 檢查：

- SD 卡是 FAT32 格式
- 接線正確（MISO=19, MOSI=23, SCK=18, CS=5）
- SD 卡是 Class 10+

### Q: 沒有聲音？

**A**: 檢查：

- DAC 接線（BCK=4, WS=15, DATA=2）
- DAC 有供電
- 音量 > 0% (`status` 查看)
- 按 PAUSE 開始播放

### Q: 音質不好？

**A**: 確認：

- WAV 是 44.1kHz, 16-bit
- 使用 `audio_converter.py` 轉換
- DAC 品質（建議 PCM5102 或更好）

### Q: 按鈕沒反應？

**A**:

- 確認 GPIO 接線
- 按鈕接 3.3V (使用 INPUT_PULLDOWN)
- 查看 `settings` 確認 GPIO 配置

---

## 📚 進階閱讀

完成基本設定？繼續學習：

- 📖 **[USER_GUIDE.md](USER_GUIDE.md)** - 完整使用手冊
- 🔧 **[SERIAL_COMMANDS.md](SERIAL_COMMANDS.md)** - 指令參考
- 💾 **[NVS_EXPLAINED.md](NVS_EXPLAINED.md)** - 斷點續播原理
- 🎵 **[audio_converter_guide.md](audio_converter_guide.md)** - 音訊轉換
- 🏗️ **[PRODUCTION_SUMMARY.md](PRODUCTION_SUMMARY.md)** - 產品級功能

---

## 🆘 需要幫助？

1. **查看文檔** - `docs/` 目錄下的所有文件
2. **Serial Debug** - 輸入 `help` 查看所有指令
3. **檢查接線** - 輸入 `settings` 確認 GPIO
4. **GitHub Issues** - 開 issue 回報問題

---

**恭喜！你已經成功設定 ESP32 WAV Player！** 🎉

現在可以：

- 🎵 享受高品質音樂
- 📊 監控系統狀態
- 💾 使用斷點續播
- 🔧 探索進階功能

**版本**: v2.0.1 Production  
**更新**: 2026-01-01
