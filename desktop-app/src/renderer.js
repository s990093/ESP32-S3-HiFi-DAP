
// State
let isConnected = false;
let currentTrack = "";
let currentStatus = {};

// Elements
// Elements
const portSelector = document.getElementById('port-selector');
const connectionOverlay = document.getElementById('connection-overlay');
const fileTableBody = document.getElementById('file-table-body');
const fileList = document.getElementById('file-list'); // Track list container
const deviceInfo = document.getElementById('device-info');

// Upload Elements (Embedded)
const uploadStatusBox = document.getElementById('upload-status-box');
const uploadBar = document.getElementById('upload-bar-embedded');
const uploadPercent = document.getElementById('upload-percent-embedded');
const uploadFilename = document.getElementById('upload-filename-embedded');

// Init
scanPorts();
window.electronAPI.onSerialStatus(handleSerialStatus);
window.electronAPI.onSerialData(handleSerialData);
window.electronAPI.onUploadProgress(handleUploadProgress);

// ========== VIEWS ==========
function switchView(viewName) {
    // Hide all
    document.querySelectorAll('.view-section').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(el => el.classList.remove('active'));

    // Show Target
    document.getElementById(`view-${viewName}`).classList.add('active');
    document.getElementById(`nav-${viewName}`).classList.add('active');
}

window.switchView = switchView; // Expose to global for HTML onclick

// ========== UPLOAD ==========

// Handle "Select File" Button input
async function handleFileSelect(input) {
    if (input.files.length > 0) {
        await startUpload(input.files[0]);
    }
}
window.handleFileSelect = handleFileSelect;

// Drag & Drop on Drop Zone
const dropZone = document.getElementById('drop-zone');
if (dropZone) {
    ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
        dropZone.addEventListener(eventName, preventDefaults, false);
    });

    function preventDefaults(e) {
        e.preventDefault();
        e.stopPropagation();
    }

    dropZone.addEventListener('dragover', () => dropZone.classList.add('dragover'));
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
    dropZone.addEventListener('drop', async (e) => {
        dropZone.classList.remove('dragover');
        const dt = e.dataTransfer;
        const files = dt.files;
        if (files.length > 0) await startUpload(files[0]);
    });
}

async function startUpload(file) {
    if (!isConnected) { alert("Please Connect Device First!"); return; }

    // Show Progress UI (Embedded)
    uploadStatusBox.style.display = 'block';
    uploadFilename.innerText = `Uploading ${file.name}...`;
    uploadBar.style.width = '0%';
    uploadPercent.innerText = '0%';

    // Fix: Use exposed helper to get real path
    let localPath;
    try {
        localPath = window.electronAPI.getPathForFile(file);
    } catch (e) {
        // Fallback for older electron or if exposing failed, though it shouldn't
        localPath = file.path;
    }

    if (!localPath) {
        alert("Error: Could not determine file path. Please try 'Select File' button instead of Drag & Drop.");
        return;
    }

    const remotePath = `/${file.name}`;
    console.log("Uploading", localPath, "to", remotePath);

    const result = await window.electronAPI.uploadFile(localPath, remotePath);

    if (result.success) {
        uploadFilename.innerText = "✅ Upload Complete!";
        uploadBar.style.width = '100%';
        setTimeout(() => {
            uploadStatusBox.style.display = 'none';
            switchView('library'); // Go back to library
            refreshLibrary();
        }, 1500);
    } else {
        alert("Upload Failed: " + (result.message || "Unknown error"));
        uploadStatusBox.style.display = 'none';
    }
}

function handleUploadProgress(percent) {
    if (uploadBar) uploadBar.style.width = `${percent}%`;
    if (uploadPercent) uploadPercent.innerText = `${percent}%`;
}

// ========== CONNECTION ==========

async function scanPorts() {
    portSelector.innerHTML = '<option>Scanning...</option>';
    const ports = await window.electronAPI.listPorts();
    portSelector.innerHTML = '';

    if (ports.length === 0) {
        portSelector.innerHTML = '<option>No devices found</option>';
        return;
    }

    const savedPort = localStorage.getItem('savedPort');
    let foundSaved = false;

    ports.forEach(port => {
        const option = document.createElement('option');
        option.value = port.path;
        option.text = `${port.path} ${port.manufacturer || ''}`;
        portSelector.appendChild(option);

        if (port.path === savedPort) {
            option.selected = true;
            foundSaved = true;
        }
    });

    // Auto-Connect if saved port exists
    if (foundSaved) {
        console.log("Auto-connecting to saved port:", savedPort);
        connectDevice();
    }
}

async function connectDevice() {
    const path = portSelector.value;
    if (!path) return;

    localStorage.setItem('savedPort', path); // Save for next time
    await window.electronAPI.connect(path);
}

function handleSerialStatus(status) {
    console.log("Serial Status:", status);
    if (status === 'connected') {
        isConnected = true;
        connectionOverlay.style.display = 'none';
        deviceInfo.innerText = "Connected";
        deviceInfo.style.color = "#4CAF50";

        // Initial Fetch
        setTimeout(() => sendCmd('info_json'), 500);
        setTimeout(() => sendCmd('status_json'), 1000);
        setTimeout(() => sendCmd('list_json'), 1500);

        // Start Polling
        setInterval(() => {
            if (isConnected) sendCmd('status_json');
        }, 1000);

        setInterval(() => {
            if (isConnected) sendCmd('sys_json');
        }, 3000); // Less frequent for system stats

    } else if (status === 'disconnected' || status === 'error') {
        isConnected = false;
        connectionOverlay.style.display = 'flex';
        deviceInfo.innerText = "Disconnected";
        deviceInfo.style.color = "#FF5555";
    }
}

// ========== COMMANDS ==========

function sendCmd(cmd) {
    window.electronAPI.sendCommand(cmd);
}

function refreshLibrary() {
    sendCmd('list_json');
}

function togglePlay() {
    if (currentStatus.state === 'playing') {
        sendCmd('pause');
    } else {
        // If paused, resume. If stopped, play current.
        if (currentStatus.state === 'paused') sendCmd('resume');
        else sendCmd('play'); // Default play
    }
}

function playFile(filename) {
    // Escape filename just in case, though basic play command handles spaces mostly fine
    sendCmd(`play ${filename}`);
}

// ========== DATA HANDLING ==========

function handleSerialData(data) {
    console.log("RX:", data);

    // 1. INFO
    if (data.device) {
        deviceInfo.innerText = `${data.device} (v${data.version})`;
    }

    // 2. STATUS
    if (data.state) {
        currentStatus = data;
        document.getElementById('status-state').innerText = data.state;
        document.getElementById('status-track').innerText = `${data.track_index + 1}/${data.track_total}`;
        document.getElementById('status-vol').innerText = `${data.volume}%`;
        document.getElementById('vol-display').innerText = `${data.volume}%`;

        document.getElementById('now-playing-title').innerText = data.file ? data.file.replace('/', '') : "No Track";

        const btnPlay = document.getElementById('btn-play');
        btnPlay.innerText = (data.state === 'playing') ? '⏸' : '▶';

        // Highlight active track
        if (data.file) highlightTrack(data.file);
    }

    // 3. SYSTEM
    if (data.heap_free) {
        document.getElementById('sys-heap').innerText = `${Math.round(data.heap_free / 1024)} KB`;
        document.getElementById('sys-uptime').innerText = `${Math.floor(data.uptime / 60)}m ${data.uptime % 60}s`;
    }

    // 4. LIST (Library)
    if (data.files) {
        renderFileList(data.files);
    }
}

function renderFileList(files) {
    fileList.innerHTML = '';
    if (!files || files.length === 0) {
        fileList.innerHTML = '\u003cdiv style="padding: 40px; text-align: center; color: #999;">No Files\u003c/div>';
        return;
    }

    files.forEach((file, index) => {
        const li = document.createElement('li');
        li.className = 'track-item';
        li.dataset.path = file.name;

        // Track number
        const trackNum = document.createElement('span');
        trackNum.className = 'track-number';
        trackNum.textContent = index + 1;

        // Track name (editable)
        const trackName = document.createElement('span');
        trackName.className = 'track-name';
        trackName.textContent = file.name.replace(/^\//, '');
        trackName.contentEditable = false;

        // Double-click to edit
        trackName.addEventListener('dblclick', (e) => {
            e.stopPropagation();
            trackName.contentEditable = true;
            trackName.focus();
            // Select all text
            const range = document.createRange();
            range.selectNodeContents(trackName);
            const sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(range);
        });

        // Save on Enter, cancel on Escape
        trackName.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                trackName.blur();
            } else if (e.key === 'Escape') {
                trackName.textContent = file.name.replace(/^\//, '');
                trackName.contentEditable = false;
            }
        });

        // Save rename on blur
        trackName.addEventListener('blur', async () => {
            const oldName = file.name;
            const newName = '/' + trackName.textContent.trim();
            trackName.contentEditable = false;

            if (newName !== oldName && newName !== '/') {
                await renameTrack(oldName, newName);
            } else {
                trackName.textContent = file.name.replace(/^\//, '');
            }
        });

        // Track duration/info
        const trackInfo = document.createElement('span');
        trackInfo.className = 'track-info';
        trackInfo.textContent = (file.size / 1024 / 1024).toFixed(2) + ' MB';

        // Delete button
        const deleteBtn = document.createElement('button');
        deleteBtn.className = 'delete-btn';
        deleteBtn.innerHTML = '🗑️';
        deleteBtn.title = 'Delete';
        deleteBtn.onclick = (e) => {
            e.stopPropagation();
            deleteTrack(file.name);
        };

        // Click row to play
        li.addEventListener('click', () => {
            playFile(file.name);
        });

        li.appendChild(trackNum);
        li.appendChild(trackName);
        li.appendChild(trackInfo);
        li.appendChild(deleteBtn);
        fileList.appendChild(li);
    });
}

// ========== FILE OPERATIONS ==========

async function deleteTrack(path) {
    if (!confirm(`Are you sure you want to delete ${path}?`)) return;

    console.log("Deleting:", path);
    const result = await window.electronAPI.deleteFile(path);
    if (result.success) {
        setTimeout(refreshLibrary, 500);
    } else {
        alert("Delete failed");
    }
}

async function renameTrack(oldPath, newPath) {
    const result = await window.electronAPI.renameFile(oldPath, newPath);
    if (result.success) {
        setTimeout(refreshLibrary, 500);
    } else {
        alert("Rename failed");
    }
}

function highlightTrack(filename) {
    // Remove old active/playing
    document.querySelectorAll('.track-item.playing').forEach(el => el.classList.remove('playing'));

    // Find and highlight currently playing track
    const trackItem = document.querySelector(`.track-item[data-path="${filename}"]`);
    if (trackItem) {
        trackItem.classList.add('playing');
        // Smooth scroll into view
        trackItem.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
}

// Play Control Functions
function playFile(filepath) {
    window.electronAPI.sendCommand(`play ${filepath}`);
}
