/**
 * @file relative_display.cpp
 * @brief Relative Display Drawing Abstraction Implementation
 *
 * This module implements resolution-independent drawing by converting
 * relative coordinates (percentages) to absolute pixel coordinates using
 * the HAL-provided screen dimensions.
 */

#include "relative_display.h"
#include "../hal/display.h"
#include <math.h>

// Cached screen dimensions (queried from HAL on init)
static int32_t g_screen_width = 0;
static int32_t g_screen_height = 0;

/**
 * @brief Converts a percentage to a pixel coordinate
 *
 * @param percent The percentage value (0.0 to 100.0)
 * @param dimension The dimension (width or height) in pixels
 * @return int32_t The pixel coordinate
 */
static inline int32_t percent_to_pixel(float percent, int32_t dimension) {
    return (int32_t)roundf((percent / 100.0f) * (float)dimension);
}

void display_relative_init(void) {
    // Query screen dimensions from the HAL
    g_screen_width = hal_display_get_width_pixels();
    g_screen_height = hal_display_get_height_pixels();
}

void display_relative_draw_pixel(float x_percent, float y_percent, uint16_t color) {
    // Convert relative coordinates to pixel coordinates
    int32_t x_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_pixel = percent_to_pixel(y_percent, g_screen_height);

    // Draw the pixel using the HAL
    hal_display_draw_pixel(x_pixel, y_pixel, color);
}

void display_relative_draw_horizontal_line(float y_percent, float x_start_percent, float x_end_percent, uint16_t color) {
    // Convert relative coordinates to pixel coordinates
    int32_t y_pixel = percent_to_pixel(y_percent, g_screen_height);
    int32_t x_start_pixel = percent_to_pixel(x_start_percent, g_screen_width);
    int32_t x_end_pixel = percent_to_pixel(x_end_percent, g_screen_width);

    // Ensure start <= end
    if (x_start_pixel > x_end_pixel) {
        int32_t temp = x_start_pixel;
        x_start_pixel = x_end_pixel;
        x_end_pixel = temp;
    }

    // Draw the horizontal line pixel by pixel
    for (int32_t x = x_start_pixel; x <= x_end_pixel; x++) {
        hal_display_draw_pixel(x, y_pixel, color);
    }
}

void display_relative_draw_vertical_line(float x_percent, float y_start_percent, float y_end_percent, uint16_t color) {
    // Convert relative coordinates to pixel coordinates
    int32_t x_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_start_percent, g_screen_height);
    int32_t y_end_pixel = percent_to_pixel(y_end_percent, g_screen_height);

    // Ensure start <= end
    if (y_start_pixel > y_end_pixel) {
        int32_t temp = y_start_pixel;
        y_start_pixel = y_end_pixel;
        y_end_pixel = temp;
    }

    // Draw the vertical line pixel by pixel
    for (int32_t y = y_start_pixel; y <= y_end_pixel; y++) {
        hal_display_draw_pixel(x_pixel, y, color);
    }
}

void display_relative_fill_rectangle(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color) {
    // Convert relative coordinates to pixel coordinates
    int32_t x_start_pixel = percent_to_pixel(x_start_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_start_percent, g_screen_height);
    int32_t width_pixels = percent_to_pixel(width_percent, g_screen_width);
    int32_t height_pixels = percent_to_pixel(height_percent, g_screen_height);

    // Calculate end coordinates
    int32_t x_end_pixel = x_start_pixel + width_pixels;
    int32_t y_end_pixel = y_start_pixel + height_pixels;

    // Fill the rectangle pixel by pixel
    for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
        for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
            hal_display_draw_pixel(x, y, color);
        }
    }
}
