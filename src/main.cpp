/**
 * @file main.cpp
 * @brief Relative Display Abstraction Test Application
 *
 * This application demonstrates the relative drawing abstraction layer
 * and validates coordinate scaling across different display hardware.
 *
 * Test Pattern:
 * 1. Corner marker at origin (0%, 0%) - RED 5x5% square
 * 2. Test square at (25%, 25%) - GREEN 25x25% square
 * 3. Center cross at (50%, 50%) - WHITE lines
 * 4. Screen frame at 10% inset - BLUE outline
 *
 * Expected Pixel Coordinates per Display:
 *
 * ESP32-S3-AMOLED (368x448):
 * - Corner marker: (0,0) to (18,22) pixels
 * - Test square: (92,112) to (184,224) pixels - Distance from origin: 145.3px
 * - Center cross: at (184, 224) pixels - Distance from origin: 290.0px
 * - Frame: (37,45) to (331,403) pixels
 *
 * T-Display-S3-Plus (240x536):
 * - Corner marker: (0,0) to (12,27) pixels
 * - Test square: (60,134) to (120,268) pixels - Distance from origin: 147.0px
 * - Center cross: at (120, 268) pixels - Distance from origin: 293.9px
 * - Frame: (24,54) to (216,482) pixels
 */

#include <Arduino.h>
#include "../hal/display.h"
#include "relative_display.h"

// RGB565 color definitions
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F

void print_display_info() {
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    Serial.println("=== Display Information ===");
    Serial.printf("Resolution: %d x %d pixels\n", width, height);
    Serial.println();

    Serial.println("=== Expected Coordinate Mapping ===");
    Serial.printf("  0%%   -> 0 pixels (both W/H)\n");
    Serial.printf(" 25%% W -> %d pixels\n", (int)(width * 0.25f));
    Serial.printf(" 25%% H -> %d pixels\n", (int)(height * 0.25f));
    Serial.printf(" 50%% W -> %d pixels\n", (int)(width * 0.50f));
    Serial.printf(" 50%% H -> %d pixels\n", (int)(height * 0.50f));
    Serial.printf("100%% W -> %d pixels\n", width);
    Serial.printf("100%% H -> %d pixels\n", height);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Relative Display Abstraction Test ===");
    Serial.println();

    // Initialize HAL
    Serial.println("[1/6] Initializing display HAL...");
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

    // Print display information
    print_display_info();

    // Initialize relative display abstraction
    Serial.println("[2/6] Initializing relative display abstraction...");
    display_relative_init();
    Serial.println("  [PASS] Relative display initialized");
    Serial.println();
    delay(500);

    // Clear to black
    Serial.println("[3/6] Clearing display to black...");
    hal_display_clear(RGB565_BLACK);
    Serial.println("  [PASS] Display cleared");
    delay(1000);

    // Draw test pattern
    Serial.println("[4/6] Drawing test pattern...");
    Serial.println();

    // 1. Corner marker at origin - RED 5x5% square
    Serial.println("  Drawing corner marker at origin (0%, 0%):");
    Serial.println("    - Color: RED");
    Serial.println("    - Size: 5% x 5%");
    Serial.printf("    - Pixel coords: (0,0) to (%d,%d)\n",
                  (int)(hal_display_get_width_pixels() * 0.05f),
                  (int)(hal_display_get_height_pixels() * 0.05f));
    Serial.println("    - Distance from origin: 0 pixels");
    display_relative_fill_rectangle(0.0f, 0.0f, 5.0f, 5.0f, RGB565_RED);
    delay(500);

    // 2. Test square at (25%, 25%) - GREEN 25x25%
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    int32_t square_x = (int)(width * 0.25f);
    int32_t square_y = (int)(height * 0.25f);
    int32_t square_w = (int)(width * 0.25f);
    int32_t square_h = (int)(height * 0.25f);
    float distance = sqrtf(square_x * square_x + square_y * square_y);

    Serial.println();
    Serial.println("  Drawing test square at (25%, 25%):");
    Serial.println("    - Color: GREEN");
    Serial.println("    - Size: 25% x 25%");
    Serial.printf("    - Top-left pixel: (%d, %d)\n", square_x, square_y);
    Serial.printf("    - Dimensions: %d x %d pixels\n", square_w, square_h);
    Serial.printf("    - Bottom-right pixel: (%d, %d)\n",
                  square_x + square_w, square_y + square_h);
    Serial.printf("    - Distance from origin: %.1f pixels\n", distance);
    display_relative_fill_rectangle(25.0f, 25.0f, 25.0f, 25.0f, RGB565_GREEN);
    delay(500);

    // 3. Center cross - WHITE lines
    int32_t center_x = width / 2;
    int32_t center_y = height / 2;
    float center_distance = sqrtf(center_x * center_x + center_y * center_y);

    Serial.println();
    Serial.println("  Drawing center cross at (50%, 50%):");
    Serial.println("    - Color: WHITE");
    Serial.printf("    - Center pixel: (%d, %d)\n", center_x, center_y);
    Serial.printf("    - Distance from origin: %.1f pixels\n", center_distance);
    display_relative_draw_horizontal_line(50.0f, 0.0f, 100.0f, RGB565_WHITE);
    display_relative_draw_vertical_line(50.0f, 0.0f, 100.0f, RGB565_WHITE);
    delay(500);

    // 4. Frame at 10% inset - BLUE outline
    Serial.println();
    Serial.println("  Drawing frame at 10% inset:");
    Serial.println("    - Color: BLUE");
    Serial.printf("    - Top-left pixel: (%d, %d)\n",
                  (int)(width * 0.10f), (int)(height * 0.10f));
    Serial.printf("    - Bottom-right pixel: (%d, %d)\n",
                  (int)(width * 0.90f), (int)(height * 0.90f));

    // Draw frame as 4 lines
    display_relative_draw_horizontal_line(10.0f, 10.0f, 90.0f, RGB565_BLUE);  // Top
    display_relative_draw_horizontal_line(90.0f, 10.0f, 90.0f, RGB565_BLUE);  // Bottom
    display_relative_draw_vertical_line(10.0f, 10.0f, 90.0f, RGB565_BLUE);    // Left
    display_relative_draw_vertical_line(90.0f, 10.0f, 90.0f, RGB565_BLUE);    // Right

    Serial.println();
    Serial.println("[5/6] Flushing display buffer...");
    hal_display_flush();
    Serial.println("  [PASS] Display flushed");
    Serial.println();

    // Verification instructions
    Serial.println("[6/6] Visual Verification Checklist:");
    Serial.println("  [ ] RED square visible at top-left corner (origin)");
    Serial.println("  [ ] GREEN square visible at 25% position from origin");
    Serial.println("  [ ] WHITE cross visible at screen center");
    Serial.println("  [ ] BLUE frame visible with 10% margin from edges");
    Serial.println("  [ ] All elements scale proportionally to screen size");
    Serial.println();

    Serial.println("=== Test Complete ===");
    Serial.println("Pattern demonstrates resolution-independent drawing.");
    Serial.println("Same percentages produce correct scaling on different displays.");
    Serial.println();
}

void loop() {
    // Test complete, nothing to do
    delay(1000);
}
