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
#include "input/touch_gesture_engine.h"
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
    Wire.setClock(100000);  // Set to 100kHz for stability
    Serial.println("[HAL Touch] I2C bus initialized");

    // CRITICAL: Force wake the controller in case it's stuck in sleep mode
    // Previous sleep() call might have persisted across power cycles
    Serial.println("[HAL Touch] Attempting to wake touch controller...");
    g_touch.setPins(TOUCH_RST, TOUCH_INT);

    // Try to wake it first (this will fail if not initialized, but that's OK)
    g_touch.wakeup();
    delay(200);

    Serial.println("[HAL Touch] Initializing touch controller...");
    // Initialize touch controller
    if (!g_touch.begin(Wire, TOUCH_ADDR, TOUCH_SDA, TOUCH_SCL)) {
        Serial.println("[HAL Touch] Failed to initialize CST816 controller");
        Serial.println("[HAL Touch] Trying one more time with wake...");

        // One more attempt with wake
        Wire.beginTransmission(TOUCH_ADDR);
        Wire.write(0xFE);  // Wake command register
        Wire.write(0x01);  // Wake value
        Wire.endTransmission();
        delay(100);

        if (!g_touch.begin(Wire, TOUCH_ADDR, TOUCH_SDA, TOUCH_SCL)) {
            Serial.println("[HAL Touch] Failed again - controller may be damaged or stuck");
            return false;
        }
    }

    // Get display dimensions for coordinate mapping
    g_display_width = static_cast<int16_t>(hal_display_get_width_pixels());
    g_display_height = static_cast<int16_t>(hal_display_get_height_pixels());

    // Configure touch panel for T-Display S3 AMOLED Plus
    Serial.println("[HAL Touch] Configuring touch panel...");

    #if defined(APP_DISPLAY_ROTATION)
        // T-Display S3 AMOLED Plus specific configuration
        g_touch.setMaxCoordinates(536, 240);
        Serial.println("[HAL Touch] Max coordinates set to 536x240");

        // Disable home button by setting it to an impossible coordinate
        g_touch.setCenterButtonCoordinate(9999, 9999);
        Serial.println("[HAL Touch] Home button disabled (moved to 9999, 9999)");

        // Try different swap/mirror settings to see if touch digitizer is connected differently
        // g_touch.setSwapXY(true);
        // g_touch.setMirrorXY(false, false);
    #endif

    g_touch_initialized = true;
    Serial.println("[HAL Touch] CST816 initialized successfully");
    return true;
}

bool hal_touch_read(hal_touch_point_t* point) {
    if (!g_touch_initialized || !point) {
        return false;
    }

    // Rate-limit polling to reduce load on I2C bus
    // Poll every 3rd frame (10Hz instead of 30Hz) to avoid keeping controller stuck
    static uint8_t poll_counter = 0;
    poll_counter++;
    if (poll_counter < 3) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;  // Skip this frame
    }
    poll_counter = 0;

    // Read touch coordinates
    int16_t x[1], y[1];
    uint8_t point_count = g_touch.getPoint(x, y, 1);

    if (point_count == 0) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;  // Success, just not pressed
    }

    // DEBUG: Track if we EVER see coordinates other than 600, 120
    static bool seen_non_home_button = false;
    static uint32_t touch_count = 0;
    touch_count++;

    // Log EVERY touch for the first 10 seconds to diagnose
    if (millis() < 20000 || !seen_non_home_button) {
        if (!(x[0] == 600 && y[0] == 120)) {
            seen_non_home_button = true;
            Serial.printf("[HAL Touch] !!! NON-HOME-BUTTON TOUCH DETECTED: x=%d, y=%d !!!\n", x[0], y[0]);
        }
    }

    // Sample logging every 60 touches
    if (touch_count % 60 == 0) {
        Serial.printf("[HAL Touch] Sample #%u: x=%d, y=%d (seen_real_touch=%s)\n",
                      touch_count, x[0], y[0], seen_non_home_button ? "YES" : "NO");
    }

    // CRITICAL: Filter out home button coordinate (x=600, y=120)
    // The CST816T reports the home button as a regular touch event
    // Silently ignore it to prevent spam
    if (x[0] == 600 && y[0] == 120) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;  // Silently reject home button as not pressed
    }

    // Debug: Log raw coordinates (only on change to reduce noise)
    static int16_t last_x = -1, last_y = -1;
    static uint32_t last_warning_time = 0;
    bool coords_changed = (x[0] != last_x || y[0] != last_y);
    if (coords_changed) {
        Serial.printf("[HAL Touch] RAW: x=%d, y=%d\n", x[0], y[0]);
        last_x = x[0];
        last_y = y[0];
    }

    // TEMPORARILY DISABLED: Validate raw coordinates before processing
    // Allowing ALL coordinates through to see if real touches have unexpected values
    // if (x[0] < 0 || x[0] > 540 || y[0] < 0 || y[0] > 250) {
    //     Serial.printf("[HAL Touch] WARNING: Out-of-range RAW coordinates: x=%d, y=%d\n", x[0], y[0]);
    //     point->is_pressed = false;
    //     point->x = 0;
    //     point->y = 0;
    //     return true;
    // }

    // CRITICAL: Touch controller appears to report in near-display coordinates!
    // HIL testing shows:
    //   - RAW X range: 2-536 (matches display width 536!)
    //   - RAW Y range: 46-239 (close to display height 240!)
    // This suggests minimal scaling needed - touch controller is pre-scaled

    // Use raw coordinates directly (touch controller pre-scaled to display)
    int16_t scaled_x = x[0];
    int16_t scaled_y = y[0];

    // Apply coordinate transformation based on display rotation
    // CRITICAL: Touch panel coordinates may already be in display orientation!
    // Testing NO rotation transform (touch panel pre-rotated by hardware/firmware)
    int16_t transformed_x, transformed_y;

    #if DISPLAY_ROTATION == 0
        // Portrait mode (no transformation)
        transformed_x = scaled_x;
        transformed_y = scaled_y;
    #elif DISPLAY_ROTATION == 90
        // Landscape 90° CW: NO axis swap needed!
        // Touch controller already reports in rotated coordinate system
        // HIL corner data shows axes are correctly aligned, just need direct mapping
        transformed_x = scaled_x;  // Touch X → Display X (NO swap!)
        transformed_y = scaled_y;  // Touch Y → Display Y (NO swap!)
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

    // Clamp coordinates to display bounds
    if (transformed_x < 0) transformed_x = 0;
    if (transformed_x >= g_display_width) transformed_x = g_display_width - 1;
    if (transformed_y < 0) transformed_y = 0;
    if (transformed_y >= g_display_height) transformed_y = g_display_height - 1;

    point->x = transformed_x;
    point->y = transformed_y;
    point->is_pressed = true;

    // DEBUG: Log when we successfully accept a touch
    static int16_t last_accepted_x = -1, last_accepted_y = -1;
    if (transformed_x != last_accepted_x || transformed_y != last_accepted_y) {
        Serial.printf("[HAL Touch] ACCEPTED TOUCH: (%d, %d) -> transformed (%d, %d)\n",
                      x[0], y[0], transformed_x, transformed_y);
        last_accepted_x = transformed_x;
        last_accepted_y = transformed_y;
    }

    return true;
}

void hal_touch_configure_gesture_engine(TouchGestureEngine* engine) {
    if (!engine) return;

    // Board-specific edge zone configuration
    #if defined(APP_DISPLAY_ROTATION)
        // T-Display S3 AMOLED Plus (1.91") with 90° rotation
        // Coordinate system: (0,0) at top-left, X-INVERTED (x = 535 - scaled_y)
        //
        // Touch panel has limited range after X-inversion:
        //   X: ~308-517 (inverted from scaled_y 18-227: 535-227=308, 535-18=517)
        //   Y: ~2-214 (from scaled_x range)
        //
        // Simple axis swap with no X-inversion
        // Edge zones tuned for comfortable edge drag detection:
        engine->setEdgeZones(
            80,   // left_threshold: x < 80 (14.9% of width)
            215,  // right_threshold: x > 215 (40% from left, 60% coverage)
            80,   // top_threshold: y < 80 (33.3% of height - more forgiving for top drags)
            180   // bottom_threshold: y > 180 (25% of height)
        );
    #else
        // ESP32-S3 AMOLED (1.8") - uses default percentage-based thresholds
        // Touch panel covers full screen area, no special configuration needed
    #endif
}

#endif // !UNIT_TEST
