/**
 * @file demo_incremental.cpp
 * @brief Incremental test to identify where the full demo fails
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n=== INCREMENTAL DEMO ===");
    Serial.flush();

    // Test 1: Include display.h
    Serial.println("[1/8] Testing display.h include...");
    Serial.flush();
    #include "display.h"
    Serial.println("  OK: display.h included");
    Serial.flush();

    // Test 2: Initialize display
    Serial.println("[2/8] Testing display init...");
    Serial.flush();
    if (!hal_display_init()) {
        Serial.println("  FAIL: Display init failed");
        while(1) delay(1000);
    }
    Serial.println("  OK: Display initialized");
    Serial.flush();

    // Test 3: Test relative_display
    Serial.println("[3/8] Testing relative_display include...");
    Serial.flush();
    #include "relative_display.h"
    display_relative_init();
    Serial.println("  OK: RelativeDisplay initialized");
    Serial.flush();

    // Test 4: Get GFX object
    Serial.println("[4/8] Testing GFX object...");
    Serial.flush();
    Arduino_GFX* display = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    if (display == nullptr) {
        Serial.println("  FAIL: GFX object is null");
        while(1) delay(1000);
    }
    Serial.println("  OK: GFX object obtained");
    Serial.flush();

    // Test 5: Test AnimationTicker
    Serial.println("[5/8] Testing AnimationTicker...");
    Serial.flush();
    #include "animation_ticker.h"
    static AnimationTicker ticker(30);
    Serial.println("  OK: AnimationTicker created");
    Serial.flush();

    // Test 6: Test YahooChartParser with small data
    Serial.println("[6/8] Testing YahooChartParser...");
    Serial.flush();
    #include "yahoo_chart_parser.h"
    const char* small_json = R"({"chart":{"result":[{"timestamp":[1,2,3],"indicators":{"quote":[{"close":[4.27,4.28,4.29]}]}}]}})";
    YahooChartParser parser("");
    if (!parser.parseFromString(small_json)) {
        Serial.println("  FAIL: Parser failed on small data");
    } else {
        Serial.println("  OK: Parser works");
    }
    Serial.flush();

    // Test 7: Test TimeSeriesGraph include
    Serial.println("[7/8] Testing TimeSeriesGraph include...");
    Serial.flush();
    #include "ui_time_series_graph.h"
    Serial.println("  OK: TimeSeriesGraph header included");
    Serial.flush();

    // Test 8: Try to create TimeSeriesGraph
    Serial.println("[8/8] Testing TimeSeriesGraph creation...");
    Serial.flush();

    GraphTheme theme = {};
    theme.backgroundColor = 0x4810;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("  Display size: %d x %d\n", width, height);
    Serial.flush();

    Serial.println("  Creating TimeSeriesGraph object...");
    Serial.flush();

    static TimeSeriesGraph graph(theme, display, width, height);
    Serial.println("  OK: TimeSeriesGraph object created");
    Serial.flush();

    Serial.println("  Calling graph.begin()...");
    Serial.flush();

    if (!graph.begin()) {
        Serial.println("  FAIL: graph.begin() returned false");
        while(1) delay(1000);
    }

    Serial.println("  OK: graph.begin() succeeded");
    Serial.flush();

    hal_display_clear(RGB565_GREEN);
    hal_display_flush();

    Serial.println("\n=== ALL TESTS PASSED ===");
    Serial.println("Screen should be GREEN");
    Serial.flush();
}

void loop() {
    delay(1000);
}
