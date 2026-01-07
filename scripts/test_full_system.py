import serial
import time
import json
import argparse
import sys
import os
import random
import string

# ========== UTILS ==========
def send_cmd(ser, cmd, timeout=2.0):
    print(f"\n> Sending: {cmd.strip()}")
    ser.write((cmd + "\n").encode())
    
    start = time.time()
    response = ""
    while time.time() - start < timeout:
        if ser.in_waiting:
            chunk = ser.read_all().decode(errors='ignore')
            response += chunk
            if "\n" in chunk:
                break
        time.sleep(0.05)
    
    print(f"  Response: {response.strip()}")
    return response.strip()

# ========== TESTS ==========
def test_full_system(port, baud=460800):
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2) # DTR reset wait
        ser.reset_input_buffer()
        print(f"Connected to {port}")

        # ---------------------------------------------------------
        # 1. READ APIs
        # ---------------------------------------------------------
        print("\n=== TEST PHASE 1: READ APIs ===")
        
        # ping
        res = send_cmd(ser, "ping")
        assert "pong" in res, "Ping Failed"
        print("✅ PING Passed")

        # info_json
        res = send_cmd(ser, "info_json")
        try:
            data = json.loads(res)
            assert data["device"] == "ESP32-S3-HiFi-DAP", "Device ID mismatch"
            print("✅ info_json Passed")
        except: print("❌ info_json Failed")

        # status_json
        res = send_cmd(ser, "status_json")
        if json.loads(res): print("✅ status_json Passed")
        else: print("❌ status_json Failed")

        # config_json
        res = send_cmd(ser, "config_json")
        if json.loads(res): print("✅ config_json Passed")
        else: print("❌ config_json Failed")

        # ---------------------------------------------------------
        # 2. WRITE APIs (File Management)
        # ---------------------------------------------------------
        print("\n=== TEST PHASE 2: WRITE / UPLOAD / DELETE ===")
        
        test_filename = "/api_test_song.bin"
        renamed_filename = "/api_test_renamed.bin"
        
        # A. Create Dummy File
        payload_size = 4096 # 4KB
        dummy_data = os.urandom(payload_size)
        with open("temp_test_payload.bin", "wb") as f:
            f.write(dummy_data)
            
        # B. Upload File (Using raw protocol here for integration test)
        print(f"> Uploading {test_filename} ({payload_size} bytes)...")
        ser.write(f"upload {test_filename} {payload_size}\n".encode())
        
        # Wait for READY
        ready = False
        start = time.time()
        while time.time() - start < 2:
            if "READY" in ser.read_all().decode(errors='ignore'):
                ready = True
                break
        
        if ready:
            print("  Device READY. Streaming...")
            time.sleep(0.1)
            ser.write(dummy_data)
            time.sleep(0.5) # Wait for write
            
            # Wait for SUCCESS
            start = time.time()
            success = False
            while time.time() - start < 5:
                res = ser.read_all().decode(errors='ignore')
                if "SUCCESS" in res:
                    success = True
                    break
            
            if success: print("✅ Upload Passed")
            else: print("❌ Upload Failed (No SUCCESS msg)")
        else:
            print("❌ Upload Failed (No READY msg)")
            
        # C. Verify File Exists (list_json)
        print("> Verifying existence...")
        ser.reset_input_buffer()
        res = send_cmd(ser, "list_json")
        file_found = False
        if test_filename[1:] in res: # Remove leading slash for search
             file_found = True
             print(f"✅ File {test_filename} confirmed in list")
        else:
             print(f"❌ File {test_filename} not found in list")

        # D. Rename File
        print(f"> Renaming to {renamed_filename}...")
        res = send_cmd(ser, f"rename {test_filename} {renamed_filename}")
        if "SUCCESS" in res: print("✅ Rename Passed")
        else: print("❌ Rename Failed")

        # E. Verify Rename
        res = send_cmd(ser, "list_json")
        if renamed_filename[1:] in res and test_filename[1:] not in res:
             print("✅ Rename Verification Passed")
        else:
             print("❌ Rename Verification Failed")
             
        # F. Delete File
        print(f"> Deleting {renamed_filename}...")
        res = send_cmd(ser, f"delete {renamed_filename}")
        if "SUCCESS" in res: print("✅ Delete Passed")
        else: print("❌ Delete Failed")
        
        # G. Verify Delete
        res = send_cmd(ser, "list_json")
        if renamed_filename[1:] not in res:
             print("✅ Delete Verification Passed")
        else:
             print("❌ Delete Verification Failed (File still exists)")

        print("\n=== ALL TESTS COMPLETED ===")
        ser.close()
        os.remove("temp_test_payload.bin")

    except Exception as e:
        print(f"CRITICAL ERROR: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 test_full_system.py <port>")
        sys.exit(1)
    test_full_system(sys.argv[1])
