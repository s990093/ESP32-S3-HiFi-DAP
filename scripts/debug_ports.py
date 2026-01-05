
import serial.tools.list_ports
import glob

print("--- serial.tools.list_ports.comports() ---")
ports = serial.tools.list_ports.comports()
for p in ports:
    print(f"{p.device} - {p.description}")

print("\n--- glob /dev/cu.* ---")
cu_ports = glob.glob('/dev/cu.*')
for p in cu_ports:
    print(p)

print("\n--- glob /dev/tty.* ---")
tty_ports = glob.glob('/dev/tty.*')
for p in tty_ports:
    print(p)
