#!/usr/bin/env python3
"""
ESP32-S3 HiFi-DAP Audio Converter
Supports high-quality audio conversion with FLAC support
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn
from rich.table import Table
from rich import print as rprint
import re

console = Console()

# Maximum quality settings
QUALITY_PRESETS = {
    'wav': {
        'codec': 'pcm_s16le',
        'sample_rate': '44100',
        'channels': '2',
        'bit_depth': '16'
    },
    'flac': {
        'codec': 'flac',
        'sample_rate': '44100',
        'channels': '2',
        'compression_level': '8',  # 0-12, 8 is default
        'lpc_type': 'cheby',       # Best for compression
        'exact_rice_param': True
    }
}

SUPPORTED_INPUT = ['.mp3', '.m4a', '.aac', '.flac', '.wav', '.ogg', '.wma', '.ape', '.alac']
SUPPORTED_OUTPUT = ['wav', 'flac']

def check_ffmpeg():
    """Check if FFmpeg is installed"""
    try:
        result = subprocess.run(
            ["ffmpeg", "-version"], 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            check=True,
            text=True
        )
        version = result.stdout.split('\n')[0]
        console.print(f"[green]✓ {version}[/green]")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def get_audio_info(file_path):
    """Get audio file information using ffprobe"""
    try:
        cmd = [
            'ffprobe',
            '-v', 'quiet',
            '-print_format', 'json',
            '-show_format',
            '-show_streams',
            file_path
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        import json
        data = json.loads(result.stdout)
        
        # Find audio stream
        audio_stream = None
        for stream in data.get('streams', []):
            if stream.get('codec_type') == 'audio':
                audio_stream = stream
                break
        
        if audio_stream:
            return {
                'codec': audio_stream.get('codec_name', 'unknown'),
                'sample_rate': audio_stream.get('sample_rate', 'unknown'),
                'channels': audio_stream.get('channels', 'unknown'),
                'bit_rate': audio_stream.get('bit_rate', 'unknown'),
                'duration': float(data.get('format', {}).get('duration', 0))
            }
    except:
        return None

def convert_to_wav(input_path, output_path=None, verbose=False):
    """
    Convert audio to WAV format (16-bit PCM, 44.1kHz, Stereo)
    Maximum quality for ESP32 playback
    """
    if not os.path.exists(input_path):
        console.print(f"[red]❌ File not found: {input_path}[/red]")
        return False

    if output_path is None:
        output_path = Path(input_path).with_suffix('.wav')

    preset = QUALITY_PRESETS['wav']
    
    cmd = [
        'ffmpeg',
        '-i', str(input_path),
        '-acodec', preset['codec'],
        '-ar', preset['sample_rate'],
        '-ac', preset['channels'],
        '-sample_fmt', 's16',  # 16-bit signed
        '-y',
        '-loglevel', 'error' if not verbose else 'info',
        str(output_path)
    ]

    try:
        subprocess.run(cmd, check=True, capture_output=not verbose)
        return True
    except subprocess.CalledProcessError:
        return False

def convert_to_flac(input_path, output_path=None, verbose=False):
    """
    Convert audio to FLAC format (lossless, maximum quality)
    Best for archival and high-fidelity playback
    """
    if not os.path.exists(input_path):
        console.print(f"[red]❌ File not found: {input_path}[/red]")
        return False

    if output_path is None:
        output_path = Path(input_path).with_suffix('.flac')

    preset = QUALITY_PRESETS['flac']
    
    cmd = [
        'ffmpeg',
        '-i', str(input_path),
        '-c:a', preset['codec'],
        '-ar', preset['sample_rate'],
        '-ac', preset['channels'],
        '-compression_level', preset['compression_level'],
        '-sample_fmt', 's32',  # 32-bit signed for maximum quality
        '-y',
        '-loglevel', 'error' if not verbose else 'info',
        str(output_path)
    ]

    try:
        subprocess.run(cmd, check=True, capture_output=not verbose)
        return True
    except subprocess.CalledProcessError:
        return False

def convert_audio(input_path, output_format='wav', output_path=None, verbose=False):
    """Main conversion function"""
    input_path = Path(input_path)
    
    if not input_path.exists():
        console.print(f"[red]❌ File not found: {input_path}[/red]")
        return False
    
    if input_path.suffix.lower() not in SUPPORTED_INPUT:
        console.print(f"[yellow]⚠️  Unsupported format: {input_path.suffix}[/yellow]")
        return False
    
    # Get input info
    info = get_audio_info(str(input_path))
    
    console.print(f"\n[cyan]🎵 Input:[/cyan] {input_path.name}")
    if info:
        console.print(f"   [dim]Codec: {info['codec']}, "
                     f"Sample Rate: {info['sample_rate']}Hz, "
                     f"Channels: {info['channels']}[/dim]")
    
    # Convert based on format
    if output_format == 'wav':
        success = convert_to_wav(input_path, output_path, verbose)
        target_format = "WAV (16-bit PCM, 44.1kHz, Stereo)"
    elif output_format == 'flac':
        success = convert_to_flac(input_path, output_path, verbose)
        target_format = "FLAC (Lossless, 44.1kHz, Stereo)"
    else:
        console.print(f"[red]❌ Unknown output format: {output_format}[/red]")
        return False
    
    if success:
        out_file = output_path if output_path else input_path.with_suffix(f'.{output_format}')
        out_size = os.path.getsize(out_file) / (1024 * 1024)
        console.print(f"[green]✓ Output:[/green] {Path(out_file).name} ({out_size:.2f} MB)")
        console.print(f"   [dim]Format: {target_format}[/dim]")
        return True
    else:
        console.print(f"[red]❌ Conversion failed[/red]")
        return False


def sanitize_filename(path):
    """
    Sanitize filename to ensure it's safe for filesystems and ESP32.
    - Removes illegal characters
    - Truncates to safe length (100 chars)
    - Returns new path if changed, else None
    """
    path = Path(path)
    directory = path.parent
    original_name = path.name
    stem = path.stem
    suffix = path.suffix
    
    # 1. Remove illegal characters for FAT32/ExFAT
    # Illegal: " * / : < > ? \ |
    clean_stem = re.sub(r'[<>:"/\\|?*]', '_', stem)
    
    # 2. Check Length (keep < 100 chars for safety)
    if len(clean_stem) > 200:
        clean_stem = clean_stem[:200]
        
    clean_name = f"{clean_stem}{suffix}"
    
    if clean_name != original_name:
        new_path = directory / clean_name
        return new_path
    
    return None

def process_input(input_path, output_format, output_path, verbose):
    """Process a single input file or directory"""
    input_path = Path(input_path)
    
    if input_path.is_dir():
        console.print(f"[bold cyan]📂 Scanning Directory:[/bold cyan] {input_path}")
        results = []
        # glob all supported audio files
        files = []
        for ext in SUPPORTED_INPUT:
            files.extend(list(input_path.glob(f"*{ext}")))
        
        if not files:
            console.print("[yellow]No audio files found in directory.[/yellow]")
            return []
            
        console.print(f"Found {len(files)} audio files.")
        
        for file_path in files:
            # 1. Sanitize Filename first
            new_path = sanitize_filename(file_path)
            
            final_path = file_path
            if new_path:
                try:
                    console.print(f"[yellow]⚠️  Renaming:[/yellow] {file_path.name} -> {new_path.name}")
                    file_path.rename(new_path)
                    final_path = new_path
                except OSError as e:
                    console.print(f"[red]❌ Rename failed:[/red] {e}")
                    continue
            
            # 2. Convert
            success = convert_audio(
                final_path,
                output_format=output_format,
                output_path=None, # Auto-generate output path
                verbose=verbose
            )
            results.append((final_path, success))
            
        return results
    else:
        # Single file
        # 1. Sanitize
        new_path = sanitize_filename(input_path)
        final_path = input_path
        if new_path:
             try:
                console.print(f"[yellow]⚠️  Renaming:[/yellow] {input_path.name} -> {new_path.name}")
                input_path.rename(new_path)
                final_path = new_path
             except OSError as e:
                console.print(f"[red]❌ Rename failed:[/red] {e}")
                return [(input_path, False)]
        
        success = convert_audio(
            final_path, 
            output_format=output_format, 
            output_path=output_path, 
            verbose=verbose
        )
        return [(final_path, success)]

def main():
    parser = argparse.ArgumentParser(
        description='ESP32-S3 HiFi-DAP Audio Converter with FLAC support',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Convert a folder of files
  python3 audio_converter.py /path/to/music_folder

  # Convert to WAV (for ESP32 playback)
  python3 audio_converter.py song.mp3
  
  # Convert to FLAC (lossless archival)
  python3 audio_converter.py song.mp3 --format flac
  
  # Batch convert multiple files
  python3 audio_converter.py *.mp3 --format wav
        '''
    )
    
    parser.add_argument('inputs', nargs='+', help='Input audio files or directories')
    parser.add_argument('-f', '--format', choices=SUPPORTED_OUTPUT, default='wav',
                       help='Output format (default: wav)')
    parser.add_argument('-o', '--output', help='Output file path (single file only)')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose FFmpeg output')
    
    args = parser.parse_args()

    # Check FFmpeg
    console.print("\n[bold cyan]ESP32-S3 HiFi-DAP Audio Converter[/bold cyan]")
    console.print("[dim]Maximum quality audio conversion + Filename Sanitizer[/dim]\n")
    
    if not check_ffmpeg():
        console.print("[red]❌ FFmpeg not found![/red]")
        console.print("Install with: [cyan]brew install ffmpeg[/cyan]")
        sys.exit(1)

    # Check output path constraint
    if args.output and (len(args.inputs) > 1 or Path(args.inputs[0]).is_dir()):
        console.print("[red]❌ Cannot specify --output with multiple files or directory input[/red]")
        sys.exit(1)

    # Convert files
    all_results = []
    
    for input_item in args.inputs:
        results = process_input(input_item, args.format, args.output, args.verbose)
        all_results.extend(results)

    # Summary
    if all_results:
        console.print(f"\n[bold]Summary[/bold]")
        success_count = sum(1 for _, success in all_results if success)
        console.print(f"Converted: [green]{success_count}[/green]/{len(all_results)} files")
        
        failed = [(f, s) for f, s in all_results if not s]
        if failed:
            console.print("\n[yellow]Failed files:[/yellow]")
            for file, success in failed:
                 console.print(f"  • {file.name}")

if __name__ == '__main__':
    main()

