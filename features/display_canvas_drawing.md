> Prerequisite: features/hal_spec_display.md
> Prerequisite: features/hal_dma_blitting.md

# Feature: Layered Canvas Drawing

> Label: "Layered Canvas Drawing"
> Category: "UI Framework"

This feature adds support for canvas-based, or layered, drawing. It extends the HAL contract with functions to create, manage, and draw off-screen canvases (also known as sprites or buffers). This enables rendering complex scenes with multiple layers that can be cleared and redrawn independently.

The implementation will rely on the `Arduino_Canvas` class provided by the `Arduino_GFX` library.

## 1. HAL Contract Extension

The file `hal/display.h` must be updated to include the new data types and function declarations for canvas management.

**Scenario: Update display.h with Canvas API**
*   **Given:** The `hal/display.h` file exists.
*   **When:** The agent extends the HAL contract.
*   **Then:** The following declarations must be added to `hal/display.h`:
    ```cpp
    // A handle representing an off-screen drawing surface
    typedef void* hal_canvas_handle_t;

    /**
     * @brief Creates an off-screen drawing canvas.
     *
     * @param width The width of the canvas in pixels.
     * @param height The height of the canvas in pixels.
     * @return A handle to the created canvas, or nullptr on failure.
     */
    hal_canvas_handle_t hal_display_canvas_create(int16_t width, int16_t height);

    /**
     * @brief Deletes a canvas and frees its memory.
     *
     * @param canvas The handle to the canvas to delete.
     */
    void hal_display_canvas_delete(hal_canvas_handle_t canvas);

    /**
     * @brief Selects a canvas as the current target for all subsequent drawing operations.
     *
     * Pass nullptr to select the main display again.
     *
     * @param canvas The handle to the canvas to draw on, or nullptr for the screen.
     */
    void hal_display_canvas_select(hal_canvas_handle_t canvas);

    /**
     * @brief Draws a canvas onto the main display.
     *
     * @param canvas The handle of the canvas to draw.
     * @param x The destination X-coordinate on the main display.
     * @param y The destination Y-coordinate on the main display.
     */
    void hal_display_canvas_draw(hal_canvas_handle_t canvas, int32_t x, int32_t y);

    /**
     * @brief Fills a canvas with a specific color.
     *
     * @param canvas The handle of the canvas to clear.
     * @param color The 16-bit color to fill with.
     */
    void hal_display_canvas_fill(hal_canvas_handle_t canvas, uint16_t color);
    ```

## 2. HAL Implementation

The new canvas functions must be implemented in all concrete HAL files that use the `Arduino_GFX` library.

**Scenario: Implement Canvas API in GFX-based HALs**
*   **Given:** A HAL implementation file (e.g., `hal/display_esp32_s3_amoled.cpp`, `hal/display_tdisplay_s3_plus.cpp`) that uses `Arduino_GFX`.
*   **When:** The agent implements the canvas functions.
*   **Then:** The implementation must use an `Arduino_Canvas` object to represent the canvas.
*   **And:** `hal_display_canvas_create` should `new` an `Arduino_Canvas` and return it as a `hal_canvas_handle_t`.
*   **And:** `hal_display_canvas_delete` should `delete` the provided canvas handle.
*   **And:** `hal_display_canvas_select` should set a global pointer to the selected canvas, which drawing functions will use as their target.
*   **And:** `hal_display_canvas_draw` should call the main `g_gfx->draw16bitBeRGBBitmap()` method to blit the canvas to the screen.
*   **And:** `hal_display_canvas_fill` should call the canvas object's `fillScreen()` method.

## 3. Unit Test

**Scenario: Drawing to a Canvas**
*   **Given:** A new test environment in `test/test_display_canvas/`.
*   **And:** The display is initialized.
*   **When:** A canvas is created with `hal_display_canvas_create(50, 50)`.
*   **And:** The canvas is selected with `hal_display_canvas_select()`.
*   **And:** A pixel is drawn at `(10, 10)` on the canvas.
*   **And:** The main display is selected again with `hal_display_canvas_select(nullptr)`.
*   **And:** The canvas is drawn to the screen at `(20, 20)` with `hal_display_canvas_draw()`.
*   **Then:** A pixel should be visible on the main display at coordinates `(30, 30)`.
*   **And:** The test should pass.

## 4. Hardware (HIL) Test

To verify this feature on the device, the Builder must temporarily modify `src/main.cpp` to run a visual demonstration.

**Scenario: Visual Confirmation of Canvas Layering**
*   **Given:** The `setup()` function in `src/main.cpp`.
*   **When:** The agent modifies the file for a HIL test.
*   **Then:** The `setup()` function must be modified to:
    1. Initialize the display.
    2. Create a 100x100 pixel canvas (`bg_canvas`) and fill it with blue.
    3. Create a 40x40 pixel canvas (`fg_canvas`) and fill it with red.
    4. Draw `bg_canvas` to the screen at coordinates (50, 50).
    5. Draw `fg_canvas` to the screen at coordinates (80, 80), so it overlaps the background canvas.
    6. Clean up by deleting both canvases.
*   **And:** The `loop()` function should be empty.
