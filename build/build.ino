/**
 * Production-Grade Multi-Core WAV Player - Refactored
 * FreeRTOS + Professional Audio Quality
 * 
 * Key Features:
 *   - Modular architecture
 *   - Configurable bit depth (16/24/32-bit)
 *   - Robust chunk-based WAV parsing
 *   - APLL for precise audio clock
 *   - Logarithmic volume control
 *   - Fade in/out (no pop noise)
 *   - NVS playback position resume
 */

#include <SPI.h>
#include <SD.h>
#include <ESP32I2SAudio.h>
#include <BackgroundAudioMP3.h>
#include <BackgroundAudioWAV.h>

// Include all module headers
#include "Config.h"
#include "AudioProcessor.h"
#include "PlaylistManager.h"
#include "ButtonHandler.h"
#include "SerialCommands.h"

// ========== Global State Definitions ==========
Preferences prefs;
SemaphoreHandle_t stateMutex;

volatile PlaybackState playbackState = STATE_PAUSED;
volatile int currentVolume = 20;
volatile int currentTrack = 0;
volatile bool trackChanged = false;
volatile uint32_t currentPosition = 0;
volatile uint32_t totalDataSize = 0;
volatile LoopMode loopMode = LOOP_ALL;

char playlist[MAX_TRACKS][MAX_FILENAME];
int playlistSize = 0;

volatile bool isReceivingFile = false;
File uploadFile;
size_t uploadRemaining = 0;
unsigned long lastUploadActivity = 0;

volatile AudioFormat currentFormat = FORMAT_UNKNOWN;

volatile bool btnPressed[5] = {false, false, false, false, false};
volatile unsigned long lastInterruptTime[5] = {0, 0, 0, 0, 0};
volatile bool btnLongPress[2] = {false, false};

TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;

uint8_t audioBuffer[BUFFER_SIZE];

// BackgroundAudio Objects
BackgroundAudioMP3 *mp3Decoder = NULL;
BackgroundAudioWAV *wavDecoder = NULL;
File audioFile;

// ========== Audio Playback Task (Core 1) ==========
void audioPlaybackTask(void* parameter) {
  Serial.printf("🎵 Audio Task started on Core %d\n", xPortGetCoreID());
  
  bool needNewFile = true;
  uint8_t buffer[512];
  
  // Initialize I2S Audio Output
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
    if (trackChanged) {
      trackChanged = false;
      
      if (mp3Decoder) { delete mp3Decoder; mp3Decoder = NULL; }
      if (wavDecoder) { delete wavDecoder; wavDecoder = NULL; }
      if (audioOut) { 
          audioOut->end();
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
          audioOut->end();
          delete audioOut; 
          audioOut = NULL; 
      } 
      
      if (audioOut == NULL) {
          audioOut = new AudioOutputWithEQ(I2S_BCK, I2S_WS, I2S_DATA);
          audioOut->begin();
          
          float gain = pow((float)currentVolume / 20.0f, 3.0f);
          audioOut->updateLoudness(currentVolume); 
      }

      xSemaphoreTake(stateMutex, portMAX_DELAY);
      int track = currentTrack;
      xSemaphoreGive(stateMutex);
      
      Serial.printf("\n▶️  Playing: %s\n", playlist[track]);
      currentFormat = detectAudioFormat(playlist[track]);
      
      audioFile = SD.open(playlist[track], FILE_READ);
      if (!audioFile) {
        Serial.printf("❌ Failed to open: %s\n", playlist[track]);
        currentTrack = (currentTrack + 1) % playlistSize;
        trackChanged = true;
        vTaskDelay(500 / portTICK_PERIOD_MS);
        continue;
      }
      
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
      static int lastAppliedVolume = -1;
      
      if (currentVolume != lastAppliedVolume) {
          audioOut->updateLoudness(currentVolume);
          lastAppliedVolume = currentVolume;
      }

      float lin_vol = currentVolume / 100.0f;
      float vol = lin_vol * lin_vol * lin_vol; 
      
      if (mp3Decoder) mp3Decoder->setGain(vol);
      if (wavDecoder) wavDecoder->setGain(vol);
      
      bool readyForData = false;
      if (mp3Decoder) readyForData = (mp3Decoder->availableForWrite() > sizeof(buffer));
      if (wavDecoder) readyForData = (wavDecoder->availableForWrite() > sizeof(buffer));
      
      if (readyForData) {
        int bytesRead = audioFile.read(buffer, sizeof(buffer));
        if (bytesRead > 0) {
          if (mp3Decoder) mp3Decoder->write(buffer, bytesRead);
          if (wavDecoder) wavDecoder->write(buffer, bytesRead);
          currentPosition += bytesRead;
        } else {
          Serial.println("✅ Track finished");
          audioFile.close();
          
          xSemaphoreTake(stateMutex, portMAX_DELAY);
          if (loopMode == LOOP_SINGLE) {
             trackChanged = true;
          } else if (loopMode == LOOP_ALL) {
             currentTrack = (currentTrack + 1) % playlistSize;
             trackChanged = true;
          } else {
             currentTrack++;
             if (currentTrack >= playlistSize) {
               currentTrack = 0;
               playbackState = STATE_STOPPED;
               trackChanged = true;
             } else {
               trackChanged = true;
             }
          }
           xSemaphoreGive(stateMutex);
        }
      } else {
        vTaskDelay(2 / portTICK_PERIOD_MS);
      }
    } else {
       vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
}

// ========== Setup ==========
void setup() {
  Serial.setRxBufferSize(8192);
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  Production-Grade WAV Player (Modular) ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  stateMutex = xSemaphoreCreateMutex();
  
  loadPlaybackState();
  
  loopMode = LOOP_ALL;
  Serial.println("🔁 Loop mode forced to: All (Loop playlist)");
  
  Serial.println("💾 Initializing SD card...");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 10000000)) {
    Serial.println("❌ SD Card failed!");
    while (1) delay(1000);
  }
  Serial.println("✅ SD Card OK\n");
  
  scanPlaylist();
  if (playlistSize == 0) {
    Serial.println("❌ No WAV files found!");
    while (1) delay(1000);
  }
  
  if (currentTrack >= playlistSize) {
    currentTrack = 0;
  }
  
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
  Serial.println("✅ Audio System Ready\n");
  
  EVENT_LOG("Initializing buttons");
  Serial.println("🎮 Initializing buttons...");
  
  pinMode(BTN_VOL_UP, INPUT_PULLDOWN);
  pinMode(BTN_VOL_DOWN, INPUT_PULLDOWN);
  pinMode(BTN_PREV, INPUT_PULLDOWN);
  pinMode(BTN_NEXT, INPUT_PULLDOWN);
  pinMode(BTN_PAUSE, INPUT_PULLDOWN);
  
  delay(100);
  
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_UP), isr_vol_up, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_DOWN), isr_vol_down, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PREV), isr_prev, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), isr_next, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), isr_pause, RISING);
  
  Serial.println("✅ Buttons OK\n");
  
  Serial.println("🚀 Starting FreeRTOS tasks...");
  
  xTaskCreatePinnedToCore(
    audioPlaybackTask,
    "AudioTask",
    16384,
    NULL,
    2,
    &audioTaskHandle,
    1
  );
  
  xTaskCreatePinnedToCore(
    buttonHandlerTask,
    "ButtonTask",
    4096,
    NULL,
    1,
    &buttonTaskHandle,
    0
  );
  
  Serial.println("✅ Tasks started\n");
  Serial.println("═══════════════════════════════════════");
  Serial.println("Ready! Type 'help' for commands\n");
  
  playbackState = STATE_PLAYING;
}

// ========== Loop ==========
void loop() {
  handleSerialCommand();
  handleFileUpload();
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
