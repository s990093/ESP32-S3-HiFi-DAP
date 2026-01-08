/**
 * Serial Commands Module - Header
 * Handles all serial command processing and file upload
 */

#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <Arduino.h>
#include "Config.h"
#include "AudioProcessor.h"

// ========== Functions ==========
void handleSerialCommand();
void handleFileUpload();

#endif // SERIAL_COMMANDS_H
