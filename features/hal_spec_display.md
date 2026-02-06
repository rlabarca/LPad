# Feature: HAL Display Specification

> Label: "HAL Display Specification"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md

## Introduction

This document defines the abstract interface for display operations within the Hardware Abstraction Layer (HAL). It focuses on pixel-level drawing, canvas management, and fast blitting.

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
