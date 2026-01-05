# Getting Started Guide

## Prerequisites

### 1. Install Arduino CLI

```bash
# macOS
brew install arduino-cli
# Or download from https://arduino.github.io/arduino-cli/
```

### 2. Install ESP32 Platform

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

## Compilation & Upload

This project uses a Python script to wrap `arduino-cli` and `esptool`, providing a uniform build environment.

### 1. Compile

```bash
# Basic compile
python3 scripts/upload.py .

# Compile for a specific board variant (default is esp32)
python3 scripts/upload.py . --board esp32
```

### 2. Upload

The script automatically detects the serial port.

```bash
python3 scripts/upload.py .
```

If auto-detection fails, specify the port:

```bash
python3 scripts/upload.py . --port /dev/cu.usbserial-1234
```

### 3. Monitor

To view serial output and debug logs:

```bash
python3 scripts/monitor.py /dev/cu.usbserial-1234
# Default baud rate is 460800
```

## SD Card Setup

1.  Format a microSD card to **FAT32**.
2.  Copy `.wav` audio files to the root directory.
    - Format: 16-bit PCM WAV (44.1kHz / 48kHz recommended).
3.  Insert into the SD card slot.

## Troubleshooting

### "Plugin needed to handle lto object"

If you see linker errors related to LTO, it means your toolchain is mismatched.
**Fix**: The `scripts/upload.py` script automatically disables LTO flags if they cause issues. Ensure you are using the provided script, not raw `arduino-cli` commands.

### "No USB serial port found"

1.  Check USB cable (must support data).
2.  Install drivers for CP210x or CH340.
3.  On macOS, check `ls /dev/cu.*`.

### "SD Init Failed"

1.  Check if card is FAT32.
2.  Check SPI pin wiring (see `docs/hardware_reference.md`).
