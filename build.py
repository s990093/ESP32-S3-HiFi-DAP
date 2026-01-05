import os
import shutil
import sys
from pathlib import Path

# Configuration
SOURCE_DIR = "src"
BUILD_DIR = "build"
SKETCH_NAME = "build.ino"

def clean_build_dir():
    """Removes the existing build directory."""
    if os.path.exists(BUILD_DIR):
        print(f"Cleaning {BUILD_DIR}...")
        shutil.rmtree(BUILD_DIR)
    os.makedirs(BUILD_DIR)

def process_files():
    """Copies files from src to build, renaming .c to .cpp and handling main.c."""
    print(f"Processing source files from {SOURCE_DIR} to {BUILD_DIR}...")
    
    for root, _, files in os.walk(SOURCE_DIR):
        for file in files:
            src_path = Path(root) / file
            
            # Determine destination filename and extension
            if src_path.name == "main.c" and root.endswith("sys"):
                # Special case: src/sys/main.c -> build/build.ino
                dest_name = SKETCH_NAME
            elif src_path.suffix == ".ino":
                # Treat any .ino file as the main sketch
                dest_name = SKETCH_NAME
            elif src_path.suffix == ".c":
                # General case: .c -> .cpp (masquerade)
                dest_name = src_path.with_suffix(".cpp").name
            else:
                # Keep other files (headers, etc.) as is
                dest_name = src_path.name
            
            dest_path = Path(BUILD_DIR) / dest_name
            
            # Check for name collisions (since we are flattening)
            if dest_path.exists():
                print(f"WARNING: Name collision for {dest_name} (from {src_path})")
            
            shutil.copy2(src_path, dest_path)
            print(f"  {src_path} -> {dest_path}")

def main():
    try:
        clean_build_dir()
        process_files()
        print("\nBuild preparation complete.")
        print(f"You can now compile the '{BUILD_DIR}' directory with Arduino.")
        print(f"Example: arduino-cli compile --fqbn esp32:esp32:esp32 {BUILD_DIR}")
    except Exception as e:
        print(f"\nError: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
