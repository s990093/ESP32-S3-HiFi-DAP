#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DIR="$PROJECT_ROOT/tests/TestSD"
PORT="/dev/cu.usbserial-2130"
FQBN="esp32:esp32:esp32:UploadSpeed=115200,FlashMode=dio,FlashFreq=40,PSRAM=enabled,PartitionScheme=huge_app"

echo "Compiling TestSD in $TEST_DIR ..."
arduino-cli compile --fqbn $FQBN "$TEST_DIR"
if [ $? -ne 0 ]; then echo "Compile failed"; exit 1; fi

echo "Uploading to $PORT with reduced speed..."
arduino-cli upload -p $PORT --fqbn $FQBN "$TEST_DIR"
if [ $? -ne 0 ]; then echo "Upload failed"; exit 1; fi

echo "Monitoring... (Ctrl+C to stop)"
arduino-cli monitor -p $PORT --config baudrate=115200
