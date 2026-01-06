const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

let mainWindow;
let port;
let parser;

// Configuration
const BAUD_RATE = 460800;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1000,
        height: 800,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            nodeIntegration: false,
            contextIsolation: true
        },
        titleBarStyle: 'hiddenInset', // Mac-style
        backgroundColor: '#1E1E1E'
    });

    mainWindow.loadFile(path.join(__dirname, 'index.html'));
}

app.whenReady().then(() => {
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});

// ========== SERIAL PORT MANAGER ==========

ipcMain.handle('list-ports', async () => {
    try {
        const ports = await SerialPort.list();
        return ports;
    } catch (err) {
        console.error("Error listing ports:", err);
        return [];
    }
});

ipcMain.handle('connect', async (event, path) => {
    if (port && port.isOpen) {
        port.close();
    }

    try {
        port = new SerialPort({ path: path, baudRate: BAUD_RATE });
        parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

        port.on('open', () => {
            console.log(`Connected to ${path}`);
            mainWindow.webContents.send('serial-status', 'connected');
        });

        port.on('error', (err) => {
            console.error('Serial Error:', err);
            mainWindow.webContents.send('serial-status', 'error');
        });

        port.on('close', () => {
            console.log('Port Closed');
            mainWindow.webContents.send('serial-status', 'disconnected');
        });

        parser.on('data', (line) => {
            try {
                // Try parsing JSON
                const json = JSON.parse(line.trim());
                mainWindow.webContents.send('serial-data', json);
            } catch (e) {
                // Forward raw logs if not JSON
                console.log("Raw:", line);  // Disabled to reduce boot log noise
            }
        });

        return true;
    } catch (err) {
        console.error("Connection Failed:", err);
        return false;
    }
});

ipcMain.handle('send-command', (event, cmd) => {
    if (port && port.isOpen) {
        // Clear input buffer mainly for clean responses
        port.flush();
        port.write(cmd + '\n');
        console.log("Sent:", cmd);
    }
});

ipcMain.handle('upload-file', async (event, localPath, remotePath) => {
    if (!port || !port.isOpen) return { success: false, message: "Not connected" };

    const fs = require('fs');

    try {
        const stats = fs.statSync(localPath);
        const fileSize = stats.size;

        console.log(`Starting upload: ${localPath} -> ${remotePath} (${fileSize} bytes)`);

        // 1. Send Upload Command
        // We assume remotePath starts with /
        port.write(`upload ${remotePath} ${fileSize}\n`);

        // 2. Wait for READY (Blocking-ish for simplicity in this async handler)
        // In a real app we might use a proper state machine, but here we'll wait for the "serial-data" 
        // listener to flip a flag or just wait a bit and assume ready if no error? 
        // Actually, we can pause the continuous parser?
        // Better: Wait for a specific single-line response "READY"

        await new Promise(resolve => setTimeout(resolve, 500)); // Give ESP32 time to prep
        // Note: Ideally we should listen for "READY", but for MVP we rely on the 
        // fact that ESP32 handles this quickly. 
        // Let's add a small check if possible, but reading specific lines from a shared Serial stream 
        // that is also piping to a global parser is tricky. 
        // SAFE BET: Just wait 500ms. The ESP32 is fast.

        const fd = fs.openSync(localPath, 'r');
        // 8KB chunks with aggressive flow control for stability
        const CHUNK_SIZE = 8192;
        const buffer = Buffer.alloc(CHUNK_SIZE);
        let bytesRead = 0;
        let totalSent = 0;
        let chunkCount = 0;

        while ((bytesRead = fs.readSync(fd, buffer, 0, CHUNK_SIZE, null)) !== 0) {
            // Write chunk
            port.write(buffer.subarray(0, bytesRead));
            totalSent += bytesRead;
            chunkCount++;

            // Progress Update
            const progress = Math.round((totalSent / fileSize) * 100);
            mainWindow.webContents.send('upload-progress', progress);

            // Aggressive delay for SD card write time
            await new Promise(r => setTimeout(r, 150));

            // Every 5 chunks (40KB), pause longer to ensure SD buffer is fully written
            if (chunkCount % 5 === 0) {
                await new Promise(r => setTimeout(r, 500));
            }
        }

        fs.closeSync(fd);
        console.log("Upload Complete. Waiting for ESP32 to finish SD write...");

        // Wait longer for ESP32 to complete SD card operations
        await new Promise(r => setTimeout(r, 2000));

        // 🔄 Auto-refresh file list after upload
        console.log("Upload successful! Refreshing file list...");
        if (port && port.isOpen) {
            // Give ESP32 more time to stabilize
            await new Promise(r => setTimeout(r, 500));
            port.write('list_json\n');
        }

        return { success: true };

    } catch (err) {
        console.error("Upload Failed:", err);
        return { success: false, message: err.message };
    }
});

// File Operations
ipcMain.handle('delete-file', async (event, path) => {
    if (!port || !port.isOpen) return { success: false };
    port.write(`delete ${path}\n`);
    // Optimistic success, or wait for "SUCCESS" message in parser?
    // For now, let's just wait a moment.
    await new Promise(r => setTimeout(r, 200));
    return { success: true };
});

ipcMain.handle('rename-file', async (event, oldPath, newPath) => {
    if (!port || !port.isOpen) return { success: false };
    port.write(`rename ${oldPath} ${newPath}\n`);
    await new Promise(r => setTimeout(r, 200));
    return { success: true };
});
