#!/bin/bash

# Ensure we are in the desktop-app directory
cd "$(dirname "$0")"

echo "🧹 Cleaning previous builds..."
rm -rf out
rm -rf release-builds

echo "📦 Packaging Electron App (macOS x64 + arm64)..."
# Pack for both architectures (universal) or just one. Since current OS is mac, we often default to the host or universal.
# electron-packager <sourcedir> <appname> --platform=<platform> --arch=<arch> [optional flags...]
# We'll explicitly name it to match APP_NAME below

npx electron-packager . "ESP32-S3-HiFi-DAP" --overwrite --platform=darwin --arch=x64 --out=release-builds --icon=assets/icon.icns
# npx electron-packager . "ESP32-S3-HiFi-DAP" --overwrite --platform=darwin --arch=arm64 --out=release-builds --icon=assets/icon.icns

echo "💿 Creating DMG..."
# Check if electron-installer-dmg is available
if ! npm list electron-installer-dmg > /dev/null 2>&1; then
    echo "Installing electron-installer-dmg..."
    npm install --save-dev electron-installer-dmg
fi

# Define paths
APP_NAME="ESP32-S3-HiFi-DAP"
APP_PATH="release-builds/${APP_NAME}-darwin-x64/${APP_NAME}.app"
DMG_NAME="${APP_NAME}.dmg"

if [ -d "$APP_PATH" ]; then
    npx electron-installer-dmg "$APP_PATH" "${APP_NAME}" --overwrite --out=release-builds
    echo "✅ DMG Created at release-builds/${DMG_NAME}"
    open release-builds
else
    echo "❌ Error: App bundle not found at $APP_PATH"
    exit 1
fi
