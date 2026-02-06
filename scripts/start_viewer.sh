#!/bin/bash

# Start the interactive graph viewer in the background
# Usage: ./scripts/start_viewer.sh

PORT=8000
SCRIPT_PATH="scripts/serve_graph.py"
LOG_FILE=".pio/graph_server.log"

mkdir -p .pio

# Restart if already running
PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)
if [ -n "$PID" ]; then
    echo "Graph server is already running (PID: $PID). Restarting..."
    ./scripts/stop_viewer.sh
    sleep 1 # Wait for port to clear
fi

if [ ! -f "$SCRIPT_PATH" ]; then
    echo "Error: $SCRIPT_PATH not found."
    exit 1
fi

echo "Starting Graph Viewer in background..."
python3 "$SCRIPT_PATH" > "$LOG_FILE" 2>&1 &

# Store PID
PID=$!
echo "Graph Viewer started with PID: $PID"
echo "Log file: $LOG_FILE"
echo "URL: http://localhost:$PORT"
