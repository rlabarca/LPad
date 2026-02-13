#!/bin/bash
# start.sh - Runs the CDD web server in the background

# Navigate to the script's directory to ensure correct relative paths
cd "$(dirname "$0")"

PID_FILE=".pid"
LOG_FILE="cdd.log"

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p $PID > /dev/null; then
        echo "Server is already running with PID: $PID"
        exit 1
    fi
fi

# Start the server using nohup to run it in the background and survive terminal closure
# Redirect stdout and stderr to a log file
nohup python3 serve.py > "$LOG_FILE" 2>&1 &

# Get the PID of the background process and store it
SERVER_PID=$!
echo $SERVER_PID > "$PID_FILE"

echo "CDD server started with PID: $SERVER_PID. Output is in $LOG_FILE."
echo "Access it at http://localhost:8086"
