/**
 * Playlist Manager Module - Header
 * Handles playlist scanning and audio format detection
 */

#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include "Config.h"

// ========== Functions ==========
void scanPlaylist();
AudioFormat detectAudioFormat(const char* filename);

#endif // PLAYLIST_MANAGER_H
