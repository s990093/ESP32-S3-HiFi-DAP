# Hardware Reference

## Device Setup: **TTGO T-Display S3** / Custom ESP32 Board

## Pin Configuration (`config.h`)

### SD Card (SPI)

| Signal   | GPIO (VSPI) | Notes               |
| :------- | :---------- | :------------------ |
| **CS**   | 5           | Chip Select         |
| **MOSI** | 23          | Master Out Slave In |
| **MISO** | 19          | Master In Slave Out |
| **SCK**  | 18          | Serial Clock        |

### I2S DAC Configuration (PCM5102A)

The Audiophile Edition is strictly tuned for the **PCM5102A** DAC.

| Pin Name              | ESP32 GPIO | Function     | Note                                                 |
| :-------------------- | :--------- | :----------- | :--------------------------------------------------- |
| **BCK**               | GPIO 4     | Bit Clock    |                                                      |
| **WS**                | GPIO 15    | Word Select  |                                                      |
| **DATA**              | GPIO 2     | Data In      |                                                      |
| **SCK**               | **GND**    | System Clock | **CRITICAL**: Grounding SCK forces internal PLL mode |
| **VCC**               | 5V / 3.3V  | Power        | **rec:** Add 100uF + 0.1uF caps for stability        |
| Left/Right Clock (WS) |
| **DOUT**              | 4          | Data Out     |

### OLED Display (I2C) - New

| Signal  | GPIO | Notes        |
| :------ | :--- | :----------- |
| **SDA** | 21   | Serial Data  |
| **SCL** | 22   | Serial Clock |

### User Interface

| Button   | GPIO | Active State  |
| :------- | :--- | :------------ |
| **Prev** | 32   | Low (Pull-up) |
| **Next** | 33   | Low (Pull-up) |
| **Play** | 25   | Low (Pull-up) |
| **Stop** | 26   | Low (Pull-up) |

## Partition Scheme

We use a custom `huge_app` partition scheme to enable large Bluetooth stacks.

**File:** `partitions.csv`

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x300000, # 3MB for Application
spiffs,   data, spiffs,  0x310000,0x100000, # 1MB for Files
```
