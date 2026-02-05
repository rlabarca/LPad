#!/bin/bash
# Script to generate theme fonts using Adafruit's fontconvert utility
# This script assumes 'fontconvert' is in your PATH or built in the current directory.
#
# Usage: ./scripts/generate_theme_fonts.sh [path_to_fontconvert]

FONTCONVERT=${1:-./lib/Adafruit-GFX-Library/fontconvert/fontconvert}

# Fallback: Check if it's in the current directory or PATH
if ! [ -x "$FONTCONVERT" ]; then
    if [ -x "./fontconvert" ]; then
        FONTCONVERT="./fontconvert"
    elif command -v fontconvert >/dev/null 2>&1; then
        FONTCONVERT="fontconvert"
    fi
fi

if ! [ -x "$FONTCONVERT" ]; then
  echo "Error: fontconvert executable not found at '$FONTCONVERT'"
  echo "Please build it from lib/Adafruit-GFX-Library/fontconvert"
  echo "Instructions:"
  echo "1. cd lib/Adafruit-GFX-Library/fontconvert"
  echo "2. make (or gcc -I/opt/homebrew/include/freetype2 fontconvert.c -L/opt/homebrew/lib -lfreetype -o fontconvert)"
  exit 1
fi

SOURCE_DIR="/Users/richardlabarca/Desktop/theme_default"
TARGET_DIR="src/themes/default/fonts"
mkdir -p "$TARGET_DIR"

echo "Generating fonts to $TARGET_DIR..."

# 1. Smallest (Ticks/Data) -> SystemUI (JetBrains Mono) 9pt (~12px)
$FONTCONVERT "$SOURCE_DIR/Font_SystemUI.ttf" 9 > "$TARGET_DIR/Font_SystemUI_9pt7b.h"
echo "Generated Font_SystemUI_9pt7b.h"

# 2. Normal (Paragraph) -> General (Inter) 12pt (~16px)
$FONTCONVERT "$SOURCE_DIR/Font_General.ttf" 12 > "$TARGET_DIR/Font_General_12pt7b.h"
echo "Generated Font_General_12pt7b.h"

# 3. UI/Axis (Labels) -> SystemUI (JetBrains Mono) 18pt (~24px)
$FONTCONVERT "$SOURCE_DIR/Font_SystemUI.ttf" 18 > "$TARGET_DIR/Font_SystemUI_18pt7b.h"
echo "Generated Font_SystemUI_18pt7b.h"

# 4. Heading (Groups) -> General (Inter) 24pt (~32px)
$FONTCONVERT "$SOURCE_DIR/Font_General.ttf" 24 > "$TARGET_DIR/Font_General_24pt7b.h"
echo "Generated Font_General_24pt7b.h"

# 5. Title (Splash) -> Logo (Outfit) 48pt (~64px) - Use Logo font for big titles
$FONTCONVERT "$SOURCE_DIR/Font_Logo.ttf" 48 > "$TARGET_DIR/Font_Logo_48pt7b.h"
echo "Generated Font_Logo_48pt7b.h"

echo "Done."
