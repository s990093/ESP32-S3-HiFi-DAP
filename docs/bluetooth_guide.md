# Bluetooth Usage Guide

## Hybrid Bluetooth Architecture

This device functions as a **Dual-Mode Bluetooth Central**:

1.  **Scanner Mode (GAP)**: Searches for nearby Bluetooth speakers/headphones.
2.  **Source Mode (A2DP)**: Streams high-quality audio to the connected device.

## Step-by-Step Connection

### 1. Scan for Devices

Send the scan command. The device will pause audio (if playing) to dedicate radio resources to scanning.

```bash
> BT_SCAN
OK:Scanning... Please wait 15s
```

After 15 seconds, it returns a list:

```text
OK:2:[0]JBL Flip 5|A0:B1:C2:D3:E4:F5|-45,[1]Unknown|11:22:33:44:55:66|-80
```

### 2. Connect

Connect using the index from the scan list (e.g., `0` for JBL Flip 5).

```bash
> A2DP_CONNECT:0
INFO:Connecting to JBL Flip 5
OK:Connecting...
```

_Note: You can also connect by raw MAC address: `A2DP_CONNECT:A0:B1:C2:D3:E4:F5`_

### 3. Stream Audio

Once connected (Speaker usually beeps), playing any file will route audio to Bluetooth instead of the DAC.

```bash
> PLAY:test.wav
```

### 4. Disconnect

```bash
> A2DP_STOP
> BT_DISCONNECT
```

## Troubleshooting

- **Choppy Audio**:
  - Ensure the antenna is not obstructed.
  - WAV files > 16-bit/48kHz might exceed Bluetooth bandwidth; use standard CD quality (16/44.1).
  - Run `MEM` to check if PSRAM is full.
- **Scan Finds Nothing**:
  - Ensure the speaker is in **Pairing Mode**.
  - Some devices filter MAC addresses; check your speaker manual.
