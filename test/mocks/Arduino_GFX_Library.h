/**
 * @file Arduino_GFX_Library.h
 * @brief Minimal stub for Arduino_GFX for native unit tests
 *
 * This is a minimal stub that provides just enough of the Arduino_GFX
 * interface to allow unit tests to compile and run in the native environment.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

// Define PROGMEM for native environment (no-op)
#ifndef PROGMEM
#define PROGMEM
#endif

// GFX font structures (from Adafruit_GFX)
typedef struct {
    uint16_t bitmapOffset;  ///< Pointer into GFXfont->bitmap
    uint8_t width;          ///< Bitmap dimensions in pixels
    uint8_t height;         ///< Bitmap dimensions in pixels
    uint8_t xAdvance;       ///< Distance to advance cursor (x axis)
    int8_t xOffset;         ///< X dist from cursor pos to UL corner
    int8_t yOffset;         ///< Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
    uint8_t *bitmap;        ///< Glyph bitmaps, concatenated
    GFXglyph *glyph;        ///< Glyph array
    uint16_t first;         ///< ASCII extents (first char)
    uint16_t last;          ///< ASCII extents (last char)
    uint8_t yAdvance;       ///< Newline distance (y axis)
} GFXfont;

// Minimal stub of Arduino_GFX class for testing
class Arduino_GFX {
public:
    Arduino_GFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual ~Arduino_GFX() {}

    // Pure virtual methods that must be implemented by subclasses
    virtual bool begin(int32_t speed = 0) = 0;
    virtual void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) = 0;

    // Drawing methods that can be overridden
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) {}
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {}
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {}
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {}
    virtual void fillTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color) {}
    virtual void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) { (void)x; (void)y; (void)r; (void)color; }

    // Text methods
    virtual void setFont(const GFXfont* f = nullptr) { (void)f; }
    virtual void setTextColor(uint16_t color) { (void)color; }
    virtual void setCursor(int16_t x, int16_t y) { (void)x; (void)y; }
    virtual size_t print(const char* str) { (void)str; return 0; }
    virtual void getTextBounds(const char* str, int16_t x, int16_t y, int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
        (void)str; (void)x; (void)y;
        if (x1) *x1 = 0;
        if (y1) *y1 = 0;
        if (w) *w = 0;
        if (h) *h = 0;
    }

    int16_t width() const { return _width; }
    int16_t height() const { return _height; }

protected:
    int16_t _width;
    int16_t _height;
};

// Minimal stub of Arduino_Canvas class for testing
class Arduino_Canvas : public Arduino_GFX {
public:
    Arduino_Canvas(int16_t w, int16_t h, Arduino_GFX *output) : Arduino_GFX(w, h), _output(output) {}
    virtual ~Arduino_Canvas() {}

    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {}
    void fillScreen(uint16_t color) { (void)color; }
    void flush() {}
    uint16_t* getFramebuffer() { return nullptr; }

protected:
    Arduino_GFX *_output;
};
