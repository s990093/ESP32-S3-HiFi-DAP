#!/usr/bin/env python3
"""
Arduino 程式編譯與燒入模組（美化版）
支援 .ino 檔案的編譯和上傳，使用 Rich 美化終端輸出
"""

import subprocess
import os
import sys
import time
from pathlib import Path
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, TaskProgressColumn
from rich.panel import Panel
from rich.text import Text
from rich import print as rprint
import re
import click

console = Console()

# FQBN Map for common boards
BOARD_FQBN = {
    'uno': 'arduino:avr:uno',
    'nano': 'arduino:avr:nano',
    'nano_old': 'arduino:avr:nano:cpu=atmega328old',
    'mega': 'arduino:avr:mega',
    'mega2560': 'arduino:avr:mega:cpu=atmega2560',
    'leonardo': 'arduino:avr:leonardo',
    'micro': 'arduino:avr:micro',
    'mini': 'arduino:avr:mini',
    # ESP32 Boards
    'esp32': 'esp32:esp32:esp32:PartitionScheme=huge_app',
    'esp32_with_psram': 'esp32:esp32:esp32:UploadSpeed=115200,FlashMode=dio,FlashFreq=40,PSRAM=enabled,PartitionScheme=huge_app',
    'esp32s3': 'esp32:esp32:esp32s3',
}


def find_serial_port():
    """
    Auto-detect ESP32 serial port.
    
    Returns:
        str: Serial port path or None
    """
    port_patterns = [
        '/dev/cu.usbserial-*',
        '/dev/cu.SLAB_USBtoUART',
        '/dev/cu.usbmodem*',
        '/dev/cu.wchusbserial*',
    ]
    
    for pattern in port_patterns:
        try:
            result = subprocess.run(
                f'ls {pattern} 2>/dev/null',
                shell=True,
                capture_output=True,
                text=True
            )
            if result.stdout.strip():
                port = result.stdout.strip().split('\n')[0]
                return port
        except Exception:
            continue
    
    return None


def run_build_script():
    """
    Executes the build.py script to prepare the build directory.
    """
    build_script = Path("build.py").resolve()
    if not build_script.exists():
        # Try looking one level up if we are in scripts/
        build_script = Path("../build.py").resolve()
    
    if not build_script.exists():
        console.print("[bold red]✗ Error:[/bold red] build.py not found in current or parent directory.")
        return False

    console.print(Panel(f"[cyan]Running Build Script: {build_script}[/cyan]", title="[bold blue]🔨 Build Step[/bold blue]", border_style="blue"))
    
    try:
        # Run build.py using the same python interpreter
        result = subprocess.run([sys.executable, str(build_script)], capture_output=True, text=True)
        
        if result.returncode != 0:
            console.print("[bold red]✗ Build Failed![/bold red]")
            console.print(result.stderr)
            console.print(result.stdout)
            return False
            
        console.print(result.stdout)
        console.print("[bold green]✓ Build Preparation Complete[/bold green]")
        return True
    except Exception as e:
        console.print(f"[bold red]✗ Build Script Error:[/bold red] {e}")
        return False

def compile_sketch(sketch_path, fqbn='esp32:esp32:esp32', build_path=None, verbose=False, optimize=True):
    """
    Compile Arduino sketch.
    """
    sketch_path = Path(sketch_path).resolve()
    
    if sketch_path.is_dir():
        ino_files = list(sketch_path.glob('*.ino'))
        if not ino_files:
            console.print(f"[bold red]✗ Error:[/bold red] No .ino files found in {sketch_path}")
            return False
        sketch_path = ino_files[0]
    
    if not sketch_path.exists():
        console.print(f"[bold red]✗ Error:[/bold red] File not found: {sketch_path}")
        return False
    
    # Build Info Panel
    from rich.table import Table
    
    grid = Table.grid(expand=True)
    grid.add_column(style="cyan bold")
    grid.add_column(style="white")
    
    grid.add_row("Sketch:", sketch_path.name)
    grid.add_row("Board:", fqbn)
    if build_path:
        grid.add_row("Output:", build_path)

    console.print(Panel(grid, title="[bold blue]🚀 Build Configuration[/bold blue]", border_style="blue"))
    
    cmd = ['arduino-cli', 'compile', '--fqbn', fqbn, str(sketch_path)]
    
    # Auto-clean and disable optimize for sensitive configs
    if 'PSRAM=disabled' in fqbn:
        cmd.append('--clean')
        optimize = False
        console.print("[yellow]🧹 Clean build forced & Optimizations disabled (PSRAM disabled)[/yellow]")
    
    if build_path:
        Path(build_path).mkdir(parents=True, exist_ok=True)
        cmd.extend(['--output-dir', str(build_path)])
        
    if optimize:
        optimization_flags = [
            '--build-property', 'compiler.c.extra_flags=-O3 -funroll-loops -finline-functions',
            '--build-property', 'compiler.cpp.extra_flags=-O3 -funroll-loops -finline-functions -ffast-math',
            '--build-property', 'compiler.c.elf.extra_flags=-O3',
        ]
        cmd.extend(optimization_flags)
        console.print("[yellow]⚡ Performance Optimizations Enabled (-O3, LTO)[/yellow]")
    
    if verbose:
        cmd.append('--verbose')
    
    try:
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            BarColumn(),
            TaskProgressColumn(),
            console=console
        ) as progress:
            task = progress.add_task("[cyan]Compiling...[/cyan]", total=100)
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True
            )
            
            stdout_full = []
            
            # Regex patterns for stats
            flash_pattern = re.compile(r"Sketch uses (\d+) bytes.*Maximum is (\d+) bytes")
            ram_pattern = re.compile(r"Global variables use (\d+) bytes.*Maximum is (\d+) bytes")
            
            stats = {}
            current_progress = 0

            for line in process.stdout:
                line_str = line.strip()
                stdout_full.append(line_str)
                
                if verbose:
                    console.print(line_str, style="dim")
                
                # Parse Stats
                flash_match = flash_pattern.search(line_str)
                if flash_match:
                    stats['flash_used'] = int(flash_match.group(1))
                    stats['flash_total'] = int(flash_match.group(2))
                
                ram_match = ram_pattern.search(line_str)
                if ram_match:
                    stats['ram_used'] = int(ram_match.group(1))
                    stats['ram_total'] = int(ram_match.group(2))

                # Update progress
                if current_progress < 95:
                    progress.update(task, advance=2)
                    current_progress += 2
            
            process.wait()
            progress.update(task, completed=100)
            
            if process.returncode == 0:
                console.print("[bold green]✓ Compilation Successful![/bold green]")
                
                # Display Stats
                if 'flash_used' in stats:
                    flash_used = stats['flash_used']
                    flash_total = stats['flash_total']
                    flash_pct = (flash_used / flash_total) * 100
                    
                    ram_used = stats['ram_used']
                    ram_total = stats['ram_total']
                    ram_pct = (ram_used / ram_total) * 100
                    
                    # Create Stat Table
                    stat_table = Table(box=None, show_header=True, header_style="bold cyan", padding=(0, 2))
                    stat_table.add_column("Resource")
                    stat_table.add_column("Used", justify="right")
                    stat_table.add_column("Total", justify="right")
                    stat_table.add_column("Usage", justify="right")
                    
                    def get_color(pct):
                        return "green" if pct < 70 else "yellow" if pct < 90 else "red"

                    stat_table.add_row(
                        "Flash Memory", 
                        f"{flash_used:,}", 
                        f"{flash_total:,}", 
                        f"[{get_color(flash_pct)}]{flash_pct:.1f}%[/{get_color(flash_pct)}]"
                    )
                    stat_table.add_row(
                        "Global Variables (SRAM)", 
                        f"{ram_used:,}", 
                        f"{ram_total:,}", 
                        f"[{get_color(ram_pct)}]{ram_pct:.1f}%[/{get_color(ram_pct)}]"
                    )
                                        
                    console.print(Panel(stat_table, title="[bold]📊 Resource Usage[/bold]", border_style="cyan"))
                
                return True
            else:
                console.print("[bold red]✗ Compilation Failed![/bold red]")
                if not verbose:
                     console.print("\n".join(stdout_full[-20:]), style="dim red")
                return False
        
    except FileNotFoundError:
        console.print("[bold red]✗ Error:[/bold red] arduino-cli not found. Please install it first.")
        console.print("  https://arduino.github.io/arduino-cli/")
        return False
    except Exception as e:
        console.print(f"[bold red]✗ Error:[/bold red] {e}")
        return False


def monitor_boot_status(port, baud=115200, duration=5):
    """
    Monitor serial for boot messages
    """
    try:
        import serial
    except ImportError:
        console.print("[yellow]⚠️  pyserial not found, skipping boot check[/yellow]")
        return

    console.print(f"\n[cyan]Connecting to {port} for Boot Diagnostic...[/cyan]")
    
    try:
        with serial.Serial(port, baud, timeout=0.1) as ser:
            # Reset Sequence
            ser.dtr = False
            ser.rts = False
            time.sleep(0.1)
            ser.dtr = True
            ser.rts = True
            time.sleep(0.1)
            ser.dtr = False
            ser.rts = False
            
            start_time = time.time()
            buffer = ""
            
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                console=console
            ) as progress:
                task = progress.add_task("Listening for boot log...", total=None)
                
                while time.time() - start_time < duration:
                    if ser.in_waiting:
                        try:
                            c = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                            buffer += c
                            # Only print if not just whitespace/newlines to keep output clean
                            if c.strip():
                                sys.stdout.write(c) 
                        except Exception:
                            pass
                    time.sleep(0.01)
            
            # Analyze
            from rich.table import Table
            diag_table = Table(show_header=False, box=None, padding=(0,2))
            diag_table.add_column("Item")
            diag_table.add_column("Status")
            
            if "PSRAM" in buffer or "SPIRAM" in buffer:
                diag_table.add_row("PSRAM", "[bold green]DETECTED[/bold green]")
            else:
                diag_table.add_row("PSRAM", "[yellow]NOT DETECTED[/yellow]")
                
            if "Guru Meditation Error" in buffer:
                diag_table.add_row("System Integrity", "[bold red]CRASH DETECTED[/bold red]")
            else:
                diag_table.add_row("System Integrity", "[green]STABLE[/green]")

            console.print(Panel(diag_table, title="[bold]🔍 System Diagnostics[/bold]", border_style="blue"))
            
    except Exception as e:
        console.print(f"[red]Serial Port Error: {e}[/red]")


def upload_sketch_esp32(sketch_path, port, build_path, verbose=False, baud=115200):
    """
    ESP32 Upload
    """
    sketch_path = Path(sketch_path).resolve()
    build_dir = Path(build_path).resolve()
    sketch_name = sketch_path.stem
    
    # Check Binaries
    bin_file = build_dir / f'{sketch_name}.ino.bin'
    bootloader = build_dir / f'{sketch_name}.ino.bootloader.bin'
    partitions = build_dir / f'{sketch_name}.ino.partitions.bin'
    boot_app0 = build_dir / 'boot_app0.bin'
    
    if not boot_app0.exists():
        esp32_hardware_path = Path.home() / 'Library' / 'Arduino15' / 'packages' / 'esp32' / 'hardware' / 'esp32'
        esp32_versions = sorted(esp32_hardware_path.glob('*'), key=lambda x: x.stat().st_mtime, reverse=True)
        if esp32_versions:
            boot_app0 = esp32_versions[0] / 'tools' / 'partitions' / 'boot_app0.bin'
    
    if not all([bin_file.exists(), bootloader.exists(), partitions.exists(), boot_app0.exists()]):
        console.print(f"[bold red]✗ Error:[/bold red] Missing build artifacts in {build_dir}")
        return False

    # Find esptool
    esptool_path = Path.home() / 'Library' / 'Arduino15' / 'packages' / 'esp32' / 'tools' / 'esptool_py'
    esptool_versions = list(esptool_path.glob('*'))
    if not esptool_versions:
        console.print('[bold red]✗ Error:[/bold red] esptool not found')
        return False
    esptool = esptool_versions[0] / 'esptool'

    # Upload Info Panel
    from rich.table import Table
    grid = Table.grid(expand=True)
    grid.add_column(style="green")
    grid.add_column(style="white")
    grid.add_row("Target:", str(sketch_name))
    grid.add_row("Port:", port)
    
    console.print(Panel(grid, title="[bold green]📤 Starting Upload[/bold green]", border_style="green"))

    cmd = [
        str(esptool),
        '--chip', 'esp32',
        '--port', port,
        '--baud', str(baud),
        '--before', 'default_reset',
        '--after', 'hard_reset',
        'write_flash', '-z',
        '--flash_mode', 'dio',
        '--flash_freq', '40m',
        '--flash_size', 'detect',
        '0x1000', str(bootloader),
        '0x8000', str(partitions),
        '0xe000', str(boot_app0),
        '0x10000', str(bin_file)
    ]
    
    try:
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            BarColumn(),
            TaskProgressColumn(),
            console=console
        ) as progress:
            task = progress.add_task("[green]Uploading...[/green]", total=100)
            
            process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            
            current_progress = 0
            for line in process.stdout:
                if verbose:
                    console.print(line.rstrip(), style="dim")
                # Progress simulation/parsing is tricky for esptool, simple advance for now
                if current_progress < 95:
                    progress.update(task, advance=2)
                    current_progress += 2
            
            process.wait()
            progress.update(task, completed=100)
            
            if process.returncode == 0:
                console.print("[bold green]✓ Upload Complete![/bold green]")
                return True
            else:
                console.print("[bold red]✗ Upload Failed![/bold red]")
                return False
    except Exception as e:
        console.print(f"[bold red]✗ Upload Error:[/bold red] {e}")
        return False


def compile_and_upload(sketch_path, port=None, fqbn='esp32:esp32:esp32', verbose=False, monitor=True, baud=115200):
    """
    Orchestrate Compile and Upload
    """
    build_dir = Path("build").resolve()
    
    # Validate Port if provided
    if port and not Path(port).exists():
        console.print(f"[bold red]✗ Error:[/bold red] Serial port '{port}' not found.")
        return False
    
    # Compile
    success = compile_sketch(
        sketch_path, 
        fqbn, 
        build_path=str(build_dir), 
        verbose=verbose,
        optimize=True
    )
    
    if not success:
        return False
    
    # Auto-detect Port
    if port is None:
        console.print("\n[cyan]Auto-detecting port...[/cyan]")
        port = find_serial_port()
        if port is None:
            console.print("[bold red]✗ Error:[/bold red] No USB serial port found.")
            return False
        console.print(f"[green]✓ Found Port: {port}[/green]")
    
    # Upload
    upload_success = False
    if 'esp32' in fqbn.lower():
        upload_success = upload_sketch_esp32(sketch_path, port, str(build_dir), verbose, baud)
    else:
        cmd = ['arduino-cli', 'upload', '-p', port, '--fqbn', fqbn, '--input-dir', str(build_dir)]
        try:
            subprocess.run(cmd, check=True)
            console.print("[bold green]✓ Upload Complete![/bold green]")
            upload_success = True
        except subprocess.CalledProcessError:
            console.print("[bold red]✗ Upload Failed![/bold red]")
            upload_success = False
            
    if upload_success and monitor:
        monitor_boot_status(port)
        
    return upload_success


def get_fqbn_from_board_name(board_name):
    return BOARD_FQBN.get(board_name.lower(), 'esp32:esp32:esp32')


@click.command()
@click.argument('sketch', type=click.Path(exists=True), default=".")
@click.option('--port', '-p', help='Serial port detection (Auto-detect if not provided)')
@click.option('--board', '-b', default='esp32', show_default=True, help='Board type (esp32, esp32_nopsram, uno, etc)')
@click.option('--baud', default=115200, show_default=True, help='Upload speed (baud rate)')
@click.option('--verbose', '-v', is_flag=True, default=True, help='Show verbose output')
@click.option('--monitor/--no-monitor', default=True, show_default=True, help='Monitor serial output after upload')
@click.option('--no-build', is_flag=True, default=False, help='Skip build.py step (for test sketches)')
def cli(sketch, port, board, baud, verbose, monitor, no_build):
    """
    ESP32 Professional Build & Upload Tool
    
    Powered by Arduino CLI & Esptool
    """
    console.print(Panel.fit(
        "[bold cyan]ESP32 Professional Build & Upload Tool II[/bold cyan]\n"
        "[dim]Powered by Arduino CLI & Click[/dim]",
        border_style="cyan"
    ))

    fqbn = get_fqbn_from_board_name(board)
    
    # 1. Run the build script (unless --no-build is specified)
    if not no_build:
        if not run_build_script():
            console.print("[bold red]Aborting: Build step failed.[/bold red]")
            sys.exit(1)
            
        # 2. Redirect sketch path to the build directory
        build_dir = Path("build").resolve()
        if not build_dir.exists():
             build_dir = Path("../build").resolve()
             
        target_sketch = build_dir / "build.ino"
        
        if not target_sketch.exists():
             console.print(f"[bold red]✗ Error:[/bold red] Generated sketch not found at {target_sketch}")
             sys.exit(1)
             
        console.print(f"[dim]Redirecting compilation to generated sketch: {target_sketch}[/dim]")
        sketch = target_sketch
    else:
        console.print("[yellow]⚠  Skipping build step (--no-build)[/yellow]")
    
    success = compile_and_upload(sketch, port, fqbn, verbose, monitor, baud)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    cli()
