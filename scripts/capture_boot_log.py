import serial
import time
import glob
import sys

def find_port():
    patterns = ['/dev/cu.usbserial-*', '/dev/cu.SLAB_USBtoUART', '/dev/cu.usbmodem*']
    for p in patterns:
        matches = glob.glob(p)
        if matches:
            return matches[0]
    return None

port = find_port()
if not port:
    print("No port found")
    sys.exit(1)

print(f"Connecting to {port}...")

try:
    ser = serial.Serial(port, 115200, timeout=0.1)
    
    # Toggle DTR to reset ESP32 (Standard reset)
    print("Resetting board...")
    ser.dtr = False
    time.sleep(0.1)
    ser.dtr = True
    time.sleep(0.1)
    ser.dtr = False 
    
    # Also try RTS for some boards (ESP32-CAM etc)
    # ser.rts = True
    # time.sleep(0.1)   
    # ser.rts = False

    print("Listening for 10 seconds...")
    end_time = time.time() + 10
    while time.time() < end_time:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"[LOG] {line}")
    
    ser.close()

except Exception as e:
    print(f"Error: {e}")
