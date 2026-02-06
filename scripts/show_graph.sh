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

update_graph() {
    # Generate the MMD file first
    ./scripts/generate_graph.sh > /dev/null

    # Generate PNG
    # -s 4: Scale up 4x for high DPI/resolution
    # -b transparent: Transparent background
    if mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent -s 4 &> /dev/null; then
        # Clear screen and move cursor to top
        clear
        
        # Display graph using iTerm2 inline image protocol
        cat "$IMG_FILE" | base64 | tr -d '\n' | awk '{print "\033]1337;File=inline=1;width=100%;preserveAspectRatio=1:" $0 "\a"}'
        echo "Last updated: $(date '+%Y-%m-%d %H:%M:%S')"
    else
        echo "Failed to generate image."
    fi
}

# Initial display
update_graph

# Watch for changes if fswatch is available
if command -v fswatch &> /dev/null; then
    echo "Watching $FEATURES_DIR for changes... (Ctrl+C to stop)"
    fswatch -o "$FEATURES_DIR" | while read -r line; do
        update_graph
    done
else
    echo "fswatch not found. Please install it for auto-update: brew install fswatch"
    echo "Current graph displayed. Script exiting."
fi
