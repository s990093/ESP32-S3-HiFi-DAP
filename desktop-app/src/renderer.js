
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

// Startup Animation
setTimeout(() => {
    const splash = document.getElementById('startup-splash');
    if (splash) {
        splash.style.opacity = '0';
        setTimeout(() => splash.style.display = 'none', 500);
    }
}, 2000);

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
        setTimeout(() => sendCmd('storage_json'), 1800); // Fetch storage info on connect

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
    setTimeout(() => sendCmd('storage_json'), 200); // Fetch storage too
    // Scroll to top
    document.getElementById('file-list').scrollTop = 0;
}

function toggleLoop() {
    sendCmd('loop');
    // Force immediate status update to reflect change in UI
    setTimeout(() => sendCmd('status_json'), 100);
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

        // Update Volume UI if it wasn't just dragged by user (simple check)
        // ideally we sync this always but avoid jitter while dragging
        const slider = document.getElementById('vol-slider');
        if (document.activeElement !== slider) {
            slider.value = data.volume;
            document.getElementById('vol-display').innerText = `${data.volume}%`;
            updateVolumeIcon(data.volume);
        }
        document.getElementById('status-vol').innerText = `${data.volume}%`;

        // Strip extension for cleaner display
        let displayName = data.file ? data.file.replace('/', '') : "No Track";
        displayName = displayName.replace(/\.(mp3|wav)$/i, '');

        const titleEl = document.getElementById('now-playing-title');
        titleEl.innerText = displayName;

        // Reset animation
        titleEl.classList.remove('marquee-active');
        titleEl.style.removeProperty('--scroll-offset');
        void titleEl.offsetWidth; // Trigger reflow

        // Check for overflow
        if (titleEl.scrollWidth > titleEl.clientWidth) {
            const offset = titleEl.scrollWidth - titleEl.clientWidth;
            // Add a small buffer for visuals
            titleEl.style.setProperty('--scroll-offset', `-${offset + 10}px`);
            titleEl.classList.add('marquee-active');
        }

        const btnPlay = document.getElementById('btn-play');
        btnPlay.innerText = (data.state === 'playing') ? '⏸' : '▶';

        // Highlight active track
        if (data.file) highlightTrack(data.file);

        // Update Loop Icon
        const loopBtn = document.getElementById('btn-loop');
        if (loopBtn && data.loop) {
            if (data.loop === 'single') {
                loopBtn.innerText = '🔂';
                loopBtn.style.color = 'var(--accent-color)';
                loopBtn.style.borderColor = 'var(--accent-color)';
            } else if (data.loop === 'all') {
                loopBtn.innerText = '🔁';
                loopBtn.style.color = 'var(--accent-color)';
                loopBtn.style.borderColor = 'var(--accent-color)';
            } else {
                loopBtn.innerText = '➡️'; // Or dimmed 🔁
                loopBtn.style.color = 'var(--text-secondary)';
                loopBtn.style.borderColor = 'rgba(255, 255, 255, 0.1)';
            }
        }
    }

    // 3. SYSTEM
    if (data.heap_free) {
        document.getElementById('sys-heap').innerText = `${Math.round(data.heap_free / 1024)} KB`;
        document.getElementById('sys-uptime').innerText = `${Math.floor(data.uptime / 60)}m ${data.uptime % 60}s`;
    }

    // 4. STORAGE
    if (data.total && data.used) {
        updateStorageInfo(data);
    }

    // 5. LIST (Library)
    if (data.files) {
        renderFileList(data.files);
    }

    // 6. TASKS / DIAGNOSTICS
    if (data.tasks || data.heap_free) {
        handleDiagnostics(data);
    }
}

// Add global listener for animation end to cleanup marquee
document.addEventListener('DOMContentLoaded', () => {
    const titleEl = document.getElementById('now-playing-title');
    if (titleEl) {
        titleEl.addEventListener('animationend', () => {
            titleEl.classList.remove('marquee-active');
        });
    }
});

// Diagnostics Poller
setInterval(() => {
    if (!isConnected) return;
    // Only poll if diagnostic view is active
    if (document.getElementById('view-diagnostics').classList.contains('active')) {
        sendCmd('tasks_json');
        setTimeout(() => sendCmd('sys_json'), 100);
    }
}, 2000);

// ========== DIAGNOSTICS RENDERER ==========
class RealTimeChart {
    constructor(canvasId, color) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.color = color;
        this.data = new Array(60).fill(0); // 60 data points
        this.maxVal = 100;

        // Handle resizing
        this.resize();
        window.addEventListener('resize', () => this.resize());
    }

    resize() {
        // Get actual display size
        const rect = this.canvas.parentElement.getBoundingClientRect();

        // HiDPI Scaling
        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = (rect.width - 24) * dpr;
        this.canvas.height = (rect.height - 30) * dpr;

        this.canvas.style.width = `${rect.width - 24}px`;
        this.canvas.style.height = `${rect.height - 30}px`;

        // RESET TRANSFORM to avoid cumulative scaling!
        this.ctx.setTransform(1, 0, 0, 1, 0, 0);
        this.ctx.scale(dpr, dpr);
    }

    push(val) {
        this.data.push(val);
        this.data.shift();
        this.draw();
    }

    draw() {
        const rect = this.canvas.getBoundingClientRect(); // Use logic width
        const w = rect.width;
        const h = rect.height;
        const ctx = this.ctx;

        ctx.clearRect(0, 0, w, h);

        // Dynamic scaling for heap, fixed 100 for CPU
        let max = Math.max(...this.data) * 1.2;
        if (max === 0) max = 100;

        ctx.beginPath();
        ctx.strokeStyle = this.color;
        ctx.lineWidth = 2;
        ctx.lineJoin = 'round';

        const step = w / (this.data.length - 1);

        this.data.forEach((val, i) => {
            const x = i * step;
            const y = h - (val / max * h);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });

        ctx.stroke();

        // Gradient fill
        ctx.lineTo(w, h);
        ctx.lineTo(0, h);
        ctx.closePath();
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, this.color.replace(')', ', 0.2)').replace('rgb', 'rgba'));
        grad.addColorStop(1, this.color.replace(')', ', 0.0)').replace('rgb', 'rgba'));
        ctx.fillStyle = grad;
        ctx.fill();

        // Current Value Text
        const lastVal = this.data[this.data.length - 1];
        ctx.fillStyle = '#fff';
        ctx.font = '12px SF Mono, monospace'; // Monospace for numbers
        ctx.fillText(lastVal.toFixed(1), w - 40, 15);
    }
}

let cpuChart = null;
let heapChart = null;

let isEngineerMode = false;

function toggleEngineerMode(enabled) {
    isEngineerMode = enabled;
    const cols = document.querySelectorAll('.col-eng');
    cols.forEach(el => {
        el.style.display = enabled ? 'table-cell' : 'none';
    });
}

// Global System State
let globalCore0Load = 0;
let globalCore1Load = 0;

function handleDiagnostics(data) {
    // Init charts if needed
    if (!cpuChart && document.getElementById('chart-cpu')) {
        cpuChart = new RealTimeChart('chart-cpu', 'rgb(99, 102, 241)'); // Indigo
        heapChart = new RealTimeChart('chart-heap', 'rgb(16, 185, 129)'); // Emerald
    }

    if (data.tasks) {
        const tbody = document.getElementById('diag-table-body');
        if (tbody) {
            tbody.innerHTML = '';
            // Sort by CPU usage
            data.tasks.sort((a, b) => (b.cpu || 0) - (a.cpu || 0));

            // Find IDLE tasks for accurate Core Load (Inverse Idle)
            const idle0 = data.tasks.find(t => t.name === 'IDLE0');
            const idle1 = data.tasks.find(t => t.name === 'IDLE1');

            let idle0Val = idle0 ? (idle0.cpu || 0) : 0;
            let idle1Val = idle1 ? (idle1.cpu || 0) : 0;

            globalCore0Load = 100 - idle0Val;
            globalCore1Load = 100 - idle1Val;

            // Calc System Load (Avg of inverse idle)
            const sysLoad = (globalCore0Load + globalCore1Load) / 2;

            // Sanity clamp
            if (globalCore0Load < 0) globalCore0Load = 0;
            if (globalCore1Load < 0) globalCore1Load = 0;

            // Find Audio Task for chart
            const audioTask = data.tasks.find(t => t.name === 'AudioTask');
            if (audioTask && cpuChart) {
                cpuChart.push(audioTask.cpu || 0);
            } else if (cpuChart) {
                // If no audio task, plot System Load instead of random task
                cpuChart.push(sysLoad);
            }

            data.tasks.forEach(task => {
                const tr = document.createElement('tr');
                const isIdle = task.name.startsWith('IDLE');

                // Style IDLE rows differently (dimmed)
                if (isIdle) {
                    tr.style.opacity = '0.5';
                    tr.style.fontStyle = 'italic';
                }

                // State Badge color & Pulse
                let stateClass = '';
                let stateLabel = task.state;
                let pulseClass = '';

                switch (task.state) {
                    case 'X':
                        stateClass = 'diag-state-running';
                        stateLabel = 'RUNNING';
                        if (!isIdle) pulseClass = 'pulse'; // Don't pulse IDLE
                        break;
                    case 'B': stateClass = 'diag-state-blocked'; stateLabel = 'BLOCKED'; break;
                    case 'R': stateClass = 'diag-state-ready'; stateLabel = 'READY'; break;
                    case 'S': stateClass = 'diag-state-suspended'; stateLabel = 'SUSPENDED'; break;
                    case 'D': stateClass = 'diag-state-suspended'; stateLabel = 'DELETED'; break;
                }

                if (stateLabel === 'SUSPENDED') {
                    stateLabel = '⏸ PAUSED';
                }

                // Stack Safety Color (Min Free Bytes)
                // Red < 350 (Critical), Yellow < 600 (Warning - catches ipc tasks)
                let stackColor = '#10b981'; // Green
                let stackIcon = '';
                if (task.stack < 350) {
                    stackColor = '#ef4444'; // Red
                    stackIcon = '⚠️ ';
                } else if (task.stack < 600) {
                    stackColor = '#f59e0b'; // Yellow
                }

                // Engineer Mode Columns visibility
                const engDisplay = isEngineerMode ? 'table-cell' : 'none';

                tr.innerHTML = `
                    <td>${task.id}</td>
                    <td style="font-weight: 500; color: var(--text-primary)">${task.name}</td>
                    <td><span class="diag-state-badge ${stateClass} ${pulseClass}">${stateLabel}</span></td>
                    <td>${task.prio}</td>
                    <td style="color: ${stackColor}; font-weight: bold;">${stackIcon}${task.stack} B</td>
                    <td>
                        <div style="display: flex; align-items: center; gap: 8px;">
                            <div class="diag-bar-bg" style="width: 60px;">
                                <div class="diag-bar-fill" style="width: ${task.cpu || 0}%;"></div>
                            </div>
                            <span style="font-size: 11px;">${(task.cpu || 0).toFixed(1)}%</span>
                        </div>
                    </td>
                    <td class="col-eng" style="display: ${engDisplay}; color: #888;">${task.core === -1 ? 'Any' : task.core}</td>
                    <td class="col-eng" style="display: ${engDisplay}; color: #888;">N/A</td>
        `;
                tbody.appendChild(tr);
            });
        }
    }

    // System Stats update in System Bar
    if (data.heap_free) {
        const heapKB = data.heap_free / 1024;
        const psramKB = (data.psram_size > 0) ? (data.psram_free / 1024) : 0;

        if (heapChart) heapChart.push(heapKB);

        // Update System Bar Elements
        if (document.getElementById('sys-cpu0')) document.getElementById('sys-cpu0').innerText = `${globalCore0Load.toFixed(0)}% `;
        if (document.getElementById('sys-cpu1')) document.getElementById('sys-cpu1').innerText = `${globalCore1Load.toFixed(0)}% `;
        if (document.getElementById('sys-heap-bar')) document.getElementById('sys-heap-bar').innerText = `${heapKB.toFixed(0)} KB`;
        if (document.getElementById('sys-psram-bar')) document.getElementById('sys-psram-bar').innerText = psramKB > 0 ? `${psramKB.toFixed(0)} KB` : 'N/A';

        const h = Math.floor(data.uptime / 3600);
        const m = Math.floor((data.uptime % 3600) / 60);
        const s = data.uptime % 60;
        if (document.getElementById('sys-uptime-bar')) document.getElementById('sys-uptime-bar').innerText = `${h}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')} `;

        // Remove old simple stats to avoid duplicate (optional)
        if (document.getElementById('diag-heap')) document.getElementById('diag-heap').parentElement.parentElement.style.display = 'none';
    }
}


function renderFileList(files) {
    // Check if we are incorrectly calling this with diag data, just in case
    if (!files || !Array.isArray(files)) return;

    fileList.innerHTML = '';
    if (files.length === 0) {
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
    const trackItem = document.querySelector(`.track - item[data - path="${filename}"]`);
    if (trackItem) {
        trackItem.classList.add('playing');
        // Smooth scroll into view
        trackItem.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
}

// Play Control Functions
function playFile(filepath) {
    window.electronAPI.sendCommand(`play ${filepath} `);
}

// ========== VOLUME CONTROL ==========
let volumeDebounceTimer = null;

function handleVolumeInput(val) {
    // Immediate UI update
    document.getElementById('vol-display').innerText = `${val}% `;
    updateVolumeIcon(val);
}

function handleVolumeChange(val) {
    // Committed change (mouse up)
    sendCmd(`volume ${val} `);
}

function updateVolumeIcon(val) {
    const icon = document.getElementById('vol-icon');
    if (val == 0) icon.innerText = '🔇';
    else if (val < 30) icon.innerText = '🔈';
    else if (val < 70) icon.innerText = '🔉';
    else icon.innerText = '🔊';
}

function toggleMute() {
    // Simple mute toggle logic (optional, for now just focus slider)
    const slider = document.getElementById('vol-slider');
    // logical implementation would need state tracking, skipping for now to keep it simple
}

// ========== STORAGE INFO ==========
function updateStorageInfo(data) {
    // Expect: {total: bytes, used: bytes, free: bytes}
    if (!data.total) return;

    const usedGB = (data.used / 1024 / 1024 / 1024).toFixed(2);
    const totalGB = (data.total / 1024 / 1024 / 1024).toFixed(2);
    const percent = Math.round((data.used / data.total) * 100);

    document.getElementById('sd-text').innerText = `${usedGB} GB / ${totalGB} GB`;
    document.getElementById('sd-bar-fill').style.width = `${percent}% `;

    // Color coding
    const bar = document.getElementById('sd-bar-fill');
    if (percent > 90) bar.style.background = 'var(--danger)';
    else if (percent > 75) bar.style.background = 'var(--warning)';
    else bar.style.background = 'var(--accent-gradient)';
}
