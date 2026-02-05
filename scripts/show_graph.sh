#!/bin/bash

# Define files
MMD_FILE="feature_graph.mmd"
IMG_FILE="feature_graph.png"

# Check if mmdc is installed
if ! command -v mmdc &> /dev/null; then
    echo "Error: mermaid-cli is not installed."
    echo "Please run: npm install -g @mermaid-js/mermaid-cli"
    exit 1
fi

# Generate PNG
echo "Generating graph image..."
# -s 4: Scale up 4x for high DPI/resolution
# -b transparent: Transparent background
mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent -s 4

if [ $? -eq 0 ]; then
    echo "Displaying graph..."
    # Always use manual escape sequence to force width=100%
    # This ensures it fills the terminal pane width
    cat "$IMG_FILE" | base64 | tr -d '\n' | awk '{print "\033]1337;File=inline=1;width=100%;preserveAspectRatio=1:" $0 "\a"}'
    echo "" # Add a newline after the image
else
    echo "Failed to generate image."
    exit 1
fi