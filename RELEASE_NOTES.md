# Release Notes - v3.1.0 "Dynamic Loudness" 🎵

## 🚀 Major Updates

### 1. Dynamic Loudness Compensation (Fletcher-Munson)

Implementing "Product-Grade" DSP logic that adapts real-time to your listening volume:

- **Low Volume (<30%)**: **+8dB Bass / +4dB Treble**. Maintains deep, rich sound even at night.
- **High Volume (>80%)**: **+2dB Bass / +1dB Treble**. Tights up the sound for high-SPL listening without booming.
- **Smooth Transition**: EQ curve interpolates linearly between volume points.

### 2. Refined EQ Profile (Audiophile V3)

- **Treble Cutoff**: Moved from **750Hz** to **3000Hz (3kHz)**.
  - _Why?_ To strictly target "Air" and "Clarity" while preserving the natural body of vocals (1kHz-2kHz).
- **Bass Cutoff**: Maintained at **100Hz** for solid fundamental impact.

### 3. Audiophile Sound Engine (Retained)

- **💎 TPDF Dithering**: Active.
- **🛡️ Anti-Clipping**: 0.7x Headroom Scaler.
- **⚡ PCM5102A Optimization**: Internal PLL mode (SCK->GND).

### 4. Hybrid Audio Core

Robust playback support powered by `BackgroundAudio`:

- **Universal Playback**: Seamlessly handles **WAV** (16-bit PCM) and **MP3** files in the same playlist.
- **Smart Parsing**: Robust chunk-based WAV parser prevents crashes on metadata-heavy files.

### 3. Professional Controls

- **Loop Modes**: Toggle between "Loop All" and "Loop Single" (Hold PREV+NEXT).
- **Cubic Volume**: `vol^3` curve for precise low-volume adjustment.
- **Persistence**: Remembers Track, Volume, Position, and Loop Mode across reboots.

## 🛠️ Technical Improvements

- **FreeRTOS Architecture**:
  - **Core 0**: Button ISRs, UI Logic, NVS Storage.
  - **Core 1**: Audio Decoding, DSP Pipeline, I2S DMA.
- **Memory Safety**: Replaced all `String` usage with fixed `char` arrays to prevent heap fragmentation.
- **NVS Optimization**: Only saves state on significant events or auto-save intervals to reduce flash wear.

## 📋 Recommended Hardware

| Component | Note                                           |
| :-------- | :--------------------------------------------- |
| **DAC**   | **PCM5102A** (Connect SCK to GND for PLL mode) |
| **Power** | Add **100uF + 0.1uF** caps near DAC VCC        |
| **SD**    | Class 10 card (formatted FAT32)                |

---

_Verified on ESP32-S3 HiFi-DAP Hardware._
