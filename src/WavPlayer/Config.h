/**
 * Configuration Header for WavPlayer
 * Contains all pin definitions, constants, enums, and global state
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <FS.h>

// ========== Debug Configuration ==========
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

// ========== Configuration Constants ==========
#define I2S_NUM         I2S_NUM_0
#define BUFFER_SIZE     32768  // 32KB - Large buffer for MP3 decoder stability
#define MAX_TRACKS      32
#define MAX_FILENAME    256    // Increased for long Unicode filenames (日本語対応)
#define DEBOUNCE_MS     200
#define LONG_PRESS_MS   500
#define DOUBLE_CLICK_MS 400
#define FADE_SAMPLES    2048   // ~46ms @ 44.1kHz

// ========== Enums ==========
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

enum BitDepth {
  BIT_DEPTH_16 = 16,
  BIT_DEPTH_24 = 24,
  BIT_DEPTH_32 = 32
};

// ========== Global State Declarations ==========
extern Preferences prefs;
extern SemaphoreHandle_t stateMutex;

extern volatile PlaybackState playbackState;
extern volatile int currentVolume;
extern volatile int currentTrack;
extern volatile bool trackChanged;
extern volatile uint32_t currentPosition;
extern volatile uint32_t totalDataSize;
extern volatile LoopMode loopMode;

// Playlist
extern char playlist[MAX_TRACKS][MAX_FILENAME];
extern int playlistSize;

// File Upload State
extern volatile bool isReceivingFile;
extern File uploadFile;
extern size_t uploadRemaining;
extern unsigned long lastUploadActivity;

// Audio Format
extern volatile AudioFormat currentFormat;

// Button ISR Flags
extern volatile bool btnPressed[5];
extern volatile unsigned long lastInterruptTime[5];
extern volatile bool btnLongPress[2];

// Task Handles
extern TaskHandle_t audioTaskHandle;
extern TaskHandle_t buttonTaskHandle;

// Audio Buffer
extern uint8_t audioBuffer[BUFFER_SIZE];

#endif // CONFIG_H
