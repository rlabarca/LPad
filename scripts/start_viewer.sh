#!/bin/bash

# Start the interactive graph viewer in the background
# Usage: ./scripts/start_viewer.sh

PORT=8000
SCRIPT_PATH="scripts/serve_graph.py"
LOG_FILE=".pio/graph_server.log"

mkdir -p .pio

# Check if already running
if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null ; then
    echo "Graph server is already running on port $PORT."
    exit 0
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
