/**
 * Audio Processor Module - Header
 * Contains AudioOutputWithEQ class with EQ, dithering, and bit depth control
 */

#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <Arduino.h>
#include <ESP32I2SAudio.h>
#include "Config.h"

// ========== EQ Settings ==========
#define SAMPLE_RATE 44100.0f

// Gain Settings
#define TARGET_BASS_DB  4.5f   // Bass Boost
#define TARGET_TREB_DB  2.5f   // Treble Clarity

// Frequency Ranges
#define BASS_CUTOFF_HZ  100.0f 
#define TREB_CUTOFF_HZ  3000.0f // Updated to 3kHz for "Air" and Vocal Clarity

// Headroom (Crucial for V-Shape EQ to prevent clipping)
#define HEADROOM_SCALER 0.707f // -3dB

// ========== AudioOutputWithEQ Class ==========
class AudioOutputWithEQ : public ESP32I2SAudio {
public:
    AudioOutputWithEQ(int bck, int ws, int data, int mclk = -1);
    
    // Dynamic Loudness Compensation (Fletcher-Munson inspired)
    void updateLoudness(int volume_percent);
    
    // Bit Depth Control
    void setBitDepth(BitDepth depth);
    BitDepth getBitDepth() const { return currentBitDepth; }
    
    // Override the raw write function to intercept ALL audio data (MP3 & WAV)
    virtual size_t write(const uint8_t *buffer, size_t size) override;

private:
    unsigned long rand_state;
    BitDepth currentBitDepth;
    
    // Dynamic Gain State
    float current_bass_gain;
    float current_treb_gain;

    // EQ State Variables
    float bass_l_prev_in = 0, bass_l_prev_out = 0;
    float bass_r_prev_in = 0, bass_r_prev_out = 0;
    float treb_l_prev_in = 0, treb_l_prev_out = 0;
    float treb_r_prev_in = 0, treb_r_prev_out = 0;

    // Internal Helpers
    int16_t applyShelving(int16_t sample, float& prev_in, float& prev_out, bool highpass);
    inline int16_t get_tpdf_dither();
    void writeWithBitDepth(int16_t* samples, int count);
};

// Global audio output instance
extern AudioOutputWithEQ *audioOut;

#endif // AUDIO_PROCESSOR_H
