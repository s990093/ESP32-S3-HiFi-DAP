# ESP32-S3 Hi-Fi Digital Audio Player - System Architecture

> **Version:** 2.1
> **Last Updated:** 2026-01-06
> **Device:** TTGO T-Display S3 (ESP32-S3) / Classic ESP32

## 1. System Overview (系統總覽)

This project implements a high-fidelity digital audio player (DAP) capabilities on the ESP32 platform. It features a unique **Hybrid Bluetooth Architecture** that allows it to function as both a device scanner (central) and a high-quality audio source (A2DP Source) seamlessly.

### Key Features

- **Dual-Core Processing**: Dedicated cores for Audio/Bluetooth (Core 1) and System/UI (Core 0).
- **Hybrid Bluetooth**: Intelligent coexistence between GAP Scanning and A2DP Streaming.
- **High-Resolution Audio**: Supports WAV playback from SD Card via I2S.
- **Interactive UI**: TFT Display integration with physical button controls.

---

## 2. Architecture Diagram (系統架構圖)

The system is layered to separate hardware abstraction, logic management, and application control.

```mermaid
graph TD
    subgraph "Hardware Layer"
        ESP[ESP32-S3 SoC]
        SD[SD Card (SPI)]
        DAC[I2S DAC / Bluetooth]
        TFT[TFT Display (SPI)]
        BTN[Buttons (GPIO)]
    end

    subgraph "Driver / HAL Layer"
        SD_HAL[sd_card.cpp]
        DISP_HAL[display.cpp]
        BTN_HAL[button_handler.cpp]
        BT_STACK[ESP-IDF Bluetooth Stack]
    end

    subgraph "Manager Layer"
        BT_MGR[Bluetooth Manager (Scanner)]
        A2DP[A2DP Source (Streamer)]
        PLAY[Audio Player (Logic)]
    end

    subgraph "Application Layer"
        MAIN[Main Loop (Arduino)]
        UI[UI Controller]
        CMD[Serial Command Interface]
    end

    %% Connections
    BTN --> BTN_HAL --> MAIN
    MAIN --> UI --> DISP_HAL --> TFT
    MAIN --> CMD

    MAIN --> PLAY
    PLAY --> SD_HAL --> SD
    PLAY --> A2DP

    MAIN --> BT_MGR
    BT_MGR -- "Inquiry" --> BT_STACK --> ESP
    A2DP -- "Audio Stream" --> BT_STACK --> ESP
```

---

## 3. Subsystem Detail (子系統詳解)

### 3.1 Audiophile DSP Pipeline (Core 1)

The "Audiophile Edition" uses a dedicated `AudioOutputWithEQ` class that inherits from `ESP32I2SAudio`. This ensures a unified processing chain for **both** WAV and MP3 formats.

**Signal Path:**

```
[ Decoder Output (WAV/MP3) ]
         │
         ▼
[ 1. Headroom Scaler ]  <-- Multiplies by 0.707 (-3dB) to prevent clipping
         │
         ▼
[ 2. Float EQ Engine ]  <-- High-precision shelving filters
         │                  (Bass +4.5dB @ 100Hz, Treble +2.5dB @ 750Hz)
         ▼
[ 3. TPDF Dithering ]   <-- Adds triangular noise to linearize quantization error
         │
         ▼
[ 4. Hard Limiter ]     <-- Clamps values to int16 range
         │
         ▼
[ I2S DMA Buffer ]
         │
         ▼
[ PCM5102A DAC ]
```

**Key Class:**

```cpp
class AudioOutputWithEQ : public ESP32I2SAudio {
    virtual size_t write(const uint8_t *buffer, size_t size) override {
        // Intercepts all audio data here for DSP processing
    }
}
```

### 3.2 Hybrid Bluetooth Architecture (藍牙混合架構)

This is the most complex part of the system, solving the "Scanner vs Streamer" conflict.

- **State A: Scanning (Discovery)**

  - **Module**: `bluetooth_manager.cpp`
  - **Action**: Uses `esp_bt_gap_start_discovery` to find peripherals.
  - **Restriction**: Audio streaming is paused to dedicate radio time to scanning.
  - **Data**: POPULATES a list of nearby `bt_device_t`.

- **State B: Streaming (A2DP)**

  - **Module**: `bt_audio_stream.cpp`
  - **Action**: Connects to a specific MAC address using `BluetoothA2DPSource`.
  - **Priority**: High real-time priority. Scanning is disabled.

- **Coexistence Logic**: The system checks `bluetooth_is_connected()`. If connected, SCAN commands are rejected or deferred to prevent audio dropouts.

---

## 4. Software Modules (軟體模組說明)

| Module        | File                    | Responsibility                                        | Core Affinity    |
| :------------ | :---------------------- | :---------------------------------------------------- | :--------------- |
| **Main**      | `ESP32-S3-HiFi-DAP.ino` | Entry point, setup, main loop, event dispatching.     | Core 1 (Arduino) |
| **Audio**     | `audio_player.cpp`      | State machine (PLAY/PAUSE/STOP), playlist management. | Core 1           |
| **SD**        | `sd_card.cpp`           | SPI file system mounting, file scanning, raw reading. | Core 0 (I/O)     |
| **Bluetooth** | `bluetooth_manager.cpp` | GAP scanning, device list management.                 | Core 1           |
| **A2DP**      | `bt_audio_stream.cpp`   | A2DP source implementation, audio callbacks.          | Core 0 (Stack)   |
| **Display**   | `display.cpp`           | TFT rendering, progress bars, cover art (future).     | Core 1           |
| **Serial**    | `serial_manager.cpp`    | PC communication protocol handling.                   | Core 1           |

---

## 5. Directory Structure (目錄結構)

```text
ESP32-S3-HiFi-DAP/
├── ESP32-S3-HiFi-DAP.ino    # Main Application application
├── config.h                 # Global Configuration (Pins, Settings)
├── docs/                    # Documentation
│   ├── architecture.md      # This file
│   ├── remote_control.md    # Serial Protocol
│   └── pin_map.md           # Hardware Wiring
├── scripts/                 # Host-side Tools
│   ├── upload.py            # Build & Upload Tool
│   ├── monitor.py           # Serial Monitor
│   └── convert_audio.sh     # Audio preparation
├── src/                     # (Optional) Source folder if migrated
├── libraries/               # Local library overrides
└── partitons.csv            # Partition Scheme (App/Data/Factory)
```

## 6. Memory Map (Partition Scheme)

We use a custom `huge_app` partition scheme to accommodate the Bluetooth stack.

| Name    | Type | SubType | Offset  | Size                |
| :------ | :--- | :------ | :------ | :------------------ |
| nvs     | data | nvs     | 0x9000  | 20KB                |
| otadata | data | ota     | 0xe000  | 8KB                 |
| app0    | app  | ota_0   | 0x10000 | **3MB** (Large App) |
| spiffs  | data | spiffs  | ...     | 1MB (Assets)        |
