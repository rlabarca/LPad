/**
 * @file relative_display.cpp
 * @brief Object-oriented RelativeDisplay class implementation
 *
 * This module provides a resolution-independent drawing API using
 * an object-oriented approach. The RelativeDisplay class wraps an
 * Arduino_GFX canvas/display and provides drawing methods that use
 * relative coordinates (percentages) instead of absolute pixels.
 *
 * See features/display_relative_drawing.md for complete specification.
 */

#define _USE_MATH_DEFINES
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "../hal/display.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RelativeDisplay::RelativeDisplay(Arduino_GFX* gfx, int32_t width, int32_t height)
    : _gfx(gfx), _width(width), _height(height) {
}

void RelativeDisplay::init() {
    // Initialization logic if needed in the future
    // For now, this is a placeholder for setting up color formats
    // or any other initialization required by the underlying GFX object
}

int32_t RelativeDisplay::relativeToAbsoluteX(float x_percent) const {
    return static_cast<int32_t>(roundf((x_percent / 100.0f) * static_cast<float>(_width)));
}

int32_t RelativeDisplay::relativeToAbsoluteY(float y_percent) const {
    return static_cast<int32_t>(roundf((y_percent / 100.0f) * static_cast<float>(_height)));
}

int32_t RelativeDisplay::relativeToAbsoluteWidth(float width_percent) const {
    return static_cast<int32_t>(roundf((width_percent / 100.0f) * static_cast<float>(_width)));
}

int32_t RelativeDisplay::relativeToAbsoluteHeight(float height_percent) const {
    return static_cast<int32_t>(roundf((height_percent / 100.0f) * static_cast<float>(_height)));
}

void RelativeDisplay::drawPixel(float x_percent, float y_percent, uint16_t color) {
    int32_t x = relativeToAbsoluteX(x_percent);
    int32_t y = relativeToAbsoluteY(y_percent);
    _gfx->drawPixel(x, y, color);
}

void RelativeDisplay::drawHorizontalLine(float y_percent, float x_start_percent, float x_end_percent, uint16_t color) {
    int32_t y = relativeToAbsoluteY(y_percent);
    int32_t x_start = relativeToAbsoluteX(x_start_percent);
    int32_t x_end = relativeToAbsoluteX(x_end_percent);

    // Ensure start <= end
    if (x_start > x_end) {
        int32_t temp = x_start;
        x_start = x_end;
        x_end = temp;
    }

    // Draw horizontal line
    int32_t width = x_end - x_start + 1;
    _gfx->drawFastHLine(x_start, y, width, color);
}

void RelativeDisplay::drawVerticalLine(float x_percent, float y_start_percent, float y_end_percent, uint16_t color) {
    int32_t x = relativeToAbsoluteX(x_percent);
    int32_t y_start = relativeToAbsoluteY(y_start_percent);
    int32_t y_end = relativeToAbsoluteY(y_end_percent);

    // Ensure start <= end
    if (y_start > y_end) {
        int32_t temp = y_start;
        y_start = y_end;
        y_end = temp;
    }

    // Draw vertical line
    int32_t height = y_end - y_start + 1;
    _gfx->drawFastVLine(x, y_start, height, color);
}

void RelativeDisplay::fillRect(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color) {
    int32_t x = relativeToAbsoluteX(x_start_percent);
    int32_t y = relativeToAbsoluteY(y_start_percent);
    int32_t w = relativeToAbsoluteWidth(width_percent);
    int32_t h = relativeToAbsoluteHeight(height_percent);

    _gfx->fillRect(x, y, w, h, color);
}

Arduino_GFX* RelativeDisplay::getGfx() const {
    return _gfx;
}

// ============================================================================
// Backward Compatibility Layer - Procedural API Implementation
// ============================================================================

// Global state for backward compatibility with procedural API
static int32_t g_screen_width = 0;
static int32_t g_screen_height = 0;

// Helper function for coordinate conversion (matches old implementation)
static inline int32_t percent_to_pixel(float percent, int32_t dimension) {
    return static_cast<int32_t>(roundf((percent / 100.0f) * static_cast<float>(dimension)));
}

void display_relative_init(void) {
    // Query screen dimensions from the HAL
    g_screen_width = hal_display_get_width_pixels();
    g_screen_height = hal_display_get_height_pixels();
}

void display_relative_draw_pixel(float x_percent, float y_percent, uint16_t color) {
    int32_t x_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_pixel = percent_to_pixel(y_percent, g_screen_height);
    hal_display_draw_pixel(x_pixel, y_pixel, color);
}

void display_relative_draw_horizontal_line(float y_percent, float x_start_percent, float x_end_percent, uint16_t color) {
    int32_t y_pixel = percent_to_pixel(y_percent, g_screen_height);
    int32_t x_start_pixel = percent_to_pixel(x_start_percent, g_screen_width);
    int32_t x_end_pixel = percent_to_pixel(x_end_percent, g_screen_width);

    if (x_start_pixel > x_end_pixel) {
        int32_t temp = x_start_pixel;
        x_start_pixel = x_end_pixel;
        x_end_pixel = temp;
    }

    for (int32_t x = x_start_pixel; x <= x_end_pixel; x++) {
        hal_display_draw_pixel(x, y_pixel, color);
    }
}

void display_relative_draw_vertical_line(float x_percent, float y_start_percent, float y_end_percent, uint16_t color) {
    int32_t x_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_start_percent, g_screen_height);
    int32_t y_end_pixel = percent_to_pixel(y_end_percent, g_screen_height);

    if (y_start_pixel > y_end_pixel) {
        int32_t temp = y_start_pixel;
        y_start_pixel = y_end_pixel;
        y_end_pixel = temp;
    }

    for (int32_t y = y_start_pixel; y <= y_end_pixel; y++) {
        hal_display_draw_pixel(x_pixel, y, color);
    }
}

void display_relative_fill_rectangle(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color) {
    int32_t x_start_pixel = percent_to_pixel(x_start_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_start_percent, g_screen_height);
    int32_t width_pixels = percent_to_pixel(width_percent, g_screen_width);
    int32_t height_pixels = percent_to_pixel(height_percent, g_screen_height);

    int32_t x_end_pixel = x_start_pixel + width_pixels;
    int32_t y_end_pixel = y_start_pixel + height_pixels;

    for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
        for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
            hal_display_draw_pixel(x, y, color);
        }
    }
}

void display_relative_draw_line_thick(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, uint16_t color) {
    int32_t x1_pixel = percent_to_pixel(x1_percent, g_screen_width);
    int32_t y1_pixel = percent_to_pixel(y1_percent, g_screen_height);
    int32_t x2_pixel = percent_to_pixel(x2_percent, g_screen_width);
    int32_t y2_pixel = percent_to_pixel(y2_percent, g_screen_height);

    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t thickness_pixels = percent_to_pixel(thickness_percent, static_cast<int32_t>(avg_dimension));
    if (thickness_pixels < 1) thickness_pixels = 1;

    int32_t dx = abs(x2_pixel - x1_pixel);
    int32_t dy = abs(y2_pixel - y1_pixel);
    int32_t sx = (x1_pixel < x2_pixel) ? 1 : -1;
    int32_t sy = (y1_pixel < y2_pixel) ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1_pixel;
    int32_t y = y1_pixel;
    int32_t half_thickness = thickness_pixels / 2;

    while (true) {
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

// Helper for color interpolation
static uint16_t interpolate_color(uint16_t color1, uint16_t color2, float t) {
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;

    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    uint8_t r = static_cast<uint8_t>(r1 + t * (r2 - r1));
    uint8_t g = static_cast<uint8_t>(g1 + t * (g2 - g1));
    uint8_t b = static_cast<uint8_t>(b1 + t * (b2 - b1));

    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

static uint16_t get_gradient_color(const LinearGradient& gradient, float t) {
    if (gradient.num_stops < 2) {
        return gradient.color_stops[0];
    }

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (gradient.num_stops == 2) {
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t);
    }

    if (t < 0.5f) {
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t * 2.0f);
    } else {
        return interpolate_color(gradient.color_stops[1], gradient.color_stops[2], (t - 0.5f) * 2.0f);
    }
}

void display_relative_fill_rect_gradient(float x_percent, float y_percent, float w_percent, float h_percent, const LinearGradient& gradient) {
    int32_t x_start_pixel = percent_to_pixel(x_percent, g_screen_width);
    int32_t y_start_pixel = percent_to_pixel(y_percent, g_screen_height);
    int32_t width_pixels = percent_to_pixel(w_percent, g_screen_width);
    int32_t height_pixels = percent_to_pixel(h_percent, g_screen_height);

    int32_t x_end_pixel = x_start_pixel + width_pixels;
    int32_t y_end_pixel = y_start_pixel + height_pixels;

    float angle_rad = gradient.angle_deg * M_PI / 180.0f;
    float dx = cosf(angle_rad);
    float dy = sinf(angle_rad);

    if (fabsf(gradient.angle_deg) < 5.0f || fabsf(gradient.angle_deg - 360.0f) < 5.0f) {
        for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
            for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
                float t = static_cast<float>(x - x_start_pixel) / static_cast<float>(width_pixels - 1);
                uint16_t color = get_gradient_color(gradient, t);
                hal_display_draw_pixel(x, y, color);
            }
        }
        return;
    }

    if (fabsf(gradient.angle_deg - 90.0f) < 5.0f || fabsf(gradient.angle_deg - 270.0f) < 5.0f) {
        for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
            float t = static_cast<float>(y - y_start_pixel) / static_cast<float>(height_pixels - 1);
            if (gradient.angle_deg > 180.0f) t = 1.0f - t;
            uint16_t color = get_gradient_color(gradient, t);
            for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
                hal_display_draw_pixel(x, y, color);
            }
        }
        return;
    }

    for (int32_t y = y_start_pixel; y < y_end_pixel; y++) {
        for (int32_t x = x_start_pixel; x < x_end_pixel; x++) {
            float rel_x = static_cast<float>(x - x_start_pixel) / static_cast<float>(width_pixels);
            float rel_y = static_cast<float>(y - y_start_pixel) / static_cast<float>(height_pixels);
            float t = rel_x * dx + rel_y * dy;
            t = (t + 1.0f) / 2.0f;
            uint16_t color = get_gradient_color(gradient, t);
            hal_display_draw_pixel(x, y, color);
        }
    }
}

void display_relative_draw_line_thick_gradient(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, const LinearGradient& gradient) {
    int32_t x1_pixel = percent_to_pixel(x1_percent, g_screen_width);
    int32_t y1_pixel = percent_to_pixel(y1_percent, g_screen_height);
    int32_t x2_pixel = percent_to_pixel(x2_percent, g_screen_width);
    int32_t y2_pixel = percent_to_pixel(y2_percent, g_screen_height);

    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t thickness_pixels = percent_to_pixel(thickness_percent, static_cast<int32_t>(avg_dimension));
    if (thickness_pixels < 1) thickness_pixels = 1;

    int32_t dx = abs(x2_pixel - x1_pixel);
    int32_t dy = abs(y2_pixel - y1_pixel);
    int32_t sx = (x1_pixel < x2_pixel) ? 1 : -1;
    int32_t sy = (y1_pixel < y2_pixel) ? 1 : -1;
    int32_t err = dx - dy;

    int32_t x = x1_pixel;
    int32_t y = y1_pixel;
    int32_t half_thickness = thickness_pixels / 2;
    float line_length = sqrtf(static_cast<float>(dx * dx + dy * dy));

    while (true) {
        float dist = sqrtf(static_cast<float>((x - x1_pixel) * (x - x1_pixel) + (y - y1_pixel) * (y - y1_pixel)));
        float t = (line_length > 0) ? (dist / line_length) : 0.0f;
        uint16_t color = get_gradient_color(gradient, t);

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
    int32_t center_x_pixel = percent_to_pixel(center_x_percent, g_screen_width);
    int32_t center_y_pixel = percent_to_pixel(center_y_percent, g_screen_height);

    float avg_dimension = (g_screen_width + g_screen_height) / 2.0f;
    int32_t radius_pixels = percent_to_pixel(radius_percent, static_cast<int32_t>(avg_dimension));

    int32_t x_start = center_x_pixel - radius_pixels;
    int32_t x_end = center_x_pixel + radius_pixels;
    int32_t y_start = center_y_pixel - radius_pixels;
    int32_t y_end = center_y_pixel + radius_pixels;

    for (int32_t y = y_start; y <= y_end; y++) {
        for (int32_t x = x_start; x <= x_end; x++) {
            int32_t dx = x - center_x_pixel;
            int32_t dy = y - center_y_pixel;
            float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));

            if (dist <= radius_pixels) {
                float t = dist / static_cast<float>(radius_pixels);
                uint16_t color = interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t);

                if (x >= 0 && x < g_screen_width && y >= 0 && y < g_screen_height) {
                    hal_display_draw_pixel(x, y, color);
                }
            }
        }
    }
}
