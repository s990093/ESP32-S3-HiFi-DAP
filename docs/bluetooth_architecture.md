# Bluetooth Architecture

ESP32 藍牙音訊播放器的藍牙架構文件。

## 系統架構概覽

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32 Bluetooth Architecture                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────┐      ┌──────────────────────┐        │
│  │ bluetooth_manager.c  │      │  bt_audio_stream.c   │        │
│  │                      │      │                      │        │
│  │  - Device Scanning   │      │  - A2DP Source       │        │
│  │  - GAP Control       │      │  - Audio Streaming   │        │
│  │  - Device List       │      │  - Connection Mgmt   │        │
│  └──────────┬───────────┘      └──────────┬───────────┘        │
│             │                             │                    │
└─────────────┼─────────────────────────────┼────────────────────┘
              │                             │
              ▼                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                     ESP-IDF Bluetooth APIs                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────┐      ┌──────────────────────┐        │
│  │   GAP (Generic       │      │   A2DP (Advanced     │        │
│  │   Access Profile)    │      │   Audio Distribution │        │
│  │                      │      │   Profile)           │        │
│  │  esp_bt_gap_*        │      │  esp_a2d_*           │        │
│  └──────────┬───────────┘      └──────────┬───────────┘        │
│             │                             │                    │
│             └──────────┬──────────────────┘                    │
│                        ▼                                        │
│              ┌─────────────────────┐                           │
│              │   Bluedroid Stack   │                           │
│              │                     │                           │
│              │  - Protocol Layers  │                           │
│              │  - SDP, RFCOMM      │                           │
│              │  - L2CAP, HCI       │                           │
│              └──────────┬──────────┘                           │
└─────────────────────────┼──────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Bluetooth Controller (HW)                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌────────────────────────────────────────────────────┐        │
│  │        ESP32 Bluetooth Classic Radio               │        │
│  │                                                     │        │
│  │   - 2.4GHz Transceiver                             │        │
│  │   - Baseband Processing                            │        │
│  │   - RF Frontend                                    │        │
│  └────────────────────────────────────────────────────┘        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                          │
                          │ Bluetooth Classic
                          ▼
                   ╔═════════════╗
                   ║  🎧 Headset ║
                   ║  📱 Speaker ║
                   ╚═════════════╝
```

## 工作流程：從掃描到音訊串流

```
┌─────────────┐       ┌──────────────┐       ┌─────────────┐
│ 1. SCAN     │──────▶│ 2. CONNECT   │──────▶│ 3. STREAM   │
│             │       │              │       │             │
│ GAP: Inquiry│       │ A2DP: Pair   │       │ A2DP: Audio │
│ Find Devices│       │ Establish    │       │ PCM→SBC→BT  │
│             │       │ Connection   │       │             │
└─────────────┘       └──────────────┘       └─────────────┘
      ▲                                              │
      │                                              │
      └──────────────────────────────────────────────┘
                  4. DISCONNECT (optional)
```

## 音訊資料流

```
SD Card (WAV)
     │
     ▼
┌──────────────────┐
│ Audio Player     │  Read WAV, decode PCM
│ (audio_player.c) │
└────────┬─────────┘
         │ PCM samples (16-bit, 44.1kHz)
         ▼
┌──────────────────┐
│ Audio Buffer     │  Ring buffer (AUDIO_BUFFER_SIZE)
│ (bt_audio_stream)│
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│ A2DP Callback    │  bt_audio_data_callback()
│                  │  Encode PCM → SBC
└────────┬─────────┘
         │ SBC encoded data
         ▼
┌──────────────────┐
│ Bluetooth TX     │  RF transmission
│ (Controller)     │
└────────┬─────────┘
         │
         ▼
    🎧 Headphones
```

## 記憶體架構

```
┌─────────────────────────────────────────────────────────────────┐
│  SRAM (327KB total)                                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───── Bluedroid Stack (~60KB) ──────────────────┐            │
│  │  - Connection Tables                           │            │
│  │  - Protocol State                              │            │
│  │  - L2CAP Buffers                               │            │
│  └────────────────────────────────────────────────┘            │
│                                                                 │
│  ┌───── Audio Buffers (~32KB) ────────────────────┐            │
│  │  - Ring Buffer (AUDIO_BUFFER_SIZE)             │            │
│  │  - I2S DMA Buffers                             │            │
│  └────────────────────────────────────────────────┘            │
│                                                                 │
│  ┌───── Application (~60KB) ───────────────────────┐           │
│  │  - SD Card FS                                   │           │
│  │  - Display Framebuffer                          │           │
│  │  - Playlist Data                                │           │
│  └────────────────────────────────────────────────┘            │
│                                                                 │
│  ┌───── System (~120KB) ───────────────────────────┐           │
│  │  - FreeRTOS Kernel                              │           │
│  │  - WiFi (disabled to save RAM)                  │           │
│  │  - Network Stack (minimal)                      │           │
│  └────────────────────────────────────────────────┘            │
│                                                                 │
│  Free: ~55KB (dynamic allocation headroom)                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 模組說明

### bluetooth_manager.c

- **職責**：藍牙 GAP 層管理
- **功能**：
  - 設備掃描（Inquiry）
  - 設備列表維護
  - RSSI 資訊收集
- **API**：
  - `bluetooth_manager_init()` - 初始化 GAP
  - `bluetooth_start_scan()` - 開始掃描
  - `bluetooth_get_scanned_devices()` - 取得掃描結果

### bt_audio_stream.c

- **職責**：A2DP Source 音訊串流
- **功能**：
  - A2DP 連接建立
  - 音訊編碼（PCM → SBC）
  - 資料傳輸管理
- **API**：
  - `bt_audio_init()` - 初始化 A2DP
  - `bt_audio_connect()` - 連接設備
  - `bt_audio_write()` - 寫入音訊資料

## 協定層說明

### GAP (Generic Access Profile)

- 負責設備掃描與發現
- 管理可見性與連接性
- 處理設備配對

### A2DP (Advanced Audio Distribution Profile)

- 高品質音訊串流協定
- 支援 SBC 編碼
- 提供音訊同步

### SBC (Subband Coding)

- 藍牙音訊標準編碼器
- 低延遲、中等壓縮率
- 適合即時音訊傳輸

## 記憶體優化策略

1. **停用 WiFi**：釋放 ~80KB RAM
2. **使用 Classic BT Only**：釋放 BLE 記憶體
3. **調整緩衝區大小**：平衡記憶體與流暢度
4. **雙核心分工**：
   - Core 0: 音訊解碼
   - Core 1: 藍牙傳輸

## 相關文件

- [A2DP Guide](a2dp_guide.md) - A2DP 使用指南
- [Serial Protocol](serial_protocol.md) - 序列埠通訊協定
