#!/bin/bash
# Serial monitor script

# Find USB serial port
PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -n 1)

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.SLAB_USBtoUART 2>/dev/null | head -n 1)
fi

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -n 1)
fi

if [ -z "$PORT" ]; then
    echo "ERROR: No USB serial port found"
    exit 1
fi

echo "Opening serial monitor on $PORT at 115200 baud..."
echo "Press Ctrl+C to exit"
echo ""

arduino-cli monitor -p "$PORT" -c baudrate=460800
