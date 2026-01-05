/**
 * @file TestBluetooth.ino
 * @brief Bluetooth scanner and connection test for ESP32-S3
 * 
 * 功能:
 * - 藍牙設備掃描
 * - 設備列表顯示
 * - 設備連線/斷線
 * - 透過 UART 指令控制
 * 
 * UART 指令格式:
 * - BT_SCAN        - 掃描藍牙設備
 * - BT_CONNECT:0   - 連接到索引 0 的設備
 * - BT_DISCONNECT  - 斷開連線
 * - BT_STATUS      - 查詢狀態
 */

#include <Arduino.h>
#include "BluetoothSerial.h"

// 檢查藍牙是否啟用
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please enable it in Arduino IDE Tools menu
#endif

// 配置
#define SERIAL_BAUD_RATE    115200
#define BT_DEVICE_NAME      "ESP32-Test"
#define MAX_BT_DEVICES      20
#define BT_SCAN_DURATION    10  // 秒

// Bluetooth Serial 實例
BluetoothSerial SerialBT;

// 設備資訊結構
struct BTDevice {
    String name;
    String address;
    int rssi;
    bool valid;
};

// 全域變數
BTDevice scannedDevices[MAX_BT_DEVICES];
int deviceCount = 0;
bool isScanning = false;
bool isConnected = false;
String connectedDeviceName = "";
String connectedDeviceAddress = "";

// 函數聲明
void handleCommand(String cmd);
void scanBluetoothDevices();
void connectToDevice(int index);
void disconnectBluetooth();
void printStatus();
void printDeviceList();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("ESP32 Bluetooth Scanner Test");
    Serial.println("=================================\n");
    
    // 初始化藍牙
    if (!SerialBT.begin(BT_DEVICE_NAME)) {
        Serial.println("ERROR: Failed to initialize Bluetooth!");
        while (1) delay(1000);
    }
    
    Serial.printf("Bluetooth initialized as '%s'\n", BT_DEVICE_NAME);
    Serial.println("\nAvailable commands:");
    Serial.println("  BT_SCAN        - Scan for Bluetooth devices");
    Serial.println("  BT_CONNECT:N   - Connect to device N (N = 0, 1, 2...)");
    Serial.println("  BT_DISCONNECT  - Disconnect from device");
    Serial.println("  BT_STATUS      - Show connection status");
    Serial.println("  LIST           - List scanned devices");
    Serial.println("=================================\n");
}

void loop() {
    // 處理 UART 指令
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() > 0) {
            handleCommand(cmd);
        }
    }
    
    // 檢查藍牙連線狀態
    static bool lastConnected = false;
    isConnected = SerialBT.connected();
    
    if (isConnected != lastConnected) {
        if (isConnected) {
            Serial.println("\n[EVENT] Bluetooth device connected!");
        } else {
            Serial.println("\n[EVENT] Bluetooth device disconnected!");
            connectedDeviceName = "";
            connectedDeviceAddress = "";
        }
        lastConnected = isConnected;
    }
    
    // 如果已連接，轉發藍牙數據到串口
    if (SerialBT.available()) {
        while (SerialBT.available()) {
            Serial.write(SerialBT.read());
        }
    }
    
    delay(10);
}

void handleCommand(String cmd) {
    Serial.printf("\n> Command: %s\n", cmd.c_str());
    
    if (cmd == "BT_SCAN") {
        scanBluetoothDevices();
        
    } else if (cmd.startsWith("BT_CONNECT:")) {
        int index = cmd.substring(11).toInt();
        connectToDevice(index);
        
    } else if (cmd == "BT_DISCONNECT") {
        disconnectBluetooth();
        
    } else if (cmd == "BT_STATUS") {
        printStatus();
        
    } else if (cmd == "LIST") {
        printDeviceList();
        
    } else {
        Serial.println("ERROR: Unknown command");
    }
}

void scanBluetoothDevices() {
    Serial.printf("\n[BT] Scanning for %d seconds...\n", BT_SCAN_DURATION);
    isScanning = true;
    deviceCount = 0;
    
    // 清空設備列表
    for (int i = 0; i < MAX_BT_DEVICES; i++) {
        scannedDevices[i].valid = false;
    }
    
    // 開始掃描
    BTScanResults* results = SerialBT.discover(BT_SCAN_DURATION * 1000);
    
    if (results) {
        int found = results->getCount();
        Serial.printf("[BT] Scan complete. Found %d devices:\n\n", found);
        
        // 處理掃描結果
        for (int i = 0; i < found && deviceCount < MAX_BT_DEVICES; i++) {
            BTAdvertisedDevice* device = results->getDevice(i);
            
            scannedDevices[deviceCount].name = device->getName().c_str();
            if (scannedDevices[deviceCount].name.length() == 0) {
                scannedDevices[deviceCount].name = "Unknown";
            }
            scannedDevices[deviceCount].address = device->getAddress().toString().c_str();
            scannedDevices[deviceCount].rssi = device->getRSSI();
            scannedDevices[deviceCount].valid = true;
            
            Serial.printf("  [%d] %s\n", deviceCount, 
                         scannedDevices[deviceCount].name.c_str());
            Serial.printf("      MAC: %s\n", 
                         scannedDevices[deviceCount].address.c_str());
            Serial.printf("      RSSI: %d dBm\n\n", 
                         scannedDevices[deviceCount].rssi);
            
            deviceCount++;
        }
        
        Serial.printf("Total: %d devices stored\n", deviceCount);
        Serial.println("Use 'BT_CONNECT:N' to connect (N = device index)");
        
    } else {
        Serial.println("[BT] ERROR: Scan failed");
    }
    
    isScanning = false;
}

void connectToDevice(int index) {
    if (index < 0 || index >= deviceCount) {
        Serial.printf("ERROR: Invalid device index %d (valid: 0-%d)\n", 
                     index, deviceCount - 1);
        return;
    }
    
    if (!scannedDevices[index].valid) {
        Serial.println("ERROR: Invalid device");
        return;
    }
    
    Serial.printf("[BT] Connecting to '%s' (%s)...\n",
                 scannedDevices[index].name.c_str(),
                 scannedDevices[index].address.c_str());
    
    // 如果已連接，先斷開
    if (SerialBT.connected()) {
        Serial.println("[BT] Disconnecting from current device...");
        SerialBT.disconnect();
        delay(1000);
    }
    
    // 解析 MAC 地址
    String macStr = scannedDevices[index].address;
    uint8_t mac[6];
    int values[6];
    
    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            mac[i] = (uint8_t)values[i];
        }
    } else {
        Serial.println("ERROR: Invalid MAC address format");
        return;
    }
    
    // 嘗試連接
    if (SerialBT.connect(mac)) {
        Serial.println("[BT] Connected successfully!");
        connectedDeviceName = scannedDevices[index].name;
        connectedDeviceAddress = scannedDevices[index].address;
        isConnected = true;
    } else {
        Serial.println("[BT] ERROR: Connection failed");
        isConnected = false;
    }
}

void disconnectBluetooth() {
    if (SerialBT.connected()) {
        Serial.println("[BT] Disconnecting...");
        SerialBT.disconnect();
        delay(500);
        Serial.println("[BT] Disconnected");
    } else {
        Serial.println("[BT] Not connected");
    }
    
    isConnected = false;
    connectedDeviceName = "";
    connectedDeviceAddress = "";
}

void printStatus() {
    Serial.println("\n--- Bluetooth Status ---");
    Serial.printf("Scanning: %s\n", isScanning ? "Yes" : "No");
    Serial.printf("Connected: %s\n", SerialBT.connected() ? "Yes" : "No");
    
    if (SerialBT.connected()) {
        Serial.printf("Device: %s\n", connectedDeviceName.c_str());
        Serial.printf("MAC: %s\n", connectedDeviceAddress.c_str());
    }
    
    Serial.printf("Scanned devices: %d\n", deviceCount);
    Serial.println("----------------------\n");
}

void printDeviceList() {
    if (deviceCount == 0) {
        Serial.println("\nNo devices scanned yet. Use 'BT_SCAN' first.\n");
        return;
    }
    
    Serial.println("\n--- Scanned Devices ---");
    for (int i = 0; i < deviceCount; i++) {
        if (scannedDevices[i].valid) {
            Serial.printf("[%d] %s\n", i, scannedDevices[i].name.c_str());
            Serial.printf("    %s (RSSI: %d dBm)\n", 
                         scannedDevices[i].address.c_str(),
                         scannedDevices[i].rssi);
        }
    }
    Serial.println("----------------------\n");
}
