#!/usr/bin/env python3
"""
Serial Screenshot Capture for LPad

Sends 'S' to the device via serial, captures RGB565 pixel data,
and saves as a PNG file.

Requirements:
    pip install pyserial Pillow

Usage:
    python scripts/capture_screenshot.py                  # auto-detect port
    python scripts/capture_screenshot.py -p /dev/ttyACM0  # specify port
    python scripts/capture_screenshot.py -b 921600        # custom baud rate
"""

import sys
import time
import struct
import argparse
from datetime import datetime
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Error: pyserial is required. Install with: pip install pyserial")
    sys.exit(1)

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)


def find_device_port():
    """Auto-detect the ESP32-S3 serial port."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # ESP32-S3 USB CDC (Espressif VID)
        if port.vid == 0x303A:
            return port.device
        # Common USB-UART bridges
        desc = port.description or ""
        if any(chip in desc for chip in ("CP210", "CH340", "ESP32")):
            return port.device
    return None


def capture_screenshot(port, baud=115200, output_dir="captures"):
    """Send screenshot command and capture the result as PNG."""
    out_path = Path(output_dir)
    out_path.mkdir(exist_ok=True)

    print(f"Connecting to {port} at {baud} baud...")
    ser = serial.Serial(port, baud, timeout=30)
    time.sleep(0.5)

    # Flush any pending data
    ser.reset_input_buffer()

    # Send screenshot trigger
    print("Sending screenshot trigger 'S'...")
    ser.write(b"S")
    ser.flush()

    # Wait for START marker
    print("Waiting for START marker...")
    start_line = b""
    deadline = time.time() + 30
    while time.time() < deadline:
        byte = ser.read(1)
        if not byte:
            continue
        if byte == b"\n":
            line = start_line.decode("ascii", errors="ignore").strip()
            if line.startswith("START:"):
                break
            start_line = b""
        else:
            start_line += byte
    else:
        print("ERROR: Timed out waiting for START marker")
        ser.close()
        return None

    # Parse dimensions
    header = start_line.decode("ascii", errors="ignore").strip()
    parts = header.replace("START:", "").split(",")
    width = int(parts[0])
    height = int(parts[1])
    total_bytes = width * height * 2
    print(f"Capturing {width}x{height} ({total_bytes} bytes)...")

    # Read raw pixel data
    data = b""
    last_progress = -1
    while len(data) < total_bytes:
        remaining = total_bytes - len(data)
        chunk = ser.read(min(4096, remaining))
        if not chunk:
            print(f"\nWARNING: Read timeout. Got {len(data)}/{total_bytes} bytes")
            break
        data += chunk
        pct = len(data) * 100 // total_bytes
        if pct != last_progress:
            last_progress = pct
            print(f"\r  Progress: {pct}% ({len(data)}/{total_bytes} bytes)", end="")

    print()

    # Consume the trailing \nEND\n marker
    ser.read_until(b"END\n")
    ser.close()

    if len(data) < total_bytes:
        print(f"WARNING: Incomplete data ({len(data)}/{total_bytes} bytes)")

    # Convert RGB565 to RGB888 and create PNG
    print("Converting RGB565 to PNG...")
    img = Image.new("RGB", (width, height))
    pixels = img.load()

    for i in range(0, min(len(data), total_bytes), 2):
        pixel_index = i // 2
        px = pixel_index % width
        py = pixel_index // width

        # RGB565 little-endian (ESP32 byte order)
        pixel = struct.unpack("<H", data[i : i + 2])[0]

        r = (pixel >> 11) & 0x1F
        g = (pixel >> 5) & 0x3F
        b = pixel & 0x1F

        # Scale to 8-bit
        r = (r * 255) // 31
        g = (g * 255) // 63
        b = (b * 255) // 31

        pixels[px, py] = (r, g, b)

    # Save with timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = out_path / f"screenshot_{timestamp}.png"
    img.save(str(filename))
    print(f"Saved: {filename}")
    return str(filename)


def main():
    parser = argparse.ArgumentParser(
        description="Capture screenshot from LPad device"
    )
    parser.add_argument(
        "-p", "--port", help="Serial port (auto-detect if omitted)"
    )
    parser.add_argument(
        "-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)"
    )
    parser.add_argument(
        "-o", "--output", default="captures", help="Output directory (default: captures)"
    )
    args = parser.parse_args()

    port = args.port
    if not port:
        port = find_device_port()
        if not port:
            print("ERROR: Could not auto-detect device port. Use -p to specify.")
            sys.exit(1)
        print(f"Auto-detected device: {port}")

    result = capture_screenshot(port, args.baud, args.output)
    if result:
        print(f"Screenshot captured successfully: {result}")
    else:
        print("Screenshot capture failed.")
        sys.exit(1)


if __name__ == "__main__":
    main()
