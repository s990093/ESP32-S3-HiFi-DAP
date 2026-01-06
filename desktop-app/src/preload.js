const { contextBridge, ipcRenderer, webUtils } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
    getPathForFile: (file) => webUtils.getPathForFile(file),
    listPorts: () => ipcRenderer.invoke('list-ports'),
    connect: (path) => ipcRenderer.invoke('connect', path),
    sendCommand: (cmd) => ipcRenderer.invoke('send-command', cmd),
    uploadFile: (local, remote) => ipcRenderer.invoke('upload-file', local, remote),
    deleteFile: (path) => ipcRenderer.invoke('delete-file', path),
    renameFile: (oldPath, newPath) => ipcRenderer.invoke('rename-file', oldPath, newPath),
    onSerialStatus: (callback) => ipcRenderer.on('serial-status', (_event, value) => callback(value)),
    onSerialData: (callback) => ipcRenderer.on('serial-data', (_event, value) => callback(value)),
    onUploadProgress: (callback) => ipcRenderer.on('upload-progress', (_event, value) => callback(value))
});
