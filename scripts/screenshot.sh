#!/bin/bash
# Screenshot capture wrapper for LPad
#
# Usage:
#   ./scripts/screenshot.sh                    # auto-detect port, 115200 baud
#   ./scripts/screenshot.sh -p /dev/ttyACM0    # specify port
#   ./scripts/screenshot.sh -b 921600          # custom baud rate
#
# Requirements: Python 3, pyserial, Pillow
#   pip install pyserial Pillow

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="${SCRIPT_DIR}/capture_screenshot.py"

# Verify Python script exists
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "ERROR: capture_screenshot.py not found at ${PYTHON_SCRIPT}"
    exit 1
fi

# Check Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 is required but not found in PATH"
    exit 1
fi

# Check required Python packages
python3 -c "import serial" 2>/dev/null || {
    echo "ERROR: pyserial is not installed. Run: pip install pyserial"
    exit 1
}
python3 -c "from PIL import Image" 2>/dev/null || {
    echo "ERROR: Pillow is not installed. Run: pip install Pillow"
    exit 1
}

echo "=== LPad Screenshot Capture ==="
python3 "$PYTHON_SCRIPT" "$@"
