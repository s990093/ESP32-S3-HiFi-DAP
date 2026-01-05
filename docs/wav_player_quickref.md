# WAV Player Quick Reference

## Pin Connections

### SD Card (SPI)

```
SD → ESP32
───────────
MISO → GPIO36
MOSI → GPIO32
SCK  → GPIO33
CS   → GPIO25
VCC  → 3.3V
GND  → GND
```

### PCM5102 DAC (I2S)

```
PCM5102 → ESP32
────────────────
BCK     → GPIO26
LRCK/WS → GPIO27
DIN     → GPIO22
VIN     → 5V
GND     → GND
AGND    → GND ⚠️
```

### Audio Output

```
PCM5102 → Device
────────────────
LROUT → Left channel
ROUT  → Right channel
```

## Quick Start Commands

```bash
# 1. Create test file
ffmpeg -f lavfi -i "sine=frequency=1000:duration=10" -ar 44100 -ac 2 -c:a pcm_s16le test2.wav

# 2. Copy to SD card root

# 3. Compile & Upload
arduino-cli compile --fqbn esp32:esp32:esp32s3 src/WavPlayer
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p /dev/cu.usbserial-* src/WavPlayer

# 4. Monitor
arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=460800
```

## File Location

- **Code**: `src/WavPlayer/WavPlayer.ino`
- **Guide**: `docs/wav_player_guide.md`
- **WAV File**: `/test2.wav` (on SD card root)

## Common Issues

| Problem        | Fix                              |
| -------------- | -------------------------------- |
| No audio       | Check AGND→GND connection        |
| SD error       | Format SD as FAT32               |
| File not found | Name must be `test2.wav` in root |

## Supported Formats

- ✅ PCM WAV
- ✅ 16-bit
- ✅ Mono/Stereo
- ✅ 44.1kHz / 48kHz (auto-detected)

🎶 **Audio plays in loop!**
