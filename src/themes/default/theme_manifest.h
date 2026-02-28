#ifndef LPAD_THEME_MANIFEST_H
#define LPAD_THEME_MANIFEST_H

#include "theme_colors.h"
#include "Arduino_GFX_Library.h" // For GFXfont struct definition

// Include generated font files
#include "fonts/Font_SystemUI_9pt7b.h"
#include "fonts/Font_General_12pt7b.h"
#include "fonts/Font_SystemUI_18pt7b.h"
#include "fonts/Font_General_24pt7b.h"
#include "fonts/Font_Logo_48pt7b.h"

namespace LPad {

    // ==========================================
    // Font Pointers (Direct References)
    // ==========================================
    // These reference the fonts defined in the included headers above

    // 1. Smallest (9pt) -> SystemUI
    static const GFXfont* FONT_SMALLEST = &Font_SystemUI9pt7b;

    // 2. Normal (12pt) -> General
    static const GFXfont* FONT_NORMAL = &Font_General12pt7b;

    // 3. UI/Axis (18pt) -> SystemUI
    static const GFXfont* FONT_UI = &Font_SystemUI18pt7b;

    // 4. Heading (24pt) -> General
    static const GFXfont* FONT_HEADING = &Font_General24pt7b;

    // 5. Title (48pt) -> Logo
    static const GFXfont* FONT_TITLE = &Font_Logo48pt7b;

    // ==========================================
    // Semantic Font Mappings (Graph)
    // ==========================================
    static const GFXfont* FONT_GRAPH_TICKS       = FONT_SMALLEST;
    static const GFXfont* FONT_GRAPH_AXIS_LABELS = FONT_UI;
    static const GFXfont* FONT_GRAPH_DATA_LABELS = FONT_SMALLEST;

}

#endif // LPAD_THEME_MANIFEST_H
