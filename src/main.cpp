/**
 * @file main.cpp
 * @brief TEMPORARY: Canvas Drawing HIL Test
 *
 * This is a temporary modification to test the canvas-based drawing feature.
 * See features/display_canvas_drawing.md for specification.
 *
 * This will be reverted after HIL testing is complete.
 */

#include <Arduino.h>
#include "../hal/display.h"

// RGB565 color definitions
#define RGB565_BLUE   0x001F
#define RGB565_RED    0xF800
#define RGB565_BLACK  0x0000

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Canvas Drawing HIL Test ===");
    Serial.println();

    // Step 1: Initialize the display
    Serial.println("[1/6] Initializing display HAL...");
    if (!hal_display_init()) {
        Serial.println("  [FAIL] Display initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Display initialized");

    // Apply rotation if configured
#ifdef APP_DISPLAY_ROTATION
    Serial.printf("  [INFO] Applying rotation: %d degrees\n", APP_DISPLAY_ROTATION);
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
#endif

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("  [INFO] Display resolution: %d x %d pixels\n", width, height);
    Serial.println();

    // Clear display to black
    Serial.println("[2/6] Clearing display to black...");
    hal_display_clear(RGB565_BLACK);
    hal_display_flush();
    Serial.println("  [PASS] Display cleared");
    Serial.println();
    delay(1000);

    // Step 2: Create a 100x100 pixel canvas and fill it with blue
    Serial.println("[3/6] Creating background canvas (100x100 blue)...");
    hal_canvas_handle_t bg_canvas = hal_display_canvas_create(100, 100);
    if (bg_canvas == nullptr) {
        Serial.println("  [FAIL] Failed to create background canvas");
        while (1) delay(1000);
    }
    hal_display_canvas_fill(bg_canvas, RGB565_BLUE);
    Serial.println("  [PASS] Background canvas created and filled with blue");
    Serial.println();

    // Step 3: Create a 40x40 pixel canvas and fill it with red
    Serial.println("[4/6] Creating foreground canvas (40x40 red)...");
    hal_canvas_handle_t fg_canvas = hal_display_canvas_create(40, 40);
    if (fg_canvas == nullptr) {
        Serial.println("  [FAIL] Failed to create foreground canvas");
        hal_display_canvas_delete(bg_canvas);
        while (1) delay(1000);
    }
    hal_display_canvas_fill(fg_canvas, RGB565_RED);
    Serial.println("  [PASS] Foreground canvas created and filled with red");
    Serial.println();

    // Step 4: Draw background canvas to screen at (50, 50)
    Serial.println("[5/6] Drawing background canvas at (50, 50)...");
    hal_display_canvas_draw(bg_canvas, 50, 50);
    hal_display_flush();
    Serial.println("  [PASS] Background canvas drawn");
    delay(500);

    // Step 5: Draw foreground canvas at (80, 80) to overlap
    Serial.println("[6/6] Drawing foreground canvas at (80, 80)...");
    hal_display_canvas_draw(fg_canvas, 80, 80);
    hal_display_flush();
    Serial.println("  [PASS] Foreground canvas drawn (overlapping)");
    Serial.println();

    // Visual verification instructions
    Serial.println("=== Visual Verification ===");
    Serial.println("Expected result:");
    Serial.println("  - Blue square (100x100) at position (50, 50)");
    Serial.println("  - Red square (40x40) at position (80, 80)");
    Serial.println("  - Red square should overlap the bottom-right corner of blue square");
    Serial.println();
    Serial.println("If you see this pattern, the canvas feature is working correctly!");
    Serial.println();

    // Step 6: Clean up canvases
    Serial.println("Cleaning up canvases...");
    hal_display_canvas_delete(bg_canvas);
    hal_display_canvas_delete(fg_canvas);
    Serial.println("  [PASS] Canvases deleted");
    Serial.println();

    Serial.println("=== HIL Test Complete ===");
}

void loop() {
    // Empty loop as per specification
    delay(1000);
}
