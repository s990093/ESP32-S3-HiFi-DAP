# Audio Converter - 使用指南

## 🎵 功能特色

### **支援格式**

**輸入格式：**

- MP3, M4A, AAC
- FLAC, WAV, OGG
- WMA, APE, ALAC
- 所有 FFmpeg 支援的音訊格式

**輸出格式：**

- **WAV** - 16-bit PCM, 44.1kHz (ESP32 播放用)
- **FLAC** - Lossless 無損, 44.1kHz (封存用)

### **最高音質設定**

#### WAV 格式

```
編碼: PCM 16-bit signed
取樣率: 44.1kHz
聲道: Stereo (2)
適用: ESP32-S3 HiFi-DAP 播放
```

#### FLAC 格式

```
編碼: FLAC lossless
取樣率: 44.1kHz
聲道: Stereo (2)
位元深度: 32-bit
壓縮等級: 8 (default, 0-12)
適用: 高保真封存、備份
```

## 📝 使用範例

### 基本用法

```bash
# 轉換為 WAV (預設)
python3 scripts/audio_converter.py song.mp3

# 轉換為 FLAC (無損)
python3 scripts/audio_converter.py song.mp3 --format flac

# 指定輸出檔名
python3 scripts/audio_converter.py song.mp3 -o output.wav
```

### 批次轉換

```bash
# 轉換資料夾內所有 MP3
python3 scripts/audio_converter.py *.mp3

# 轉換多個 FLAC 為 WAV
python3 scripts/audio_converter.py song1.flac song2.flac song3.flac --format wav

# 使用 verbose 查看詳細資訊
python3 scripts/audio_converter.py *.m4a -v
```

### 工作流程範例

#### 從 FLAC 封存轉為 ESP32 格式

```bash
# 1. 從 CD 擷取或下載 FLAC 無損檔案
# 2. 轉換為 ESP32 可播放的 WAV
python3 scripts/audio_converter.py album/*.flac --format wav

# 3. 將 WAV 複製到 SD 卡
cp *.wav /Volumes/SD_CARD/
```

#### 建立高品質封存

```bash
# 將各種格式統一為 FLAC 封存
python3 scripts/audio_converter.py *.mp3 *.m4a --format flac

# 結果：高品質無損 FLAC 檔案
```

## 🔧 命令列選項

```
usage: audio_converter.py [-h] [-f {wav,flac}] [-o OUTPUT] [-v] inputs [inputs ...]

ESP32-S3 HiFi-DAP Audio Converter with FLAC support

positional arguments:
  inputs                Input audio files

options:
  -h, --help            show this help message
  -f, --format {wav,flac}
                        Output format (default: wav)
  -o, --output OUTPUT   Output file path (single file only)
  -v, --verbose         Verbose FFmpeg output
```

## 📊 輸出資訊

### 範例輸出

```
ESP32-S3 HiFi-DAP Audio Converter
Maximum quality audio conversion

✓ ffmpeg version 6.0

🎵 Input: song.mp3
   Codec: mp3, Sample Rate: 44100Hz, Channels: 2
✓ Output: song.wav (12.45 MB)
   Format: WAV (16-bit PCM, 44.1kHz, Stereo)

Summary
Converted: 1/1 files
```

## ⚙️ 技術細節

### WAV 轉換參數

```bash
ffmpeg -i input.mp3 \
       -acodec pcm_s16le \    # 16-bit PCM
       -ar 44100 \            # 44.1kHz
       -ac 2 \                # Stereo
       -sample_fmt s16 \      # 16-bit signed
       output.wav
```

### FLAC 轉換參數

```bash
ffmpeg -i input.mp3 \
       -c:a flac \            # FLAC codec
       -ar 44100 \            # 44.1kHz
       -ac 2 \                # Stereo
       -compression_level 8 \ # Best balance
       -sample_fmt s32 \      # 32-bit signed
       output.flac
```

## 🎯 最佳實踐

### 1. **音樂庫管理**

```
原始檔案 (FLAC) → 封存保存
            ↓
        轉換為 WAV
            ↓
      複製到 SD 卡 → ESP32 播放
```

### 2. **檔案命名建議**

```bash
# 使用有意義的檔名
artist_-_song_title.wav

# 批次重命名範例
for f in *.wav; do
  mv "$f" "${f// /_}"  # 空格換底線
done
```

### 3. **SD 卡整理**

```
SD_CARD/
├── album1/
│   ├── 01_song1.wav
│   ├── 02_song2.wav
│   └── 03_song3.wav
└── album2/
    ├── 01_song1.wav
    └── 02_song2.wav
```

## ❓ 常見問題

### Q: WAV 檔案太大？

**A:** 使用 FLAC 格式封存，FLAC 約為 WAV 的 50-60% 大小且無損。

### Q: ESP32 可以播放 FLAC 嗎？

**A:** 目前只支援 WAV，未來可能加入 FLAC 解碼支援。

### Q: 如何批次轉換整個資料夾？

**A:**

```bash
# 使用 find + xargs
find music/ -name "*.mp3" -exec python3 scripts/audio_converter.py {} \;

# 或使用萬用字元
python3 scripts/audio_converter.py music/**/*.mp3
```

### Q: 轉換失敗怎麼辦？

**A:** 使用 `-v` 參數查看詳細錯誤：

```bash
python3 scripts/audio_converter.py song.mp3 -v
```

## 📦 依賴套件

```bash
# 安裝 FFmpeg (required)
brew install ffmpeg

# 安裝 Python 套件
pip install rich
```

## 🔗 相關資源

- FFmpeg 文件：https://ffmpeg.org/documentation.html
- FLAC 規格：https://xiph.org/flac/
- WAV 規格：https://en.wikipedia.org/wiki/WAV
