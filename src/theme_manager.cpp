#include "theme_manager.h"

namespace LPad {

// ==========================================
// Default Theme Definition
// ==========================================

Theme ThemeManager::default_theme_ = {
    // colors
    {
        .background = THEME_BACKGROUND,
        .surface = COLOR_FOREST_16,
        .primary = THEME_PRIMARY,
        .secondary = THEME_SECONDARY,
        .accent = THEME_ACCENT,
        .text_main = THEME_TEXT,
        .text_secondary = COLOR_MOSS_16,
        .text_error = THEME_TEXT_ERROR,
        .text_version = THEME_TEXT_VERSION,
        .text_status = THEME_TEXT_STATUS,

        // Graph-specific colors
        .graph_axes = THEME_GRAPH_AXES,
        .graph_ticks = THEME_GRAPH_TICKS,
        .axis_labels = THEME_AXIS_LABELS,
        .data_labels = THEME_DATA_LABELS,

        .system_menu_bg = THEME_SYSTEM_MENU_BG
    },
    // fonts
    {
        .smallest = FONT_SMALLEST,
        .normal = FONT_NORMAL,
        .ui = FONT_UI,
        .heading = FONT_HEADING,
        .title = FONT_TITLE
    }
};

// ==========================================
// ThemeManager Implementation
// ==========================================

ThemeManager::ThemeManager()
    : active_theme_(&default_theme_) {
}

ThemeManager& ThemeManager::getInstance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::setTheme(const Theme* theme) {
    if (theme != nullptr) {
        active_theme_ = theme;
    }
}

const Theme* ThemeManager::getDefaultTheme() {
    return &default_theme_;
}

} // namespace LPad
