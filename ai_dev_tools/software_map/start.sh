#!/bin/bash

# Start the interactive software map in the background
# Usage: ./ai_dev_tools/software_map/start.sh

PORT=8085
# Script path relative to project root (assuming we run from project root)
GENERATE_SCRIPT="ai_dev_tools/software_map/generate_tree.py"
LOG_FILE=".pio/software_map.log"

mkdir -p .pio

if [ "$1" == "update" ]; then
    echo "Generating graph..."
    python3 "$GENERATE_SCRIPT"
    echo "Graph updated."
    exit 0
fi

# Restart if already running
PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)
if [ -n "$PID" ]; then
    echo "Software Map server is already running (PID: $PID). Restarting..."
    ./ai_dev_tools/software_map/stop.sh
    sleep 1 # Wait for port to clear
fi

if [ ! -f "$GENERATE_SCRIPT" ]; then
    echo "Error: $GENERATE_SCRIPT not found. Make sure you are running from the project root."
    exit 1
fi

echo "Starting Software Map server in background..."
# Start a simple HTTP server to host the interactive map
# Note: generate_tree.py is now separate from the serving logic
cd ai_dev_tools/software_map
python3 -m http.server $PORT > "../../$LOG_FILE" 2>&1 &

# Store PID
PID=$!
echo "Software Map started with PID: $PID"
echo "Log file: $LOG_FILE"
echo "URL: http://localhost:$PORT"
