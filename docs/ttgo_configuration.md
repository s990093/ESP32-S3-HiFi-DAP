# TTGO T-Display V1.1 Configuration Notes

## 板子規格確認

- **型號**: TTGO T-Display V1.1 (ESP32 標準版)
- **處理器**: ESP32 (非 S3)
- **PSRAM**: 無
- **顯示器**: ST7789V，1.14" IPS，135x240，SPI 接口

## 引腳配置總結

### TFT 顯示器 (SPI - 來自引腳圖)

```c
MOSI: GPIO19
SCLK: GPIO18
CS:   GPIO5
DC:   GPIO16
RST:  GPIO23
BL:   GPIO4
```

### SD 卡 (SPI - 用戶指定，需確認)

```c
MISO: GPIO12 ⚠️ Strapping pin
MOSI: GPIO13
SCK:  GPIO14
CS:   GPIO15 ⚠️ Strapping pin (MTDO)
```

**⚠️ 重要警告**:

- **GPIO12** 是 strapping pin，上電時必須為 LOW 才能從 flash 啟動
- **GPIO15** 也是 strapping pin (MTDO)
- 如果 SD 卡模組在開機時讓這些引腳為 HIGH，可能導致啟動失敗

**建議替代方案** (如果遇到啟動問題):

```c
// 更安全的選擇（避開 strapping pins）
SD_MISO: GPIO19  // 與 TFT 共用 MOSI - 需測試
SD_MOSI: GPIO23  // 與 TFT 共用 RST - 需測試
SD_SCK:  GPIO18  // 與 TFT 共用 SCLK - 需測試
SD_CS:   GPIO5   // 與 TFT 共用 CS - 需測試

// 或使用完全不同的引腳
SD_MISO: GPIO36  // Input only - 可作為 MISO
SD_MOSI: GPIO32
SD_SCK:  GPIO33
SD_CS:   GPIO25
```

### I2S 音訊輸出

```c
BCLK: GPIO25 (ADC18, DAC1)
LRC:  GPIO26 (ADC19, DAC2)
DOUT: GPIO27 (ADC17, TOUCH7)
```

### 按鈕

```c
BTN_PREVIOUS (GPIO35): 右上方按鈕 (Input Only)
BTN_NEXT (GPIO0):      Boot 按鈕
```

### I2C (來自引腳圖標註)

```c
SDA: GPIO21
SCL: GPIO22
```

## 記憶體配置

- **Audio Ring Buffer**: 16KB (無 PSRAM)
- **PSRAM**: 停用

## 測試建議

### 1. 先測試顯示器

```cpp
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
tft.init();
tft.fillScreen(TFT_RED);
```

### 2. 再測試 SD 卡

- 觀察開機是否正常
- 如果無法啟動，需要更換 SD 卡引腳

### 3. 最後測試音訊

- 確認 I2S DAC 連接正確
- GPIO25/26 有硬體 DAC，可用於測試

## 潛在問題與解決

### 問題 1: 啟動失敗

**原因**: GPIO12 在啟動時為 HIGH  
**解決**:

1. 移除 SD 卡模組重新上電測試
2. 更換 SD 卡引腳為非 strapping pins

### 問題 2: TFT 與 SD 卡衝突

**原因**: 可能共用 SPI 匯流排  
**解決**:

1. 確保 CS 引腳不同
2. 正確初始化順序：先 TFT，再 SD

### 問題 3: 記憶體不足

**原因**: 無 PSRAM，buffer 太小  
**解決**:

1. 降低 buffer 大小
2. 停用不必要功能
3. 優化任務堆疊大小

## 參考

- 引腳圖: uploaded_image_1767197409399.png
- [TTGO T-Display V1.1 官方倉庫](https://github.com/Xinyuan-LilyGO/TTGO-T-Display)
