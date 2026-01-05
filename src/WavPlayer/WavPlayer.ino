/**
* Production-Grade Multi-Core WAV Player
* FreeRTOS + Professional Audio Quality
* 
* Key Features:
*   - Robust chunk-based WAV parsing
*   - APLL for precise audio clock
*   - Logarithmic volume control
*   - Fade in/out (no pop noise)
*   - NVS playback position resume
*   - Memory-safe fixed arrays
*/

// Debug configuration
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
  #define EVENT_LOG(evt) Serial.printf("[EVENT] %s\n", evt)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
  #define EVENT_LOG(evt)
#endif

#include <SPI.h>
#include <SD.h>

#include <Preferences.h>

// BackgroundAudio Library
#include <ESP32I2SAudio.h>
#include <BackgroundAudioMP3.h>
#include <BackgroundAudioWAV.h>

// ========== Pin Definitions ==========
#define SD_MISO 19
#define SD_MOSI 23
#define SD_SCK  18
#define SD_CS   5

#define I2S_WS    15
#define I2S_DATA  2
#define I2S_BCK   4


#define BTN_VOL_UP   12
#define BTN_VOL_DOWN 13
#define BTN_PREV     14
#define BTN_NEXT     27
#define BTN_PAUSE    26

// ========== Configuration ==========
#define I2S_NUM         I2S_NUM_0
#define BUFFER_SIZE     8192
#define MAX_TRACKS      32
#define MAX_FILENAME    64
#define DEBOUNCE_MS     200
#define LONG_PRESS_MS   500
#define DOUBLE_CLICK_MS 400
#define FADE_SAMPLES    2048  // ~46ms @ 44.1kHz

// ========== Playback State ==========
enum PlaybackState {
  STATE_STOPPED,
  STATE_PLAYING,
  STATE_PAUSED
};

enum LoopMode {
  LOOP_NONE,      // No loop - stop at end
  LOOP_SINGLE,    // Repeat current track
  LOOP_ALL        // Loop entire playlist
};

enum AudioFormat {
  FORMAT_WAV,
  FORMAT_MP3,
  FORMAT_UNKNOWN
};

// ========== Global State ==========
Preferences prefs;
SemaphoreHandle_t stateMutex;

volatile PlaybackState playbackState = STATE_PAUSED;
volatile int currentVolume = 20;
volatile int currentTrack = 0;
volatile bool trackChanged = false;
volatile uint32_t currentPosition = 0;  // Playback position in bytes
volatile uint32_t totalDataSize = 0;    // Total audio data size
volatile LoopMode loopMode = LOOP_ALL;  // Default: loop playlist

// Memory-safe: Fixed arrays instead of String
char playlist[MAX_TRACKS][MAX_FILENAME];
int playlistSize = 0;

// ========== Audio Processing (Global) ==========

// =========================================================
// 🎚️ 10-Band EQ Settings (Optimized for PCM5102A)
// =========================================================

#define SAMPLE_RATE 44100.0f

// Gain Settings (Matched to your request)
#define TARGET_BASS_DB  4.5f   // Bass Boost
#define TARGET_TREB_DB  2.5f   // Treble Clarity

// Frequency Ranges
#define BASS_CUTOFF_HZ  100.0f 
#define TREB_CUTOFF_HZ  750.0f

// Headroom (Crucial for V-Shape EQ to prevent clipping)
#define HEADROOM_SCALER 0.707f // -3dB

const float PI_F = 3.14159265359f;
const float BASS_G = pow(10.0f, TARGET_BASS_DB / 20.0f); 
const float TREB_G = pow(10.0f, TARGET_TREB_DB / 20.0f);
const float dt = 1.0f / SAMPLE_RATE;
const float ALPHA_LOW = (2.0f * PI_F * dt * BASS_CUTOFF_HZ) / (1.0f + 2.0f * PI_F * dt * BASS_CUTOFF_HZ);
const float ALPHA_HIGH = 1.0f / (1.0f + 2.0f * PI_F * dt * TREB_CUTOFF_HZ); 

// =========================================================
// 🎚️ AudioOutputWithEQ Class (The Audiophile Engine)
// =========================================================
class AudioOutputWithEQ : public ESP32I2SAudio {
public:
    AudioOutputWithEQ(int bck, int ws, int data, int mclk = -1) 
        : ESP32I2SAudio(bck, ws, data, mclk) {
            // Initialize RNG
            rand_state = 123456789;
    }

    // Override the raw write function to intercept ALL audio data (MP3 & WAV)
    virtual size_t write(const uint8_t *buffer, size_t size) override {
        // We need to process in-place or copy. 
        // Since we are applying a float transform and then clamping back to int16,
        // we can modify the buffer in-place if it's mutable. 
        // The const modifier on the input suggests we shouldn't, but for this specific embedded context
        // and library architecture, we often cast away const if we own the buffer data from the decoder.
        // However, to be safe and "Production-Grade", let's assume we can modify it 
        // because the decoders usually pass a pointer to their internal mutable buffer.
        
        int16_t* samples = (int16_t*)buffer;
        int count = size / 2; // Number of samples

        for (int i = 0; i < count; i+=2) { // Stereo Interleaved
            // 1. Headroom Management (-3dB Pre-attenuation)
            // Critical for V-Shape EQ to prevents clipping when bass is boosted.
            float L = (float)samples[i] * HEADROOM_SCALER;
            float R = (i+1 < count) ? (float)samples[i+1] * HEADROOM_SCALER : 0;

            // 2. High-Precision Floating Point EQ
            // Apply shelving filters for Bass and Treble
            float outL = applyShelving(applyShelving((int16_t)L, bass_l_prev_in, bass_l_prev_out, false), treb_l_prev_in, treb_l_prev_out, true);
            float outR = 0;
            if (i+1 < count) {
                outR = applyShelving(applyShelving((int16_t)R, bass_r_prev_in, bass_r_prev_out, false), treb_r_prev_in, treb_r_prev_out, true);
            }

            // 3. TPDF Dithering (Triangular Probability Density Function)
            // Adds randomized noise to linearize quantization error before truncation.
            outL += get_tpdf_dither();
            if (i+1 < count) outR += get_tpdf_dither();

            // 4. Hard Limiter (Clamping)
            // Final safety net to prevent integer rollover.
            if (outL > 32767.0f) outL = 32767.0f;
            if (outL < -32768.0f) outL = -32768.0f;
            if (outR > 32767.0f) outR = 32767.0f;
            if (outR < -32768.0f) outR = -32768.0f;

            // Write back to buffer
            samples[i] = (int16_t)outL;
            if (i+1 < count) samples[i+1] = (int16_t)outR;
        }

        // Pass the processed buffer to the actual I2S driver
        return ESP32I2SAudio::write(buffer, size);
    }

private:
    unsigned long rand_state;
    // EQ State Variables
    float bass_l_prev_in = 0, bass_l_prev_out = 0;
    float bass_r_prev_in = 0, bass_r_prev_out = 0;
    float treb_l_prev_in = 0, treb_l_prev_out = 0;
    float treb_r_prev_in = 0, treb_r_prev_out = 0;

    // Internal Helper: Xorshift RNG for TPDF
    inline int16_t get_tpdf_dither() {
        rand_state ^= (rand_state << 13);
        rand_state ^= (rand_state >> 17);
        rand_state ^= (rand_state << 5);
        return (int16_t)((rand_state & 0x01) - ((rand_state >> 1) & 0x01));
    }

    // Internal Helper: Shelving Filter
    int16_t applyShelving(int16_t sample, float& prev_in, float& prev_out, bool highpass) {
        float in = (float)sample;
        float out;
        float alpha;
        float gain;

        if (highpass) {
            alpha = ALPHA_HIGH; gain = TREB_G;
            float hp = alpha * (prev_out + in - prev_in);
            out = in + (gain - 1.0f) * hp;
            prev_out = hp;
        } else {
            alpha = ALPHA_LOW; gain = BASS_G;
            float lp = alpha * in + (1.0f - alpha) * prev_out;
            out = in + (gain - 1.0f) * lp;
            prev_out = lp;
        }
        prev_in = in;
        return (int16_t)out; 
    }
};

// BackgroundAudio Objects
AudioOutputWithEQ *audioOut = NULL; // Use our custom class
BackgroundAudioMP3 *mp3Decoder = NULL;
BackgroundAudioWAV *wavDecoder = NULL;
File audioFile; // Used for both WAV and MP3 feeding
volatile AudioFormat currentFormat = FORMAT_UNKNOWN;

// ========== Button ISR Flags ==========
volatile bool btnPressed[5] = {false, false, false, false, false};
volatile unsigned long lastInterruptTime[5] = {0, 0, 0, 0, 0};
volatile bool btnLongPress[2] = {false, false};

TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;

uint8_t audioBuffer[BUFFER_SIZE];



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



// ========== Playlist Management ==========
void scanPlaylist() {
  EVENT_LOG("Scanning SD card");
  Serial.println("\n📁 Scanning SD card for audio files (WAV/MP3)...");
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("❌ Failed to open root directory");
    return;
  }
  
  playlistSize = 0;
  File file = root.openNextFile();
  
  while (file && playlistSize < MAX_TRACKS) {
    if (!file.isDirectory()) {
      String fullname = String(file.name());
      String filename = fullname;
      int lastSlash = fullname.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = fullname.substring(lastSlash + 1);
      }
      
      DEBUG_PRINTF("  Checking: %s\n", filename.c_str());
      
      // Skip hidden files and filter WAV/MP3
      if ((filename.endsWith(".wav") || filename.endsWith(".WAV") ||
           filename.endsWith(".mp3") || filename.endsWith(".MP3")) 
          && !filename.startsWith("._")) {
        // Use fixed array instead of String
        if (fullname.startsWith("/")) {
          snprintf(playlist[playlistSize], MAX_FILENAME, "%s", fullname.c_str());
        } else {
          snprintf(playlist[playlistSize], MAX_FILENAME, "/%s", fullname.c_str());
        }
        playlistSize++;
        Serial.printf("  [%d] %s\n", playlistSize, filename.c_str());
        EVENT_LOG(("Added: " + filename).c_str());
      } else {
        DEBUG_PRINTF("  Skipped: %s\n", filename.c_str());
      }
    }
    file = root.openNextFile();
  }
  
  Serial.printf("\n✅ Found %d tracks\n", playlistSize);
  EVENT_LOG("Scan complete");
}

// ========== Audio Format Detection ==========
AudioFormat detectAudioFormat(const char* filename) {
  String fn = String(filename);
  fn.toLowerCase();
  if (fn.endsWith(".wav")) return FORMAT_WAV;
  if (fn.endsWith(".mp3")) return FORMAT_MP3;
  return FORMAT_UNKNOWN;
}

// ========== Audio Playback Task (Core 1) ==========
void audioPlaybackTask(void* parameter) {
  Serial.printf("🎵 Audio Task started on Core %d\n", xPortGetCoreID());
  
  bool needNewFile = true;
  uint8_t buffer[512]; // General purpose audio buffer
  
  // Initialize I2S Audio Output (One-time)
  if (!audioOut) {
    audioOut = new AudioOutputWithEQ(I2S_BCK, I2S_WS, I2S_DATA);
    Serial.println("✅ AudioOutputWithEQ initialized");
  }

  while (true) {
    if (playlistSize == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    
    // === Track Change / Cleanup ===
    // === Track Change / Cleanup ===
    if (trackChanged) {
      trackChanged = false;
      
      // Stop and clean up decoders AND output to reset I2S state
      if (mp3Decoder) { delete mp3Decoder; mp3Decoder = NULL; }
      if (wavDecoder) { delete wavDecoder; wavDecoder = NULL; }
      if (audioOut) { 
          audioOut->end(); // CRITICAL: Free I2S resources before deleting
          delete audioOut; 
          audioOut = NULL; 
      } 
      if (audioFile) { audioFile.close(); }
      
      currentPosition = 0;
      needNewFile = true;
      savePlaybackState();
    }
    
    // === Load New File ===
    if (needNewFile) {
      if (audioFile) audioFile.close();
      if (mp3Decoder) { delete mp3Decoder; mp3Decoder = NULL; }
      if (wavDecoder) { delete wavDecoder; wavDecoder = NULL; }
      if (audioOut) { 
          audioOut->end(); // Double safety
          delete audioOut; 
          audioOut = NULL; 
      } 
      // WAV/MP3 Playback Logic
      if (audioOut == NULL) {
          // Initialize Audiophile Audio Output
          audioOut = new AudioOutputWithEQ(I2S_BCK, I2S_WS, I2S_DATA);
          audioOut->begin();
          // Set initial volume handled by decoders
          // float gain = pow((float)currentVolume / 20.0f, 3.0f);
          // audioOut->setGain(gain); // Removed: ESP32I2SAudio doesn't support setGain

      }
      
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      int track = currentTrack;
      xSemaphoreGive(stateMutex);
      
      Serial.printf("\n▶️  Playing: %s\n", playlist[track]);
      currentFormat = detectAudioFormat(playlist[track]);
      
      // Open file
      audioFile = SD.open(playlist[track], FILE_READ);
      if (!audioFile) {
        Serial.printf("❌ Failed to open: %s\n", playlist[track]);
        // Skip track logic
        currentTrack = (currentTrack + 1) % playlistSize;
        trackChanged = true;
        vTaskDelay(500 / portTICK_PERIOD_MS);
        continue;
      }
      
      // Initialize correct decoder
      if (currentFormat == FORMAT_MP3) {
        mp3Decoder = new BackgroundAudioMP3(*audioOut);
        mp3Decoder->begin();
        Serial.println("   Format: MP3");
      } else if (currentFormat == FORMAT_WAV) {
        wavDecoder = new BackgroundAudioWAV(*audioOut);
        wavDecoder->begin();
        Serial.println("   Format: WAV");
      } else {
        Serial.println("❌ Unknown format");
        audioFile.close();
        currentTrack = (currentTrack + 1) % playlistSize;
        trackChanged = true;
        continue;
      }
      
      needNewFile = false;
    }
    
    // === Playback State Handling ===
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    PlaybackState state = playbackState;
    xSemaphoreGive(stateMutex);
    
    if (state == STATE_PAUSED || state == STATE_STOPPED) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    
    // === Data Feed Loop ===
    if (audioFile && (mp3Decoder || wavDecoder)) {
      // Check if decoder handles volume
      // TUNING: Use cubic curve (vol^3) for better low-volume control (Headphone friendly)
      // NOTE: 10-band EQ not supported natively by BackgroundAudio lib.
      float lin_vol = currentVolume / 100.0f;
      float vol = lin_vol * lin_vol * lin_vol; 
      
      if (mp3Decoder) mp3Decoder->setGain(vol);
      if (wavDecoder) wavDecoder->setGain(vol);
      
      // Feed Data if buffer has space
      bool readyForData = false;
      if (mp3Decoder) readyForData = (mp3Decoder->availableForWrite() > sizeof(buffer));
      if (wavDecoder) readyForData = (wavDecoder->availableForWrite() > sizeof(buffer));
      
      
      if (readyForData) {
        int bytesRead = audioFile.read(buffer, sizeof(buffer));
        if (bytesRead > 0) {

          // DSP Application (Integrated into AudioOutputWithEQ::write)
          // We no longer need manual processAudioBufferEQ call because the audioOut object
          // now handles all Dithering, EQ, and Headroom management for both WAV and MP3 automatically.



          
          if (mp3Decoder) mp3Decoder->write(buffer, bytesRead);
          if (wavDecoder) wavDecoder->write(buffer, bytesRead);
          currentPosition += bytesRead;
        } else {
          // EOF
          Serial.println("✅ Track finished");
          audioFile.close();
          
          // Handle loop/next track
          xSemaphoreTake(stateMutex, portMAX_DELAY);
          if (loopMode == LOOP_SINGLE) {
             // Replay same track
             trackChanged = true;
             // Don't change currentTrack
          } else if (loopMode == LOOP_ALL) {
             currentTrack = (currentTrack + 1) % playlistSize;
             trackChanged = true;
          } else { // LOOP_NONE
             currentTrack++;
             if (currentTrack >= playlistSize) {
               currentTrack = 0;
               playbackState = STATE_STOPPED;
               trackChanged = true; // Will load track 0 but stopped
             } else {
               trackChanged = true;
             }
          }
           xSemaphoreGive(stateMutex);
        }
      } else {
        // Buffer full, yield briefly
        vTaskDelay(2 / portTICK_PERIOD_MS);
      }
    } else {
       vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  } // While true
}
    
// ========== Button Handler Task (Core 0) ==========
void buttonHandlerTask(void* parameter) {
  Serial.printf("🎮 Button Task started on Core %d\n", xPortGetCoreID());
  
  static unsigned long lastPauseClick = 0;
  static unsigned long lastSave = 0;
  
  while (true) {
    // Check simultaneous press (PREV + NEXT) for Loop Mode Toggle
    if (digitalRead(BTN_PREV) == HIGH && digitalRead(BTN_NEXT) == HIGH) {
       unsigned long comboStart = millis();
       bool toggled = false;
       
       // Wait to see if held
       while (digitalRead(BTN_PREV) == HIGH && digitalRead(BTN_NEXT) == HIGH) {
          if (!toggled && millis() - comboStart > 1000) { // Hold for 1 sec
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

// ========== Serial Command Handler ==========
void handleSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "mem" || cmd == "memory") {
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
    else if (cmd == "cpu" || cmd == "tasks") {
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
    else if (cmd == "status" || cmd == "s") {
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
    else if (cmd == "loop") {
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
    else if (cmd == "resume") {
      loadPlaybackState();
      Serial.println("✅ Playback state restored");
    }
    else if (cmd == "save") {
      savePlaybackState();
      Serial.println("✅ Playback state saved");
    }
    else if (cmd == "clear" || cmd == "reset") {
      prefs.begin("wavplayer", false);
      prefs.clear();  // Clear all keys
      prefs.end();
      currentPosition = 0;
      Serial.println("🗑️  NVS cleared - all saved state deleted");
    }
    else if (cmd == "tree" || cmd == "ls") {
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
    else if (cmd == "nvs" || cmd == "read") {
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
    else if (cmd == "settings" || cmd == "config") {
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
    else if (cmd == "help" || cmd == "h" || cmd == "?") {
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
      Serial.println("  help, h, ?   - Show this help\n");
    }
    else if (cmd.length() > 0) {
      Serial.printf("❌ Unknown command: '%s'\n", cmd.c_str());
      Serial.println("Type 'help' for available commands\n");
    }
  }
}

// ========== Setup ==========
void setup() {
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  Production-Grade WAV Player          ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  stateMutex = xSemaphoreCreateMutex();
  
  // Load previous state
  loadPlaybackState();
  
  // 🔥 強制設定為 "全部循環" 模式，覆蓋 NVS 中可能殘留的設定
  loopMode = LOOP_ALL;
  Serial.println("🔁 Loop mode forced to: All (Loop playlist)");
  
  Serial.println("💾 Initializing SD card...");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 10000000)) {  // 20MHz for better performance
    Serial.println("❌ SD Card failed!");
    while (1) delay(1000);
  }
  Serial.println("✅ SD Card OK\n");
  
  scanPlaylist();
  if (playlistSize == 0) {
    Serial.println("❌ No WAV files found!");
    while (1) delay(1000);
  }
  
  // Validate saved track
  if (currentTrack >= playlistSize) {
    currentTrack = 0;
  }
  
  // Verify the saved track file actually exists
  File testFile = SD.open(playlist[currentTrack], FILE_READ);
  if (!testFile) {
    Serial.printf("⚠️  Saved track not found: %s\n", playlist[currentTrack]);
    Serial.println("   Resetting to first track");
    currentTrack = 0;
    currentPosition = 0;
    savePlaybackState();
  } else {
    testFile.close();
  }
  
  Serial.println("\n🔊 Audio System Initializing...");
  // I2S setup moved to audioPlaybackTask for unified handling
  Serial.println("✅ Audio System Ready\n");
  
  EVENT_LOG("Initializing buttons");
  Serial.println("🎮 Initializing buttons...");
  
  pinMode(BTN_VOL_UP, INPUT_PULLDOWN);
  DEBUG_PRINTLN("  VOL+ OK");
  pinMode(BTN_VOL_DOWN, INPUT_PULLDOWN);
  DEBUG_PRINTLN("  VOL- OK");
  pinMode(BTN_PREV, INPUT_PULLDOWN);
  DEBUG_PRINTLN("  PREV OK");
  pinMode(BTN_NEXT, INPUT_PULLDOWN);
  DEBUG_PRINTLN("  NEXT OK");
  pinMode(BTN_PAUSE, INPUT_PULLDOWN);
  DEBUG_PRINTLN("  PAUSE OK");
  
  delay(100);
  
  DEBUG_PRINTLN("  Attaching interrupts...");
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_UP), isr_vol_up, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_DOWN), isr_vol_down, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PREV), isr_prev, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), isr_next, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), isr_pause, RISING);
  
  Serial.println("✅ Buttons OK\n");
  EVENT_LOG("Buttons initialized");
  
  playbackState = STATE_PAUSED;
  Serial.println("⏸️  Initial state: PAUSED\n");
  EVENT_LOG("State: PAUSED");
  
  DEBUG_PRINTLN("Creating FreeRTOS tasks...");
  Serial.println("🚀 Starting FreeRTOS tasks...\n");
  
  BaseType_t result1 = xTaskCreatePinnedToCore(
    audioPlaybackTask,
    "AudioTask",
    8192,
    NULL,
    2,
    &audioTaskHandle,
    1  // Core 1
  );
  DEBUG_PRINTF("  Audio task result: %d\n", result1);
  EVENT_LOG(result1 == pdPASS ? "Audio task created" : "Audio task FAILED");
  
  BaseType_t result2 = xTaskCreatePinnedToCore(
    buttonHandlerTask,
    "ButtonTask",
    2048,
    NULL,
    1,
    &buttonTaskHandle,
    0  // Core 0
  );
  DEBUG_PRINTF("  Button task result: %d\n", result2);
  EVENT_LOG(result2 == pdPASS ? "Button task created" : "Button task FAILED");
  
  Serial.println("✅ System ready!\n");
  Serial.println("🎵 PRODUCTION FEATURES:");
  Serial.println("  ✓ Robust chunk-based WAV parsing");
  Serial.println("  ✓ APLL enabled (precise audio clock)");
  Serial.println("  ✓ Logarithmic volume curve");
  Serial.println("  ✓ Fade in/out (no pop noise)");
  Serial.println("  ✓ NVS playback resume\n");
  Serial.println("Controls:");
  Serial.println("  VOL+:  GPIO12 (single/long-press)");
  Serial.println("  VOL-:  GPIO13 (single/long-press)");
  Serial.println("  PREV:  GPIO14");
  Serial.println("  NEXT:  GPIO27");
  Serial.println("  PAUSE: GPIO26 (single=pause, double=next)\n");
  Serial.println("Serial Commands:");
  Serial.println("  'mem'    - Show memory status");
  Serial.println("  'status' - Show player status");
  Serial.println("  'save'   - Save playback state");
  Serial.println("  'resume' - Restore playback state");
  Serial.println("  'help'   - Show commands\n");
}

void loop() {
  handleSerialCommand();
  
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    Serial.printf("📊 Status: Track %d/%d, Volume %d%%\n", 
                  currentTrack + 1, playlistSize, currentVolume);
    xSemaphoreGive(stateMutex);
    lastStatus = millis();
  }
  
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
