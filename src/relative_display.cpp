/**
 * @file relative_display.cpp
 * @brief Relative Display Drawing Abstraction Implementation
 *
 * This module implements resolution-independent drawing by converting
 * relative coordinates (percentages) to absolute pixel coordinates using
 * the HAL-provided screen dimensions.
 */

#define _USE_MATH_DEFINES
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "../hal/display.h"
#include <math.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

void display_relative_draw_line_thick(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, uint16_t color) {
    // Convert relative coordinates to pixel coordinates
    int32_t x1_pixel = percent_to_pixel(x1_percent, g_screen_width);
    int32_t y1_pixel = percent_to_pixel(y1_percent, g_screen_height);
    int32_t x2_pixel = percent_to_pixel(x2_percent, g_screen_width);
    int32_t y2_pixel = percent_to_pixel(y2_percent, g_screen_height);

    // Calculate thickness in pixels (use average of width/height for diagonal lines)
    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t thickness_pixels = percent_to_pixel(thickness_percent, (int32_t)avg_dimension);
    if (thickness_pixels < 1) thickness_pixels = 1;

    // Draw the line using Bresenham's algorithm with thickness
    int32_t dx = abs(x2_pixel - x1_pixel);
    int32_t dy = abs(y2_pixel - y1_pixel);
    int32_t sx = (x1_pixel < x2_pixel) ? 1 : -1;
    int32_t sy = (y1_pixel < y2_pixel) ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1_pixel;
    int32_t y = y1_pixel;

    int32_t half_thickness = thickness_pixels / 2;

    while (true) {
        // Draw a filled circle at each point along the line for thickness
        for (int32_t ty = -half_thickness; ty <= half_thickness; ty++) {
            for (int32_t tx = -half_thickness; tx <= half_thickness; tx++) {
                if (tx * tx + ty * ty <= half_thickness * half_thickness) {
                    int32_t draw_x = x + tx;
                    int32_t draw_y = y + ty;
                    if (draw_x >= 0 && draw_x < g_screen_width && draw_y >= 0 && draw_y < g_screen_height) {
                        hal_display_draw_pixel(draw_x, draw_y, color);
                    }
                }
            }
        }

        if (x == x2_pixel && y == y2_pixel) break;

        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

/**
 * @brief Interpolates between colors for gradient effects
 *
 * @param color1 First color (RGB565)
 * @param color2 Second color (RGB565)
 * @param t Interpolation factor (0.0 to 1.0)
 * @return uint16_t Interpolated color (RGB565)
 */
static uint16_t interpolate_color(uint16_t color1, uint16_t color2, float t) {
    // Extract RGB components from RGB565
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;

    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    // Interpolate
    uint8_t r = (uint8_t)(r1 + t * (r2 - r1));
    uint8_t g = (uint8_t)(g1 + t * (g2 - g1));
    uint8_t b = (uint8_t)(b1 + t * (b2 - b1));

    // Pack back to RGB565
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

/**
 * @brief Gets color at position t in a gradient with multiple color stops
 *
 * @param gradient The linear gradient
 * @param t Position along gradient (0.0 to 1.0)
 * @return uint16_t Color at that position
 */
static uint16_t get_gradient_color(const LinearGradient& gradient, float t) {
    if (gradient.num_stops < 2) {
        return gradient.color_stops[0];
    }

    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (gradient.num_stops == 2) {
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t);
    }

    // For 3 stops, divide into two segments
    if (t < 0.5f) {
        // Interpolate between stop 0 and stop 1
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t * 2.0f);
    } else {
        // Interpolate between stop 1 and stop 2
        return interpolate_color(gradient.color_stops[1], gradient.color_stops[2], (t - 0.5f) * 2.0f);
    }
}

void display_relative_fill_rect_gradient(float x_percent, float y_percent, float w_percent, float h_percent, const LinearGradient& gradient) {
    // Convert relative coordinates to pixel coordinates
    int32_t x_start_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_percent, g_screen_height);
    int32_t width_pixels = percent_to_pixel(w_percent, g_screen_width);
    int32_t height_pixels = percent_to_pixel(h_percent, g_screen_height);

    int32_t x_end_pixel = x_start_pixel + width_pixels;
    int32_t y_end_pixel = y_start_pixel + height_pixels;

    // Compute gradient direction based on angle
    float angle_rad = gradient.angle_deg * M_PI / 180.0f;
    float dx = cosf(angle_rad);
    float dy = sinf(angle_rad);

    // For each pixel, compute gradient position
    for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
        for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
            // Compute relative position in rectangle (0 to 1)
            float rel_x = (float)(x - x_start_pixel) / (float)width_pixels;
            float rel_y = (float)(y - y_start_pixel) / (float)height_pixels;

            // Project onto gradient direction
            float t = rel_x * dx + rel_y * dy;

            // Normalize to 0-1 range based on the gradient direction
            t = (t + 1.0f) / 2.0f;

            uint16_t color = get_gradient_color(gradient, t);
            hal_display_draw_pixel(x, y, color);
        }
    }
}

void display_relative_draw_line_thick_gradient(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, const LinearGradient& gradient) {
    // Convert relative coordinates to pixel coordinates
    int32_t x1_pixel = percent_to_pixel(x1_percent, g_screen_width);
    int32_t y1_pixel = percent_to_pixel(y1_percent, g_screen_height);
    int32_t x2_pixel = percent_to_pixel(x2_percent, g_screen_width);
    int32_t y2_pixel = percent_to_pixel(y2_percent, g_screen_height);

    // Calculate thickness in pixels
    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t thickness_pixels = percent_to_pixel(thickness_percent, (int32_t)avg_dimension);
    if (thickness_pixels < 1) thickness_pixels = 1;

    // Draw the line using Bresenham's algorithm with gradient
    int32_t dx = abs(x2_pixel - x1_pixel);
    int32_t dy = abs(y2_pixel - y1_pixel);
    int32_t sx = (x1_pixel < x2_pixel) ? 1 : -1;
    int32_t sy = (y1_pixel < y2_pixel) ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1_pixel;
    int32_t y = y1_pixel;

    int32_t half_thickness = thickness_pixels / 2;
    float line_length = sqrtf((float)(dx * dx + dy * dy));

    while (true) {
        // Calculate position along line for gradient
        float dist = sqrtf((float)((x - x1_pixel) * (x - x1_pixel) + (y - y1_pixel) * (y - y1_pixel)));
        float t = (line_length > 0) ? (dist / line_length) : 0.0f;

        uint16_t color = get_gradient_color(gradient, t);

        // Draw a filled circle at each point along the line for thickness
        for (int32_t ty = -half_thickness; ty <= half_thickness; ty++) {
            for (int32_t tx = -half_thickness; tx <= half_thickness; tx++) {
                if (tx * tx + ty * ty <= half_thickness * half_thickness) {
                    int32_t draw_x = x + tx;
                    int32_t draw_y = y + ty;
                    if (draw_x >= 0 && draw_x < g_screen_width && draw_y >= 0 && draw_y < g_screen_height) {
                        hal_display_draw_pixel(draw_x, draw_y, color);
                    }
                }
            }
        }

        if (x == x2_pixel && y == y2_pixel) break;

        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void display_relative_fill_circle_gradient(float center_x_percent, float center_y_percent, float radius_percent, const RadialGradient& gradient) {
    // Convert relative coordinates to pixel coordinates
    int32_t center_x_pixel = percent_to_pixel(center_x_percent, g_screen_width);
    int32_t center_y_pixel = percent_to_pixel(center_y_percent, g_screen_height);

    // Calculate radius in pixels (use average of width/height)
    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t radius_pixels = percent_to_pixel(radius_percent, (int32_t)avg_dimension);

    // Draw filled circle with radial gradient
    int32_t x_start = center_x_pixel - radius_pixels;
    int32_t x_end = center_x_pixel + radius_pixels;
    int32_t y_start = center_y_pixel - radius_pixels;
    int32_t y_end = center_y_pixel + radius_pixels;

    for (int32_t y = y_start; y <= y_end; y++) {
        for (int32_t x = x_start; x <= x_end; x++) {
            int32_t dx = x - center_x_pixel;
            int32_t dy = y - center_y_pixel;
            float dist = sqrtf((float)(dx * dx + dy * dy));

            if (dist <= radius_pixels) {
                // Calculate gradient position (0 at center, 1 at edge)
                float t = dist / (float)radius_pixels;

                // Interpolate between inner and outer colors
                uint16_t color = interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t);

                if (x >= 0 && x < g_screen_width && y >= 0 && y < g_screen_height) {
                    hal_display_draw_pixel(x, y, color);
                }
            }
        }
    }
}
