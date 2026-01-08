/**
 * Audio Processor Module - Implementation
 * Contains AudioOutputWithEQ class implementation
 */

#include "AudioProcessor.h"

// Constants
const float PI_F = 3.14159265359f;
const float dt = 1.0f / SAMPLE_RATE;
const float ALPHA_LOW = (2.0f * PI_F * dt * BASS_CUTOFF_HZ) / (1.0f + 2.0f * PI_F * dt * BASS_CUTOFF_HZ);
const float ALPHA_HIGH = 1.0f / (1.0f + 2.0f * PI_F * dt * TREB_CUTOFF_HZ);

// Global instance
AudioOutputWithEQ *audioOut = NULL;

// ========== Constructor ==========
AudioOutputWithEQ::AudioOutputWithEQ(int bck, int ws, int data, int mclk) 
    : ESP32I2SAudio(bck, ws, data, mclk) {
    // Initialize RNG
    rand_state = 123456789;
    // Initialize Gains (Default to flat or updated immediately)
    current_bass_gain = 1.0f; 
    current_treb_gain = 1.0f;
    // Default bit depth
    currentBitDepth = BIT_DEPTH_16;
}

// ========== Dynamic Loudness Compensation ==========
void AudioOutputWithEQ::updateLoudness(int volume_percent) {
    float vol = (float)volume_percent / 100.0f;
    
    // Inverse relationship: Lower volume = Higher boost
    // Max Bass Boost (at low vol): +8dB
    // Min Bass Boost (at max vol): +2dB
    float target_bass_db = 8.0f * (1.0f - vol); 
    float target_treb_db = 4.0f * (1.0f - vol);
    
    // Clamp minimums (Keep V-Shape character even at max volume)
    if (target_bass_db < 2.0f) target_bass_db = 2.0f; 
    if (target_treb_db < 1.0f) target_treb_db = 1.0f;

    current_bass_gain = pow(10.0f, target_bass_db / 20.0f);
    current_treb_gain = pow(10.0f, target_treb_db / 20.0f);
}

// ========== Bit Depth Control ==========
void AudioOutputWithEQ::setBitDepth(BitDepth depth) {
    currentBitDepth = depth;
    Serial.printf("🎚️ I2S Bit Depth set to: %d-bit\n", (int)depth);
}

// ========== Audio Processing Write Override ==========
size_t AudioOutputWithEQ::write(const uint8_t *buffer, size_t size) {
    int16_t* samples = (int16_t*)buffer;
    int count = size / 2; // Number of samples

    for (int i = 0; i < count; i+=2) { // Stereo Interleaved
        // 1. Headroom Management (-3dB Pre-attenuation)
        float L = (float)samples[i] * HEADROOM_SCALER;
        float R = (i+1 < count) ? (float)samples[i+1] * HEADROOM_SCALER : 0;

        // 2. High-Precision Floating Point EQ
        float outL = applyShelving(applyShelving((int16_t)L, bass_l_prev_in, bass_l_prev_out, false), treb_l_prev_in, treb_l_prev_out, true);
        float outR = 0;
        if (i+1 < count) {
            outR = applyShelving(applyShelving((int16_t)R, bass_r_prev_in, bass_r_prev_out, false), treb_r_prev_in, treb_r_prev_out, true);
        }

        // 3. TPDF Dithering
        outL += get_tpdf_dither();
        if (i+1 < count) outR += get_tpdf_dither();

        // 4. Hard Limiter (Clamping)
        if (outL > 32767.0f) outL = 32767.0f;
        if (outL < -32768.0f) outL = -32768.0f;
        if (outR > 32767.0f) outR = 32767.0f;
        if (outR < -32768.0f) outR = -32768.0f;

        // Write back to buffer
        samples[i] = (int16_t)outL;
        if (i+1 < count) samples[i+1] = (int16_t)outR;
    }

    // Apply bit depth conversion if needed
    if (currentBitDepth != BIT_DEPTH_16) {
        writeWithBitDepth(samples, count);
        return size; // Already written
    }

    // Pass the processed buffer to the actual I2S driver
    return ESP32I2SAudio::write(buffer, size);
}

// ========== Shelving Filter ==========
int16_t AudioOutputWithEQ::applyShelving(int16_t sample, float& prev_in, float& prev_out, bool highpass) {
    float in = (float)sample;
    float out;
    float alpha;
    float gain;

    if (highpass) {
        alpha = ALPHA_HIGH; gain = current_treb_gain;
        float hp = alpha * (prev_out + in - prev_in);
        out = in + (gain - 1.0f) * hp;
        prev_out = hp;
    } else {
        alpha = ALPHA_LOW; gain = current_bass_gain;
        float lp = alpha * in + (1.0f - alpha) * prev_out;
        out = in + (gain - 1.0f) * lp;
        prev_out = lp;
    }
    prev_in = in;
    return (int16_t)out; 
}

// ========== TPDF Dither ==========
inline int16_t AudioOutputWithEQ::get_tpdf_dither() {
    rand_state ^= (rand_state << 13);
    rand_state ^= (rand_state >> 17);
    rand_state ^= (rand_state << 5);
    return (int16_t)((rand_state & 0x01) - ((rand_state >> 1) & 0x01));
}

// ========== Bit Depth Conversion ==========
void AudioOutputWithEQ::writeWithBitDepth(int16_t* samples, int count) {
    // For 24-bit and 32-bit output, we need to convert int16_t to the appropriate format
    // ESP32 I2S expects 32-bit frames for 24/32-bit audio
    
    if (currentBitDepth == BIT_DEPTH_24) {
        // 24-bit: Left-shift 8 bits (int16 -> int24 in 32-bit frame)
        int32_t buffer32[count];
        for (int i = 0; i < count; i++) {
            buffer32[i] = ((int32_t)samples[i]) << 8;
        }
        ESP32I2SAudio::write((uint8_t*)buffer32, count * 4);
        
    } else if (currentBitDepth == BIT_DEPTH_32) {
        // 32-bit: Left-shift 16 bits (int16 -> int32)
        int32_t buffer32[count];
        for (int i = 0; i < count; i++) {
            buffer32[i] = ((int32_t)samples[i]) << 16;
        }
        ESP32I2SAudio::write((uint8_t*)buffer32, count * 4);
    }
}
