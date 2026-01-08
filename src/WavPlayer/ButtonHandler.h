/**
 * Button Handler Module - Header
 * Handles all button interrupts and button processing task
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "Config.h"

// ========== ISR Functions ==========
void IRAM_ATTR isr_vol_up();
void IRAM_ATTR isr_vol_down();
void IRAM_ATTR isr_prev();
void IRAM_ATTR isr_next();
void IRAM_ATTR isr_pause();

// ========== Task Function ==========
void buttonHandlerTask(void* parameter);

// ========== NVS Functions ==========
void savePlaybackState();
void loadPlaybackState();

#endif // BUTTON_HANDLER_H
