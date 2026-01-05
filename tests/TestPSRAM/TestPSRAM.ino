#include "Arduino.h"
#include "esp_heap_caps.h"

// Explicitly requested PSRAM allocation test
// Based on user provided example and best practices for WROVER

void setup() {
    Serial.begin(115200);
    delay(500); // Wait for Serial to stabilize

    Serial.println("\n\n========================================");
    Serial.println("      ESP32-WROVER PSRAM TEST");
    Serial.println("========================================");

    // 1. Basic PSRAM Size Check (Arduino Core)
    size_t psram_size = ESP.getPsramSize();
    Serial.printf("[Info] Total PSRAM Reported: %d bytes (%.2f MB)\n", psram_size, psram_size / (1024.0 * 1024.0));
    
    if (psram_size == 0) {
        Serial.println("[Error] ❌ No PSRAM detected! Check 'Tools > PSRAM' setting.");
        Serial.println("         If enabled, check hardware connections (CS/CLK/D0-D3).");
    } else {
        Serial.println("[Info] ✅ PSRAM is detected and enabled in Core.");
    }

    // 2. Heap Capabilities Check
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    Serial.printf("[Info] Free PSRAM Heap: %d bytes (%.2f MB)\n", free_psram, free_psram / (1024.0 * 1024.0));

    // 3. Rigorous Allocation & R/W Test
    size_t test_size = 320 * 240 * 2; // ~150KB (Frame Buffer Size)
    Serial.printf("[Test] Attempting to allocate %d bytes in PSRAM...\n", test_size);

    uint8_t *buffer = (uint8_t*) heap_caps_malloc(test_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (buffer == NULL) {
        Serial.println("[Test] ❌ Allocation FAILED! (Returned NULL)");
    } else {
        Serial.printf("[Test] ✅ Allocation SUCCESS! Address: %p\n", buffer);
        
        // Write Test
        Serial.println("[Test] Performing Write/Read validation...");
        bool success = true;
        
        // Write pattern
        for (size_t i = 0; i < test_size; i += 1024) {
            buffer[i] = (uint8_t)(i & 0xFF);
        }
        buffer[test_size - 1] = 0xAA; // End marker

        // Read pattern
        for (size_t i = 0; i < test_size; i += 1024) {
            if (buffer[i] != (uint8_t)(i & 0xFF)) {
                Serial.printf("[Test] ❌ Data mismatch at index %d! Expected %02X, Got %02X\n", i, (uint8_t)(i & 0xFF), buffer[i]);
                success = false;
                break;
            }
        }
        if (buffer[test_size - 1] != 0xAA) {
             Serial.println("[Test] ❌ End marker mismatch!");
             success = false;
        }2

        if (success) {
            Serial.println("[Test] ✅ Write/Read Validation PASSED!");
        } else {
            Serial.println("[Test] ❌ Validation FAILED!");
        }

        free(buffer);
        Serial.println("[Info] Memory freed.");
    }

    Serial.println("========================================");
    Serial.println("Test Complete.");
}

void loop() {
    delay(1000);
}
