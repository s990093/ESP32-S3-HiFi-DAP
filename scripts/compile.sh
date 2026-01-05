#!/bin/bash
# Arduino CLI compilation script for ESP32

echo "Compiling ESP32-HiFi-DAP..."

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "ERROR: arduino-cli is not installed"
    echo "Please install it from: https://arduino.github.io/arduino-cli/"
    exit 1
fi

# Get the project root directory (one level up from scripts)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Compile the project for ESP32 (not ESP32-S3)
arduino-cli compile --fqbn esp32:esp32:esp32 "$PROJECT_ROOT" --verbose

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Compilation successful!"
    echo "Binary location: build/esp32.esp32.esp32/ESP32-S3-HiFi-DAP.ino.bin"
else
    echo ""
    echo "✗ Compilation failed!"
    exit 1
fi
