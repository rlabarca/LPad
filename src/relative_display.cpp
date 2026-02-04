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

#include "relative_display.h"
#include <cmath>

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
