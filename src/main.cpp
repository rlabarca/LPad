/**
 * @file main.cpp
 * @brief TimeSeriesGraph Component Demo
 *
 * This application demonstrates the TimeSeriesGraph UI component
 * rendering time-series data with automatic scaling and a vaporwave theme.
 *
 * Demo Sequence:
 * 1. Graph with rising trend (simulated bond yields)
 * 2. Graph with volatility (price fluctuations)
 * 3. Graph with different scale (larger values)
 *
 * Each graph demonstrates:
 * - Resolution-independent rendering via RelativeDisplay
 * - Automatic Y-axis scaling
 * - Smooth line drawing
 * - Vaporwave aesthetic (dark purple, cyan, magenta)
 */

#include <Arduino.h>
#include "../hal/display.h"
#include "relative_display.h"
#include "ui_time_series_graph.h"

// RGB565 color definitions
#define RGB565_BLACK       0x0000
#define RGB565_WHITE       0xFFFF
#define RGB565_CYAN        0x07FF
#define RGB565_MAGENTA     0xF81F
#define RGB565_DARK_PURPLE 0x4810
#define RGB565_PINK        0xFE19

// Create vaporwave theme
GraphTheme createVaporwaveTheme() {
    GraphTheme theme;
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;
    return theme;
}

// Create alternate pink theme
GraphTheme createPinkTheme() {
    GraphTheme theme;
    theme.backgroundColor = RGB565_BLACK;
    theme.lineColor = RGB565_PINK;
    theme.axisColor = RGB565_CYAN;
    return theme;
}

void print_display_info() {
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    Serial.println("=== Display Information ===");
    Serial.printf("Resolution: %d x %d pixels\n", width, height);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== TimeSeriesGraph Component Demo ===");
    Serial.println();

    // Initialize HAL
    Serial.println("[1/4] Initializing display HAL...");
    if (!hal_display_init()) {
        Serial.println("  [FAIL] Display initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Display initialized");

    // Apply rotation if configured via build flag
#ifdef APP_DISPLAY_ROTATION
    Serial.printf("  [INFO] Applying rotation: %d degrees\n", APP_DISPLAY_ROTATION);
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
#endif

    delay(500);
    print_display_info();

    // Initialize relative display abstraction
    Serial.println("[2/4] Initializing relative display abstraction...");
    display_relative_init();
    Serial.println("  [PASS] Relative display initialized");
    Serial.println();
    delay(500);

    // Create vaporwave theme graph
    Serial.println("[3/4] Creating TimeSeriesGraph with vaporwave theme...");
    Serial.println("  Theme: Dark Purple background, Cyan line, Magenta axes");
    GraphTheme vaporwave = createVaporwaveTheme();
    TimeSeriesGraph graph(vaporwave);
    Serial.println("  [PASS] Graph created");
    Serial.println();
    delay(500);

    // Demo 1: Rising trend (simulated bond yields)
    Serial.println("[4/4] Demo 1: Rising Bond Yields");
    Serial.println("  Data: 10-year treasury yields rising from 4.0% to 4.5%");
    GraphData data1;
    data1.x_values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    data1.y_values = {4.00, 4.05, 4.08, 4.12, 4.15, 4.18, 4.22, 4.25, 4.28, 4.32, 4.35, 4.38, 4.42, 4.45, 4.48};

    Serial.printf("  Y-axis range: %.2f to %.2f\n", 4.00, 4.48);
    Serial.printf("  Data points: %d\n", data1.y_values.size());

    graph.setData(data1);
    graph.draw();
    hal_display_flush();

    Serial.println("  [PASS] Graph 1 displayed");
    Serial.println();
    delay(5000);

    // Demo 2: Volatile price action
    Serial.println("Demo 2: Volatile Price Action");
    Serial.println("  Data: Price fluctuations with peaks and valleys");
    GraphData data2;
    data2.x_values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    data2.y_values = {100.0, 105.0, 98.0, 110.0, 95.0, 115.0, 102.0, 108.0, 112.0, 106.0, 118.0, 120.0};

    Serial.printf("  Y-axis range: %.2f to %.2f\n", 95.0, 120.0);
    Serial.printf("  Data points: %d\n", data2.y_values.size());

    graph.setData(data2);
    graph.draw();
    hal_display_flush();

    Serial.println("  [PASS] Graph 2 displayed");
    Serial.println();
    delay(5000);

    // Demo 3: Different scale with pink theme
    Serial.println("Demo 3: Large Values with Pink Theme");
    Serial.println("  Theme: Black background, Pink line, Cyan axes");
    GraphTheme pink = createPinkTheme();
    TimeSeriesGraph graph2(pink);

    GraphData data3;
    data3.x_values = {1, 2, 3, 4, 5, 6, 7, 8};
    data3.y_values = {1500.0, 1600.0, 1550.0, 1700.0, 1650.0, 1750.0, 1800.0, 1850.0};

    Serial.printf("  Y-axis range: %.2f to %.2f\n", 1500.0, 1850.0);
    Serial.printf("  Data points: %d\n", data3.y_values.size());

    graph2.setData(data3);
    graph2.draw();
    hal_display_flush();

    Serial.println("  [PASS] Graph 3 displayed");
    Serial.println();

    Serial.println("=== Demo Complete ===");
    Serial.println("Visual Verification:");
    Serial.println("  [ ] Graph background fills entire screen");
    Serial.println("  [ ] Axes drawn at graph margins (left & bottom)");
    Serial.println("  [ ] Data line smoothly connects all points");
    Serial.println("  [ ] Line color matches theme");
    Serial.println("  [ ] Graphs rescale automatically for different data ranges");
    Serial.println();
}

void loop() {
    // Demo complete, could cycle through graphs here
    delay(5000);

    // Cycle back to demo 1
    Serial.println("Cycling back to Demo 1...");
    GraphTheme vaporwave = createVaporwaveTheme();
    TimeSeriesGraph graph(vaporwave);

    GraphData data1;
    data1.x_values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    data1.y_values = {4.00, 4.05, 4.08, 4.12, 4.15, 4.18, 4.22, 4.25, 4.28, 4.32, 4.35, 4.38, 4.42, 4.45, 4.48};

    graph.setData(data1);
    graph.draw();
    hal_display_flush();

    delay(5000);

    // Show demo 2
    Serial.println("Showing Demo 2...");
    GraphData data2;
    data2.x_values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    data2.y_values = {100.0, 105.0, 98.0, 110.0, 95.0, 115.0, 102.0, 108.0, 112.0, 106.0, 118.0, 120.0};

    graph.setData(data2);
    graph.draw();
    hal_display_flush();

    delay(5000);

    // Show demo 3
    Serial.println("Showing Demo 3...");
    GraphTheme pink = createPinkTheme();
    TimeSeriesGraph graph2(pink);

    GraphData data3;
    data3.x_values = {1, 2, 3, 4, 5, 6, 7, 8};
    data3.y_values = {1500.0, 1600.0, 1550.0, 1700.0, 1650.0, 1750.0, 1800.0, 1850.0};

    graph2.setData(data3);
    graph2.draw();
    hal_display_flush();
}
