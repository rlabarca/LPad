# Feature: HAL Display Specification

> Label: "HAL Display Specification"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md

## Introduction

This document defines the abstract interface for display operations within the Hardware Abstraction Layer (HAL). It focuses on pixel-level drawing, canvas management, rotation, and fast blitting.

## Display HAL API

### `hal_display_init(void)`
*   **Description:** Initializes display hardware and communication.
*   **Returns:** `bool` - `true` if success.

### `hal_display_clear(uint16_t color)`
*   **Description:** Fills the entire display with an RGB565 color.

### `hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color)`
*   **Description:** Draws a single RGB565 pixel.

### `hal_display_flush(void)`
*   **Description:** Synchronizes the frame buffer with the physical screen.

### `hal_display_get_width_pixels(void)` / `hal_display_get_height_pixels(void)`
*   **Returns:** `int32_t` dimensions of the current orientation.

### `hal_display_set_rotation(int degrees)`
*   **Description:** Sets orientation (0, 90, 180, 270).

### `hal_display_read_pixel(int32_t x, int32_t y)`
*   **Description:** Reads a single RGB565 pixel from the display memory or back-buffer.
*   **Returns:** `uint16_t` - RGB565 color value.

### `hal_display_dump_screen(void)`
*   **Description:** Triggers a sequential read of the entire screen buffer and outputs it to Serial.
*   **Constraint:** This MUST be non-blocking for other high-priority interrupts if possible, but the `UIRenderManager` should be paused.

## Rotation Requirements (Consolidated)

The display HAL MUST support screen rotation. This ensures that all display drivers can be instructed to rotate their output and that higher-level modules can query the resulting dimensions.

### Scenario: Add Rotation Method to IDisplay Interface

**Given** the `IDisplay` interface defined in `hal/display.h`,
**When** the `setRotation(int degrees)` method is implemented,
**Then** the following conditions must be met:
1.  The `setRotation` method in each concrete implementation (`hal/display_tdisplay_s3_plus.cpp`, `hal/display_esp32_s3_amoled.cpp`) must call the underlying graphics library's rotation command.
2.  After `setRotation` is called on a display instance, subsequent calls to its `getWidth()` and `getHeight()` methods must return the dimensions corresponding to the new orientation.
3.  The `hal/display_stub.cpp` implementation must be updated:
    *   It should store the original width and height provided during construction.
    *   Its `setRotation` method should store the given rotation.
    *   Its `getWidth()` and `getHeight()` methods must calculate and return the correct dimension (swapping if rotation is 90 or 270) based on the stored original dimensions and the current rotation.

## Canvas (Layered) Drawing API

### `hal_display_canvas_create(int16_t w, int16_t h)`
*   **Returns:** `hal_canvas_handle_t` (opaque handle).

### `hal_display_canvas_delete(hal_canvas_handle_t canvas)`
*   **Description:** Frees canvas memory.

### `hal_display_canvas_select(hal_canvas_handle_t canvas)`
*   **Description:** Redirects all subsequent drawing to the canvas. Pass `NULL` for main screen.

### `hal_display_canvas_draw(hal_canvas_handle_t canvas, int32_t x, int32_t y)`
*   **Description:** Blits canvas to the main display.

## Fast Blitting API

### `hal_display_fast_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data)`
*   **Description:** DMA-accelerated transfer of RGB565 buffer.

### `hal_display_fast_blit_transparent(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data, uint16_t transparent_color)`
*   **Description:** Scanline-optimized blit with transparency.