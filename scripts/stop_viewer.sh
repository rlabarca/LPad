#!/bin/bash

# Stop the interactive graph viewer
# Usage: ./scripts/stop_viewer.sh

PORT=8000

PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)

if [ -n "$PID" ]; then
    echo "Stopping Graph Viewer (PID: $PID)..."
    kill $PID
    
    # Wait for the process to actually terminate and release the port
    MAX_WAIT=5
    COUNT=0
    while lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null && [ $COUNT -lt $MAX_WAIT ]; do
        sleep 0.5
        ((COUNT++))
    done
    
    if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null; then
        echo "Process didn't stop, forcing..."
        kill -9 $PID
        sleep 1
    fi
    echo "Stopped."
else
    echo "Graph Viewer is not running."
fi
