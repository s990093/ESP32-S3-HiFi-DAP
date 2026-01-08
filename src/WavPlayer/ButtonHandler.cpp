/**
 * Button Handler Module - Implementation
 * Handles all button interrupts and button processing task
 */

#include "ButtonHandler.h"

// ========== ISR Functions ==========
void IRAM_ATTR isr_vol_up() {
  unsigned long now = millis();
  if (now - lastInterruptTime[0] > DEBOUNCE_MS) {
    btnPressed[0] = true;
    lastInterruptTime[0] = now;
  }
}

void IRAM_ATTR isr_vol_down() {
  unsigned long now = millis();
  if (now - lastInterruptTime[1] > DEBOUNCE_MS) {
    btnPressed[1] = true;
    lastInterruptTime[1] = now;
  }
}

void IRAM_ATTR isr_prev() {
  unsigned long now = millis();
  if (now - lastInterruptTime[2] > DEBOUNCE_MS) {
    btnPressed[2] = true;
    lastInterruptTime[2] = now;
  }
}

void IRAM_ATTR isr_next() {
  unsigned long now = millis();
  if (now - lastInterruptTime[3] > DEBOUNCE_MS) {
    btnPressed[3] = true;
    lastInterruptTime[3] = now;
  }
}

void IRAM_ATTR isr_pause() {
  unsigned long now = millis();
  if (now - lastInterruptTime[4] > DEBOUNCE_MS) {
    btnPressed[4] = true;
    lastInterruptTime[4] = now;
  }
}

// ========== NVS Persistence ==========
void savePlaybackState() {
  prefs.begin("wavplayer", false);
  prefs.putInt("track", currentTrack);
  prefs.putInt("volume", currentVolume);
  prefs.putBool("playing", playbackState == STATE_PLAYING);
  prefs.putInt("loopMode", (int)loopMode);
  
  // If track just changed, force position to 0 (don't save old track's position)
  if (trackChanged) {
    prefs.putUInt("position", 0);  // New track always starts from 0
  } else {
    prefs.putUInt("position", currentPosition);  // Save current position
  }
  prefs.end();
  
  DEBUG_PRINTLN("💾 State saved to NVS");
}

void loadPlaybackState() {
  prefs.begin("wavplayer", true);
  currentTrack = prefs.getInt("track", 0);
  currentVolume = prefs.getInt("volume", 30);
  bool wasPlaying = prefs.getBool("playing", false);
  currentPosition = prefs.getUInt("position", 0);  // Load playback position
  loopMode = (LoopMode)prefs.getInt("loopMode", LOOP_ALL);
  prefs.end();
  
  if (wasPlaying || currentPosition > 0) {
    Serial.println("🔄 Resuming from last session");
    Serial.printf("   Track: %d, Volume: %d%%, Position: %.1fs\n", 
                  currentTrack + 1, currentVolume, currentPosition / (44100.0 * 2 * 2));
  }
}

// ========== Button Handler Task (Core 0) ==========
void buttonHandlerTask(void* parameter) {
  Serial.printf("🎮 Button Task started on Core %d\n", xPortGetCoreID());
  
  static unsigned long lastPauseClick = 0;
  static unsigned long lastSave = 0;
  
  while (true) {
    // 1. Sync Wait: If PREV or NEXT is pressed, wait briefly to check for COMBO
    if (btnPressed[2] || btnPressed[3]) {
        vTaskDelay(50 / portTICK_PERIOD_MS); 
    }

    // Check simultaneous press (PREV + NEXT) for Loop Mode Toggle
    if (digitalRead(BTN_PREV) == HIGH && digitalRead(BTN_NEXT) == HIGH) {
       unsigned long comboStart = millis();
       bool toggled = false;
       
       // Wait to see if held
       while (digitalRead(BTN_PREV) == HIGH && digitalRead(BTN_NEXT) == HIGH) {
          if (!toggled && millis() - comboStart > 500) { // Hold for 0.5 sec
             // Trigger Toggle
             xSemaphoreTake(stateMutex, portMAX_DELAY);
             if (loopMode == LOOP_SINGLE) {
                loopMode = LOOP_ALL;
                Serial.println("🔁 Loop Mode: ALL (Sequence)");
             } else {
                loopMode = LOOP_SINGLE;
                Serial.println("🔂 Loop Mode: SINGLE (Repeat Track)");
             }
             savePlaybackState(); 
             xSemaphoreGive(stateMutex);
             
             toggled = true;
             
             // Clear individual button flags to prevent skip
             btnPressed[2] = false; 
             btnPressed[3] = false;
          }
          vTaskDelay(10 / portTICK_PERIOD_MS);
       }
       
       // If toggled, wait for release to avoid accidental clicks
       if (toggled) {
          while (digitalRead(BTN_PREV) == HIGH || digitalRead(BTN_NEXT) == HIGH) {
             vTaskDelay(10 / portTICK_PERIOD_MS);
          }
           // Clear again just in case interrupt fired on release
           btnPressed[2] = false; 
           btnPressed[3] = false;
           continue; 
       }
    }

    // Check long press (Volume)
    for (int i = 0; i < 2; i++) {
      if (digitalRead(i == 0 ? BTN_VOL_UP : BTN_VOL_DOWN) == HIGH) {
        if (millis() - lastInterruptTime[i] > LONG_PRESS_MS) {
          btnLongPress[i] = true;
          xSemaphoreTake(stateMutex, portMAX_DELAY);
          if (i == 0) {
            currentVolume = min(100, currentVolume + 1);
          } else {
            currentVolume = max(0, currentVolume - 1);
          }
          xSemaphoreGive(stateMutex);
          vTaskDelay(50 / portTICK_PERIOD_MS);
          continue;
        }
      } else if (btnLongPress[i]) {
        btnLongPress[i] = false;
        Serial.printf("🔊 Volume: %d%%\n", currentVolume);
        savePlaybackState();
      }
    }
    
    // Check button flags
    for (int i = 0; i < 5; i++) {
      if (btnPressed[i]) {
        btnPressed[i] = false;
        
        if (i < 2 && btnLongPress[i]) continue;
        
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        
        switch (i) {
          case 0: // VOL+
            currentVolume = min(100, currentVolume + 5);
            Serial.printf("🔊 Volume: %d%%\n", currentVolume);
            EVENT_LOG("BTN: VOL+");
            savePlaybackState();
            break;
            
          case 1: // VOL-
            currentVolume = max(0, currentVolume - 5);
            Serial.printf("🔉 Volume: %d%%\n", currentVolume);
            EVENT_LOG("BTN: VOL-");
            savePlaybackState();
            break;
            
          case 2: // PREV
            currentTrack = (currentTrack - 1 + playlistSize) % playlistSize;
            trackChanged = true;
            currentPosition = 0;  // Reset position for new track
            Serial.printf("⏮️  Track: %d/%d\n", currentTrack + 1, playlistSize);
            EVENT_LOG("BTN: PREV");
            break;
            
          case 3: // NEXT
            currentTrack = (currentTrack + 1) % playlistSize;
            trackChanged = true;
            currentPosition = 0;  // Reset position for new track
            Serial.printf("⏭️  Track: %d/%d\n", currentTrack + 1, playlistSize);
            EVENT_LOG("BTN: NEXT");
            break;
            
          case 4: // PAUSE
            if (millis() - lastPauseClick < DOUBLE_CLICK_MS) {
              currentTrack = (currentTrack + 1) % playlistSize;
              trackChanged = true;
              currentPosition = 0;  // Reset position for new track
              Serial.printf("⏭️⏭️  Double-click: Next %d/%d\n", currentTrack + 1, playlistSize);
              EVENT_LOG("BTN: DOUBLE-CLICK");
            } else {
              playbackState = (playbackState == STATE_PLAYING) ? STATE_PAUSED : STATE_PLAYING;
              Serial.printf("%s\n", playbackState == STATE_PLAYING ? "▶️  Playing" : "⏸️  Paused");
              EVENT_LOG(playbackState == STATE_PLAYING ? "BTN: PLAY" : "BTN: PAUSE");
              savePlaybackState();
            }
            lastPauseClick = millis();
            break;
        }
        
        xSemaphoreGive(stateMutex);
      }
    }
    
    // Auto-save every 10 seconds (for position tracking)
    if (millis() - lastSave > 10000) {
      savePlaybackState();
      lastSave = millis();
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
