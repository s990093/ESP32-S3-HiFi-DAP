# Button Test Guide

## 📌 按鈕接線圖

基於您的 ESP32-WROOM 開發板：

```
按鈕      GPIO    接線方式
─────────────────────────────
VOL+  →  GPIO12  →  按鈕 → 3.3V
VOL-  →  GPIO13  →  按鈕 → 3.3V
PREV  →  GPIO14  →  按鈕 → 3.3V
NEXT  →  GPIO27  →  按鈕 → 3.3V
```

### 實體接線

每個按鈕需要：

- 一端接到對應的 GPIO
- 另一端接到 3.3V

**不需要外接電阻！** 程式已啟用內部下拉電阻。

### 在開發板上的位置（左側針腳）

```
[ESP32-WROOM 左側]
    ...
    GPIO27  ← NEXT 按鈕
    GPIO26
    GPIO25
    GPIO14  ← PREV 按鈕
    GPIO12  ← VOL+ 按鈕
    GPIO13  ← VOL- 按鈕
    ...
```

## 🚫 避開的腳位

**已使用（不要碰）：**

- SD 卡：GPIO19, GPIO23, GPIO18, GPIO5
- I2S DAC：GPIO4, GPIO15, GPIO2

## 🧪 測試步驟

### 1. 上傳測試程式

```bash
python3 scripts/upload.py tests/TestButtons --board esp32 --no-build
```

### 2. 連接按鈕

最簡單的測試方式：

- 用杜邦線，一端接 GPIO，另一端手動碰觸 3.3V（快速測試）
- 或焊接實際按鈕

### 3. 觀察 Serial Monitor

預期輸出：

```
╔════════════════════════════════════════╗
║   ESP32 Button Test - 4 Buttons        ║
╚════════════════════════════════════════╝

Button Configuration:
┌────────┬────────┬──────────────────┐
│ Button │  GPIO  │   Connection     │
├────────┼────────┼──────────────────┤
│ VOL+   │  12    │ GPIO12 ↔ 3.3V   │
│ VOL-   │  13    │ GPIO13 ↔ 3.3V   │
│ PREV   │  14    │ GPIO14 ↔ 3.3V   │
│ NEXT   │  27    │ GPIO27 ↔ 3.3V   │
└────────┴────────┴──────────────────┘

🎮 Button test ready!
Press any button to test...
```

### 4. 按下按鈕

按下 VOL+ 時會顯示：

```
╔════════════════════════════════════════╗
║ 🔘 Button Pressed: VOL+                ║
╠════════════════════════════════════════╣
║ GPIO: 12                               ║
║ Press Count: 1                         ║
╚════════════════════════════════════════╝
🔊 Action: Increase Volume
   Volume: [███████████░░░░░░░░] 55%
```

## 🎛️ 功能說明

### VOL+ / VOL- 按鈕

- 每次按下改變音量 ±5%
- 範圍：0-100%
- 視覺化音量條顯示

### PREV / NEXT 按鈕

- 模擬切換曲目（test1.wav, test2.wav, ...）
- 顯示當前播放曲目
- 循環播放（到最後會回到第一首）

### 防彈跳

- 自動 50ms 防彈跳處理
- 不會因為按鈕抖動而重複觸發

### 狀態監控

- 每 5 秒顯示按鈕狀態摘要
- 顯示每個按鈕的按壓次數
- 系統運行時間

## ✅ 測試檢查清單

- [ ] VOL+ 按鈕能觸發
- [ ] VOL- 按鈕能觸發
- [ ] PREV 按鈕能觸發
- [ ] NEXT 按鈕能觸發
- [ ] 音量條正常顯示
- [ ] 曲目切換正常顯示
- [ ] 沒有重複觸發（防彈跳正常）

## 🔧 故障排除

### 按鈕按下沒反應

1. 檢查接線（GPIO 正確？3.3V 正確？）
2. 確認按鈕本身是好的
3. 檢查 Serial Monitor 的狀態欄（每 5 秒更新）

### 按一次觸發多次

1. 按鈕品質問題（更換按鈕）
2. 增加 DEBOUNCE_MS 值（程式碼第 18 行）

### GPIO 衝突

如果這些 GPIO 有問題，可改用：

- 備選：GPIO21, GPIO22, GPIO32, GPIO33

## 📸 接線參考

![ESP32 Pinout](file:///Users/hungwei/.gemini/antigravity/brain/e10872fe-d808-43d5-b9d4-87264289525c/uploaded_image_1767246650882.png)

按鈕接在**左側**的 GPIO12/13/14/27，方便接線且不衝突！
