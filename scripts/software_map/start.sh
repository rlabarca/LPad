#!/bin/bash

# Start the interactive software map in the background
# Usage: ./scripts/software_map/start.sh

PORT=8085
# Script path relative to project root (assuming we run from project root)
SCRIPT_PATH="scripts/software_map/serve.py"
LOG_FILE=".pio/software_map.log"

mkdir -p .pio

# Restart if already running
PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)
if [ -n "$PID" ]; then
    echo "Software Map server is already running (PID: $PID). Restarting..."
    ./scripts/software_map/stop.sh
    sleep 1 # Wait for port to clear
fi

if [ ! -f "$SCRIPT_PATH" ]; then
    echo "Error: $SCRIPT_PATH not found. Make sure you are running from the project root."
    exit 1
fi

echo "Starting Software Map in background..."
python3 "$SCRIPT_PATH" > "$LOG_FILE" 2>&1 &

# Store PID
PID=$!
echo "Software Map started with PID: $PID"
echo "Log file: $LOG_FILE"
echo "URL: http://localhost:$PORT"
