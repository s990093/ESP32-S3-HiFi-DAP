/**
 * Button Test with Interrupts (ISR)
 * Tests 4 buttons using GPIO interrupts for instant response
 * 
 * Button Configuration (Pull-down, active HIGH):
 *   VOL+  → GPIO12 (按下 = HIGH)
 *   VOL-  → GPIO13 (按下 = HIGH)
 *   PREV  → GPIO14 (按下 = HIGH)
 *   NEXT  → GPIO27 (按下 = HIGH)
 */

// Button pins
#define BTN_VOL_UP   12
#define BTN_VOL_DOWN 13
#define BTN_PREV     14
#define BTN_NEXT     27

// Debounce time (ms)
#define DEBOUNCE_MS 200

// Volatile flags for ISR (中斷標記)
volatile bool btnPressed[4] = {false, false, false, false};
volatile unsigned long lastInterruptTime[4] = {0, 0, 0, 0};

// Button info
const char* btnNames[] = {"VOL+", "VOL-", "PREV", "NEXT"};
const uint8_t btnPins[] = {BTN_VOL_UP, BTN_VOL_DOWN, BTN_PREV, BTN_NEXT};
int pressCounts[4] = {0, 0, 0, 0};

// ISR functions (must be in IRAM for speed)
void IRAM_ATTR isr_btn0() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime[0] > DEBOUNCE_MS) {
    btnPressed[0] = true;
    lastInterruptTime[0] = currentTime;
  }
}

void IRAM_ATTR isr_btn1() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime[1] > DEBOUNCE_MS) {
    btnPressed[1] = true;
    lastInterruptTime[1] = currentTime;
  }
}

void IRAM_ATTR isr_btn2() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime[2] > DEBOUNCE_MS) {
    btnPressed[2] = true;
    lastInterruptTime[2] = currentTime;
  }
}

void IRAM_ATTR isr_btn3() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTime[3] > DEBOUNCE_MS) {
    btnPressed[3] = true;
    lastInterruptTime[3] = currentTime;
  }
}

void setup() {
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   ESP32 Button Test - ISR Mode         ║");
  Serial.println("║   (中斷模式 - 即時反應)                 ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  Serial.println("Button Configuration:");
  Serial.println("┌────────┬────────┬──────────────────┐");
  Serial.println("│ Button │  GPIO  │   Connection     │");
  Serial.println("├────────┼────────┼──────────────────┤");
  Serial.printf("│ VOL+   │  %2d    │ GPIO%d ↔ 3.3V   │\n", BTN_VOL_UP, BTN_VOL_UP);
  Serial.printf("│ VOL-   │  %2d    │ GPIO%d ↔ 3.3V   │\n", BTN_VOL_DOWN, BTN_VOL_DOWN);
  Serial.printf("│ PREV   │  %2d    │ GPIO%d ↔ 3.3V   │\n", BTN_PREV, BTN_PREV);
  Serial.printf("│ NEXT   │  %2d    │ GPIO%d ↔ 3.3V   │\n", BTN_NEXT, BTN_NEXT);
  Serial.println("└────────┴────────┴──────────────────┘\n");
  
  // Initialize buttons with pull-down and attach interrupts
  pinMode(BTN_VOL_UP, INPUT_PULLDOWN);
  pinMode(BTN_VOL_DOWN, INPUT_PULLDOWN);
  pinMode(BTN_PREV, INPUT_PULLDOWN);
  pinMode(BTN_NEXT, INPUT_PULLDOWN);
  
  // Attach interrupts (RISING edge = button press)
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_UP), isr_btn0, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_VOL_DOWN), isr_btn1, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_PREV), isr_btn2, RISING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), isr_btn3, RISING);
  
  Serial.println("✓ All buttons initialized with interrupts!");
  Serial.println("✓ Debounce: 200ms");
  Serial.println("\n🎮 Ready! Press any button...\n");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  // Check interrupt flags
  for (int i = 0; i < 4; i++) {
    if (btnPressed[i]) {
      btnPressed[i] = false;  // Clear flag
      pressCounts[i]++;
      handleButtonPress(i);
    }
  }
  
  // Optional: print status every 10 seconds
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 10000) {
    printStatus();
    lastStatus = millis();
  }
}

void handleButtonPress(int btnIndex) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.printf("║ 🔘 按鈕觸發: %-27s║\n", btnNames[btnIndex]);
  Serial.println("╠════════════════════════════════════════╣");
  Serial.printf("║ GPIO: %-33d║\n", btnPins[btnIndex]);
  Serial.printf("║ 次數: %-33d║\n", pressCounts[btnIndex]);
  Serial.printf("║ 時間: %lu ms %-24s║\n", millis(), "");
  Serial.println("╚════════════════════════════════════════╝");
  
  // Simulate actions
  switch (btnIndex) {
    case 0: // VOL+
      Serial.println("🔊 動作: 音量增加");
      simulateVolumeChange(+5);
      break;
    case 1: // VOL-
      Serial.println("🔉 動作: 音量減少");
      simulateVolumeChange(-5);
      break;
    case 2: // PREV
      Serial.println("⏮️  動作: 上一首");
      simulateTrackChange(-1);
      break;
    case 3: // NEXT
      Serial.println("⏭️  動作: 下一首");
      simulateTrackChange(+1);
      break;
  }
  Serial.println();
}

void simulateVolumeChange(int delta) {
  static int currentVolume = 50;
  
  currentVolume += delta;
  if (currentVolume > 100) currentVolume = 100;
  if (currentVolume < 0) currentVolume = 0;
  
  Serial.print("   音量: [");
  int bars = currentVolume / 5;
  for (int i = 0; i < 20; i++) {
    if (i < bars) {
      Serial.print("█");
    } else {
      Serial.print("░");
    }
  }
  Serial.printf("] %d%%\n", currentVolume);
}

void simulateTrackChange(int delta) {
  static int currentTrack = 1;
  static const char* tracks[] = {
    "test1.wav", "test2.wav", "test3.wav", 
    "music1.wav", "music2.wav"
  };
  static const int numTracks = 5;
  
  currentTrack += delta;
  if (currentTrack < 1) currentTrack = numTracks;
  if (currentTrack > numTracks) currentTrack = 1;
  
  Serial.printf("   正在播放: [%d/%d] %s\n", 
                currentTrack, numTracks, tracks[currentTrack - 1]);
}

void printStatus() {
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📊 按鈕狀態總覽");
  Serial.println("┌────────┬───────────────┐");
  Serial.println("│ 按鈕   │ 按壓次數      │");
  Serial.println("├────────┼───────────────┤");
  
  for (int i = 0; i < 4; i++) {
    Serial.printf("│ %-6s │ %13d │\n", btnNames[i], pressCounts[i]);
  }
  
  Serial.println("└────────┴───────────────┘");
  Serial.printf("⏱️  運行時間: %lu 秒\n", millis() / 1000);
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

