# Release Notes - v3.0.0 "Audiophile Edition" 🎵

## 🚀 Major Updates

### 1. Audiophile Sound Engine

Incorporated professional DSP features to maximize the 112dB SNR of the PCM5102A DAC:

- **💎 TPDF Dithering**: Added Triangular Probability Density Function dithering to the output stage. This linearizes quantization error, converting digital distortion into undetectable analog noise for a smoother, warmer sound.
- **🎚️ Optimized V-Shape EQ**: A refined 10-band EQ profile specifically tuned for high-fidelity listening:
  - **Bass**: +4.5dB boost below 100Hz (Deep, punchy low-end).
  - **Treble**: +2.5dB boost above 750Hz (Clear, airy highs).
  - **Headroom**: Integrated **0.7x volume scaler** to prevent clipping during aggressive bass drops.
- **⚡ Hardware optimizations**: Configured for internal PLL usage (SCK grounded) to minimize jitter.

### 2. Hybrid Audio Core

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
