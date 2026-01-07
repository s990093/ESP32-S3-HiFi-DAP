import serial
import time
import sys
import os
import argparse

def upload_file(port, local_path, remote_path, baudrate=460800):
    if not os.path.exists(local_path):
        print(f"Error: Local file not found: {local_path}")
        return

    file_size = os.path.getsize(local_path)
    print(f"Connecting to {port}...")
    
    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2) # Wait for DTR
        ser.reset_input_buffer()

        # Step 1: Send Upload Command
        cmd = f"upload {remote_path} {file_size}\n"
        print(f"Sending command: {cmd.strip()}")
        ser.write(cmd.encode())
        
        # Step 2: Wait for READY
        print("Waiting for READY...")
        start_wait = time.time()
        ready = False
        while time.time() - start_wait < 3.0:
            if ser.in_waiting:
                line = ser.readline().decode(errors='ignore').strip()
                print(f"Device: {line}")
                if "READY" in line:
                    ready = True
                    break
        
        if not ready:
            print("Error: Device did not respond with READY")
            return

        # Step 3: Stream Binary Data
        print(f"Uploading {file_size} bytes...")
        with open(local_path, "rb") as f:
            sent = 0
            while sent < file_size:
                chunk = f.read(64) # Small chunks for safety
                if not chunk:
                    break
                ser.write(chunk)
                sent += len(chunk)
                # print(f"\rProgress: {sent/file_size*100:.1f}%", end="")
                time.sleep(0.001) # Tiny delay to prevent overflowing ESP32 buffer
        
        print("\nUpload complete. Waiting for confirmation...")
        
        # Step 4: Wait for SUCCESS
        start_wait = time.time()
        success = False
        while time.time() - start_wait < 5.0:
            if ser.in_waiting:
                line = ser.readline().decode(errors='ignore').strip()
                print(f"Device: {line}")
                if "SUCCESS" in line:
                    success = True
                    break
                if "ERROR" in line:
                    print("Upload failed reported by device.")
                    break
        
        if success:
            print("✅ File uploaded successfully!")
        else:
            print("❌ Upload validation failed.")

        ser.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Upload file to ESP32 via Serial")
    parser.add_argument("port", help="Serial port")
    parser.add_argument("local_file", help="Path to local file")
    parser.add_argument("remote_path", help="Path on SD card (e.g., /song.mp3)")
    parser.add_argument("--baud", type=int, default=460800, help="Baud rate")
    
    args = parser.parse_args()
    
    upload_file(args.port, args.local_file, args.remote_path, args.baud)
