# Feature: Hardware-Accelerated DMA Blitting

> **Prerequisite:** `features/hal_contracts.md`

## 1. Introduction

This feature addresses performance bottlenecks in rendering by introducing a hardware-accelerated blitting function to the display HAL. The goal is to leverage Direct Memory Access (DMA) or other bulk-transfer capabilities of the underlying display drivers to move canvas buffers to the screen much faster than pixel-by-pixel loops.

## 2. Extension to Display HAL Contract

The `hal/display.h` interface must be extended with a new function for high-speed blitting.

### New HAL Function:

#### `hal_display_fast_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data)`

*   **Description:** Transfers a rectangular block of 16-bit RGB565 pixel data from a memory buffer (e.g., a `GfxCanvas` buffer) to a specified area on the screen.
*   **Performance Contract:** Implementations of this function **MUST** use the fastest available hardware-accelerated method. This typically involves DMA transfers. Implementations **MUST NOT** use a simple loop that calls `hal_display_draw_pixel` for each pixel.
*   **Parameters:**
    *   `int16_t x`: The top-left X-coordinate on the destination display.
    *   `int116_t y`: The top-left Y-coordinate on the destination display.
    *   `int16_t w`: The width of the block to blit.
    *   `int16_t h`: The height of the block to blit.
    *   `const uint16_t* data`: A pointer to the source buffer containing the pixel data.

## 3. Implementation Scenario

### Scenario: Implementing `fast_blit` for `tdisplay_s3_plus`

**Given** the `hal/display_tdisplay_s3_plus.cpp` implementation file.
**When** the `hal_display_fast_blit` function is implemented.
**Then** it MUST use the underlying `Arduino_GFX` driver's optimized bulk image transfer function (e.g., `pushImage`, `draw16bitBeRGBBitmap`, or similar).
**And** the implementation must be benchmarked or verified to be significantly faster than a manual `for` loop over all pixels.
**And** the Builder is responsible for consulting the `Arduino_GFX` library's documentation or examples to find the correct, most performant function for the target hardware.
