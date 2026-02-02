/**
 * @file display_stub.cpp
 * @brief Stub implementation of Display HAL for testing
 *
 * This stub implementation provides minimal functionality for unit testing.
 * Concrete hardware implementations should be placed in separate files
 * (e.g., display_esp32_s3_amoled.cpp).
 */

#include "display.h"

// Static storage for stub state
static int32_t g_stub_original_width = 240;   // Default test dimension
static int32_t g_stub_original_height = 240;  // Default test dimension
static int g_stub_rotation = 0;               // Current rotation in degrees

// Stub implementation - returns false to indicate not initialized
bool hal_display_init(void) {
    return false;
}

// Stub implementation - does nothing
void hal_display_clear(uint16_t color) {
    (void)color;  // Unused parameter
}

// Stub implementation - does nothing
void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color) {
    (void)x;
    (void)y;
    (void)color;  // Unused parameters
}

// Stub implementation - does nothing
void hal_display_flush(void) {
    // Nothing to flush in stub
}

// Stub implementation - returns width based on current rotation
int32_t hal_display_get_width_pixels(void) {
    // Swap dimensions for 90 and 270 degree rotations
    if (g_stub_rotation == 90 || g_stub_rotation == 270) {
        return g_stub_original_height;
    }
    return g_stub_original_width;
}

// Stub implementation - returns height based on current rotation
int32_t hal_display_get_height_pixels(void) {
    // Swap dimensions for 90 and 270 degree rotations
    if (g_stub_rotation == 90 || g_stub_rotation == 270) {
        return g_stub_original_width;
    }
    return g_stub_original_height;
}

// Stub implementation - stores rotation angle
void hal_display_set_rotation(int degrees) {
    g_stub_rotation = degrees;
}
