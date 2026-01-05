#!/usr/bin/env python3
"""
ESP32 Music Upload Tool - 完整的音訊檔案上傳工具
支援多種音訊格式轉 WAV,並上傳到 ESP32 SD 卡

用法:
    python debug_upload.py                              # 互動模式
    python debug_upload.py <file_path>                  # 單檔上傳
    python debug_upload.py <folder_path> --batch        # 批次上傳
    python debug_upload.py <file_path> --port /dev/...  # 指定埠
"""

import sys
import os
import time
import glob
import subprocess
import argparse
from pathlib import Path
from typing import Optional, List, Callable

# Add backend path
sys.path.append(os.path.join(os.getcwd(), "music-manager"))
from backend.serial_comm import ESP32Serial

# Try to import Rich for beautiful output
try:
    from rich.console import Console
    from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TimeRemainingColumn
    from rich.panel import Panel
    from rich.table import Table
    from rich import print as rprint
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("提示: 安裝 Rich 可獲得更好的顯示效果: pip install rich")

# Supported audio formats
SUPPORTED_FORMATS = {
    '.mp3', '.flac', '.m4a', '.aac', '.ogg', 
    '.wma', '.opus', '.wav', '.aiff'
}

# WAV conversion settings
WAV_SAMPLE_RATE = 44100
WAV_CHANNELS = 2
WAV_FORMAT = 's16'  # 16-bit signed PCM


class MusicUploader:
    """音訊檔案上傳器"""
    
    def __init__(self, port: Optional[str] = None, use_rich: bool = True):
        self.esp = ESP32Serial()
        self.port = port
        self.use_rich = use_rich and RICH_AVAILABLE
        self.console = Console() if self.use_rich else None
        self.temp_files = []  # Track temp files for cleanup
        
    def log(self, message: str, style: str = ""):
        """統一的日誌輸出"""
        if self.use_rich:
            self.console.print(message, style=style)
        else:
            print(message)
    
    def log_error(self, message: str):
        """錯誤日誌"""
        self.log(f"❌ {message}", "bold red")
    
    def log_success(self, message: str):
        """成功日誌"""
        self.log(f"✅ {message}", "bold green")
    
    def log_info(self, message: str):
        """資訊日誌"""
        self.log(f"ℹ️  {message}", "cyan")
    
    def log_warning(self, message: str):
        """警告日誌"""
        self.log(f"⚠️  {message}", "yellow")
    
    def find_port(self) -> Optional[str]:
        """自動尋找 ESP32 連接埠"""
        if self.port:
            return self.port
        
        self.log_info("正在掃描可用連接埠...")
        ports_info = self.esp.list_ports()
        recommended = ports_info.get('recommended')
        
        if recommended:
            self.log_success(f"找到推薦連接埠: {recommended}")
            return recommended
        
        # Fallback search
        patterns = [
            '/dev/cu.usbserial-*', 
            '/dev/cu.SLAB_USBtoUART', 
            '/dev/cu.usbmodem*',
            '/dev/ttyUSB*',
            '/dev/ttyACM*'
        ]
        
        for pattern in patterns:
            matches = glob.glob(pattern)
            if matches:
                port = matches[0]
                self.log_warning(f"使用備用連接埠: {port}")
                return port
        
        return None
    
    def connect(self) -> bool:
        """連接到 ESP32"""
        port = self.find_port()
        if not port:
            self.log_error("找不到 ESP32 連接埠")
            self.log_info("請確認:")
            self.log_info("  1. ESP32 已通過 USB 連接")
            self.log_info("  2. 驅動程式已安裝")
            self.log_info("  3. 連接埠權限正確")
            return False
        
        self.log_info(f"正在連接到 {port}...")
        try:
            if self.esp.connect(port):
                self.log_success(f"已連接到 {port}")
                return True
            else:
                self.log_error("連接失敗")
                return False
        except Exception as e:
            self.log_error(f"連接異常: {e}")
            return False
    
    def convert_to_wav(self, input_file: str, output_file: Optional[str] = None) -> Optional[str]:
        """將音訊檔案轉換為 WAV 格式"""
        input_path = Path(input_file)
        
        # Check if file exists
        if not input_path.exists():
            self.log_error(f"檔案不存在: {input_file}")
            return None
        
        # Check if format is supported
        if input_path.suffix.lower() not in SUPPORTED_FORMATS:
            self.log_error(f"不支援的格式: {input_path.suffix}")
            self.log_info(f"支援的格式: {', '.join(SUPPORTED_FORMATS)}")
            return None
        
        # If already WAV, check if it needs conversion
        if input_path.suffix.lower() == '.wav':
            self.log_info(f"檔案已是 WAV 格式: {input_file}")
            # TODO: Could check WAV specs here
            return input_file
        
        # Generate output filename
        if output_file is None:
            output_file = f"{input_path.stem}_converted.wav"
        
        output_path = Path(output_file)
        
        # Convert using ffmpeg
        self.log_info(f"正在轉換 {input_path.name} 為 WAV...")
        
        cmd = [
            'ffmpeg', '-y', '-i', str(input_path),
            '-ar', str(WAV_SAMPLE_RATE),
            '-ac', str(WAV_CHANNELS),
            '-sample_fmt', WAV_FORMAT,
            str(output_path)
        ]
        
        try:
            result = subprocess.run(
                cmd, 
                check=True, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE,
                text=True
            )
            
            if output_path.exists():
                size_mb = output_path.stat().st_size / (1024 * 1024)
                self.log_success(f"轉換成功! 大小: {size_mb:.2f} MB")
                self.temp_files.append(str(output_path))
                return str(output_path)
            else:
                self.log_error("轉換失敗: 輸出檔案不存在")
                return None
                
        except subprocess.CalledProcessError as e:
            self.log_error(f"ffmpeg 轉換失敗: {e}")
            if e.stderr:
                self.log_error(f"錯誤訊息: {e.stderr}")
            return None
        except FileNotFoundError:
            self.log_error("找不到 ffmpeg,請先安裝:")
            self.log_info("  macOS: brew install ffmpeg")
            self.log_info("  Ubuntu: sudo apt install ffmpeg")
            return None
    
    def upload_file(self, local_file: str, remote_filename: Optional[str] = None) -> bool:
        """上傳檔案到 ESP32 SD 卡"""
        local_path = Path(local_file)
        
        if not local_path.exists():
            self.log_error(f"檔案不存在: {local_file}")
            return False
        
        # Generate remote filename
        if remote_filename is None:
            remote_filename = local_path.name
        
        # Get file size
        file_size = local_path.stat().st_size
        size_mb = file_size / (1024 * 1024)
        
        self.log_info(f"準備上傳: {remote_filename} ({size_mb:.2f} MB)")
        
        # Progress callback
        if self.use_rich:
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                BarColumn(),
                TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
                TextColumn("•"),
                TextColumn("{task.completed}/{task.total} bytes"),
                TimeRemainingColumn(),
                console=self.console
            ) as progress:
                task = progress.add_task(f"上傳 {remote_filename}", total=file_size)
                
                def progress_callback(current, total):
                    progress.update(task, completed=current)
                
                try:
                    success = self.esp.upload_file(local_file, remote_filename, progress_callback)
                    if success:
                        self.log_success(f"上傳成功: {remote_filename}")
                        return True
                    else:
                        self.log_error(f"上傳失敗: {remote_filename}")
                        return False
                except Exception as e:
                    self.log_error(f"上傳異常: {e}")
                    return False
        else:
            # Simple progress without Rich
            def progress_callback(current, total):
                percent = int((current / total) * 100)
                sys.stdout.write(f"\r上傳進度: {percent}% ({current}/{total} bytes)")
                sys.stdout.flush()
            
            try:
                success = self.esp.upload_file(local_file, remote_filename, progress_callback)
                print()  # New line after progress
                if success:
                    self.log_success(f"上傳成功: {remote_filename}")
                    return True
                else:
                    self.log_error(f"上傳失敗: {remote_filename}")
                    return False
            except Exception as e:
                print()  # New line after progress
                self.log_error(f"上傳異常: {e}")
                return False
    
    def upload_music_file(self, input_file: str, remote_filename: Optional[str] = None) -> bool:
        """上傳音樂檔案 (會自動轉換為 WAV)"""
        # Convert to WAV if needed
        wav_file = self.convert_to_wav(input_file)
        if not wav_file:
            return False
        
        # Generate remote filename if not provided
        if remote_filename is None:
            remote_filename = Path(input_file).stem + ".wav"
        
        # Upload
        success = self.upload_file(wav_file, remote_filename)
        
        return success
    
    def batch_upload(self, folder_path: str) -> dict:
        """批次上傳資料夾中的音訊檔案"""
        folder = Path(folder_path)
        
        if not folder.is_dir():
            self.log_error(f"不是有效的資料夾: {folder_path}")
            return {'success': 0, 'failed': 0, 'skipped': 0}
        
        # Find all audio files
        audio_files = []
        for ext in SUPPORTED_FORMATS:
            audio_files.extend(folder.glob(f"*{ext}"))
        
        if not audio_files:
            self.log_warning(f"資料夾中沒有找到音訊檔案: {folder_path}")
            return {'success': 0, 'failed': 0, 'skipped': 0}
        
        self.log_info(f"找到 {len(audio_files)} 個音訊檔案")
        
        # Upload each file
        results = {'success': 0, 'failed': 0, 'skipped': 0}
        
        for i, audio_file in enumerate(audio_files, 1):
            self.log(f"\n[{i}/{len(audio_files)}] 處理: {audio_file.name}", "bold")
            
            if self.upload_music_file(str(audio_file)):
                results['success'] += 1
            else:
                results['failed'] += 1
        
        # Summary
        self.log("\n" + "="*50, "bold")
        self.log_success(f"成功: {results['success']}")
        if results['failed'] > 0:
            self.log_error(f"失敗: {results['failed']}")
        
        return results
    
    def list_sd_files(self):
        """列出 SD 卡上的檔案"""
        self.log_info("正在讀取 SD 卡檔案列表...")
        
        files = self.esp.list_files()
        
        if not files:
            self.log_warning("SD 卡是空的或無法讀取")
            return
        
        if self.use_rich:
            table = Table(title="SD 卡檔案", show_header=True, header_style="bold magenta")
            table.add_column("檔名", style="cyan")
            table.add_column("大小", justify="right", style="green")
            
            for file in files:
                size_kb = file['size'] / 1024
                if size_kb < 1024:
                    size_str = f"{size_kb:.2f} KB"
                else:
                    size_str = f"{size_kb/1024:.2f} MB"
                table.add_row(file['name'], size_str)
            
            self.console.print(table)
        else:
            print("\nSD 卡檔案:")
            print("-" * 50)
            for file in files:
                size_kb = file['size'] / 1024
                if size_kb < 1024:
                    size_str = f"{size_kb:.2f} KB"
                else:
                    size_str = f"{size_kb/1024:.2f} MB"
                print(f"  {file['name']:<30} {size_str:>15}")
            print("-" * 50)
    
    def cleanup(self):
        """清理臨時檔案"""
        for temp_file in self.temp_files:
            if os.path.exists(temp_file):
                try:
                    os.remove(temp_file)
                    self.log_info(f"已清理臨時檔案: {temp_file}")
                except Exception as e:
                    self.log_warning(f"無法刪除臨時檔案 {temp_file}: {e}")
        
        self.temp_files.clear()
    
    def disconnect(self):
        """中斷連接並清理"""
        self.cleanup()
        self.esp.disconnect()
        self.log_info("已中斷連接")


def main():
    """主程式"""
    parser = argparse.ArgumentParser(
        description="ESP32 Music Upload Tool - 音訊檔案上傳工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
範例:
  %(prog)s                                    # 互動模式
  %(prog)s song.mp3                           # 上傳單個檔案
  %(prog)s /path/to/music --batch             # 批次上傳資料夾
  %(prog)s song.flac --port /dev/cu.usbserial-0001
  %(prog)s --list                             # 列出 SD 卡檔案
        """
    )
    
    parser.add_argument('input', nargs='?', help='輸入檔案或資料夾路徑')
    parser.add_argument('--batch', action='store_true', help='批次上傳模式 (輸入為資料夾)')
    parser.add_argument('--port', help='指定連接埠')
    parser.add_argument('--list', action='store_true', help='列出 SD 卡檔案')
    parser.add_argument('--no-rich', action='store_true', help='不使用 Rich 美化輸出')
    
    args = parser.parse_args()
    
    # Create uploader
    uploader = MusicUploader(port=args.port, use_rich=not args.no_rich)
    
    # Connect to ESP32
    if not uploader.connect():
        return 1
    
    try:
        # List files mode
        if args.list:
            uploader.list_sd_files()
            return 0
        
        # Batch upload mode
        if args.batch:
            if not args.input:
                uploader.log_error("批次模式需要指定資料夾路徑")
                return 1
            
            results = uploader.batch_upload(args.input)
            return 0 if results['failed'] == 0 else 1
        
        # Single file upload mode
        if args.input:
            success = uploader.upload_music_file(args.input)
            return 0 if success else 1
        
        # Interactive mode
        uploader.log_info("進入互動模式")
        uploader.log_info("輸入檔案路徑或 'list' 查看 SD 卡檔案,'quit' 退出")
        
        while True:
            try:
                user_input = input("\n> ").strip()
                
                if not user_input:
                    continue
                
                if user_input.lower() in ('quit', 'exit', 'q'):
                    break
                
                if user_input.lower() == 'list':
                    uploader.list_sd_files()
                    continue
                
                # Try to upload
                if os.path.isfile(user_input):
                    uploader.upload_music_file(user_input)
                elif os.path.isdir(user_input):
                    uploader.batch_upload(user_input)
                else:
                    uploader.log_error(f"找不到檔案或資料夾: {user_input}")
                    
            except KeyboardInterrupt:
                print()
                break
            except EOFError:
                break
        
        return 0
        
    finally:
        uploader.disconnect()


if __name__ == "__main__":
    sys.exit(main())
