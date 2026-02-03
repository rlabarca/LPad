/**
 * @file main.cpp
 * @brief AnimationTicker HIL Test - Moving Box
 *
 * TEMPORARY: This is a Hardware-In-Loop test for the AnimationTicker feature.
 * See features/app_animation_ticker.md for specification.
 *
 * To restore bond tracker, see git history.
 */

#include <Arduino.h>
#include "../hal/display.h"
#include "relative_display.h"
#include "animation_ticker.h"

// RGB565 color definitions
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_CYAN    0x07FF
#define RGB565_RED     0xF800

// Animation state
int32_t box_x = 0;
int32_t box_y = 0;
const int32_t BOX_SIZE = 20;  // 20 pixels
const int32_t BOX_SPEED = 3;  // pixels per frame (3 pixels = 90 pixels/sec at 30fps)
int32_t display_width = 0;
int32_t display_height = 0;

// Canvas for double-buffering
hal_canvas_handle_t canvas = nullptr;

// Animation ticker for 30fps
AnimationTicker ticker(30);

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== AnimationTicker HIL Test ===");
    Serial.println();

    // Initialize display HAL
    Serial.println("[1/3] Initializing display...");
    if (!hal_display_init()) {
        Serial.println("  [FAIL] Display initialization failed");
        hal_display_clear(RGB565_RED);
        hal_display_flush();
        while (1) delay(1000);
    }

#ifdef APP_DISPLAY_ROTATION
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
#endif

    display_width = hal_display_get_width_pixels();
    display_height = hal_display_get_height_pixels();
    Serial.printf("  [PASS] Display: %d x %d pixels\n", display_width, display_height);

    // Initialize relative display
    Serial.println("[2/3] Initializing RelativeDisplay...");
    display_relative_init();
    Serial.println("  [PASS] RelativeDisplay initialized");

    // Create canvas for double-buffering
    Serial.println("[3/4] Creating canvas for double-buffering...");
    canvas = hal_display_canvas_create(display_width, display_height);
    if (canvas == nullptr) {
        Serial.println("  [FAIL] Failed to create canvas");
        hal_display_clear(RGB565_RED);
        hal_display_flush();
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Canvas created");

    // Initialize box position
    box_x = 0;
    box_y = display_height / 2 - BOX_SIZE / 2;

    // Clear screen
    Serial.println("[4/4] Starting animation test...");
    Serial.println("  Expect: Smooth 30fps moving box (cyan on black)");
    Serial.printf("  Box size: %dpx, Speed: %dpx/frame = %dpx/sec at 30fps\n",
                  BOX_SIZE, BOX_SPEED, BOX_SPEED * 30);
    hal_display_clear(RGB565_BLACK);
    hal_display_flush();

    Serial.println("=== Test Running ===");
    Serial.println();
}

void loop() {
    // Select canvas as drawing target
    hal_display_canvas_select(canvas);

    // Clear canvas to black
    hal_display_canvas_fill(canvas, RGB565_BLACK);

    // Update box position
    box_x += BOX_SPEED;

    // Wrap around when box reaches right edge
    if (box_x > display_width) {
        box_x = -BOX_SIZE;
    }

    // Draw the box at new position on canvas
    for (int32_t dy = 0; dy < BOX_SIZE; dy++) {
        for (int32_t dx = 0; dx < BOX_SIZE; dx++) {
            hal_display_draw_pixel(box_x + dx, box_y + dy, RGB565_CYAN);
        }
    }

    // Re-select main display
    hal_display_canvas_select(nullptr);

    // Blit canvas to display (fast transfer)
    hal_display_canvas_draw(canvas, 0, 0);
    hal_display_flush();

    // Wait for next frame (30fps timing)
    ticker.waitForNextFrame();
}
