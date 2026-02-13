#!/bin/bash

# Stop the interactive software map
# Usage: ./ai_dev_tools/software_map/stop.sh

PORT=8085

PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)

if [ -n "$PID" ]; then
    echo "Stopping Software Map (PID: $PID)..."
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
    echo "Software Map is not running."
fi
