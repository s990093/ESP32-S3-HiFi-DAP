# Serial 指令快速參考

## 🎯 指令總覽 (10 個)

| 指令         | 別名     | 功能          | 使用頻率   |
| ------------ | -------- | ------------- | ---------- |
| `mem`        | `memory` | 記憶體狀態    | ⭐⭐⭐     |
| `status`     | `s`      | 播放器狀態    | ⭐⭐⭐⭐⭐ |
| `settings`   | `config` | 系統設定      | ⭐⭐       |
| `cpu`        | `tasks`  | FreeRTOS 任務 | ⭐⭐⭐     |
| `nvs`        | `read`   | NVS 儲存      | ⭐⭐       |
| `tree`       | `ls`     | SD 檔案列表   | ⭐⭐⭐     |
| `cat <file>` | -        | 檔案內容      | ⭐⭐       |
| `save`       | -        | 儲存狀態      | ⭐         |
| `clear`      | `reset`  | 清除 NVS      | ⭐         |
| `resume`     | -        | 恢復狀態      | ⭐         |
| `help`       | `h`, `?` | 指令說明      | ⭐⭐⭐⭐   |

---

## 📝 常用組合

### 系統健康檢查

```bash
mem      # 記憶體
cpu      # CPU負載
status   # 播放狀態
```

### SD 卡檢查

```bash
tree           # 列出所有檔案
cat test.wav   # 檢查特定檔案
```

### 除錯流程

```bash
cpu            # 檢查任務狀態
mem            # 檢查記憶體
nvs            # 檢查儲存
settings       # 確認配置
```

---

## 🔤 指令詳細

### `mem` - 記憶體監控

**用途**: 檢查堆積使用、防止記憶體洩漏

**輸出**:

- Heap Total / Used / Free (含百分比)
- 視覺化圖表 (20 字元)
- Min Free Heap (歷史最低值)

**警告指標**:

- `Min Free Heap < 200KB` - 記憶體緊張
- `Usage > 70%` - 需要檢查

---

### `status` - 播放狀態

**用途**: 快速查看當前播放資訊

**顯示內容**:

- 播放狀態 (Playing/Paused/Stopped)
- 當前曲目 (編號/總數)
- 檔名
- 音量
- 運行時間

---

### `cpu` - CPU 監控

**用途**: 檢查 FreeRTOS 任務與 CPU 負載

**關鍵欄位**:

- **State**: X=執行中, B=等待中, R=就緒
- **Stack**: 剩餘堆疊空間 (bytes)

**健康標準**:

- AudioTask/ButtonTask 大部分時間是 `B`
- IDLE 任務能執行 (R 或 X)
- Stack > 500 bytes

**異常狀況**:

- Stack < 100 → 快崩潰
- AudioTask 一直是 X → CPU 滿載
- IDLE 消失 → 系統過載

---

### `nvs` - NVS 儲存查看

**用途**: 檢查斷點續播資料

**顯示內容**:

- 儲存在 Flash 的狀態
- 當前 RAM 中的狀態
- 自動儲存觸發條件

**對比**:

- Stored ≠ Runtime → 還沒儲存
- Stored = Runtime → 已同步

---

### `tree` - SD 卡檔案

**用途**: 快速查看 SD 卡內容

**顯示**:

- 📄 一般檔案
- 🔒 隱藏檔案 (.\_開頭)
- 檔案大小 (KB/MB)
- 總計

---

### `cat <file>` - 檔案檢查

**用途**: WAV 格式驗證、除錯播放問題

**WAV 檔案會顯示**:

- RIFF header
- Chunk 列表
- 音訊格式 (PCM, Sample Rate, Channels, Bit Depth)

**文字檔案**:

- 顯示前 1KB 內容

---

### `save` - 手動儲存

**用途**: 強制儲存當前狀態到 NVS

**儲存內容**:

- 曲目編號
- 播放位置
- 音量
- 播放狀態

---

### `clear` - 清除 NVS

**用途**: 清空所有儲存的播放狀態 (重置為預設值)

**效果**:

- 刪除 NVS 中的所有 key
- 下次開機將從頭開始播放
- `currentPosition` 重置為 0

---

### `resume` - 手動恢復

**用途**: 從 NVS 重新載入狀態

**效果**:

- 覆蓋當前 RAM 中的狀態
- 恢復到上次 `save` 的狀態

---

### `settings` - 系統設定

**用途**: 查看硬體配置與系統參數

**包含**:

- 所有 GPIO 接腳
- 音訊參數 (Sample Rate, APLL)
- 緩衝設定
- Timing 參數
- 功能列表

---

### `help` - 指令說明

**用途**: 顯示所有可用指令

快速參考！

---

## ⚡ 快捷鍵

| 輸入 | 等同於   | 說明               |
| ---- | -------- | ------------------ |
| `s`  | `status` | 最常用，快速查狀態 |
| `h`  | `help`   | 忘記指令時         |
| `?`  | `help`   | 同上               |

---

## 🎯 使用情境

### 情境 1: 播放異常

```bash
status       # 確認當前狀態
cat file.wav # 檢查WAV格式
cpu          # 看是否卡住
```

### 情境 2: 記憶體問題

```bash
mem          # 當前記憶體
cpu          # 檢查Stack是否不足
```

### 情境 3: 斷點續播測試

```bash
status       # 播放到某位置
save         # 儲存
(重啟)
nvs          # 確認載入
```

### 情境 4: SD 卡問題

```bash
tree         # 看檔案列表
cat xxx.wav  # 檢查特定檔案
```

---

**版本**: v2.0.1  
**Serial Baud**: 460800  
**指令總數**: 10
