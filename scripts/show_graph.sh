#!/bin/bash

# Define files
MMD_FILE="feature_graph.mmd"
IMG_FILE="feature_graph.png"

# Check if mmdc is installed
if ! command -v mmdc &> /dev/null;
then
    echo "Error: mermaid-cli is not installed."
    echo "Please run: npm install -g @mermaid-js/mermaid-cli"
    exit 1
fi

# Generate PNG
echo "Generating graph image..."
mmdc -i "$MMD_FILE" -o "$IMG_FILE" -b transparent

if [ $? -eq 0 ];
then
    echo "Displaying graph..."
    # iTerm2 Image Protocol
    if command -v imgcat &> /dev/null;
    then
        imgcat "$IMG_FILE"
    else
        # Fallback manual escape sequence for iTerm2
        cat "$IMG_FILE" | base64 | tr -d '\n' | awk '{print "\033]1337;File=inline=1;width=100%:"; print $0; print "\a"}'
    fi
else
    echo "Failed to generate image."
    exit 1
fi
