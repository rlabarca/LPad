#include <unity.h>
#include "theme_manager.h"

using namespace LPad;

// ==========================================
// Setup & Teardown
// ==========================================

void setUp(void) {
    // Reset to default theme before each test
    ThemeManager::getInstance().setTheme(ThemeManager::getDefaultTheme());
}

void tearDown(void) {
    // Nothing to tear down
}

// ==========================================
// Test Cases
// ==========================================

/**
 * Test: ThemeManager is a singleton
 * Verifies that multiple calls to getInstance() return the same instance
 */
void test_theme_manager_singleton() {
    ThemeManager& instance1 = ThemeManager::getInstance();
    ThemeManager& instance2 = ThemeManager::getInstance();

    TEST_ASSERT_EQUAL_PTR(&instance1, &instance2);
}

/**
 * Test: Default theme is loaded on initialization
 * Scenario: Default Theme Initialization from feature spec
 */
void test_default_theme_initialization() {
    const Theme* theme = ThemeManager::getInstance().getTheme();

    TEST_ASSERT_NOT_NULL(theme);
    TEST_ASSERT_EQUAL_UINT16(THEME_BACKGROUND, theme->colors.background);
    TEST_ASSERT_EQUAL_UINT16(THEME_PRIMARY, theme->colors.primary);
    TEST_ASSERT_EQUAL_UINT16(THEME_TEXT, theme->colors.text_main);
}

/**
 * Test: Accessing active theme colors
 * Scenario: Accessing Active Theme Colors from feature spec
 */
void test_access_theme_colors() {
    const Theme* theme = ThemeManager::getInstance().getTheme();

    // Verify we can access background color as the spec scenario describes
    uint16_t bg_color = theme->colors.background;
    TEST_ASSERT_EQUAL_UINT16(THEME_BACKGROUND, bg_color);

    // Verify other semantic colors
    TEST_ASSERT_EQUAL_UINT16(THEME_PRIMARY, theme->colors.primary);
    TEST_ASSERT_EQUAL_UINT16(THEME_SECONDARY, theme->colors.secondary);
    TEST_ASSERT_EQUAL_UINT16(THEME_ACCENT, theme->colors.accent);
}

/**
 * Test: Accessing theme fonts
 * Scenario: Accessing Theme Fonts from feature spec
 */
void test_access_theme_fonts() {
    const Theme* theme = ThemeManager::getInstance().getTheme();

    // Verify we can access the heading font as the spec scenario describes
    const GFXfont* heading_font = theme->fonts.heading;
    TEST_ASSERT_EQUAL_PTR(FONT_HEADING, heading_font);

    // Verify all typography levels are accessible
    TEST_ASSERT_EQUAL_PTR(FONT_SMALLEST, theme->fonts.smallest);
    TEST_ASSERT_EQUAL_PTR(FONT_NORMAL, theme->fonts.normal);
    TEST_ASSERT_EQUAL_PTR(FONT_UI, theme->fonts.ui);
    TEST_ASSERT_EQUAL_PTR(FONT_TITLE, theme->fonts.title);
}

/**
 * Test: Dynamic theme switching
 * Scenario: Dynamic Theme Switching from feature spec
 */
void test_dynamic_theme_switching() {
    // Create an alternative theme (HighContrastLight)
    Theme high_contrast_light = {
        // colors
        {
            .background = 0xFFFF,        // White
            .surface = 0xDEDB,           // Light grey
            .primary = 0x001F,           // Blue
            .secondary = 0x7800,         // Red
            .accent = 0xFFE0,            // Yellow
            .text_main = 0x0000,         // Black
            .text_secondary = 0x4208,    // Dark grey

            .graph_axes = 0x4208,
            .graph_ticks = 0x2104,
            .axis_labels = 0x0000,
            .data_labels = 0x001F
        },
        // fonts (use same fonts for this test)
        {
            .smallest = FONT_SMALLEST,
            .normal = FONT_NORMAL,
            .ui = FONT_UI,
            .heading = FONT_HEADING,
            .title = FONT_TITLE
        }
    };

    // Verify initial theme is default (DefaultDark)
    const Theme* initial_theme = ThemeManager::getInstance().getTheme();
    TEST_ASSERT_EQUAL_UINT16(THEME_BACKGROUND, initial_theme->colors.background);

    // Switch to HighContrastLight
    ThemeManager::getInstance().setTheme(&high_contrast_light);

    // Verify the theme has switched
    const Theme* new_theme = ThemeManager::getInstance().getTheme();
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, new_theme->colors.background);
    TEST_ASSERT_EQUAL_UINT16(0x001F, new_theme->colors.primary);
    TEST_ASSERT_EQUAL_UINT16(0x0000, new_theme->colors.text_main);
}

/**
 * Test: setTheme with nullptr should be ignored
 * Defensive programming test
 */
void test_set_theme_null_ignored() {
    const Theme* original_theme = ThemeManager::getInstance().getTheme();

    // Try to set null theme
    ThemeManager::getInstance().setTheme(nullptr);

    // Verify theme hasn't changed
    const Theme* current_theme = ThemeManager::getInstance().getTheme();
    TEST_ASSERT_EQUAL_PTR(original_theme, current_theme);
}

/**
 * Test: getDefaultTheme returns valid theme
 */
void test_get_default_theme() {
    const Theme* default_theme = ThemeManager::getDefaultTheme();

    TEST_ASSERT_NOT_NULL(default_theme);
    TEST_ASSERT_EQUAL_UINT16(THEME_BACKGROUND, default_theme->colors.background);
    TEST_ASSERT_EQUAL_PTR(FONT_HEADING, default_theme->fonts.heading);
}

/**
 * Test: Graph-specific semantic colors
 * Verifies the graph color mappings from theme_colors.h
 */
void test_graph_semantic_colors() {
    const Theme* theme = ThemeManager::getInstance().getTheme();

    TEST_ASSERT_EQUAL_UINT16(THEME_GRAPH_AXES, theme->colors.graph_axes);
    TEST_ASSERT_EQUAL_UINT16(THEME_GRAPH_TICKS, theme->colors.graph_ticks);
    TEST_ASSERT_EQUAL_UINT16(THEME_AXIS_LABELS, theme->colors.axis_labels);
    TEST_ASSERT_EQUAL_UINT16(THEME_DATA_LABELS, theme->colors.data_labels);
}

// ==========================================
// Main Test Runner
// ==========================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_theme_manager_singleton);
    RUN_TEST(test_default_theme_initialization);
    RUN_TEST(test_access_theme_colors);
    RUN_TEST(test_access_theme_fonts);
    RUN_TEST(test_dynamic_theme_switching);
    RUN_TEST(test_set_theme_null_ignored);
    RUN_TEST(test_get_default_theme);
    RUN_TEST(test_graph_semantic_colors);

    return UNITY_END();
}
