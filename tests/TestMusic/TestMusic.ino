/**
 * Music Playback Test for TTGO T-Display
 * Tests playlist functionality and serial commands
 * 
 * This test generates a simple tone WAV file and tests playback control
 */

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// SD Card pins (same as main project)
#define SD_MISO 36
#define SD_MOSI 32
#define SD_SCK  33
#define SD_CS   25

// Test WAV file generation
void generateTestTone(const char* filename, uint16_t frequency_hz, uint16_t duration_ms) {
  Serial.printf("Generating test tone: %s (%d Hz, %d ms)\n", filename, frequency_hz, duration_ms);
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file!");
    return;
  }
  
  // WAV file parameters
  const uint32_t sample_rate = 44100;
  const uint16_t channels = 2; // Stereo
  const uint16_t bits_per_sample = 16;
  const uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
  const uint16_t block_align = channels * (bits_per_sample / 8);
  
  // Calculate data size
  uint32_t num_samples = (sample_rate * duration_ms) / 1000;
  uint32_t data_size = num_samples * channels * (bits_per_sample / 8);
  uint32_t file_size = 36 + data_size;
  
  // Write WAV header
  file.write((uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&file_size, 4);
  file.write((uint8_t*)"WAVE", 4);
  file.write((uint8_t*)"fmt ", 4);
  
  uint32_t fmt_size = 16;
  uint16_t audio_format = 1; // PCM
  file.write((uint8_t*)&fmt_size, 4);
  file.write((uint8_t*)&audio_format, 2);
  file.write((uint8_t*)&channels, 2);
  file.write((uint8_t*)&sample_rate, 4);
  file.write((uint8_t*)&byte_rate, 4);
  file.write((uint8_t*)&block_align, 2);
  file.write((uint8_t*)&bits_per_sample, 2);
  
  file.write((uint8_t*)"data", 4);
  file.write((uint8_t*)&data_size, 4);
  
  // Generate sine wave tone
  for (uint32_t i = 0; i < num_samples; i++) {
    float t = (float)i / (float)sample_rate;
    float sample = sin(2.0 * PI * frequency_hz * t) * 32000.0; // Max amplitude
    
    int16_t sample_int = (int16_t)sample;
    
    // Write stereo (same sample for both channels)
    file.write((uint8_t*)&sample_int, 2); // Left
    file.write((uint8_t*)&sample_int, 2); // Right
  }
  
  file.close();
  Serial.printf("✅ Generated %s (%.2f KB)\n", filename, data_size / 1024.0);
}

void setup() {
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n=== TTGO T-Display Music Test ===");
  Serial.println("Initializing SD card...");
  
  // Initialize SPI
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS, SPI, 10000000)) {
    Serial.println("❌ SD Card initialization FAILED!");
    while(1) delay(1000);
  }
  
  Serial.println("✅ SD Card initialized\n");
  
  // Generate test WAV files (different tones)
  Serial.println("--- Generating Test WAV Files ---");
  generateTestTone("/tone_440hz.wav", 440, 2000);  // A note, 2 seconds
  generateTestTone("/tone_523hz.wav", 523, 2000);  // C note, 2 seconds
  generateTestTone("/tone_659hz.wav", 659, 2000);  // E note, 2 seconds
  
  Serial.println("\n--- Test Files Created ---");
  Serial.println("Files created:");
  Serial.println("  /tone_440hz.wav (A note)");
  Serial.println("  /tone_523hz.wav (C note)");
  Serial.println("  /tone_659hz.wav (E note)");
  
  Serial.println("\n--- Serial Commands to Test ---");
  Serial.println("1. LIST           - List all WAV files");
  Serial.println("2. PLAY_CURRENT   - Play current track");
  Serial.println("3. NEXT           - Next track");
  Serial.println("4. PREVIOUS       - Previous track");
  Serial.println("5. PAUSE          - Pause playback");
  Serial.println("6. RESUME         - Resume playback");
  Serial.println("7. STOP           - Stop playback");
  Serial.println("8. STATUS         - Get playback status");
  
  Serial.println("\n✅ Test setup complete!");
  Serial.println("Upload main firmware and test these commands.\n");
}

void loop() {
  delay(1000);
}
