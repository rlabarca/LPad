> Prerequisite: features/hal_spec_display.md
> Prerequisite: features/arch_ui_compositing.md

# Feature: Display Relative Drawing

> Label: "Relative Drawing"
> Category: "UI Framework"

## Introduction

This feature introduces a `RelativeDisplay` C++ class that acts as an abstraction layer over an `Arduino_GFX` drawing surface. This object-oriented approach allows drawing routines to use relative coordinates (percentages) on any target, be it the main hardware display or an off-screen canvas in memory. This is critical for advanced graphics techniques like layered rendering and ensures UI elements are resolution-independent.

This class supersedes the previous procedural design.

## 1. `RelativeDisplay` Class Definition

The agent (Claude) will create `src/relative_display.h` and `src/relative_display.cpp` to define the `RelativeDisplay` class.

### Header (`relative_display.h`):
```cpp
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
```

## 2. Scenarios

### Scenario: Instantiate `RelativeDisplay` for the Main Screen

**Given** a main display object `tft` is created (e.g., `Arduino_ESP32_S3_AMOLED`).
**And** its dimensions are 240x536.
**When** a `RelativeDisplay` is constructed: `RelativeDisplay rel_tft(tft, 240, 536);`
**Then** the `rel_tft` object is ready to draw on the main screen using relative coordinates.

### Scenario: Instantiate `RelativeDisplay` for an Off-screen Canvas

**Given** an off-screen canvas `canvas` is created in memory (e.g., `GfxCanvas16* canvas = new GfxCanvas16(100, 100);`).
**When** a `RelativeDisplay` is constructed: `RelativeDisplay rel_canvas(canvas, 100, 100);`
**Then** the `rel_canvas` object is ready to draw on the in-memory canvas using relative coordinates.

### Scenario: Draw a Rectangle on a Target

**Given** a `RelativeDisplay` object `rel_surface` has been instantiated for a 200x200 pixel surface (either screen or canvas).
**When** `rel_surface.fillRect(10.0f, 10.0f, 80.0f, 80.0f, 0xFFFF)` is called.
**Then** the underlying `_gfx` object's `fillRect` method should be called with absolute pixel coordinates: `_gfx->fillRect(20, 20, 160, 160, 0xFFFF)`.
**And** the drawing operation should be directed to the specific surface (main screen or canvas) that the `rel_surface` was constructed with.
