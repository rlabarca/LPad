/**
 * @file touch_cst816.cpp
 * @brief CST816 Touch Controller HAL Implementation
 *
 * Implements the touch HAL for ESP32-S3 AMOLED boards using the
 * CST816 touch controller via the SensorLib library.
 *
 * Supported Boards:
 * - ESP32-S3-Touch-AMOLED-1.8" (esp32s3 environment)
 * - T-Display S3 AMOLED Plus 1.91" (tdisplay_s3_plus environment)
 *
 * Specification: features/touch_cst816_implementation.md
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "touch.h"
#include <Arduino.h>
#include <Wire.h>
#include "TouchDrvCSTXXX.hpp"

// Board-specific pin configuration
#if defined(APP_DISPLAY_ROTATION)
    // T-Display S3 AMOLED Plus (1.91")
    #define TOUCH_SDA 3
    #define TOUCH_SCL 2
    #define TOUCH_INT 21
    #define TOUCH_RST -1  // Not used
    #define TOUCH_ADDR 0x15
    #define DISPLAY_ROTATION 90  // Landscape
#else
    // ESP32-S3-Touch-AMOLED (1.8")
    #define TOUCH_SDA 15
    #define TOUCH_SCL 14
    #define TOUCH_INT 21
    #define TOUCH_RST -1  // Not used (managed by PMU)
    #define TOUCH_ADDR 0x15
    #define DISPLAY_ROTATION 0  // Portrait
#endif

// Global touch driver instance
static TouchDrvCSTXXX g_touch;
static bool g_touch_initialized = false;

// Display dimensions (set during init based on display HAL)
static int16_t g_display_width = 0;
static int16_t g_display_height = 0;

// Forward declaration of display dimension getters from display HAL
extern "C" {
    int32_t hal_display_get_width_pixels(void);
    int32_t hal_display_get_height_pixels(void);
}

bool hal_touch_init(void) {
    if (g_touch_initialized) {
        return true;  // Already initialized
    }

    // Initialize I2C bus
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    // Configure INT pin as input for polling optimization
    pinMode(TOUCH_INT, INPUT);

    // Initialize touch controller
    g_touch.setPins(TOUCH_RST, TOUCH_INT);
    if (!g_touch.begin(Wire, TOUCH_ADDR, TOUCH_SDA, TOUCH_SCL)) {
        Serial.println("[HAL Touch] Failed to initialize CST816 controller");
        return false;
    }

    // Get display dimensions for coordinate mapping
    g_display_width = static_cast<int16_t>(hal_display_get_width_pixels());
    g_display_height = static_cast<int16_t>(hal_display_get_height_pixels());

    g_touch_initialized = true;
    Serial.println("[HAL Touch] CST816 initialized successfully");
    return true;
}

bool hal_touch_read(hal_touch_point_t* point) {
    if (!g_touch_initialized || !point) {
        return false;
    }

    // OPTIMIZATION: Check INT pin first to avoid unnecessary I2C reads
    // INT pin (GPIO 21) goes LOW when touch data is ready
    // If HIGH, no touch event is pending - skip I2C transaction
    if (digitalRead(TOUCH_INT) == HIGH) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;  // Success, just not pressed (fast path - no I2C)
    }

    // INT pin is LOW - touch data available, proceed with I2C read
    // Check if touch is pressed
    if (!g_touch.isPressed()) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;  // Success, just not pressed
    }

    // Read touch coordinates
    int16_t x[1], y[1];
    uint8_t point_count = g_touch.getPoint(x, y, 1);

    if (point_count == 0) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Debug: Log raw coordinates from touch controller
    Serial.printf("[HAL Touch] RAW: x=%d, y=%d, count=%d\n", x[0], y[0], point_count);

    // CRITICAL: The touch controller reports in a fixed resolution (e.g., 600x536)
    // that may differ from display pixel dimensions (e.g., 240x536).
    // We must scale to display dimensions BEFORE applying rotation transform.

    // Touch controller resolution (empirically determined from hardware)
    const int16_t TOUCH_WIDTH = 600;   // Touch X range: 0-599
    const int16_t TOUCH_HEIGHT = 536;  // Touch Y range: 0-535

    // Physical display dimensions (before rotation)
    const int16_t PHYSICAL_WIDTH = 240;
    const int16_t PHYSICAL_HEIGHT = 536;

    // Scale raw touch coordinates to physical display pixels
    int16_t scaled_x = (x[0] * PHYSICAL_WIDTH) / TOUCH_WIDTH;
    int16_t scaled_y = (y[0] * PHYSICAL_HEIGHT) / TOUCH_HEIGHT;

    Serial.printf("[HAL Touch] SCALED: x=%d, y=%d\n", scaled_x, scaled_y);

    // Apply coordinate transformation based on display rotation
    // The CST816 reports coordinates in the physical panel orientation,
    // but we need to match the logical display orientation set by the HAL

    int16_t transformed_x, transformed_y;

    #if DISPLAY_ROTATION == 0
        // Portrait mode (no transformation)
        transformed_x = scaled_x;
        transformed_y = scaled_y;
    #elif DISPLAY_ROTATION == 90
        // Landscape mode (90° clockwise)
        // Physical (0,0) at top-left → Logical (0,0) at top-left
        // Physical (max,0) at top-right → Logical (0,max) at bottom-left
        // Physical (0,max) at bottom-left → Logical (max,0) at top-right
        // Physical (max,max) at bottom-right → Logical (max,max) at bottom-right
        transformed_x = scaled_y;
        transformed_y = g_display_height - scaled_x;
    #elif DISPLAY_ROTATION == 180
        // Inverted portrait (180°)
        transformed_x = g_display_width - scaled_x;
        transformed_y = g_display_height - scaled_y;
    #elif DISPLAY_ROTATION == 270
        // Landscape mode (270° clockwise / 90° counter-clockwise)
        transformed_x = g_display_width - scaled_y;
        transformed_y = scaled_x;
    #else
        // Default: no transformation
        transformed_x = scaled_x;
        transformed_y = scaled_y;
    #endif

    // Debug: Log transformation
    Serial.printf("[HAL Touch] TRANSFORM: disp=%dx%d, rot=%d, trans_x=%d, trans_y=%d\n",
                  g_display_width, g_display_height, DISPLAY_ROTATION,
                  transformed_x, transformed_y);

    // Clamp coordinates to display bounds
    if (transformed_x < 0) transformed_x = 0;
    if (transformed_x >= g_display_width) transformed_x = g_display_width - 1;
    if (transformed_y < 0) transformed_y = 0;
    if (transformed_y >= g_display_height) transformed_y = g_display_height - 1;

    point->x = transformed_x;
    point->y = transformed_y;
    point->is_pressed = true;

    Serial.printf("[HAL Touch] FINAL: x=%d, y=%d\n", point->x, point->y);

    return true;
}

#endif // !UNIT_TEST
