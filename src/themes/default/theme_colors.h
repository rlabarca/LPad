#ifndef LPAD_THEME_COLORS_H
#define LPAD_THEME_COLORS_H

#include <stdint.h>

/**
 * LPad Design System - Color Palette
 * * Includes 24-bit Hex codes for Web/LVGL and 
 * 16-bit RGB565 values for TFT_eSPI/Adafruit_GFX.
 */

namespace LPad {

    // ==========================================
    // Typography Assets (Google Fonts)
    // ==========================================
    // Logo:   Outfit         -> https://fonts.google.com/specimen/Outfit
    // System: JetBrains Mono -> https://fonts.google.com/specimen/JetBrains+Mono
    // Body:   Inter          -> https://fonts.google.com/specimen/Inter

    // ==========================================
    // 24-Bit RGB888 Color Definitions
    // ==========================================
    static const uint32_t COLOR_NIGHT_24     = 0x1E231E; // Background
    static const uint32_t COLOR_FOREST_24    = 0x464C42; // Deepest Green
    static const uint32_t COLOR_RESEDA_24    = 0x6D7361; // Muted Green
    static const uint32_t COLOR_MOSS_24      = 0x989F7E; // Light Olive
    static const uint32_t COLOR_SAGE_24      = 0x89AA89; // Fresh Green
    static const uint32_t COLOR_KHAKI_24     = 0xB6AD90; // Earthy Beige
    static const uint32_t COLOR_CHAMOISEE_24 = 0xAC855E; // Accent Brown/Orange
    static const uint32_t COLOR_CREAM_24     = 0xBEBDB6; // Text/Highlight

    // ==========================================
    // 16-Bit RGB565 Color Definitions
    // (Calculated: ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
    // ==========================================
    static const uint16_t COLOR_NIGHT_16     = 0x1923;
    static const uint16_t COLOR_FOREST_16    = 0x4268;
    static const uint16_t COLOR_RESEDA_16    = 0x6B8C;
    static const uint16_t COLOR_MOSS_16      = 0x9CFF;
    static const uint16_t COLOR_SAGE_16      = 0x8D51;
    static const uint16_t COLOR_KHAKI_16     = 0xB572;
    static const uint16_t COLOR_CHAMOISEE_16 = 0xAC2B;
    static const uint16_t COLOR_CREAM_16     = 0xBED6;

    // ==========================================
    // Semantic Usage
    // ==========================================
    static const uint16_t THEME_BACKGROUND   = COLOR_NIGHT_16;
    static const uint16_t THEME_TEXT         = COLOR_CREAM_16;
    static const uint16_t THEME_ACCENT       = COLOR_CHAMOISEE_16;
    static const uint16_t THEME_PRIMARY      = COLOR_SAGE_16;
    static const uint16_t THEME_SECONDARY    = COLOR_RESEDA_16;

    // ==========================================
    // Graph Specific Semantic Usage
    // ==========================================
    static const uint16_t THEME_GRAPH_AXES   = COLOR_RESEDA_16; // Muted for background
    static const uint16_t THEME_GRAPH_TICKS  = COLOR_FOREST_16; // Subtle ticks
    static const uint16_t THEME_AXIS_LABELS  = COLOR_CREAM_16;  // Clear readable labels
    static const uint16_t THEME_DATA_LABELS  = COLOR_SAGE_16;   // Thematic accent for data
    
}

#endif // LPAD_THEME_COLORS_H
