/**
 * Pure I2S Sine Wave Test (No SD Card)
 * Generates 1kHz sine wave directly to PCM5102
 * 
 * Hackaday proven pins:
 *   BCK:  GPIO4
 *   LRCK: GPIO15
 *   DIN:  GPIO2
 */

#include <driver/i2s.h>
#include <math.h>

#define I2S_BCK   4
#define I2S_WS    15
#define I2S_DATA  2

#define SAMPLE_RATE 44100
#define FREQUENCY   1000  // 1kHz test tone

void setup() {
  Serial.begin(460800);
  delay(1000);
  
  Serial.println("\n=== I2S Sine Wave Test ===");
  Serial.printf("Generating %dHz tone at %dHz sample rate\n", FREQUENCY, SAMPLE_RATE);
  Serial.printf("I2S Pins: BCK=%d, LRCK=%d, DIN=%d\n\n", I2S_BCK, I2S_WS, I2S_DATA);
  
  // I2S configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  // Install I2S driver
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("❌ I2S driver install failed: %d\n", err);
    while(1) delay(1000);
  }
  
  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("❌ I2S pin config failed: %d\n", err);
    while(1) delay(1000);
  }
  
  i2s_zero_dma_buffer(I2S_NUM_0);
  
  Serial.println("✅ I2S initialized successfully!");
  Serial.println("🔊 You should hear a continuous 1kHz tone now...\n");
  Serial.println("If you hear NOTHING:");
  Serial.println("  1. Check AGND is connected to GND");
  Serial.println("  2. Check headphones/speaker connection");
  Serial.println("  3. Check I2S wiring (BCK/LRCK/DIN)");
  Serial.println("  4. Try different headphones/speaker\n");
}

void loop() {
  static uint32_t sample_count = 0;
  static const int buffer_size = 128;
  int16_t samples[buffer_size * 2];  // Stereo
  
  // Generate sine wave samples
  for (int i = 0; i < buffer_size; i++) {
    float t = (float)sample_count / SAMPLE_RATE;
    float sine_val = sin(2.0 * PI * FREQUENCY * t);
    int16_t sample = (int16_t)(sine_val * 16000);  // 50% volume
    
    samples[i * 2] = sample;      // Left
    samples[i * 2 + 1] = sample;  // Right
    
    sample_count++;
  }
  
  // Write to I2S
  size_t bytes_written;
  i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
  
  // Progress indicator
  static uint32_t last_print = 0;
  if (millis() - last_print > 2000) {
    Serial.printf("🎵 Playing... %lu seconds\n", millis() / 1000);
    last_print = millis();
  }
}
