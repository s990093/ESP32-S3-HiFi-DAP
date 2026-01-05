
import sys
import os
import time
import glob
sys.path.append(os.path.join(os.getcwd(), "music-manager"))
from backend.serial_comm import ESP32Serial

def debug_print(msg):
    print(f"[DEBUG] {msg}")

# 1. Find Port
print("--- Finding Port ---")
esp = ESP32Serial()
ports = esp.list_ports()
port = ports.get('recommended')

if not port:
    # Fallback search
    patterns = ['/dev/cu.usbserial-*', '/dev/cu.SLAB_USBtoUART', '/dev/cu.usbmodem*']
    for p in patterns:
        matches = glob.glob(p)
        if matches:
            port = matches[0]
            break

if not port:
    print("ERROR: No port found.")
    sys.exit(1)

print(f"Target Port: {port}")

# 2. Connect
print("--- Connecting ---")
if esp.connect(port):
    print("Connected.")
else:
    print("Failed to connect.")
    sys.exit(1)

# 3. Check SD Card (LIST command)
print("--- Checking SD Card Status ---")
try:
    # Send LIST command
    response = esp.send_command("LIST")
    print(f"Response: {response}")
    
    if response and response.startswith("OK"):
        print("SD Card Status: OK (Readable)")
        print(f"Files: {response[3:]}")
    else:
        print("SD Card Status: ERROR (Not Readable)")
        print(f"Detail: {response}")

except Exception as e:
    print(f"Exception checking status: {e}")

esp.disconnect()
