# A2DP 性能優化說明

## 已應用的優化

### 1. 自動重連功能 ✅

```cpp
a2dp_source.set_auto_reconnect(true);
```

**效果**：當藍牙連接意外斷開時，ESP32 會自動嘗試重新連接到上次配對的設備。

**適用場景**：

- 設備暫時移出範圍又回來
- 藍牙干擾導致的短暫斷線
- 提升用戶體驗，無需手動重連

---

### 2. 低延遲配置 (庫版本相關)

```cpp
// 這些方法在 ESP32-A2DP 某些版本可能不可用
a2dp_source.set_max_write_size(2048);      // 設置最大寫入塊大小
a2dp_source.set_max_write_delay_ms(5);     // 設置最大寫入延遲
```

**注意**：這些方法已註釋，因為：

- 不是所有版本的 `ESP32-A2DP` 庫都支持
- 需要根據實際庫版本調整
- 如果編譯報錯，可以移除這些行

**如何啟用**：

1. 檢查你的 ESP32-A2DP 庫版本（目前是 1.8.8）
2. 查看庫的 API 文檔確認是否支持
3. 取消註釋測試

---

## 其他可用的優化

### 3. Buffer 大小調整

當前配置：

- ESP32 (無 PSRAM): 16KB
- ESP32-S3 (有 PSRAM): 64KB

**調整建議**：

```h
// config.h
#define AUDIO_RING_BUFFER_SIZE  (32 * 1024)  // 增大 buffer 減少 underrun
```

### 4. SPI 頻率優化

當前 SD 卡：10MHz

**可嘗試**：

```c
// sd_card.c
uint32_t frequencies[] = {20000000, 10000000};  // 先試 20MHz
```

### 5. FreeRTOS 任務優先級

當前配置：

```h
#define TASK_SD_READ_PRIORITY   5
#define TASK_BT_TX_PRIORITY     20  // 最高優先級給音訊
```

**已優化** ✅

---

## 測試建議

1. **測試自動重連**：

   - 播放音樂時關閉藍牙耳機
   - 重新打開耳機
   - 觀察是否自動重連

2. **測試音訊穩定性**：

   - 長時間播放（1 小時+）
   - 監控 `buffer_underruns` 計數
   - 檢查序列埠輸出的 "BUFFER EMPTY" 警告

3. **測試範圍**：
   - 逐漸增加 ESP32 與耳機距離
   - 觀察斷線重連表現

---

## 參考資料

- [ESP32-A2DP GitHub](https://github.com/pschatzmann/ESP32-A2DP)
- [A2DP 優化文檔](docs/bluetooth_architecture.md)
