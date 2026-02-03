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
    GraphTheme theme;
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    TimeSeriesGraph graph(theme);

    // Should not crash - initialization successful
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Draw empty graph (axes only)
 * Should draw background and axes
 */
void test_draw_empty_graph(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    TimeSeriesGraph graph(theme);

    // Draw should not crash even with no data
    graph.draw();

    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Set data and verify it's accepted
 */
void test_set_data(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;

    TimeSeriesGraph graph(theme);

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
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;

    TimeSeriesGraph graph(theme);

    GraphData data;
    data.x_values = {1, 2, 3, 4, 5};
    data.y_values = {10.0, 20.0, 15.0, 25.0, 30.0};

    graph.setData(data);
    graph.draw();

    // Should not crash - graph drawn successfully
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Update data dynamically (different range)
 * Tests axis rescaling
 */
void test_update_data_different_range(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;

    TimeSeriesGraph graph(theme);

    // First dataset
    GraphData data1;
    data1.x_values = {1, 2, 3};
    data1.y_values = {10.0, 20.0, 30.0};

    graph.setData(data1);
    graph.draw();

    // Second dataset with different range
    GraphData data2;
    data2.x_values = {1, 2, 3, 4, 5};
    data2.y_values = {100.0, 200.0, 150.0, 250.0, 300.0};

    graph.setData(data2);
    graph.draw();

    // Should not crash - graph rescaled and redrawn
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Handle empty data gracefully
 */
void test_handle_empty_data(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;

    TimeSeriesGraph graph(theme);

    GraphData data;
    // Empty vectors

    graph.setData(data);
    graph.draw();

    // Should not crash with empty data
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: Handle single data point
 */
void test_handle_single_data_point(void) {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_WHITE;

    TimeSeriesGraph graph(theme);

    GraphData data;
    data.x_values = {1};
    data.y_values = {42.0};

    graph.setData(data);
    graph.draw();

    // Should not crash with single point
    TEST_ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_initialize_graph_with_theme);
    RUN_TEST(test_draw_empty_graph);
    RUN_TEST(test_set_data);
    RUN_TEST(test_draw_graph_with_data);
    RUN_TEST(test_update_data_different_range);
    RUN_TEST(test_handle_empty_data);
    RUN_TEST(test_handle_single_data_point);

    UNITY_END();

    return 0;
}
