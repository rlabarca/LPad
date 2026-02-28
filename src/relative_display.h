#pragma once

#include <Arduino_GFX_Library.h>
#include <stdint.h>

class RelativeDisplay {
public:
    // Constructor: Takes a pointer to any Arduino_GFX compatible canvas/display.
    RelativeDisplay(Arduino_GFX* gfx, int32_t width, int32_t height);

    // Initialization logic if needed, e.g., setting up color formats.
    void init();

    // Converts relative units (0.0-100.0) to absolute pixel coordinates.
    int32_t relativeToAbsoluteX(float x_percent) const;
    int32_t relativeToAbsoluteY(float y_percent) const;
    int32_t relativeToAbsoluteWidth(float width_percent) const;
    int32_t relativeToAbsoluteHeight(float height_percent) const;

    // Drawing primitives using relative coordinates.
    void drawPixel(float x_percent, float y_percent, uint16_t color);
    void drawHorizontalLine(float y_percent, float x_start_percent, float x_end_percent, uint16_t color);
    void drawVerticalLine(float x_percent, float y_start_percent, float y_end_percent, uint16_t color);
    void fillRect(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color);

    // Background drawing methods (features/display_background.md)
    void drawSolidBackground(uint16_t color);
    void drawGradientBackground(uint16_t colorA, uint16_t colorB, float angle_deg);
    void drawGradientBackground(uint16_t colorA, uint16_t colorB, uint16_t colorC, float angle_deg);

    // Direct access to the underlying GFX object if needed for advanced operations.
    Arduino_GFX* getGfx() const;

    // Dimension accessors (pixel values passed at construction)
    int32_t getWidth() const { return _width; }
    int32_t getHeight() const { return _height; }

private:
    Arduino_GFX* _gfx;
    int32_t _width;
    int32_t _height;
};

// ============================================================================
// Backward Compatibility Layer - Procedural API
// ============================================================================
// These functions provide backward compatibility with code that uses the
// old procedural API. They use a global RelativeDisplay instance internally.
// New code should use the RelativeDisplay class directly.

// Include gradient type definitions (must be before extern "C")
#ifdef __cplusplus
#include "gradients.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void display_relative_init(void);
void display_relative_draw_pixel(float x_percent, float y_percent, uint16_t color);
void display_relative_draw_horizontal_line(float y_percent, float x_start_percent, float x_end_percent, uint16_t color);
void display_relative_draw_vertical_line(float x_percent, float y_start_percent, float y_end_percent, uint16_t color);
void display_relative_fill_rectangle(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color);
void display_relative_draw_line_thick(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, uint16_t color);

#ifdef __cplusplus
}

// C++ overloads for gradient functions
void display_relative_fill_rect_gradient(float x_percent, float y_percent, float w_percent, float h_percent, const LinearGradient& gradient);
void display_relative_draw_line_thick_gradient(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, const LinearGradient& gradient);
void display_relative_fill_circle_gradient(float center_x_percent, float center_y_percent, float radius_percent, const RadialGradient& gradient);

// Background drawing functions (features/display_background.md)
void display_relative_draw_solid_background(uint16_t color);
void display_relative_draw_gradient_background_2color(uint16_t colorA, uint16_t colorB, float angle_deg);
void display_relative_draw_gradient_background_3color(uint16_t colorA, uint16_t colorB, uint16_t colorC, float angle_deg);
#endif
