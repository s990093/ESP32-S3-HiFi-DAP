# [DEPRECATED] Serial Protocol

> **⚠️ NOTE**: This document is outdated. Please refer to [SERIAL_COMMANDS.md](../docs/SERIAL_COMMANDS.md) or [USER_GUIDE.md](../docs/USER_GUIDE.md) for the current command reference.

## Overview

The device communicates via a high-speed UART interface (default **460800 bps**). You can control it using the provided Python CLI tool or raw serial commands.

## Python CLI Tool (`scripts/monitor.py`)

A fast, simple serial monitor included in the project.

```bash
# Usage
python3 scripts/monitor.py [PORT] [BAUD]

# Example
python3 scripts/monitor.py /dev/cu.usbserial-0001 460800
```

---

## Command Reference

Send these strings over UART (terminated by `\n`).

### 🎵 Playlist Control (NEW!)

| Command        | Parameter | Description                     |
| :------------- | :-------- | :------------------------------ |
| `PLAY_CURRENT` | -         | Play current track in playlist. |
| `NEXT`         | -         | Skip to next track and play.    |
| `PREVIOUS`     | -         | Go to previous track and play.  |
| `PAUSE`        | -         | Pause current playback.         |
| `RESUME`       | -         | Resume paused playback.         |

### Playback Control

| Command          | Parameter | Description                              |
| :--------------- | :-------- | :--------------------------------------- |
| `PLAY:test2.wav` | Filename  | Play a specific WAV file from SD.        |
| `STOP`           | -         | Stop current playback.                   |
| `LIST`           | -         | List all WAV files on SD card.           |
| `SD_INIT`        | -         | Re-initialize SD card.                   |
| `STATUS`         | -         | Get current player state (Idle/Playing). |

### Bluetooth Control

| Command          | Parameter | Description                              |
| :--------------- | :-------- | :--------------------------------------- |
| `A2DP_CONNECT`   | -         | Connect to Bose Revolve+ II (hardcoded). |
| `A2DP_PLAY:file` | Filename  | Stream audio file via A2DP.              |
| `A2DP_STOP`      | -         | Stop A2DP streaming.                     |

### System & Diagnostics

| Command      | Parameter | Description                           |
| :----------- | :-------- | :------------------------------------ |
| `MEM`        | -         | Detailed heap memory analysis (w/ %). |
| `MEM_STATUS` | -         | (Alias for MEM).                      |

---

## Examples

### Basic Playback

```bash
# List all WAV files
LIST

# Play current track
PLAY_CURRENT

# Next track
NEXT

# Previous track
PREVIOUS

# Pause
PAUSE

# Resume
RESUME

# Stop
STOP
```

### Specific File Playback

```bash
# Play a specific file
PLAY:test.wav

# Play via Bluetooth
A2DP_CONNECT
A2DP_PLAY:test.wav
A2DP_STOP
```

### System Commands

```bash
# Check memory usage
MEM

# Re-initialize SD card
SD_INIT

# Check status
STATUS
```

---

## Memory Analysis (`MEM`)

The `MEM` command provides a deep dive into the ESP32's memory usage to help debug OOM (Out of Memory) issues.

**Example Output:**

```text
OK:HEAP|250000|327680|23.7|200000|180000|PSRAM|0|0|0.0

Format: HEAP|free|total|usage%|min_free|max_alloc|PSRAM|free|total|usage%
```

**Explanation:**

- **free**: Current free heap (bytes)
- **total**: Total heap size (bytes)
- **usage%**: Percentage used
- **min_free**: Lowest free memory since boot
- **max_alloc**: Largest allocatable block

**Debug Console Output:**

```text
--- Memory Status ---
[Internal RAM]
Heap summary for capabilities 0x00000800:
  At 0x3fc9b000 len 40960 free 25000 allocated 15960 ...
  largest_free_block 20000 ...

[PSRAM]
Not available
```

- **largest_free_block**: If this is small (<10KB) but total free is large, memory is fragmented.
- **min_free**: The lowest amount of free memory ever recorded since boot.

---

## Status Response Format

```
OK:<data>
ERROR:<message>
INFO:<message>
```

### STATUS Command Response

```
OK:2:/test.wav

Format: state:track
States: 0=IDLE, 1=PLAYING, 2=PAUSED, 3=STOPPED
```
