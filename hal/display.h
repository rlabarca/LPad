/**
 * @file display.h
 * @brief Hardware Abstraction Layer (HAL) - Display Contracts
 *
 * This header defines the abstract interface for display operations within the HAL.
 * Any concrete implementation of a display driver must adhere to these contract definitions.
 *
 * See features/hal_spec_display.md for complete specification.
 */

#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the display hardware
 *
 * Initializes the display hardware, including power management, communication
 * interfaces (e.g., SPI, I2C), and basic display settings. This function must
 * be called once before any other display operations.
 *
 * @return true if initialization was successful, false otherwise
 */
bool hal_display_init(void);

/**
 * @brief Fills the entire display with a specified color
 *
 * @param color The 16-bit RGB565 color value to fill the screen with
 */
void hal_display_clear(uint16_t color);

/**
 * @brief Draws a single pixel at the specified coordinates
 *
 * @param x The X-coordinate of the pixel (0-indexed from the left)
 * @param y The Y-coordinate of the pixel (0-indexed from the top)
 * @param color The 16-bit RGB565 color value for the pixel
 */
void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color);

/**
 * @brief Flushes any pending display buffer changes to the physical screen
 *
 * This is crucial for buffered displays where draw_pixel or clear operations
 * only modify an off-screen buffer. For unbuffered displays, this function
 * may do nothing or act as a synchronization point.
 */
void hal_display_flush(void);

/**
 * @brief Returns the width of the active display in pixels
 *
 * @return int32_t The width of the display in pixels
 */
int32_t hal_display_get_width_pixels(void);

/**
 * @brief Returns the height of the active display in pixels
 *
 * @return int32_t The height of the display in pixels
 */
int32_t hal_display_get_height_pixels(void);

/**
 * @brief Sets the display rotation
 *
 * Sets the orientation of the display. Valid rotation values are typically
 * 0, 90, 180, and 270 degrees. After rotation, subsequent calls to
 * hal_display_get_width_pixels() and hal_display_get_height_pixels() will
 * return dimensions corresponding to the new orientation.
 *
 * @param degrees The rotation angle in degrees (0, 90, 180, or 270)
 */
void hal_display_set_rotation(int degrees);

/**
 * @brief Gets the underlying Arduino_GFX display object
 *
 * Returns a pointer to the underlying Arduino_GFX display object for
 * advanced use cases that require direct access to the GFX API.
 *
 * @return void* Pointer to the Arduino_GFX object (must be cast appropriately)
 */
void* hal_display_get_gfx(void);

// Canvas-based (Layered) Drawing API
// See features/display_canvas_drawing.md for complete specification

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

/**
 * @brief Fast DMA-accelerated blit from memory buffer to display
 *
 * Transfers a rectangular block of RGB565 pixel data from a memory buffer
 * to the display using hardware acceleration (DMA or bulk transfer).
 * This function MUST NOT use pixel-by-pixel loops.
 *
 * @param x The top-left X-coordinate on the destination display
 * @param y The top-left Y-coordinate on the destination display
 * @param w The width of the block to blit
 * @param h The height of the block to blit
 * @param data Pointer to the source buffer containing RGB565 pixel data
 */
void hal_display_fast_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);

/**
 * @brief Fast blit with transparency using scanline optimization
 *
 * Transfers pixel data from buffer to display, skipping pixels that match
 * the transparent color. Uses scanline-based DMA transfers for better
 * performance than pixel-by-pixel drawing.
 *
 * @param x The top-left X-coordinate on the destination display
 * @param y The top-left Y-coordinate on the destination display
 * @param w The width of the block to blit
 * @param h The height of the block to blit
 * @param data Pointer to the source buffer containing RGB565 pixel data
 * @param transparent_color The RGB565 color value to treat as transparent
 */
void hal_display_fast_blit_transparent(int16_t x, int16_t y, int16_t w, int16_t h,
                                       const uint16_t* data, uint16_t transparent_color);

#ifdef __cplusplus
}
#endif

#endif // HAL_DISPLAY_H
