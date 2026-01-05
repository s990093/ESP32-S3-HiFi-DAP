# PCM5102 DAC 接線指南 - TTGO T-Display V1.1

## 📌 引腳配置

### TTGO T-Display → PCM5102

| PCM5102 引腳 | TTGO GPIO | 功能說明                            |
| ------------ | --------- | ----------------------------------- |
| **BCK**      | GPIO17    | I2S Bit Clock (位元時脈)            |
| **DIN**      | GPIO15    | I2S Data Out (音訊資料)             |
| **LRCK**     | GPIO2     | I2S Left/Right Clock (左右聲道選擇) |
| **GND**      | GND       | 接地                                |
| **VIN**      | 3V3 或 5V | 電源 (建議 3.3V)                    |
| **SCK**      | 不接      | 系統時脈 (PCM5102 內建，可不接)     |
| **FMT**      | GND       | 格式選擇 (接 GND = I2S 格式)        |
| **XMT**      | 3V3       | 靜音控制 (接 3V3 = 正常輸出)        |

## ⚠️ 重要變更

### 引腳衝突修復

原本配置有衝突：

- ❌ **GPIO25** 同時用於 I2S_BCLK 和 SD_CS
- ❌ **GPIO26, GPIO27** 是 DAC 引腳，可能影響音質

### 新配置 (無衝突)

- ✅ **GPIO17** → I2S_BCLK (可用)
- ✅ **GPIO2** → I2S_LRC (可用，但不要用於按鈕)
- ✅ **GPIO15** → I2S_DOUT (可用)

## 📝 注意事項

1. **GPIO2 特殊性**

   - GPIO2 是 Strapping Pin
   - 開機時必須為 HIGH (上拉)
   - 不要接按鈕或負載

2. **電源連接**

   - **推薦**: VIN → 3V3 (乾淨電源)
   - **可選**: VIN → 5V (如果 DAC 模組支持)
   - 建議加 100µF 電容濾波

3. **音質優化**
   - GND 盡量粗短
   - 使用隔離電源
   - 遠離 WiFi/BT 模組

## 🔌 完整接線圖

```
TTGO T-Display V1.1          PCM5102 DAC Module
┌─────────────────┐         ┌──────────────────┐
│                 │         │                  │
│  GPIO17    ────────────────→ BCK             │
│  GPIO15    ────────────────→ DIN             │
│  GPIO2     ────────────────→ LRCK            │
│  GND       ────────────────→ GND             │
│  3V3       ────────────────→ VIN             │
│                 │         │                  │
│                 │         │  GND ← FMT       │
│                 │         │  3V3 ← XMT       │
│                 │         │                  │
└─────────────────┘         └──────────────────┘
                                     │
                                     ↓
                              3.5mm 音訊輸出
```

## ✅ 功能配置

- ✅ I2S PCM5102 輸出
- ✅ SD 卡播放
- ✅ TFT 顯示
- ❌ 藍牙 A2DP (已停用)

## 🧪 測試步驟

1. 上傳韌體

```bash
python3 scripts/upload.py --baud 460800
```

2. 檢查序列輸出

```
I2S initialized successfully
Audio player initialized successfully
```

3. 播放測試

```
PLAY_CURRENT  # 播放當前曲目
```

4. 確認音訊輸出

- 檢查 PCM5102 LED (如果有)
- 連接耳機/喇叭測試

## 🔧 故障排除

### 無聲音

- 檢查接線
- 確認 FMT 接 GND
- 確認 XMT 接 3V3 (不靜音)

### 雜音

- 改善電源品質
- 檢查 GND 連接
- 遠離干擾源

### I2S 初始化失敗

- 檢查引腳配置
- 確認沒有引腳衝突
- 查看序列輸出錯誤訊息
