# [DEPRECATED] WAV Player Usage Guide

> **⚠️ NOTE**: This guide is outdated. Please refer to [USER_GUIDE.md](../docs/USER_GUIDE.md) and [README.md](../README.md) for the latest instructions on MP3/WAV playback, controls, and features.

## Quick Start

### 1. Prepare Your WAV File

Create a test WAV file using ffmpeg:

```bash
# Generate a 10-second 1kHz sine wave test tone
ffmpeg -f lavfi -i "sine=frequency=1000:duration=10" -ar 44100 -ac 2 -c:a pcm_s16le test2.wav

# OR convert any audio file to the correct format
ffmpeg -i your_song.mp3 -ar 44100 -ac 2 -c:a pcm_s16le test2.wav
```

Copy `test2.wav` to the **root directory** of your FAT32-formatted SD card.

### 2. Hardware Connections

**SD Card Module (SPI):**

```
SD Card    →  ESP32
MISO       →  GPIO36 (Input Only)
MOSI       →  GPIO32
SCK        →  GPIO33
CS         →  GPIO25
VCC        →  3.3V
GND        →  GND
```

**PCM5102 DAC Module (I2S):**

```
PCM5102    →  ESP32
BCK        →  GPIO26
LRCK/WS    →  GPIO27
DIN        →  GPIO22
VIN        →  5V (or 3.3V)
GND        →  GND
AGND       →  GND (Important!)
```

**Audio Output:**

```
PCM5102    →  Output Device
LROUT      →  Left channel
ROUT       →  Right channel
AGND       →  Ground
```

Connect to:

- 🎧 Headphones (via 3.5mm jack)
- 🔊 Powered speakers
- 🎸 Amplifier AUX input

### 3. Upload Firmware

Using Arduino CLI:

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32s3 src/WavPlayer

# Upload (adjust port as needed)
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p /dev/cu.usbserial-* src/WavPlayer

# Monitor output
arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=460800
```

Or use Arduino IDE:

1. Open `src/WavPlayer/WavPlayer.ino`
2. Select your ESP32 board
3. Click Upload
4. Open Serial Monitor at 460800 baud

### 4. Expected Output

Serial Monitor will show:

```
=== WAV Player with PCM5102 I2S DAC ===

Initializing SD card...
  MISO: GPIO36 (Input Only)
  MOSI: GPIO32
  SCK:  GPIO33
  CS:   GPIO25
✅ SD Card initialized

Opening /test2.wav...

📊 WAV File Info:
  Sample Rate: 44100 Hz
  Channels: 2 (Stereo)
  Bits per Sample: 16
  Audio Format: PCM
  Data Size: 1764000 bytes (1.68 MB)
  Duration: 10.0 seconds

Initializing I2S...
  BCK:  GPIO26
  LRCK: GPIO27
  DIN:  GPIO22
✅ I2S initialized

🎵 Starting playback...

Playing... [▓▓▓▓▓▓▓▓▓░░░░░░░░░░░] 47%
```

**Audio will play continuously in a loop!** 🔁

## Troubleshooting

### No Audio?

1. **Check DAC wiring**

   - Verify BCK, LRCK, DIN are correctly connected
   - Ensure VIN has stable power (5V recommended)
   - **IMPORTANT**: Connect AGND to GND!

2. **Check output device**

   - Use powered speakers or amplifier (DAC output is line-level)
   - If using headphones, they may need amplification
   - Test with different output device

3. **Check file format**

   ```bash
   # Verify WAV format
   ffprobe test2.wav
   ```

   Must be: PCM, 16-bit, 44.1kHz, Stereo

4. **Try different sample rate**
   ```bash
   # Create 48kHz version
   ffmpeg -i test2.wav -ar 48000 test2_48k.wav
   ```

### SD Card Not Detected?

1. **Format as FAT32**

   - Use SD Card Formatter tool
   - Maximum 32GB for FAT32

2. **Check wiring**

   - GPIO36 must be MISO (it's input-only, perfect for this)
   - Double-check all SPI connections

3. **Try slower SPI speed**
   In WavPlayer.ino, change:
   ```cpp
   SD.begin(SD_CS, SPI, 10000000)  // 10MHz
   ```
   to:
   ```cpp
   SD.begin(SD_CS, SPI, 4000000)   // 4MHz
   ```

### File Not Found?

- Ensure file is named exactly `test2.wav` (lowercase)
- Place in SD card root directory (not in a folder)
- Verify SD card is properly inserted

### Choppy Audio?

This shouldn't happen with the 8KB buffer, but if it does:

- Increase `BUFFER_SIZE` in code (try 16384)
- Use Arduino IDE with "Huge App" partition scheme
- Check SD card quality (use Class 10 or better)

## Advanced Configuration

### Change File Name

Edit line in `WavPlayer.ino`:

```cpp
if (!openWAVFile("/test2.wav")) {
```

Change to your filename, e.g.:

```cpp
if (!openWAVFile("/music.wav")) {
```

### Change I2S Pins

Edit the pin definitions:

```cpp
#define I2S_BCK   26    // Change to your BCK pin
#define I2S_WS    27    // Change to your WS pin
#define I2S_DATA  22    // Change to your DATA pin
```

### Disable Loop Playback

In the `loop()` function, replace the restart code with:

```cpp
} else {
  // Stop playback
  Serial.println("\n\n✅ Playback finished!");
  while(1) delay(1000);  // Stop here
}
```

## Technical Details

- **Buffer Size**: 8KB DMA buffer for smooth playback
- **I2S Mode**: Master TX, I2S format
- **DMA Buffers**: 8 buffers × 1024 bytes
- **Supported Formats**: PCM WAV, 16-bit, Mono/Stereo
- **Auto Sample Rate**: Reads from WAV header (supports 44.1kHz, 48kHz, etc.)
- **Memory Usage**: ~10KB RAM

## PCM5102 Jumper Settings

Your PCM5102 module has jumpers (H1L/H2L/H3L). **Default settings are fine!**

- H1L: Filter Select (Normal latency)
- H2L: De-emphasis (Off)
- H3L: Soft Mute (Unmuted)

**Don't change these unless you know what you're doing!**

## What's Next?

This player is a foundation for:

- 🎮 Adding button controls (play/pause/next/previous)
- 📁 Playing multiple files from a playlist
- 📱 Adding OLED/TFT display
- 📶 Bluetooth control
- 🎛️ Volume control
- 🎚️ EQ/effects processing

Enjoy your ESP32 Hi-Fi audio player! 🎶
