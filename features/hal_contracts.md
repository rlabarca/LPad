> Prerequisite: None

# Feature: Hardware Abstraction Layer (HAL) - Display Contracts

> Label: "HAL Contracts"
> Category: "Hardware Layer"

## Introduction

This document defines the abstract interface for display operations within the Hardware Abstraction Layer (HAL). Any concrete implementation of a display driver (e.g., for a specific LCD panel or GPU) must adhere to these contract definitions. Claude will use these definitions to ensure consistent interaction with display hardware across different targets.

## Display HAL API

The following functions constitute the core display HAL API:

### `hal_display_init(void)`

*   **Description:** Initializes the display hardware, including power management, communication interfaces (e.g., SPI, I2C), and basic display settings. This function must be called once before any other display operations.
*   **Parameters:** `void` (no parameters).
*   **Returns:** `bool` - `true` if initialization was successful, `false` otherwise.
*   **Expected Behavior:**
    *   Should configure necessary GPIOs for display control.
    *   Should initialize communication protocols.
    *   Should power on the display.
    *   Should set initial display orientation and resolution if applicable.

### `hal_display_clear(uint16_t color)`

*   **Description:** Fills the entire display with a specified 16-bit RGB565 color.
*   **Parameters:**
    *   `uint16_t color`: The 16-bit RGB565 color value to fill the screen with.
*   **Returns:** `void`.
*   **Expected Behavior:**
    *   The entire visible area of the display should be set to the provided color.
    *   If the display uses a frame buffer, this function should update the buffer and potentially trigger a `flush` if necessary for immediate visibility.

### `hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color)`

*   **Description:** Draws a single pixel at the specified (x, y) coordinates with a given 16-bit RGB565 color.
*   **Parameters:**
    *   `int32_t x`: The X-coordinate of the pixel (0-indexed from the left).
    *   `int32_t y`: The Y-coordinate of the pixel (0-indexed from the top).
    *   `uint16_t color`: The 16-bit RGB565 color value for the pixel.
*   **Returns:** `void`.
*   **Expected Behavior:**
    *   A single pixel at the given coordinates should be set to the provided color.
    *   Coordinates outside the display boundaries should be handled gracefully (e.g., ignored or clipped).
    *   If the display uses a frame buffer, this function should update the buffer.

### `hal_display_flush(void)`

*   **Description:** Flushes any pending display buffer changes to the physical screen. This is crucial for buffered displays where `draw_pixel` or `clear` operations only modify an off-screen buffer.
*   **Parameters:** `void` (no parameters).
*   **Returns:** `void`.
*   **Expected Behavior:**
    *   If the display uses a frame buffer, the contents of the buffer should be transferred to the actual display hardware, making changes visible.
    *   For unbuffered displays, this function may do nothing or act as a synchronization point.

---

## Timer HAL API

The following functions constitute the core timer HAL API:

### `hal_timer_init(void)`

*   **Description:** Initializes the high-resolution hardware timer. This must be called once before `hal_timer_get_micros()` is used.
*   **Parameters:** `void` (no parameters).
*   **Returns:** `bool` - `true` if initialization was successful, `false` otherwise.

### `hal_timer_get_micros(void)`

*   **Description:** Gets the current time from the high-resolution timer in microseconds. The timer should be monotonic and should not be affected by wall-clock time adjustments.
*   **Parameters:** `void` (no parameters).
*   **Returns:** `uint64_t` - The number of microseconds since the timer was initialized or since the device booted. The value should wrap around on overflow.
*   **Expected Behavior:**
    *   Consecutive calls to this function should yield non-decreasing values that accurately reflect the passage of time.