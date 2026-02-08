#!/bin/bash
# stop.sh - Stops the CDD web server

# Navigate to the script's directory
cd "$(dirname "$0")"

PID_FILE=".pid"

if [ ! -f "$PID_FILE" ]; then
    echo "Server is not running (or .pid file not found)."
    exit 0 # Exit cleanly if already stopped
fi

PID=$(cat "$PID_FILE")

if ! ps -p $PID > /dev/null; then
    echo "Server with PID $PID is not running."
    rm "$PID_FILE"
    exit 0 # Exit cleanly
fi

echo "Stopping server with PID $PID..."
kill $PID

# Wait for the process to terminate
for i in {1..10}; do
    if ! ps -p $PID > /dev/null; then
        echo "Server with PID $PID stopped successfully."
        rm "$PID_FILE"
        exit 0
    fi
    sleep 0.5
done

# If it's still running, force kill
if ps -p $PID > /dev/null; then
    echo "Server did not respond to TERM signal. Sending KILL signal."
    kill -9 $PID
    sleep 0.5
fi

if ! ps -p $PID > /dev/null; then
    echo "Server killed forcefully."
    rm "$PID_FILE"
    exit 0
else
    echo "ERROR: Failed to stop process $PID."
    exit 1
fi