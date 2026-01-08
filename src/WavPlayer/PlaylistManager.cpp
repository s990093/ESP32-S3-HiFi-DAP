/**
 * Playlist Manager Module - Implementation
 * Handles playlist scanning and audio format detection
 */

#include "PlaylistManager.h"

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
