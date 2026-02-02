/**
 * @file display.h
 * @brief Hardware Abstraction Layer (HAL) - Display Contracts
 *
 * This header defines the abstract interface for display operations within the HAL.
 * Any concrete implementation of a display driver must adhere to these contract definitions.
 *
 * See features/hal_contracts.md for complete specification.
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

#ifdef __cplusplus
}
#endif

#endif // HAL_DISPLAY_H
