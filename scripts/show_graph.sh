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
    echo "Generating graph..."
    # Generate the MMD file first
    if ! ./scripts/generate_graph.sh; then
        echo "Error: generate_graph.sh failed"
        return 1
    fi

    # Generate PNG
    # -s 2: Scale up 2x (4x might be too large for some terminals/buffers)
    # -b transparent: Transparent background
    echo "Converting to PNG..."
    if mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent -s 2; then
        # Clear screen and move cursor to top
        clear
        
        # Display graph using iTerm2 inline image protocol
        # Using printf to avoid awk line length limits
        printf "\033]1337;File=inline=1;width=100%%;preserveAspectRatio=1:"
        base64 < "$IMG_FILE" | tr -d '\n'
        printf "\a\n"
        
        echo "Last updated: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "Watching $FEATURES_DIR for changes... (Ctrl+C to stop)"
    else
        echo "Failed to generate PNG image."
    fi
}

# Initial display
update_graph

# Watch for changes if fswatch is available
if command -v fswatch &> /dev/null; then
    fswatch -o "$FEATURES_DIR" | while read -r line; do
        update_graph
    done
else
    echo "fswatch not found. Please install it for auto-update: brew install fswatch"
    echo "Current graph displayed. Script exiting."
fi
