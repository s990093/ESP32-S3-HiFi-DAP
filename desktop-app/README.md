# ESP32 Music Center - Desktop App 🎵

> **Apple Music-Inspired Control Center for ESP32 HiFi-DAP**  
> A beautiful Electron-based desktop application for managing your ESP32 music player.

![ESP32 Music Center](https://img.shields.io/badge/platform-Electron-47848F)
![Node](https://img.shields.io/badge/node-%3E%3D16.0.0-green)
![License](https://img.shields.io/badge/license-MIT-blue)

---

## ✨ Features

### 🎨 Premium Apple Music Design

- **Glassmorphism Effects** - Backdrop blur and translucent layers
- **Vibrant Gradients** - Purple-pink-blue accent colors
- **Premium Typography** - Inter font (SF Pro-like)
- **Smooth Animations** - Micro-interactions and transitions
- **Enhanced Player Bar** - 120px tall with album art and progress bar

### 🎮 Music Management

- 📚 **Library View** - Browse all tracks on SD card
- ▶️ **Playback Controls** - Play, pause, next, previous
- 📤 **File Upload** - Drag & drop MP3/WAV files
- 🗑️ **File Management** - Delete and rename tracks
- 📊 **Real-time Status** - Track info, volume, system stats

### 🔧 Device Control

- 🔌 **Auto-Connect** - Remembers last connected device
- 📡 **Serial Communication** - 460800 baud UART
- 💾 **SD Storage Monitor** - Visual storage usage bar
- ⚙️ **Settings Panel** - Configure buffer size and preferences

---

## 📸 Screenshots

### Main Interface

![Full Interface](../.gemini/antigravity/brain/9c7da43c-fab7-4729-b9bd-7607ca7976c3/apple_style_player_full_page_1767677027668.png)

### Hover Effects

![Hover State](../.gemini/antigravity/brain/9c7da43c-fab7-4729-b9bd-7607ca7976c3/apple_style_player_hover_state_1767677037998.png)

---

## 🚀 Quick Start

### Prerequisites

```bash
# Install Node.js (v16+)
brew install node    # macOS
# Or download from https://nodejs.org

# Verify installation
node --version
npm --version
```

### Installation

```bash
# Navigate to desktop-app directory
cd desktop-app

# Install dependencies
npm install

# Start the application
npm start
```

---

## 📋 Usage Guide

### First Launch

1. **Connect ESP32**

   - Plug in your ESP32 via USB
   - The app will auto-scan for serial ports
   - Select your device and click "Connect"

2. **Browse Library**

   - The library view shows all tracks on SD card
   - Click any track to start playback
   - Hover to see delete button

3. **Upload Music**
   - Switch to "Upload" view
   - Drag & drop MP3/WAV files
   - Or click to browse files

### Controls

| Action              | Method                                 |
| ------------------- | -------------------------------------- |
| **Play Track**      | Click on track in library              |
| **Play/Pause**      | Click center play button (⏸/▶)         |
| **Next Track**      | Click next button (⏭)                  |
| **Previous Track**  | Click previous button (⏮)              |
| **Delete Track**    | Hover track → Click 🗑️ button          |
| **Rename Track**    | Double-click track name → Edit → Enter |
| **Upload File**     | Drag & drop to upload zone             |
| **Refresh Library** | Click "🔄 Refresh List"                |

---

## 🏗️ Architecture

### Tech Stack

- **Framework**: Electron (v39.2.7)
- **IPC**: Serial communication via SerialPort
- **UI**: HTML5 + Vanilla CSS + JavaScript
- **Font**: Inter (Google Fonts)
- **Design**: Apple Music-inspired glassmorphism

### File Structure

```
desktop-app/
├── src/
│   ├── main.js          # Electron main process
│   ├── preload.js       # Context bridge
│   ├── renderer.js      # UI logic (383 lines)
│   ├── index.html       # Main UI
│   ├── styles.css       # Apple-style CSS (600+ lines)
│   └── demo.html        # Standalone demo
├── package.json
└── README.md            # This file
```

### Serial Protocol

The app communicates with ESP32 via JSON commands:

```javascript
// Status Request
"status_json"

// Response
{
  "state": "playing",
  "file": "/song.mp3",
  "track_index": 2,
  "track_total": 15,
  "volume": 35
}

// File List Request
"list_json"

// Response
{
  "files": [
    {"name": "/song1.mp3", "size": 5242880},
    {"name": "/song2.mp3", "size": 7340032}
  ]
}
```

---

## 🎨 Design System

### Color Palette

```css
/* Dark Background */
--bg-primary: #0a0a0f;
--bg-secondary: #1a1a24;

/* Glassmorphism */
--glass-bg: rgba(30, 30, 45, 0.7);
backdrop-filter: blur(40px);

/* Vibrant Accents */
--accent-gradient: linear-gradient(
  135deg,
  #667eea 0%,
  #764ba2 50%,
  #f093fb 100%
);
```

### Typography

```css
font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI";
-webkit-font-smoothing: antialiased;
letter-spacing: -0.3px to -0.5px;
```

### Animations

- **Fade In**: 0.5s ease on page load
- **Slide Up**: 0.3s for modals
- **Pulse**: 2s infinite for play indicator
- **Glow**: 3s infinite for album art
- **Hover Scale**: 0.2s cubic-bezier for buttons

---

## 🛠️ Development

### Running in Dev Mode

```bash
# Install dependencies
npm install

# Start with hot reload (if using electron-reloader)
npm start

# Or run directly
npx electron .
```

### Building for Production

```bash
# Package for macOS
npx electron-packager . "ESP32 Music Center" --platform=darwin --arch=x64

# Package for Windows
npx electron-packager . "ESP32 Music Center" --platform=win32 --arch=x64

# Package for Linux
npx electron-packager . "ESP32 Music Center" --platform=linux --arch=x64
```

### Debug Mode

Open DevTools with:

- **macOS**: `Cmd + Option + I`
- **Windows/Linux**: `Ctrl + Shift + I`

---

## 🐛 Troubleshooting

### App Won't Start

```bash
# Clear node_modules and reinstall
rm -rf node_modules package-lock.json
npm install
npm start
```

### Serial Port Not Found

1. Check USB connection
2. Verify ESP32 is powered on
3. Click "Rescan" in connection modal
4. Check permissions (macOS may require):
   ```bash
   sudo chmod 666 /dev/cu.usbserial-*
   ```

### Upload Fails

- Ensure ESP32 is connected and idle
- Check SD card has free space
- Try smaller files first (\<10MB)
- Reduce upload chunk size in `main.js`:
  ```javascript
  const CHUNK_SIZE = 4096; // Default: 8192
  ```

### No Status Updates

1. Check serial baud rate (should be 460800)
2. Verify ESP32 firmware supports JSON commands
3. Monitor serial output in DevTools console

---

## 📡 Serial Commands Reference

| Command                | Response | Description                 |
| ---------------------- | -------- | --------------------------- |
| `status_json`          | JSON     | Current playback status     |
| `list_json`            | JSON     | File list with sizes        |
| `sys_json`             | JSON     | System stats (heap, uptime) |
| `info_json`            | JSON     | Device info (name, version) |
| `play <file>`          | -        | Play specific file          |
| `pause`                | -        | Pause playback              |
| `resume`               | -        | Resume playback             |
| `next`                 | -        | Next track                  |
| `prev`                 | -        | Previous track              |
| `delete <file>`        | -        | Delete file from SD         |
| `rename <old> <new>`   | -        | Rename file                 |
| `upload <path> <size>` | READY    | Start file upload           |

---

## 🎯 Features Roadmap

### v2.0 (Current)

- ✅ Apple Music-inspired design
- ✅ Glassmorphism effects
- ✅ File upload/delete/rename
- ✅ Real-time status updates
- ✅ SD storage monitoring

### v2.1 (Planned)

- [ ] Album artwork display
- [ ] Seekbar functionality
- [ ] Volume slider control
- [ ] Playlist management
- [ ] Dark/Light theme toggle

### v3.0 (Future)

- [ ] Bluetooth streaming control
- [ ] EQ settings panel
- [ ] Lyrics display
- [ ] Multi-device support
- [ ] Cloud sync

---

## 🤝 Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on macOS/Windows/Linux
5. Submit a pull request

### Code Style

- Use 2-space indentation
- Follow existing naming conventions
- Add comments for complex logic
- Test serial communication thoroughly

---

## 📄 License

MIT License - see [LICENSE](../LICENSE) file for details.

---

## 🙏 Credits

- **Design Inspiration**: Apple Music
- **Icons**: Unicode emoji
- **Font**: Inter by Rasmus Andersson
- **Framework**: Electron by GitHub

---

<p align="center">
  Made with ❤️ for ESP32 audiophiles
</p>

<p align="center">
  <sub>Part of the ESP32-S3-HiFi-DAP project</sub>
</p>
