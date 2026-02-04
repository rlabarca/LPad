#pragma once

#include <Arduino_GFX_Library.h>

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

    // Direct access to the underlying GFX object if needed for advanced operations.
    Arduino_GFX* getGfx() const;

private:
    Arduino_GFX* _gfx;
    int32_t _width;
    int32_t _height;
};
