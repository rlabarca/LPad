/**
 * @file main.cpp
 * @brief Display Baseline Test Application
 *
 * This application tests the Display HAL implementation for ESP32-S3-AMOLED.
 * It implements the scenarios from features/display_baseline.md.
 */

#include <Arduino.h>
#include "../hal/display.h"

// RGB565 color definitions
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_BLUE    0x001F

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Display Baseline Test Application ===");
    Serial.println();

    // Scenario 1: Successful Display Initialization
    Serial.println("Scenario 1: Testing display initialization...");
    if (hal_display_init()) {
        Serial.println("  [PASS] Display initialized successfully");
        Serial.println("  [INFO] Backlight should be ON (visual check required)");
    } else {
        Serial.println("  [FAIL] Display initialization failed");
        Serial.println("  [ERROR] Cannot proceed with further tests");
        while (1) {
            delay(1000);  // Halt execution
        }
    }
    delay(2000);

    // Scenario 2: Clear Display to a Solid Color (Blue)
    Serial.println();
    Serial.println("Scenario 2: Testing clear display to blue...");
    hal_display_clear(RGB565_BLUE);
    Serial.println("  [PASS] Clear command executed");
    Serial.println("  [INFO] Display should show solid BLUE (visual check required)");
    delay(3000);

    // Clear to black for better pixel visibility
    Serial.println();
    Serial.println("Clearing to black for pixel test...");
    hal_display_clear(RGB565_BLACK);
    delay(1000);

    // Scenario 3: Draw a Single Pixel
    Serial.println();
    Serial.println("Scenario 3: Testing draw single pixel...");
    Serial.println("  Drawing white pixel at (100, 100)...");
    hal_display_draw_pixel(100, 100, RGB565_WHITE);
    Serial.println("  Flushing display buffer...");
    hal_display_flush();
    Serial.println("  [PASS] Draw pixel and flush commands executed");
    Serial.println("  [INFO] White pixel should be visible at (100, 100)");
    Serial.println("        (visual check required - may be hard to see)");
    delay(3000);

    // Additional visual test: Draw a cross pattern for easier viewing
    Serial.println();
    Serial.println("Drawing cross pattern for visual verification...");
    // Horizontal line
    for (int x = 95; x <= 105; x++) {
        hal_display_draw_pixel(x, 100, RGB565_WHITE);
    }
    // Vertical line
    for (int y = 95; y <= 105; y++) {
        hal_display_draw_pixel(100, y, RGB565_WHITE);
    }
    hal_display_flush();
    Serial.println("  [INFO] White cross pattern at (100, 100) visible");

    Serial.println();
    Serial.println("=== All Display Baseline Tests Complete ===");
    Serial.println("Please visually verify:");
    Serial.println("  1. Display backlight is ON");
    Serial.println("  2. Blue screen was displayed");
    Serial.println("  3. White cross pattern is visible on black background");
}

void loop() {
    // Tests complete, nothing to do in loop
    delay(1000);
}
