/**
 * @file font_stubs.cpp
 * @brief Stub font definitions for native unit tests
 *
 * These stubs provide minimal GFXfont definitions so that theme tests
 * can compile and run in the native environment without requiring
 * actual generated font files.
 */

#include "Arduino_GFX_Library.h"

namespace LPad {

// Stub bitmap data (empty)
static uint8_t stub_bitmap[] = {0x00};

// Stub glyph data (single glyph)
static GFXglyph stub_glyphs[] = {
    {0, 5, 7, 6, 0, -7}  // Single character stub
};

// Define all the font externs from theme_manifest.h as stubs
extern const GFXfont Font_SystemUI9pt7b = {
    stub_bitmap,
    stub_glyphs,
    0x20,  // first char (space)
    0x7E,  // last char (~)
    9      // yAdvance
};

extern const GFXfont Font_General12pt7b = {
    stub_bitmap,
    stub_glyphs,
    0x20,
    0x7E,
    12
};

extern const GFXfont Font_SystemUI18pt7b = {
    stub_bitmap,
    stub_glyphs,
    0x20,
    0x7E,
    18
};

extern const GFXfont Font_General24pt7b = {
    stub_bitmap,
    stub_glyphs,
    0x20,
    0x7E,
    24
};

extern const GFXfont Font_Logo48pt7b = {
    stub_bitmap,
    stub_glyphs,
    0x20,
    0x7E,
    48
};

} // namespace LPad
