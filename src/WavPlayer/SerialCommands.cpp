/**
 * Serial Commands Module - Implementation
 * Handles all serial command processing and file upload
 */

#include "SerialCommands.h"
#include "PlaylistManager.h"
#include "ButtonHandler.h"

// File upload constants
#define UPLOAD_BUF_SIZE 8192

// ========== Serial Command Handler ==========
void handleSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    // Extract command keyword (before first space) and lowercase ONLY that
    String cmdKeyword = cmd;
    int firstSpace = cmd.indexOf(' ');
    if (firstSpace > 0) {
        cmdKeyword = cmd.substring(0, firstSpace);
    }
    cmdKeyword.toLowerCase();
    
    if (cmdKeyword == "mem" || cmdKeyword == "memory") {
      uint32_t freeHeap = ESP.getFreeHeap();
      uint32_t heapSize = ESP.getHeapSize();
      uint32_t usedHeap = heapSize - freeHeap;
      float heapUsage = (float)usedHeap / heapSize * 100.0;
      
      uint32_t freePsram = ESP.getFreePsram();
      uint32_t psramSize = ESP.getPsramSize();
      uint32_t usedPsram = psramSize - freePsram;
      float psramUsage = psramSize > 0 ? (float)usedPsram / psramSize * 100.0 : 0.0;
      
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         Memory Status                  ║");
      Serial.println("╚════════════════════════════════════════╝");
      
      Serial.println("HEAP Memory:");
      Serial.printf("  Total:     %7d bytes\n", heapSize);
      Serial.printf("  Used:      %7d bytes (%.1f%%)\n", usedHeap, heapUsage);
      Serial.printf("  Free:      %7d bytes (%.1f%%)\n", freeHeap, 100.0 - heapUsage);
      Serial.printf("  Min Free:  %7d bytes\n", ESP.getMinFreeHeap());
      
      Serial.print("  Usage: [");
      int bars = (int)(heapUsage / 5);
      for (int i = 0; i < 20; i++) {
        if (i < bars) Serial.print("█");
        else Serial.print("░");
      }
      Serial.printf("] %.1f%%\n\n", heapUsage);
      
      if (psramSize > 0) {
        Serial.println("PSRAM Memory:");
        Serial.printf("  Total:     %7d bytes\n", psramSize);
        Serial.printf("  Used:      %7d bytes (%.1f%%)\n", usedPsram, psramUsage);
        Serial.printf("  Free:      %7d bytes (%.1f%%)\n", freePsram, 100.0 - psramUsage);
        
        Serial.print("  Usage: [");
        bars = (int)(psramUsage / 5);
        for (int i = 0; i < 20; i++) {
          if (i < bars) Serial.print("█");
          else Serial.print("░");
        }
        Serial.printf("] %.1f%%\n", psramUsage);
      } else {
        Serial.println("PSRAM: Not available");
      }
      Serial.println();
    }
    else if (cmdKeyword == "cpu" || cmdKeyword == "tasks") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         Task Status (FreeRTOS)         ║");
      Serial.println("╚════════════════════════════════════════╝");
      
      // Allocate buffer for task list (each task ~40 bytes)
      char *taskListBuffer = (char *)heap_caps_malloc(768, MALLOC_CAP_8BIT);
      
      if (taskListBuffer) {
        vTaskList(taskListBuffer);
        
        Serial.println("\nName          State   Prio    Stack   ID");
        Serial.println("──────────────────────────────────────────");
        Serial.print(taskListBuffer);
        Serial.println("──────────────────────────────────────────");
        
        Serial.println("\n📊 State Legend:");
        Serial.println("  X: Running   (目前正在執行)");
        Serial.println("  B: Blocked   (等待中/閒置 - CPU 有空)");
        Serial.println("  R: Ready     (準備執行)");
        Serial.println("  S: Suspended (暫停)");
        Serial.println("  D: Deleted   (刪除中)");
        
        Serial.println("\n⚠️  Stack: 剩餘記憶體 (bytes)");
        Serial.println("  • <100  = 危險！可能 Stack Overflow");
        Serial.println("  • >500  = 安全");
        Serial.println("  • >2000 = 分配太多，可減少\n");
        
        free(taskListBuffer);
      } else {
        Serial.println("❌ Failed to allocate memory for task list");
      }
      Serial.println();
    }
    else if (cmdKeyword == "status" || cmdKeyword == "s") {
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         Player Status                  ║");
      Serial.println("╚════════════════════════════════════════╝");
      Serial.printf("State:   %s\n", playbackState == STATE_PLAYING ? "▶️  Playing" : 
                                    playbackState == STATE_PAUSED ? "⏸️  Paused" : "⏹️  Stopped");
      Serial.printf("Track:   %d/%d\n", currentTrack + 1, playlistSize);
      if (playlistSize > 0) {
        const char* formatStr = (currentFormat == FORMAT_WAV) ? "WAV" :
                                (currentFormat == FORMAT_MP3) ? "MP3" : "???";
        Serial.printf("File:    %s (%s)\n", playlist[currentTrack], formatStr);
        
        // Show next song based on loop mode
        if (loopMode == LOOP_SINGLE) {
          Serial.printf("Next:    🔁 %s (Loop)\n", playlist[currentTrack]);
        } else if (loopMode == LOOP_ALL) {
          int nextTrack = (currentTrack + 1) % playlistSize;
          Serial.printf("Next:    %s\n", playlist[nextTrack]);
        } else {
          // LOOP_NONE
          if (currentTrack + 1 < playlistSize) {
            Serial.printf("Next:    %s\n", playlist[currentTrack + 1]);
          } else {
            Serial.printf("Next:    ⏹️  (End of playlist)\n");
          }
        }
      }
      Serial.printf("Volume:  %d%%\n", currentVolume);
      const char* loopStr = (loopMode == LOOP_NONE) ? "Off" :
                            (loopMode == LOOP_SINGLE) ? "Single" : "All";
      Serial.printf("Loop:    %s\n", loopStr);
      Serial.printf("Uptime:  %lu sec\n\n", millis() / 1000);
      xSemaphoreGive(stateMutex);
    }
    else if (cmdKeyword == "loop") {
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      // Cycle through loop modes
      loopMode = (LoopMode)((loopMode + 1) % 3);
      const char* modeStr = (loopMode == LOOP_NONE) ? "Off (Stop at end)" :
                            (loopMode == LOOP_SINGLE) ? "Single (Repeat track)" :
                            "All (Loop playlist)";
      Serial.printf("🔁 Loop Mode: %s\n", modeStr);
      xSemaphoreGive(stateMutex);
      savePlaybackState();
    }
    else if (cmdKeyword == "resume") {
      loadPlaybackState();
      Serial.println("✅ Playback state restored");
    }
    else if (cmdKeyword == "save") {
      savePlaybackState();
      Serial.println("✅ Playback state saved");
    }
    else if (cmdKeyword == "clear" || cmdKeyword == "reset") {
      prefs.begin("wavplayer", false);
      prefs.clear();  // Clear all keys
      prefs.end();
      currentPosition = 0;
      Serial.println("🗑️  NVS cleared - all saved state deleted");
    }
    else if (cmdKeyword == "tree" || cmdKeyword == "ls") {
      Serial.println("\n📁 SD Card Structure:");
      Serial.println("═══════════════════════════════════════");
      
      File root = SD.open("/");
      if (!root) {
        Serial.println("❌ Failed to open root directory");
      } else {
        int fileCount = 0;
        int dirCount = 0;
        uint32_t totalSize = 0;
        
        File file = root.openNextFile();
        while (file) {
          if (file.isDirectory()) {
            Serial.printf("📂 %-30s <DIR>\n", file.name());
            dirCount++;
          } else {
            uint32_t size = file.size();
            totalSize += size;
            
            // Format size
            char sizeStr[16];
            if (size < 1024) {
              snprintf(sizeStr, sizeof(sizeStr), "%lu B", size);
            } else if (size < 1024*1024) {
              snprintf(sizeStr, sizeof(sizeStr), "%.1f KB", size / 1024.0);
            } else {
              snprintf(sizeStr, sizeof(sizeStr), "%.2f MB", size / (1024.0*1024.0));
            }
            
            // Check if hidden
            String filename = String(file.name());
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) {
              filename = filename.substring(lastSlash + 1);
            }
            
            if (filename.startsWith("._")) {
              Serial.printf("🔒 %-30s %10s (hidden)\n", file.name(), sizeStr);
            } else {
              Serial.printf("📄 %-30s %10s\n", file.name(), sizeStr);
            }
            fileCount++;
          }
          file = root.openNextFile();
        }
        
        Serial.println("═══════════════════════════════════════");
        Serial.printf("Total: %d files, %d dirs", fileCount, dirCount);
        if (totalSize < 1024*1024) {
          Serial.printf(", %.1f KB\n\n", totalSize / 1024.0);
        } else {
          Serial.printf(", %.2f MB\n\n", totalSize / (1024.0*1024.0));
        }
      }
    }
    else if (cmd.startsWith("cat ")) {
      String filename = cmd.substring(4);
      filename.trim();
      
      // Add leading slash if missing
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      
      Serial.printf("\n📄 File: %s\n", filename.c_str());
      Serial.println("═══════════════════════════════════════");
      
      File file = SD.open(filename.c_str(), FILE_READ);
      if (!file) {
        Serial.println("❌ File not found");
      } else {
        uint32_t fileSize = file.size();
        Serial.printf("Size: %lu bytes\n\n", fileSize);
        
        // Check if it's a WAV file
        if (filename.endsWith(".wav") || filename.endsWith(".WAV")) {
           Serial.println("Type: WAV Audio");
        } else if (filename.endsWith(".mp3") || filename.endsWith(".MP3")) {
           Serial.println("Type: MP3 Audio");
        }
 else {
          // Show text file content (first 1KB)
          int bytesToRead = min((int)fileSize, 1024);
          uint8_t* buffer = (uint8_t*)malloc(bytesToRead + 1);
          if (buffer) {
            int bytesRead = file.read(buffer, bytesToRead);
            buffer[bytesRead] = 0;
            Serial.write(buffer, bytesRead);
            if (fileSize > 1024) {
              Serial.printf("\n\n... (showing first 1KB of %lu bytes)\n", fileSize);
            }
            free(buffer);
          }
        }
        
        file.close();
        Serial.println();
      }
    }
    else if (cmdKeyword == "nvs" || cmdKeyword == "read") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         NVS Storage (Flash)            ║");
      Serial.println("╚════════════════════════════════════════╝");
      
      prefs.begin("wavplayer", true);  // Read-only
      
      Serial.println("\n💾 Stored Preferences:");
      int storedTrack = prefs.getInt("track", -1);
      int storedVolume = prefs.getInt("volume", -1);
      bool storedPlaying = prefs.getBool("playing", false);
      
      if (storedTrack == -1) {
        Serial.println("  ⚠️  No saved state found");
      } else {
        Serial.printf("  Track Index:   %d\n", storedTrack);
        if (storedTrack < playlistSize && playlistSize > 0) {
          Serial.printf("  Track File:    %s\n", playlist[storedTrack]);
        }
        Serial.printf("  Volume:        %d%%\n", storedVolume);
        Serial.printf("  Was Playing:   %s\n", storedPlaying ? "Yes" : "No");
      }
      
      prefs.end();
      
      Serial.println("\n📋 Current Runtime State:");
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      Serial.printf("  Track Index:   %d\n", currentTrack);
      if (playlistSize > 0 && currentTrack < playlistSize) {
        Serial.printf("  Track File:    %s\n", playlist[currentTrack]);
      }
      Serial.printf("  Volume:        %d%%\n", currentVolume);
      Serial.printf("  State:         %s\n", 
                    playbackState == STATE_PLAYING ? "Playing" : 
                    playbackState == STATE_PAUSED ? "Paused" : "Stopped");
      xSemaphoreGive(stateMutex);
      
      Serial.println("\n⚙️  NVS Operations:");
      Serial.println("  Auto-save triggers:");
      Serial.println("    - Track change");
      Serial.println("    - Volume change");
      Serial.println("    - Pause/Play toggle");
      Serial.println("    - Every 30 seconds (background)");
      Serial.println("  Manual commands:");
      Serial.println("    - 'save'   - Force save current state");
      Serial.println("    - 'resume' - Reload saved state\n");
    }
    else if (cmdKeyword == "settings" || cmdKeyword == "config") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         System Settings                ║");
      Serial.println("╚════════════════════════════════════════╝");
      
      Serial.println("\n📟 Hardware Configuration:");
      Serial.printf("  I2S BCK:       GPIO %d\n", I2S_BCK);
      Serial.printf("  I2S WS:        GPIO %d\n", I2S_WS);
      Serial.printf("  I2S DATA:      GPIO %d\n", I2S_DATA);
      Serial.printf("  SD MISO:       GPIO %d\n", SD_MISO);
      Serial.printf("  SD MOSI:       GPIO %d\n", SD_MOSI);
      Serial.printf("  SD SCK:        GPIO %d\n", SD_SCK);
      Serial.printf("  SD CS:         GPIO %d\n", SD_CS);
      
      Serial.println("\n🎮 Button Mapping:");
      Serial.printf("  VOL+:          GPIO %d\n", BTN_VOL_UP);
      Serial.printf("  VOL-:          GPIO %d\n", BTN_VOL_DOWN);
      Serial.printf("  PREV:          GPIO %d\n", BTN_PREV);
      Serial.printf("  NEXT:          GPIO %d\n", BTN_NEXT);
      Serial.printf("  PAUSE:         GPIO %d\n", BTN_PAUSE);
      
      Serial.println("\n🔧 System Parameters:");
      Serial.printf("  Buffer Size:   %d bytes\n", BUFFER_SIZE);
      Serial.printf("  Max Tracks:    %d\n", MAX_TRACKS);
      Serial.printf("  Sample Rate:   44100 Hz\n");
      Serial.printf("  Bit Depth:     16-bit\n");
      Serial.printf("  Channels:      Stereo (2)\n");
      Serial.printf("  APLL:          Enabled\n");
      Serial.printf("  SPI Speed:     20 MHz\n");
      Serial.printf("  DMA Buffers:   8 x 1024\n");
      
      Serial.println("\n⏱️  Timing Settings:");
      Serial.printf("  Debounce:      %d ms\n", DEBOUNCE_MS);
      Serial.printf("  Long Press:    %d ms\n", LONG_PRESS_MS);
      Serial.printf("  Double Click:  %d ms\n", DOUBLE_CLICK_MS);
      Serial.printf("  Fade Samples:  %d (~%.1f ms)\n", FADE_SAMPLES, FADE_SAMPLES / 44.1);
      
      Serial.println("\n🎵 Audio Features:");
      Serial.println("  ✓ Chunk-based WAV parsing");
      Serial.println("  ✓ Logarithmic volume curve");
      Serial.println("  ✓ Fade in/out transitions");
      Serial.println("  ✓ DMA buffer flush (anti-pop)");
      Serial.println("  ✓ NVS playback resume");
      Serial.println("  ✓ Hidden file filtering\n");
    }
    else if (cmdKeyword == "help" || cmdKeyword == "h" || cmdKeyword == "?") {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║         Available Commands             ║");
      Serial.println("╚════════════════════════════════════════╝");
      Serial.println("  mem, memory  - Show memory status");
      Serial.println("  status, s    - Show player status");
      Serial.println("  settings     - Show system configuration");
      Serial.println("  cpu, tasks   - Show FreeRTOS task status");
      Serial.println("  nvs, read    - Show NVS stored state");
      Serial.println("  tree, ls     - List SD card files");
      Serial.println("  cat <file>   - Show file info/content");
      Serial.println("  save         - Save playback state");
      Serial.println("  clear        - Clear NVS saved state");
      Serial.println("  resume       - Restore playback state");
      Serial.println("  bitdepth <n> - Set I2S bit depth (16/24/32)");
      Serial.println("  help, h, ?   - Show this help\n");
    }


    
    // ========== FIRMWARE JSON API (Phase 1) ==========
    else if (cmdKeyword == "info_json") {
       Serial.println("{\"device\":\"ESP32-S3-HiFi-DAP\",\"version\":\"3.2.0\",\"api\":1}");
    }
    else if (cmdKeyword == "sys_json") {
       uint32_t freeHeap = ESP.getFreeHeap();
       uint32_t heapSize = ESP.getHeapSize();
       uint32_t freePsram = ESP.getFreePsram();
       uint32_t psramSize = ESP.getPsramSize();
       
       Serial.printf("{\"heap_free\":%u,\"heap_total\":%u,\"psram_free\":%u,\"psram_total\":%u,\"uptime\":%lu}\n",
                     freeHeap, heapSize, freePsram, psramSize, millis() / 1000);
    }
    else if (cmdKeyword == "tasks_json") {
       // --- Delta CPU Calculation Globals (Static) ---
       static unsigned long prevTotalRunTime = 0;
       static unsigned long prevTaskRunTimes[16] = {0}; // Max 16 tasks for tracking
       static TaskHandle_t  prevTaskHandles[16] = {NULL};
       
       unsigned long ulTotalRunTime;
       UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
       TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

       if (pxTaskStatusArray != NULL) {
          // Get current snapshot
          uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
          
          // Calculate Total Delta
          unsigned long totalDelta = ulTotalRunTime - prevTotalRunTime;
          // Avoid div by zero or initial spike
          if (totalDelta == 0) totalDelta = 1; 
          
          prevTotalRunTime = ulTotalRunTime; // Update for next time

          // Sort or just map? We need to match tasks to their previous state to calc delta.
          // Simple approach: Linear search in prev array by TaskHandle.
          
          Serial.print("{\"tasks\":[");
          for (UBaseType_t i = 0; i < uxArraySize; i++) {
             if (i > 0) Serial.print(",");
             
             // --- Delta Logic ---
             unsigned long taskCurrentTime = pxTaskStatusArray[i].ulRunTimeCounter;
             unsigned long taskDelta = taskCurrentTime; // Default to absolute if new
             
             // Find previous record for this specific task
             for(int k=0; k<16; k++) {
                 if (prevTaskHandles[k] == pxTaskStatusArray[i].xHandle) {
                     if (taskCurrentTime >= prevTaskRunTimes[k]) {
                        taskDelta = taskCurrentTime - prevTaskRunTimes[k];
                     }
                     break; 
                 }
             }
             
             // Update history (dumb slot filling)
             bool found = false;
             for(int k=0; k<16; k++) {
                 if (prevTaskHandles[k] == pxTaskStatusArray[i].xHandle) {
                     prevTaskRunTimes[k] = taskCurrentTime;
                     found = true;
                     break;
                 }
             }
             if (!found) {
                 for(int k=0; k<16; k++) {
                     if (prevTaskHandles[k] == NULL) {
                         prevTaskHandles[k] = pxTaskStatusArray[i].xHandle;
                         prevTaskRunTimes[k] = taskCurrentTime;
                         break;
                     }
                 }
             }

             // Calc %
             float cpu = (float)taskDelta / (float)totalDelta * 100.0;
             if (cpu > 100.0) cpu = 0.0; // Overflow sanity
             
             // Map State
             char state;
             switch (pxTaskStatusArray[i].eCurrentState) {
                 case eRunning:   state = 'X'; break;
                 case eReady:     state = 'R'; break;
                 case eBlocked:   state = 'B'; break;
                 case eSuspended: state = 'S'; break;
                 case eDeleted:   state = 'D'; break;
                 default:         state = '?'; break;
             }

             // Handle ESP32 Core ID
             int core = pxTaskStatusArray[i].xCoreID;
             if (core > 1) core = -1; // Any core

             Serial.printf("{\"name\":\"%s\",\"state\":\"%c\",\"prio\":%u,\"stack\":%u,\"id\":%u,\"cpu\":%.1f,\"core\":%d}",
                pxTaskStatusArray[i].pcTaskName,
                state,
                pxTaskStatusArray[i].uxCurrentPriority,
                pxTaskStatusArray[i].usStackHighWaterMark,
                pxTaskStatusArray[i].xTaskNumber,
                cpu,
                core
             );
          }
          Serial.println("]}");
          vPortFree(pxTaskStatusArray);
       } else {
          Serial.println("{\"error\":\"malloc_failed\"}");
       }
    }
    else if (cmdKeyword == "config_json") {
       Serial.printf("{\"buffer_size\":%d,\"sample_rate\":44100,\"fade_samples\":%d,\"max_tracks\":%d}\n",
                     BUFFER_SIZE, FADE_SAMPLES, MAX_TRACKS);
    }
    else if (cmdKeyword == "status_json") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       const char* stateStr = (playbackState == STATE_PLAYING) ? "playing" : 
                              (playbackState == STATE_PAUSED) ? "paused" : "stopped";
       const char* loopStr = (loopMode == LOOP_NONE) ? "off" :
                             (loopMode == LOOP_SINGLE) ? "single" : "all";
       
       // Escape filename manually if needed (simple quote check)
       // For now assuming clean filenames
       Serial.printf("{\"state\":\"%s\",\"track_index\":%d,\"track_total\":%d,\"volume\":%d,\"loop\":\"%s\",\"file\":\"%s\",\"position\":%lu}\n",
                     stateStr, currentTrack, playlistSize, currentVolume, loopStr, 
                     (playlistSize > 0) ? playlist[currentTrack] : "", currentPosition);
       xSemaphoreGive(stateMutex);
    }
    else if (cmdKeyword == "list_json") {
       Serial.print("{\"files\":[");
       File root = SD.open("/");
       if (root) {
         File file = root.openNextFile();
         bool first = true;
         while (file) {
           if (!file.isDirectory()) {
             String fname = String(file.name());
             if (!fname.startsWith("._") && !fname.startsWith("/._")) { // Filter hidden
                 if (!first) Serial.print(",");
                 Serial.printf("{\"name\":\"%s\",\"size\":%lu}", file.name(), file.size());
                 first = false;
             }
           }
           file = root.openNextFile();
         }
       }
       Serial.println("]}");
    }
    else if (cmd.startsWith("delete ")) {
       String path = cmd.substring(7);
       if (!path.startsWith("/")) path = "/" + path;
       
       if (SD.exists(path)) {
         if (SD.remove(path)) Serial.println("SUCCESS");
         else Serial.println("ERROR: Delete failed");
       } else {
         Serial.println("ERROR: File not found");
       }
    }
    
    // ========== PLAYBACK CONTROL COMMANDS ==========
    else if (cmd.startsWith("play ")) {
       String filename = cmd.substring(5);
       if (!filename.startsWith("/")) filename = "/" + filename;
       
       Serial.printf("DEBUG: Play command received\n");
       Serial.printf("DEBUG: Filename: '%s'\n", filename.c_str());
       Serial.printf("DEBUG: Playlist size: %d\n", playlistSize);
       
       // Find the file in playlist
       bool found = false;
       for (int i = 0; i < playlistSize; i++) {
           Serial.printf("DEBUG: Comparing with playlist[%d]: '%s'\n", i, playlist[i]);
           if (String(playlist[i]) == filename) {
               xSemaphoreTake(stateMutex, portMAX_DELAY);
               currentTrack = i;
               trackChanged = true;
               currentPosition = 0;
               playbackState = STATE_PLAYING;
               xSemaphoreGive(stateMutex);
               found = true;
               Serial.printf("✅ Match found! Playing track %d\n", i);
               break;
           }
       }
       
       if (!found) {
           Serial.println("ERROR: File not in playlist");
       }
    }
    else if (cmdKeyword == "pause") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       playbackState = STATE_PAUSED;
       xSemaphoreGive(stateMutex);
       Serial.println("⏸️ Paused");
    }
    else if (cmd == "resume") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       playbackState = STATE_PLAYING;
       xSemaphoreGive(stateMutex);
       Serial.println("▶️ Resumed");
    }
    else if (cmdKeyword == "next") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       currentTrack = (currentTrack + 1) % playlistSize;
       trackChanged = true;
       currentPosition = 0;
       xSemaphoreGive(stateMutex);
       Serial.println("⏭️ Next Track");
    }
    else if (cmdKeyword == "prev") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       currentTrack = (currentTrack - 1 + playlistSize) % playlistSize;
       trackChanged = true;
       currentPosition = 0;
       xSemaphoreGive(stateMutex);
       Serial.println("⏮️ Previous Track");
    }
    else if (cmdKeyword == "loop") {
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       // Cycle: ALL -> SINGLE -> NONE
       loopMode = (LoopMode)((loopMode + 1) % 3);
       xSemaphoreGive(stateMutex);
       
       const char* modeStr = (loopMode == LOOP_NONE) ? "Off" :
                             (loopMode == LOOP_SINGLE) ? "Single" : "All";
       Serial.printf("🔁 Loop Mode: %s\n", modeStr);
       savePlaybackState();
    }
    
     else if (cmd.startsWith("volume ")) {
       String volStr = cmd.substring(7);
       int newVol = volStr.toInt();
       
       // Clamp 0-100
       newVol = max(0, min(100, newVol));
       
       xSemaphoreTake(stateMutex, portMAX_DELAY);
       currentVolume = newVol;
       xSemaphoreGive(stateMutex);
       
       Serial.printf("🔊 Volume set to %d%%\n", currentVolume);
       savePlaybackState();
     }
     else if (cmdKeyword == "storage_json") {
       uint64_t totalBytes = SD.totalBytes();
       uint64_t usedBytes = SD.usedBytes();
       uint64_t freeBytes = totalBytes - usedBytes;
       
       Serial.printf("{\"total\":%llu,\"used\":%llu,\"free\":%llu}\n", totalBytes, usedBytes, freeBytes);
     }
     
     // ========== FILE MANAGEMENT COMMANDS ==========
     else if (cmd.startsWith("rename ")) {
       int firstSpace = cmd.indexOf(' ');
       int secondSpace = cmd.indexOf(' ', firstSpace + 1);
       
       if (firstSpace > 0 && secondSpace > firstSpace) {
         String oldName = cmd.substring(firstSpace + 1, secondSpace);
         String newName = cmd.substring(secondSpace + 1);
         if (!oldName.startsWith("/")) oldName = "/" + oldName;
         if (!newName.startsWith("/")) newName = "/" + newName;
         
         if (SD.exists(oldName)) {
           if (SD.rename(oldName, newName)) Serial.println("SUCCESS");
           else Serial.println("ERROR: Rename failed");
         } else {
           Serial.println("ERROR: File not found");
         }
       } else {
         Serial.println("ERROR: Usage rename <old> <new>");
       }
     }
    
    // Old commands continue...
    else if (cmdKeyword == "ping") {
      Serial.println("pong");
    }
    else if (cmdKeyword == "test_write") {
       Serial.println("Creating /test_serial.txt...");
       // Delete if exists
       if (SD.exists("/test_serial.txt")) {
         SD.remove("/test_serial.txt");
       }
       
       File f = SD.open("/test_serial.txt", FILE_WRITE);
       if (f) {
         f.print("UART Test");
         f.close();
         Serial.println("Writing \"UART Test\"...");
         
         // Verify
         f = SD.open("/test_serial.txt", FILE_READ);
         if (f) {
           String content = f.readString();
           f.close();
           Serial.printf("Reading back: \"%s\"\n", content.c_str());
           if (content == "UART Test") {
             Serial.println("✅ SD Write Access OK");
           } else {
             Serial.println("❌ Content mismatch!");
           }
         } else {
           Serial.println("❌ Failed to open for reading");
         }
       } else {
         Serial.println("❌ Failed to open for writing");
       }
    }


    // ========== BIT DEPTH CONTROL COMMAND ==========
    else if (cmd.startsWith("bitdepth ")) {
        String depthStr = cmd.substring(9);
        int depth = depthStr.toInt();
        
        if (depth == 16 || depth == 24 || depth == 32) {
            BitDepth newDepth = (BitDepth)depth;
            if (audioOut) {
                audioOut->setBitDepth(newDepth);
                Serial.printf("✅ I2S Bit Depth set to: %d-bit\n", depth);
            } else {
                Serial.println("❌ Audio output not initialized");
            }
        } else {
            Serial.println("❌ Invalid bit depth. Use: 16, 24, or 32");
        }
    }
    // Command: upload <filename> <size>
    else if (cmd.startsWith("upload ")) {
       int firstSpace = cmd.indexOf(' ');
       int secondSpace = cmd.lastIndexOf(' ');
       
       if (firstSpace > 0 && secondSpace > firstSpace) {
           String filename = cmd.substring(firstSpace + 1, secondSpace);
           String sizeStr = cmd.substring(secondSpace + 1);
           size_t size = sizeStr.toInt();
           
           if (!filename.startsWith("/")) filename = "/" + filename;
           
           Serial.printf("Preparing upload: %s (%d bytes)\n", filename.c_str(), size);
           
           // Clean up old file
           if (SD.exists(filename)) SD.remove(filename);
           
           uploadFile = SD.open(filename, FILE_WRITE);
           if (uploadFile) {
               isReceivingFile = true;
               uploadRemaining = size;
               lastUploadActivity = millis();
               Serial.println("READY"); // Signal to script to start sending
           } else {
               Serial.println("ERROR: Create file failed");
           }
       } else {
           Serial.println("ERROR: Usage upload <file> <size>");
       }
    }
    else if (cmd.length() > 0) {
      Serial.printf("❌ Unknown command: '%s'\n", cmd.c_str());
      Serial.println("Type 'help' for available commands\n");
    }
  }
}

// =========================================================
// 🚀 FILE UPLOAD HANDLER (Optimized for Speed)
// =========================================================

void handleFileUpload() {
  if (isReceivingFile && uploadFile) {
    // Check for timeout (30 seconds)
    if (millis() - lastUploadActivity > 30000) {
      Serial.println("\nERROR: Upload timeout");
      uploadFile.close();
      isReceivingFile = false;
      return;
    }
    
    // Read available data
    int available = Serial.available();
    if (available > 0) {
      lastUploadActivity = millis();
      
      // Use large buffer for speed
      uint8_t buf[UPLOAD_BUF_SIZE];
      int toRead = min(available, (int)min((size_t)UPLOAD_BUF_SIZE, uploadRemaining));
      int bytesRead = Serial.readBytes(buf, toRead);
      
      if (bytesRead > 0) {
        uploadFile.write(buf, bytesRead);
        uploadRemaining -= bytesRead;
        
        // Progress feedback every 64KB
        static size_t lastReport = 0;
        size_t written = uploadFile.size();
        if (written - lastReport >= 65536) {
          Serial.printf("Progress: %d bytes\n", written);
          lastReport = written;
        }
        
        // Check if complete
        if (uploadRemaining == 0) {
          uploadFile.close();
          isReceivingFile = false;
          Serial.println("SUCCESS");
          
          // Rescan playlist if it's an audio file
          String fname = String(uploadFile.name());
          if (fname.endsWith(".wav") || fname.endsWith(".WAV") ||
              fname.endsWith(".mp3") || fname.endsWith(".MP3")) {
            scanPlaylist();
          }
        }
      }
    }
  }
}
