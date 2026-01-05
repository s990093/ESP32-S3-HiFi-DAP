import subprocess
import json
import sys

def get_bluetooth_devices_mac():
    try:
        # Run system_profiler to get Bluetooth data in JSON format
        print("Scanning system for known (paired/configured) Bluetooth devices...")
        result = subprocess.run(
            ['system_profiler', '-json', 'SPBluetoothDataType'],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print("Error running system_profiler")
            return

        data = json.loads(result.stdout)
        
        # Navigate the JSON structure
        # Structure usually: data['SPBluetoothDataType'][0]['device_title'] -> list of devices
        # Or data['SPBluetoothDataType'][0]['controller_properties']['devices_list']
        # The structure can vary by macOS version, so we need to be robust.

        bt_data = data.get('SPBluetoothDataType', [])
        if not bt_data:
            print("No Bluetooth data found.")
            return

        print("\n" + "="*60)
        print(f"{'Device Name':<30} | {'MAC Address':<20} | {'Type':<10}")
        print("="*60)

        found_devices = 0

        # Helper to recursively searching for devices
        def extract_devices(obj):
            devices = []
            if isinstance(obj, dict):
                # Check if this dict looks like a device entry (has 'device_addr')
                if 'device_addr' in obj:
                     # It's a device detail
                     return [obj]
                
                # Check for known keys that contain lists of devices
                for key, value in obj.items():
                    if key == 'device_title': # Often contains list of devices
                         if isinstance(value, list):
                             for item in value:
                                 devices.extend(extract_devices(item))
                         elif isinstance(value, dict):
                             devices.extend(extract_devices(value))
                    elif isinstance(value, (dict, list)):
                        devices.extend(extract_devices(value))
            elif isinstance(obj, list):
                for item in obj:
                    devices.extend(extract_devices(item))
            return devices

        # Depending on macOS version, the structure differs significantly.
        # We'll try to walk through the 'controller_properties' or 'device_title'
        
        root_item = bt_data[0]
        
        # Strategy: Look for specific device dictionaries
        devices = []
        
        # Common location on recent macOS:
        # root -> 'device_title' (list) -> dictionary with name as key -> device info
        
        if 'device_title' in root_item:
            device_list = root_item['device_title']
            if isinstance(device_list, list):
                for device_entry in device_list:
                    # device_entry is a dict like {"Device Name": { ... info ... }}
                    if isinstance(device_entry, dict):
                        for name, info in device_entry.items():
                            if isinstance(info, dict) and 'device_addr' in info:
                                devices.append((name, info))
        
        # Fallback: Check for 'connected_devices' or 'paired_devices' keys which might exist in some versions
        if not devices:
            # Debugging: Print available keys to help diagnose
            print(f"DEBUG: No devices found in standard location. Available keys in root: {list(root_item.keys())}")
            
            # Recursive search as fallback
            print("DEBUG: Attempting recursive search...")
            flat_devices = extract_devices(root_item)
            for d in flat_devices:
                # Try to find a name for this device
                name = "Unknown"
                # Search parent keys? Hard in this flat list. 
                # We'll just list them.
                addr = d.get('device_addr', 'Unknown')
                minor = d.get('device_minorClassOfDevice_string', 'Unknown')
                print(f"{'Unknown':<30} | {addr:<20} | {minor:<10}")
                found_devices += 1
            
            if found_devices > 0:
                print("="*60)
                print(f"Found {found_devices} devices via recursive search.")
                print("Note: Names might be missing in recursive mode.")
                return

        # Fallback or other structures

        if not devices and 'controller_properties' in root_item:
             pass # Logic needed if structure is different
             
        # Print devices
        for name, info in devices:
            addr = info.get('device_addr', 'Unknown')
            # services = info.get('device_services', '')
            minor_type = info.get('device_minorClassOfDevice_string', 'Unknown')
            
            print(f"{name:<30} | {addr:<20} | {minor_type:<10}")
            found_devices += 1

        print("="*60)
        print(f"Found {found_devices} devices.")
        print("\nNOTE: This script lists 'Paired' or 'Configured' devices known to macOS.")
        print("To find a new device's MAC address:")
        print("1. Put the device in Pairing Mode.")
        print("2. Pair it with this Mac.")
        print("3. Run this script again.")
        print("4. Use the MAC address with A2DP_CONNECT command on ESP32.")
        
    except FileNotFoundError:
        print("Error: 'system_profiler' command not found. Are you on macOS?")
    except json.JSONDecodeError:
        print("Error parsing system_profiler output.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    get_bluetooth_devices_mac()
