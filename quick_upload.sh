#!/bin/bash
# Quick Build and Upload Script for ESP32 HiFi DAP
# Usage: ./quick_upload.sh

set -e  # Exit on error

echo "🔨 Building project..."
python3 build.py

echo ""
echo "📤 Uploading to ESP32 at 460800 baud..."
python3 scripts/upload.py --baud 460800

echo ""
echo "✅ Done!"
