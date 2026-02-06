#!/bin/bash

# Stop the interactive graph viewer
# Usage: ./scripts/stop_viewer.sh

PORT=8000

PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)

if [ -n "$PID" ]; then
    echo "Stopping Graph Viewer (PID: $PID)..."
    kill $PID
    echo "Stopped."
else
    echo "Graph Viewer is not running."
fi
