// SoftSD.ino - Bit Bang SPI to diagnose SD Card
#include <Arduino.h>

#define SOFT_MISO 12
#define SOFT_MOSI 15
#define SOFT_SCK  13
#define SOFT_CS   2

void sw_spi_init() {
    pinMode(SOFT_CS, OUTPUT);
    pinMode(SOFT_SCK, OUTPUT);
    pinMode(SOFT_MOSI, OUTPUT);
    pinMode(SOFT_MISO, INPUT_PULLUP);
    
    digitalWrite(SOFT_CS, HIGH);
    digitalWrite(SOFT_SCK, LOW);
    digitalWrite(SOFT_MOSI, HIGH); // IDLE
}

uint8_t sw_spi_transfer(uint8_t d) {
    uint8_t q = 0;
    for (uint8_t i = 0; i < 8; i++) {
        // Write MOSI
        if (d & 0x80) digitalWrite(SOFT_MOSI, HIGH);
        else          digitalWrite(SOFT_MOSI, LOW);
        d <<= 1;
        
        // Clock High
        digitalWrite(SOFT_SCK, HIGH);
        delayMicroseconds(10); // Slow speed
        
        // Read MISO
        q <<= 1;
        if (digitalRead(SOFT_MISO)) q |= 1;
        
        // Clock Low
        digitalWrite(SOFT_SCK, LOW);
        delayMicroseconds(10);
    }
    return q;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== Bit-Bang SD Diagnostics ===");
    Serial.printf("CS:%d SCK:%d MOSI:%d MISO:%d\n", SOFT_CS, SOFT_SCK, SOFT_MOSI, SOFT_MISO);
    
    sw_spi_init();
    
    // 1. Power Up Sequence (74+ clocks with CS high)
    Serial.print("Sending 80 clocks... ");
    digitalWrite(SOFT_CS, HIGH);
    digitalWrite(SOFT_MOSI, HIGH);
    for(int i=0; i<10; i++) sw_spi_transfer(0xFF);
    Serial.println("Done.");
    
    // 2. CMD0 (Go Idle State) - Reset
    // CMD0: 0x40, Arg: 0x00000000, CRC: 0x95
    Serial.print("Sending CMD0 (Reset)... ");
    digitalWrite(SOFT_CS, LOW);
    
    uint8_t r1 = 0xFF;
    // Send CMD0
    sw_spi_transfer(0x40);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x95);
    
    // Wait for response (0x01 means Idle/Reset Success)
    for(int i=0; i<100; i++) {
        r1 = sw_spi_transfer(0xFF);
        if(r1 != 0xFF) break;
    }
    digitalWrite(SOFT_CS, HIGH);
    
    if (r1 == 0x01) {
        Serial.println("SUCCESS (Response: 0x01)");
    } else {
        Serial.printf("FAILED (Response: 0x%02X)\n", r1);
        Serial.println("  Expected 0x01. If 0xFF, MISO is stuck high. If 0x00, stuck low?");
        return; 
    }
    
    // 3. CMD8 (Send Interface Condition) - Check Voltage
    // CMD8: 0x48, Arg: 0x000001AA, CRC: 0x87
    Serial.print("Sending CMD8 (Check Voltage)... ");
    digitalWrite(SOFT_CS, LOW);
    
    sw_spi_transfer(0x48);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x00);
    sw_spi_transfer(0x01);
    sw_spi_transfer(0xAA);
    sw_spi_transfer(0x87);
    
    for(int i=0; i<100; i++) {
        r1 = sw_spi_transfer(0xFF);
        if(r1 != 0xFF) break;
    }
    
    // Read 32-bit output
    uint8_t r7[4];
    r7[0] = sw_spi_transfer(0xFF);
    r7[1] = sw_spi_transfer(0xFF);
    r7[2] = sw_spi_transfer(0xFF);
    r7[3] = sw_spi_transfer(0xFF);
    
    digitalWrite(SOFT_CS, HIGH);
    
    Serial.printf("Response R1: 0x%02X\n", r1);
    if (r1 == 0x01) {
        Serial.printf("Voltage Check: 0x%02X%02X%02X%02X\n", r7[0], r7[1], r7[2], r7[3]);
        if (r7[3] == 0xAA) {
            Serial.println("✓ Wiring & Card Logic Valid!");
        } else {
            Serial.println("⚠ Card responded, but pattern mismatch (Wiring likely OK, card confused?)");
        }
    } else {
        Serial.println("FAILED CMD8 (Old V1 card or error)");
    }
    
    Serial.println("--- End Diagnostics ---");
}

void loop() {
    delay(1000);
}
