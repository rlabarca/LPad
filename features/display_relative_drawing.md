> Prerequisite: hal_contracts.md

# Feature: Display Relative Drawing Abstraction

## Introduction

This feature introduces a new display abstraction layer that allows drawing routines, text rendering, and animations to be defined using relative screen dimensions (percentages of width and height). This ensures that visual elements automatically scale and adapt across different display hardware with varying resolutions, maintaining consistent proportions and timing.

To achieve this, the HAL contract will first be extended to provide screen dimensions in pixels. A new module will then provide functions that convert relative coordinates to pixel coordinates and offer drawing primitives based on these relative units.

## 1. Extension to Display HAL Contract

The `hal/display.h` interface needs to be extended to allow the abstraction layer to query the display's dimensions.

### New HAL Functions:

#### `hal_display_get_width_pixels(void)`

*   **Description:** Returns the width of the active display in pixels.
*   **Parameters:** `void`
*   **Returns:** `int32_t` - The width of the display in pixels.

#### `hal_display_get_height_pixels(void)`

*   **Description:** Returns the height of the active display in pixels.
*   **Parameters:** `void`
*   **Returns:** `int32_t` - The height of the display in pixels.

## 2. Implementation Plan

The agent (Claude) will:

1.  **Modify `hal/display.h`**: Add the declarations for `hal_display_get_width_pixels` and `hal_display_get_height_pixels`.
2.  **Modify existing HAL implementations**:
    *   Implement `hal_display_get_width_pixels` and `hal_display_get_height_pixels` in `hal/display_esp32_s3_amoled.cpp` to return the specific dimensions for that display.
    *   Implement `hal_display_get_width_pixels` and `hal_display_get_height_pixels` in `hal/display_tdisplay_s3_plus.cpp` to return the specific dimensions for that display.
3.  **Create a new module**: `src/relative_display.h` and `src/relative_display.cpp`. This module will provide the relative drawing functions.

### New Relative Drawing Functions:

#### `display_relative_init(void)`

*   **Description:** Initializes the relative display abstraction layer, querying the underlying HAL for screen dimensions.
*   **Parameters:** `void`
*   **Returns:** `void`

#### `display_relative_draw_pixel(float x_percent, float y_percent, uint16_t color)`

*   **Description:** Draws a single pixel at a relative (x, y) coordinate (0.0 to 100.0 percent) with a given 16-bit RGB565 color.
*   **Parameters:**
    *   `float x_percent`: The X-coordinate as a percentage of screen width (0.0 = left, 100.0 = right).
    *   `float y_percent`: The Y-coordinate as a percentage of screen height (0.0 = top, 100.0 = bottom).
    *   `uint16_t color`: The 16-bit RGB565 color value.
*   **Returns:** `void`.

#### `display_relative_draw_horizontal_line(float y_percent, float x_start_percent, float x_end_percent, uint16_t color)`

*   **Description:** Draws a horizontal line across a relative segment of the display.
*   **Parameters:**
    *   `float y_percent`: The Y-coordinate as a percentage of screen height.
    *   `float x_start_percent`: The starting X-coordinate as a percentage.
    *   `float x_end_percent`: The ending X-coordinate as a percentage.
    *   `uint16_t color`: The 16-bit RGB565 color value.
*   **Returns:** `void`.

#### `display_relative_draw_vertical_line(float x_percent, float y_start_percent, float y_end_percent, uint16_t color)`

*   **Description:** Draws a vertical line across a relative segment of the display.
*   **Parameters:**
    *   `float x_percent`: The X-coordinate as a percentage of screen width.
    *   `float y_start_percent`: The starting Y-coordinate as a percentage.
    *   `float y_end_percent`: The ending Y-coordinate as a percentage.
    *   `uint16_t color`: The 16-bit RGB565 color value.
*   **Returns:** `void`.

#### `display_relative_fill_rectangle(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color)`

*   **Description:** Fills a rectangle defined by relative top-left corner and relative width/height.
*   **Parameters:**
    *   `float x_start_percent`: Top-left X-coordinate as a percentage.
    *   `float y_start_percent`: Top-left Y-coordinate as a percentage.
    *   `float width_percent`: Width of the rectangle as a percentage.
    *   `float height_percent`: Height of the rectangle as a percentage.
    *   `uint16_t color`: The 16-bit RGB565 color value.
*   **Returns:** `void`.

## 3. Scenarios

**Scenario: Retrieve Display Dimensions via HAL**
*   **Given:** The `hal_display_get_width_pixels` and `hal_display_get_height_pixels` functions are implemented in the specific HAL files.
*   **When:** `hal_display_get_width_pixels()` is called for the `esp32s3` build target.
*   **Then:** It must return `368`.
*   **When:** `hal_display_get_height_pixels()` is called for the `esp32s3` build target.
*   **Then:** It must return `448`.
*   **When:** `hal_display_get_width_pixels()` is called for the `tdisplay_s3_plus` build target.
*   **Then:** It must return `240`.
*   **When:** `hal_display_get_height_pixels()` is called for the `tdisplay_s3_plus` build target.
*   **Then:** It must return `536`.

**Scenario: Initialize Relative Drawing Abstraction**
*   **Given:** The HAL display contract including dimension retrieval is implemented.
*   **When:** `display_relative_init()` is called.
*   **Then:** The abstraction layer should correctly query and store the pixel dimensions from the HAL.

**Scenario: Draw a Pixel at 50% / 50%**
*   **Given:** The relative drawing abstraction is initialized.
*   **And:** The underlying HAL drawing functions (`hal_display_draw_pixel` and `hal_display_flush`) are functional.
*   **When:** `display_relative_draw_pixel(50.0f, 50.0f, 0xFFFF)` is called.
*   **Then:** `hal_display_draw_pixel` should be called with the center pixel coordinates for the current build target. (e.g., (184, 224) for a 368x448 display, (120, 268) for a 240x536 display)

**Scenario: Draw a Frame around the Screen at 10% Inset**
*   **Given:** The relative drawing abstraction is initialized.
*   **When:** `display_relative_fill_rectangle(10.0f, 10.0f, 80.0f, 80.0f, 0x07E0)` is called.
*   **Then:** The underlying HAL drawing functions should be called to draw a green rectangle with a 10% margin on all sides.

**Scenario: Draw a Horizontal Line across the top 10%**
*   **Given:** The relative drawing abstraction is initialized.
*   **When:** `display_relative_draw_horizontal_line(10.0f, 0.0f, 100.0f, 0xF800)` is called.
*   **Then:** The underlying HAL drawing functions should be called to draw a red horizontal line at a 10% vertical position.
