#!/bin/bash

# Define files
MMD_FILE="feature_graph.mmd"
IMG_FILE="feature_graph.png"
FEATURES_DIR="features"

# Check if mmdc is installed
if ! command -v mmdc &> /dev/null; then
    echo "Error: mermaid-cli is not installed."
    echo "Please run: npm install -g @mermaid-js/mermaid-cli"
    exit 1
fi

LOG_FILE=".pio/show_graph.log"
mkdir -p .pio

update_graph() {
    echo "--- Update Triggered at $(date '+%H:%M:%S') ---"
    
    # Generate the MMD file first
    echo "Generating graph structure..."
    if ! ./scripts/generate_graph.sh > "$LOG_FILE" 2>&1; then
        echo "Error: generate_graph.sh failed. Check $LOG_FILE"
        return 1
    fi

    # Generate PNG
    echo "Converting to PNG (this may take a few seconds)..."
    if mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent -s 2 >> "$LOG_FILE" 2>&1; then
        if [ ! -f "$IMG_FILE" ]; then
            echo "Error: $IMG_FILE was not created. Check $LOG_FILE"
            return 1
        fi

        # Clear screen only on success
        clear
        
        # Display graph using iTerm2 inline image protocol
        printf "\033]1337;File=inline=1;width=100%%;preserveAspectRatio=1:"
        base64 < "$IMG_FILE" | tr -d '\n'
        printf "\a\n"
        
        echo "Last updated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Watching $FEATURES_DIR for changes..."
    else
        echo "Error: mmdc failed. Check $LOG_FILE"
        # Print the last few lines of the log for immediate feedback
        tail -n 5 "$LOG_FILE"
    fi
}

# Initial display
update_graph

# Watch for changes if fswatch is available
if command -v fswatch &> /dev/null; then
    # Use -1 to wait for at least one event, and -r to watch recursively if needed
    # fswatch -o only outputs a number, which we use to trigger the update
    fswatch -o "$FEATURES_DIR" | while read -r event_count; do
        # Debounce a bit if many files change at once
        sleep 0.2
        update_graph
    done
else
    echo "fswatch not found. Please install it for auto-update: brew install fswatch"
    echo "Current graph displayed. Script exiting."
fi
