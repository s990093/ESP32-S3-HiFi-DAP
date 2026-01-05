/**
 * SD Card Test for TTGO T-Display
 * Tests SD card initialization and file operations
 * 
 * SD pins:
 * MISO: GPIO36 (Input Only)
 * MOSI: GPIO32
 * SCK:  GPIO33
 * CS:   GPIO25
 */

#include <SPI.h>
#include <SD.h>

// SD Card pins
#define SD_MISO 36
#define SD_MOSI 32
#define SD_SCK  33
#define SD_CS   25

void setup() {
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n=== TTGO T-Display SD Card Test ===");
  Serial.printf("MISO: GPIO%d (Input Only)\n", SD_MISO);
  Serial.printf("MOSI: GPIO%d\n", SD_MOSI);
  Serial.printf("SCK:  GPIO%d\n", SD_SCK);
  Serial.printf("CS:   GPIO%d\n\n", SD_CS);
  
  // Initialize SPI with custom pins
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  Serial.println("Initializing SD card...");
  
  if (!SD.begin(SD_CS, SPI, 10000000)) {  // 10MHz SPI
    Serial.println("❌ SD Card initialization FAILED!");
    Serial.println("\nPossible issues:");
    Serial.println("1. No SD card inserted");
    Serial.println("2. Wrong wiring");
    Serial.println("3. Bad SD card");
    Serial.println("4. SPI pin conflict");
    while(1) delay(1000);
  }
  
  Serial.println("✅ SD Card initialized successfully!");
  
  // Get card info
  uint8_t cardType = SD.cardType();
  
  Serial.print("\nCard Type: ");
  switch(cardType) {
    case CARD_NONE:
      Serial.println("NONE (No card detected)");
      while(1) delay(1000);
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SD");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("UNKNOWN");
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("Card Size: %llu MB\n", cardSize);
  
  uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
  uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
  Serial.printf("Total space: %llu MB\n", totalBytes);
  Serial.printf("Used space: %llu MB\n", usedBytes);
  Serial.printf("Free space: %llu MB\n\n", totalBytes - usedBytes);
  
  // Test write
  Serial.println("--- Testing file write ---");
  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("SD Card test - TTGO T-Display");
    testFile.println("GPIO36/32/33/25 SPI configuration");
    testFile.printf("Test time: %lu ms\n", millis());
    testFile.close();
    Serial.println("✅ Write test PASSED");
  } else {
    Serial.println("❌ Write test FAILED");
  }
  
  // Test read
  Serial.println("\n--- Testing file read ---");
  testFile = SD.open("/test.txt", FILE_READ);
  if (testFile) {
    Serial.println("File contents:");
    Serial.println("----------------------------------------");
    while (testFile.available()) {
      Serial.write(testFile.read());
    }
    Serial.println("----------------------------------------");
    testFile.close();
    Serial.println("✅ Read test PASSED");
  } else {
    Serial.println("❌ Read test FAILED");
  }
  
  // List files in root
  Serial.println("\n--- Files in root directory ---");
  listDir(SD, "/", 0);
  
  Serial.println("\n=== SD Card test complete! ===");
  Serial.println("All tests passed. SD card is working correctly.");
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  int count = 0;
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  [DIR]  %s\n", file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.printf("  [FILE] %-20s %8llu bytes\n", file.name(), file.size());
    }
    file = root.openNextFile();
    count++;
  }
  Serial.printf("Total: %d items\n", count);
}

void loop() {
  delay(1000);
}
