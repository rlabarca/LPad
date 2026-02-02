/**
 * @file display_stub.cpp
 * @brief Stub implementation of Display HAL for testing
 *
 * This stub implementation provides minimal functionality for unit testing.
 * Concrete hardware implementations should be placed in separate files
 * (e.g., display_esp32_s3_amoled.cpp).
 */

#include "display.h"

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

// Stub implementation - returns a default width for testing
int32_t hal_display_get_width_pixels(void) {
    return 240;  // Default test dimension
}

// Stub implementation - returns a default height for testing
int32_t hal_display_get_height_pixels(void) {
    return 240;  // Default test dimension
}
