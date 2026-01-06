# ESP32-S3 HiFi-DAP 🎵

> **Production-Grade Digital Audio Player**  
> A professional-quality WAV music player built on ESP32 with FreeRTOS dual-core architecture.

[![Platform](https://img.shields.io/badge/platform-ESP32-blue)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Code Style](https://img.shields.io/badge/code%20style-embedded-orange)](.)

---

## ✨ Features

### 🎵 Audiophile Edition Features

- **🧠 Dynamic Loudness** - Fletcher-Munson inspired compensation (Bass +8dB at low vol, +2dB at high vol).
- **💎 TPDF Dithering** - Triangular Probability Density Function dithering to eliminate digital quantization distortion.
- **🎚️ 3kHz AIR Treble** - Refined Treble cutoff (3000Hz) for vocal clarity without harshness.
- **🛡️ Anti-Clipping** - 0.7x digital headroom scaler to prevent soft clipping on bass hits.
- **🎯 PCM5102A Optimized** - Tuned for internal PLL usage (SCK->GND) and Charge Pump characteristics.

### 🎯 Core Features

- **🎵 Hybrid Audio Engine** - Native support for **WAV** (16-bit PCM) and **MP3** files
- **🔊 BackgroundAudio Core** - High-performance decoding with I2S DMA offloading
- **📦 Robust File Parsing** - Unified playlist handler for mixed file types
- **💾 Playback Resume** - NVS-based position persistence across power cycles
- **🎚️ Cubic Volume Control** - Enhanced low-volume precision (`vol^3` curve)
- **🔇 Pop-Free Audio** - Fade in/out on play/pause/track changes
- **🎮 Hardware Button Control** - Physical buttons with combo actions (e.g. Loop Toggle)

### 🚀 Advanced Features

- **⚡ FreeRTOS Dual-Core** - Core 0 for UI, Core 1 for audio processing
- **🧠 Smart Memory Management** - Fixed arrays, no heap fragmentation
- **🔍 Serial Command Interface** - Debug and control via UART
- **📊 Real-Time Monitoring** - Memory usage, playback status, event logging
- **🎛️ Professional Controls** - Volume up/down, prev/next, pause/play

---

## 📋 Table of Contents

- [Desktop App](#-desktop-app-new)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Serial Commands](#serial-commands)
- [Audio Converter Tool](#audio-converter-tool)
- [Technical Architecture](#technical-architecture)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)
- [Development](#development)
- [License](#license)

---

## 🖥️ Desktop App (NEW!)

### Apple Music-Inspired Control Center

Manage your ESP32 music player with a beautiful desktop application featuring:

- **🎨 Premium Design** - Glassmorphism, vibrant gradients, Apple Music aesthetics
- **📚 Library Management** - Browse, play, delete, and rename tracks
- **📤 File Upload** - Drag & drop MP3/WAV files to SD card
- **📊 Real-time Monitoring** - Track info, volume, system stats
- **⚡ Auto-connect** - Remembers your device

![Desktop App](desktop-app/.screenshots/player_interface.png)

### Quick Start

```bash
cd desktop-app
npm install
npm start
```

👉 **[Full Desktop App Documentation →](desktop-app/README.md)**

---

## 🔧 Hardware Requirements

### Minimum Requirements

| Component   | Specification                              |
| ----------- | ------------------------------------------ |
| **MCU**     | ESP32 (ESP32-WROOM, ESP32-DevKitC)         |
| **Flash**   | 4MB minimum                                |
| **SRAM**    | 520KB (internal)                           |
| **SD Card** | Class 10+ (20MHz SPI compatible)           |
| **DAC**     | PCM5102A (Recommended for Audiophile Mode) |

> **⚠️ PCM5102A Critical Setup**:
> To enable the internal PLL and reduce jitter:
>
> 1. Connect **SCK** pin to **GND**.
> 2. Add **100uF + 0.1uF** capacitors near VCC for deep bass stability.

### Tested Hardware

- ✅ ESP32-DevKitC V4
- ✅ ESP32-WROOM-32D
- ✅ PCM5102A I2S DAC breakout
- ✅ SanDisk Ultra 32GB microSD (Class 10)

### Optional Components

- **Buttons** - 5x tactile switches (VOL+/-, PREV/NEXT, PAUSE)
- **Display** - (Future) TFT/OLED for UI
- **Amplifier** - External amplifier for speakers

---

## 📌 Pin Configuration

### SD Card (SPI)

| Pin  | ESP32 GPIO | Description |
| ---- | ---------- | ----------- |
| MISO | GPIO 19    | Data Out    |
| MOSI | GPIO 23    | Data In     |
| SCK  | GPIO 18    | Clock       |
| CS   | GPIO 5     | Chip Select |

### I2S DAC

| Pin    | ESP32 GPIO | Description             |
| ------ | ---------- | ----------------------- |
| BCK    | GPIO 4     | Bit Clock               |
| WS/LRC | GPIO 15    | Word Select (L/R Clock) |
| DATA   | GPIO 2     | Serial Data             |

### Control Buttons

| Function | ESP32 GPIO | Description                                    |
| -------- | ---------- | ---------------------------------------------- |
| VOL+     | GPIO 12    | Volume Up (single press +5%, long press +1%)   |
| VOL-     | GPIO 13    | Volume Down (single press -5%, long press -1%) |
| PREV     | GPIO 14    | Previous Track                                 |
| NEXT     | GPIO 27    | Next Track                                     |
| PAUSE    | GPIO 26    | Pause/Play (double-click = Next)               |

> **Note**: All buttons use `INPUT_PULLDOWN` mode with rising edge interrupt.

---

## 💻 Software Requirements

### Development Tools

```bash
# macOS (Homebrew)
brew install arduino-cli
brew install ffmpeg  # For audio conversion

# Python dependencies
pip install rich  # For colorful CLI output
```

### Arduino Libraries (Auto-installed)

- `SPI` v3.3.3
- `SD` v3.3.3
- `FS` v3.3.3
- `Preferences` v3.3.3 (NVS)
- `BackgroundAudio` (Native ESP32 I2S Audio Library)

---

## 🚀 Installation

### 1. Clone Repository

```bash
git clone https://github.com/yourusername/ESP32-S3-HiFi-DAP.git
cd ESP32-S3-HiFi-DAP
```

### 2. Configure Board

```bash
arduino-cli config init
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

### 3. Compile and Upload

```bash
# Using provided upload script
python3 scripts/upload.py src/WavPlayer --board esp32

# Or manually
arduino-cli compile --fqbn esp32:esp32:esp32 src/WavPlayer
arduino-cli upload -p /dev/cu.usbserial-* --fqbn esp32:esp32:esp32 src/WavPlayer
```

### 4. Prepare SD Card

```bash
# Format SD card as FAT32
# Copy WAV files to root directory

# Optional: Convert audio files
python3 scripts/audio_converter.py song.mp3 song.flac
```

### 5. Monitor Serial Output

```bash
python3 scripts/monitor.py /dev/cu.usbserial-* 460800
```

---

## 🎮 Usage

### Button Controls

| Action             | Button         | Behavior                                     |
| ------------------ | -------------- | -------------------------------------------- |
| **Volume Up**      | VOL+ (GPIO12)  | +5% per press, hold for +1%/50ms             |
| **Volume Down**    | VOL- (GPIO13)  | -5% per press, hold for -1%/50ms             |
| **Previous Track** | PREV (GPIO14)  | Jump to previous track                       |
| **Next Track**     | NEXT (GPIO27)  | Jump to next track                           |
| **Link Toggle**    | PREV + NEXT    | Hold both for 1s: Toggle Loop One / Loop All |
| **Pause/Play**     | PAUSE (GPIO26) | Single press = toggle, double-click = next   |

### First Boot

1. Insert SD card with WAV files
2. Power on ESP32
3. Wait for initialization (~3 seconds)
4. Press **PAUSE** button to start playback
5. Adjust volume with **VOL+/VOL-**

### Resume Playback

The player automatically saves:

- Current track position
- Volume level
- Play/pause state

After power cycle, it resumes from the last position.

---

## 🖥️ Serial Commands

Connect via serial terminal (460800 baud) and use these commands:

| Command  | Alias    | Description                                      |
| -------- | -------- | ------------------------------------------------ |
| `mem`    | `memory` | Show memory usage (heap, PSRAM) with visual bars |
| `status` | `s`      | Display current playback state                   |
| `save`   | -        | Manually save playback position to NVS           |
| `resume` | -        | Restore playback position from NVS               |
| `help`   | `h`, `?` | Show command list                                |

### Example: Memory Status

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

---

## 🎨 Audio Converter Tool

Convert audio files to ESP32-compatible WAV format:

### Basic Usage

```bash
# Convert to WAV (default)
python3 scripts/audio_converter.py song.mp3

# Convert to FLAC (lossless archive)
python3 scripts/audio_converter.py song.mp3 --format flac

# Batch convert
python3 scripts/audio_converter.py *.mp3 --format wav
```

### Supported Formats

**Input**: MP3, M4A, AAC, FLAC, WAV, OGG, WMA, APE, ALAC  
**Output**:

- **WAV** - 16-bit PCM, 44.1kHz stereo (ESP32 playback)
- **FLAC** - Lossless, 44.1kHz stereo (archival)

See [Audio Converter Guide](docs/audio_converter_guide.md) for details.

---

## 🏗️ Technical Architecture

### FreeRTOS Dual-Core Design

```
┌─────────────────────────────────────────────────────┐
│                    ESP32 (Dual-Core)                │
├───────────────────────────┬─────────────────────────┤
│    Core 0 (Protocol CPU)  │  Core 1 (Application CPU)│
│   ┌─────────────────────┐ │ ┌──────────────────────┐│
│   │  Button Handler     │ │ │  Audio Playback      ││
│   │  - ISR Processing   │ │ │  - SD Card Reading   ││
│   │  - State Management │ │ │  - WAV Decoding      ││
│   │  - Serial Commands  │ │ │  - Volume Control    ││
│   │  - UI Updates       │ │ │  - I2S Streaming     ││
│   └─────────────────────┘ │ └──────────────────────┘│
│          Priority: 1       │       Priority: 2       │
└───────────────────────────┴─────────────────────────┘
                      │
              ┌───────┴────────┐
              │  Shared State  │
              │  (Mutex-locked)│
              └────────────────┘
```

### Audio Pipeline

```
SD Card → Chunk Parser → Volume Control → Fade In/Out → I2S DMA → DAC → Headphones/Speakers
         (20MHz SPI)    (Logarithmic)    (2048 samples)  (8x1024)
```

### Key Technologies

- **BackgroundAudio Lib** - Core decoding engine for MP3/WAV
- **Chunk-Based Feeding** - Efficient buffer management
- **Cubic Volume Curve** - `pow(vol, 3)` calculation for 10-bit dynamic range
- **NVS (Non-Volatile Storage)** - Flash-based state persistence
- **Hybrid Task Pattern** - Core 0 (UI/Control) + Core 1 (Audio Decoding)

---

## 📁 Project Structure

```
ESP32-S3-HiFi-DAP/
├── src/
│   └── WavPlayer/
│       └── WavPlayer.ino          # Main firmware (1457 lines)
├── desktop-app/                   # 🆕 Desktop Control Center
│   ├── src/
│   │   ├── main.js                # Electron main process
│   │   ├── renderer.js            # UI logic (383 lines)
│   │   ├── styles.css             # Apple-style CSS (600+ lines)
│   │   ├── index.html             # Main interface
│   │   └── demo.html              # Standalone demo
│   ├── package.json
│   └── README.md                  # Desktop app docs
├── scripts/
│   ├── audio_converter.py         # Audio format converter
│   ├── upload.py                  # Build & upload tool
│   └── monitor.py                 # Serial monitor
├── docs/
│   ├── architecture.md            # System architecture
│   ├── wav_player_guide.md        # User guide
│   └── audio_converter_guide.md   # Converter manual
└── README.md                      # This file
```

---

## 🐛 Troubleshooting

### SD Card Not Detected

```
❌ SD Card failed!
```

**Solutions**:

1. Check wiring (MISO/MOSI/SCK/CS)
2. Ensure SD card is FAT32 formatted
3. Try reducing SPI speed to 10MHz in code:
   ```cpp
   SD.begin(SD_CS, SPI, 10000000);  // Change from 20MHz
   ```

### Audio Stuttering

**Causes**:

- Slow SD card (< Class 10)
- Corrupted WAV file
- Insufficient power supply

**Solutions**:

1. Use Class 10+ SD card
2. Re-convert WAV files:
   ```bash
   python3 scripts/audio_converter.py file.wav
   ```
3. Connect 5V/1A power supply (not USB)

### No Audio Output

**Check**:

1. DAC connections (BCK, WS, DATA)
2. DAC power supply (3.3V or 5V depending on model)
3. Headphone/speaker connection
4. Volume level (`status` command should show > 0%)

### Playback Not Resuming

```bash
# Reset NVS storage
save   # First save current state
resume # Then restore
```

If issue persists, reflash ESP32.

---

## 🛠️ Development

### Debug Mode

Enable detailed logging in `WavPlayer.ino`:

```cpp
#define DEBUG_ENABLED 1  // Set to 0 to disable
```

Output includes:

- WAV chunk parsing details
- File scanning progress
- Task creation status
- Event logs for all actions

### Performance Tuning

```cpp
// Adjust DMA buffer for lower latency
.dma_buf_count = 4,    // Default: 8
.dma_buf_len = 512,    // Default: 1024

// Increase SPI speed (if stable)
SD.begin(SD_CS, SPI, 40000000);  // 40MHz (risky)
```

### Adding New Features

1. **EQ (Equalizer)** - Apply DSP filters in `applyVolume()`
2. **Shuffle Mode** - Randomize `playlist[]` order
3. **Gapless Playback** - Pre-buffer next track
4. **Display Support** - Add TFT/OLED in `buttonHandlerTask`

---

## 📊 Performance Metrics

| Metric               | Value             |
| -------------------- | ----------------- |
| Flash Usage          | 376KB (12%)       |
| SRAM Usage           | 32KB (9.9%)       |
| Boot Time            | ~3 seconds        |
| Track Change Latency | <100ms            |
| Button Response      | <10ms (ISR)       |
| Audio Latency        | ~46ms (buffer)    |
| Max Tracks Supported | 32 (configurable) |

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **Espressif** - ESP32 platform and Arduino core
- **Hackaday** - I2S configuration reference
- **FreeRTOS** - Real-time operating system
- Community contributors and testers

---

## 📞 Contact

**Project**: [ESP32-S3-HiFi-DAP](https://github.com/yourusername/ESP32-S3-HiFi-DAP)  
**Issues**: [GitHub Issues](https://github.com/yourusername/ESP32-S3-HiFi-DAP/issues)  
**Documentation**: [Wiki](https://github.com/yourusername/ESP32-S3-HiFi-DAP/wiki)

---

<p align="center">
  Made with ❤️ for audiophiles and makers
</p>

<p align="center">
  <sub>If you found this project useful, please consider giving it a ⭐!</sub>
</p>
