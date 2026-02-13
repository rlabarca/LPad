#ifndef LPAD_THEME_MANAGER_H
#define LPAD_THEME_MANAGER_H

#include "themes/default/theme_colors.h"
#include "themes/default/theme_manifest.h"
#include <stdint.h>

namespace LPad {

// ==========================================
// Theme Structures
// ==========================================

/**
 * ThemeColors: Holds 16-bit RGB565 color values for semantic UI elements
 */
struct ThemeColors {
    uint16_t background;
    uint16_t surface;
    uint16_t primary;
    uint16_t secondary;
    uint16_t accent;
    uint16_t text_main;
    uint16_t text_secondary;
    uint16_t text_error;
    uint16_t text_version;
    uint16_t text_status;

    // Graph-specific colors
    uint16_t graph_axes;
    uint16_t graph_ticks;
    uint16_t axis_labels;
    uint16_t data_labels;

    // System menu
    uint16_t system_menu_bg;

    // Widget system
    uint16_t text_highlight;
    uint16_t bg_connecting;
    uint16_t scroll_indicator;
};

/**
 * ThemeFonts: Holds pointers to GFXfont objects for the 5 standardized typography levels
 */
struct ThemeFonts {
    const GFXfont* smallest;    // 9pt - ticks, data labels, dense info
    const GFXfont* normal;      // 12pt - body text, paragraphs
    const GFXfont* ui;          // 18pt - axis labels, button text, key values
    const GFXfont* heading;     // 24pt - section headers, group titles
    const GFXfont* title;       // 48pt - splash screens, main branding
};

/**
 * Theme: Aggregates ThemeColors and ThemeFonts
 */
struct Theme {
    ThemeColors colors;
    ThemeFonts fonts;
};

// ==========================================
// Theme Manager Singleton
// ==========================================

/**
 * ThemeManager: Controls the active theme for the application
 *
 * Provides a singleton interface for accessing and switching themes at runtime
 * without requiring recompilation.
 */
class ThemeManager {
public:
    /**
     * Get the singleton instance
     */
    static ThemeManager& getInstance();

    /**
     * Get the currently active theme
     * @return Pointer to the active Theme struct
     */
    const Theme* getTheme() const { return active_theme_; }

    /**
     * Set the active theme
     * @param theme Pointer to the new theme to activate
     */
    void setTheme(const Theme* theme);

    /**
     * Get the default theme
     * @return Pointer to the default Theme struct
     */
    static const Theme* getDefaultTheme();

    // Delete copy constructor and assignment operator (singleton pattern)
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

private:
    // Private constructor for singleton
    ThemeManager();

    // Active theme pointer
    const Theme* active_theme_;

    // Default theme instance
    static Theme default_theme_;
};

} // namespace LPad

#endif // LPAD_THEME_MANAGER_H
