#!/bin/bash
# Screenshot capture wrapper for LPad
#
# Usage:
#   ./scripts/screenshot.sh                    # auto-detect port, 115200 baud
#   ./scripts/screenshot.sh -p /dev/ttyACM0    # specify port
#   ./scripts/screenshot.sh -b 921600          # custom baud rate
#
# First-time setup:
#   python3 -m venv .venv
#   source .venv/bin/activate
#   pip install pyserial Pillow

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PYTHON_SCRIPT="${SCRIPT_DIR}/capture_screenshot.py"

# Verify Python script exists
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "ERROR: capture_screenshot.py not found at ${PYTHON_SCRIPT}"
    exit 1
fi

# Auto-activate project .venv if it exists and we're not already in a venv
if [ -z "$VIRTUAL_ENV" ] && [ -f "${PROJECT_DIR}/.venv/bin/activate" ]; then
    echo "Activating .venv..."
    source "${PROJECT_DIR}/.venv/bin/activate"
fi

# Check Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 is required but not found in PATH"
    exit 1
fi

# Check required Python packages
python3 -c "import serial" 2>/dev/null || {
    echo "ERROR: pyserial is not installed."
    echo "  python3 -m venv .venv && source .venv/bin/activate && pip install pyserial Pillow"
    exit 1
}
python3 -c "from PIL import Image" 2>/dev/null || {
    echo "ERROR: Pillow is not installed."
    echo "  python3 -m venv .venv && source .venv/bin/activate && pip install pyserial Pillow"
    exit 1
}

echo "=== LPad Screenshot Capture ==="
python3 "$PYTHON_SCRIPT" "$@"
