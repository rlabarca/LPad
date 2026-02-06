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

generate_and_render() {
    # Generate the MMD file first
    if ! ./scripts/generate_graph.sh > "$LOG_FILE" 2>&1; then
        echo "Error: generate_graph.sh failed. Check $LOG_FILE"
        return 1
    fi

    # Generate PNG
    if ! mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent -s 2 >> "$LOG_FILE" 2>&1; then
        echo "Error: mmdc failed. Check $LOG_FILE"
        return 1
    fi

    # Verify image exists and has size > 0
    if [ ! -s "$IMG_FILE" ]; then
        echo "Error: $IMG_FILE is empty or missing. Check $LOG_FILE"
        return 1
    fi
    
    return 0
}

update_graph() {
    echo "--- Update Triggered at $(date '+%H:%M:%S') ---"
    
    local max_retries=3
    local attempt=1
    local success=0

    while [ $attempt -le $max_retries ]; do
        if [ $attempt -gt 1 ]; then
             echo "Retry attempt $attempt of $max_retries..."
        else
             echo "Generating graph..."
        fi

        if generate_and_render; then
            success=1
            break
        fi
        
        echo "Generation failed. Retrying in 1 second..."
        sleep 1
        ((attempt++))
    done

    if [ $success -eq 1 ]; then
        # Clear screen only on success
        clear
        
        # Display graph using iTerm2 inline image protocol
        printf "\033]1337;File=inline=1;width=100%%;preserveAspectRatio=1:"
        base64 < "$IMG_FILE" | tr -d '\n'
        printf "\a\n"
        
        echo "Last updated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Watching $FEATURES_DIR for changes..."
    else
        echo "Failed to update graph after $max_retries attempts."
        # Print the last few lines of the log for immediate feedback
        tail -n 10 "$LOG_FILE"
    fi
}

# Initial display
update_graph

# Watch for changes if fswatch is available
if command -v fswatch &> /dev/null; then
    # Use --latency 1 to batch events and avoid rapid firing
    fswatch -o --latency 1 "$FEATURES_DIR" | while read -r event_count; do
        update_graph
    done
else
    echo "fswatch not found. Please install it for auto-update: brew install fswatch"
    echo "Current graph displayed. Script exiting."
fi
