# Memory Status Command Format

## Response Format

```
OK:HEAP|free|total|used%|free%|min_free|max_alloc|PSRAM|free|total|used%|free%
```

### HEAP Section

- **free**: 當前可用堆記憶體 (bytes)
- **total**: 總堆記憶體 (bytes)
- **used%**: 已使用百分比
- **free%**: 剩餘百分比
- **min_free**: 開機以來最低可用記憶體 (bytes)
- **max_alloc**: 最大可分配塊大小 (bytes)

### PSRAM Section

- **free**: 當前可用 PSRAM (bytes)
- **total**: 總 PSRAM (bytes)
- **used%**: 已使用百分比
- **free%**: 剩餘百分比

### 範例輸出

**有 PSRAM (ESP32-S3)**:

```
OK:HEAP|250000|327680|23.7|76.3|200000|180000|PSRAM|3800000|4194304|9.4|90.6
```

**無 PSRAM (ESP32 標準版)**:

```
OK:HEAP|58604|244680|76.0|24.0|53876|47092|PSRAM|0|0|0.0|0.0
```

## Debug Console Output

MEM 命令同時會輸出詳細訊息到序列埠：

```
--- Memory Status ---
[Heap] Free: 58604/244680 bytes (24.0% free, 76.0% used)
[Heap] Min Free: 53876, Max Alloc: 47092
[PSRAM] Not available

[Internal RAM]
Heap summary for capabilities 0x00000800:
  At 0x3ffaff10 len 240 free 8 allocated 4 min_free 8
    largest_free_block 0 alloc_blocks 1 free_blocks 1 total_blocks 2
  ...
```

## 警告標準

### 記憶體不足警告

- **Critical**: free_heap < 10KB
- **Warning**: free_heap < 20KB
- **Fragmentation**: largest_free_block << free_heap

### PSRAM 使用

- ESP32 標準版: 無 PSRAM (總是顯示 0)
- ESP32-S3: 通常有 2MB, 4MB, 或 8MB PSRAM
