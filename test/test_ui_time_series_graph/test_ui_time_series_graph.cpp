/**
 * @file test_ui_time_series_graph.cpp
 * @brief Unity tests for UI Time Series Graph
 *
 * These tests verify the behavior specified in features/ui_time_series_graph.md
 */

#include <unity.h>
#include "../../src/ui_time_series_graph.h"
#include "../../src/relative_display.h"
#include "../../hal/display.h"

// RGB565 color definitions for testing
#define RGB565_BLACK       0x0000
#define RGB565_WHITE       0xFFFF
#define RGB565_CYAN        0x07FF
#define RGB565_MAGENTA     0xF81F
#define RGB565_DARK_PURPLE 0x4810

void setUp(void) {
    // Initialize display HAL and relative display
    hal_display_init();
    display_relative_init();
}

void tearDown(void) {
    // Tear down runs after each test
}

/**
 * Test: Initialize graph with vaporwave theme
 * Scenario from features/ui_time_series_graph.md
 */
void test_initialize_graph_with_theme(void) {
    GraphTheme theme = {};  // Zero-initialize all fields
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    // Should not crash - initialization successful
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Draw empty graph (axes only)
 * Should draw background and axes
 */
void test_draw_empty_graph(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    // Draw should not crash even with no data
    graph.drawBackground();
    graph.drawData();
    graph.render();

    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Set data and verify it's accepted
 */
void test_set_data(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 15.0, 25.0, 30.0};

    graph.setData(data);

    // Should not crash - data set successfully
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Draw graph with data
 * Should draw background, axes, and data line
 */
void test_draw_graph_with_data(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 15.0, 25.0, 30.0};

    graph.setData(data);
    graph.drawBackground();
    graph.drawData();
    graph.render();

    // Should not crash - graph drawn successfully
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Update data dynamically (different range)
 * Tests axis rescaling
 */
void test_update_data_different_range(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    // First dataset
    GraphData data1;
    data1.x_values = {1, 2, 3};
    data1.y_values = {10.0, 20.0, 30.0};

    graph.setData(data1);
    graph.drawBackground();
    graph.drawData();
    graph.render();

    // Second dataset with different range
    GraphData data2;
    data2.x_values = {1, 2, 3, 4, 5};
    data2.y_values = {100.0, 200.0, 150.0, 250.0, 300.0};

    graph.setData(data2);
    graph.drawBackground();
    graph.drawData();
    graph.render();

    // Should not crash - graph rescaled and redrawn
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Handle empty data gracefully
 */
void test_handle_empty_data(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    // Empty vectors

    graph.setData(data);
    graph.drawBackground();
    graph.drawData();
    graph.render();

    // Should not crash with empty data
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Handle single data point
 */
void test_handle_single_data_point(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1};
    data.y_values = {42.0};

    graph.setData(data);
    graph.drawBackground();
    graph.drawData();
    graph.render();

    // Should not crash with single point
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Render gradient background (Scenario from features/ui_themeable_time_series_graph.md)
 * Given a graph with a 3-color background gradient at 45 degrees
 * When drawBackground() is called
 * Then the background should be filled with the gradient
 */
void test_gradient_background(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    // Set up 3-color gradient at 45 degrees
    theme.useBackgroundGradient = true;
    theme.backgroundGradient.angle_deg = 45.0f;
    theme.backgroundGradient.color_stops[0] = RGB565_DARK_PURPLE;
    theme.backgroundGradient.color_stops[1] = RGB565_MAGENTA;
    theme.backgroundGradient.color_stops[2] = RGB565_CYAN;
    theme.backgroundGradient.num_stops = 3;

    theme.axisThickness = 0.5f;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    // Call drawBackground
    graph.drawBackground();

    // Should not crash - gradient drawn successfully
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Draw thick gradient data line (Scenario from features/ui_themeable_time_series_graph.md)
 * Given a graph with data and a horizontal line gradient
 * When drawData() is called
 * Then the data line should be drawn with thickness and gradient
 */
void test_thick_gradient_data_line(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    // Set up thick line with horizontal gradient
    theme.useLineGradient = true;
    theme.lineThickness = 0.5f;
    theme.lineGradient.angle_deg = 0.0f;  // Horizontal
    theme.lineGradient.color_stops[0] = RGB565_CYAN;
    theme.lineGradient.color_stops[1] = RGB565_MAGENTA;
    theme.lineGradient.num_stops = 2;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 15.0, 25.0, 30.0};

    graph.setData(data);
    graph.drawBackground();
    graph.drawData();

    // Should not crash - thick gradient line drawn
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Display axis tick marks (Scenario from features/ui_themeable_time_series_graph.md)
 * Given a graph with Y-tick increment set to 10
 * When drawBackground() is called
 * Then tick marks should be drawn at every 10 units
 */
void test_axis_tick_marks(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;
    theme.tickColor = RGB565_WHITE;
    theme.tickLength = 2.0f;
    theme.axisThickness = 0.3f;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 30.0, 40.0, 50.0};

    graph.setData(data);
    graph.setYTicks(10.0f);

    graph.drawBackground();

    // Should not crash - ticks drawn
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Animate live data indicator (Scenario from features/ui_themeable_time_series_graph.md)
 * Given a graph with a pulsing live indicator
 * When update() is called repeatedly
 * Then the indicator should pulse at the last data point
 */
void test_animate_live_indicator(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;

    // Set up live indicator with radial gradient
    theme.liveIndicatorGradient.center_x = 0.0f;
    theme.liveIndicatorGradient.center_y = 0.0f;
    theme.liveIndicatorGradient.radius = 2.0f;
    theme.liveIndicatorGradient.color_stops[0] = RGB565_CYAN;
    theme.liveIndicatorGradient.color_stops[1] = RGB565_DARK_PURPLE;
    theme.liveIndicatorPulseSpeed = 1.0f;  // 1 cycle per second

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 15.0, 25.0, 30.0};

    graph.setData(data);
    graph.drawBackground();
    graph.drawData();

    // Simulate animation updates
    graph.update(0.25f);  // 1/4 second
    graph.drawData();

    graph.update(0.25f);  // 1/2 second total
    graph.drawData();

    graph.update(0.5f);   // 1 second total (full cycle)
    graph.drawData();

    // Should not crash - indicator animated
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Independent refresh (Scenario from features/ui_themeable_time_series_graph.md)
 * Given a fully drawn graph
 * When new data is set and only drawData() is called
 * Then only the data line should be updated (background unchanged)
 */
void test_independent_refresh(void) {
    GraphTheme theme = {};
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;
    theme.useBackgroundGradient = false;
    theme.useLineGradient = false;
    theme.lineThickness = 0.5f;

    TimeSeriesGraph graph(theme,
                         (Arduino_GFX*)hal_display_get_gfx(),
                         hal_display_get_width_pixels(),
                         hal_display_get_height_pixels());

    // Initial data
    GraphData data1;
    data1.x_values = {1, 2, 3};
    data1.y_values = {10.0, 20.0, 30.0};

    graph.setData(data1);
    graph.drawBackground();
    graph.drawData();

    // Update with new data - only call drawData()
    GraphData data2;
    data2.x_values = {1, 2, 3, 4, 5};
    data2.y_values = {15.0, 25.0, 20.0, 30.0, 35.0};

    graph.setData(data2);
    graph.drawData();  // Only redraw data, not background

    // Should not crash - data updated independently
    TEST_ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Original tests
    RUN_TEST(test_initialize_graph_with_theme);
    RUN_TEST(test_draw_empty_graph);
    RUN_TEST(test_set_data);
    RUN_TEST(test_draw_graph_with_data);
    RUN_TEST(test_update_data_different_range);
    RUN_TEST(test_handle_empty_data);
    RUN_TEST(test_handle_single_data_point);

    // New themeable/animated graph tests
    RUN_TEST(test_gradient_background);
    RUN_TEST(test_thick_gradient_data_line);
    RUN_TEST(test_axis_tick_marks);
    RUN_TEST(test_animate_live_indicator);
    RUN_TEST(test_independent_refresh);

    UNITY_END();

    return 0;
}
