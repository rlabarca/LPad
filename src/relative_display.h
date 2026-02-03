/**
 * @file relative_display.h
 * @brief Relative Display Drawing Abstraction
 *
 * This module provides a resolution-independent drawing API that uses
 * relative coordinates (percentages of screen width/height) instead of
 * absolute pixel coordinates. This allows visual elements to scale
 * automatically across different display hardware.
 *
 * See features/display_relative_drawing.md for complete specification.
 */

#ifndef RELATIVE_DISPLAY_H
#define RELATIVE_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for C++ structs (defined in ui_time_series_graph.h when included from C++)
#ifdef __cplusplus
struct LinearGradient;
struct RadialGradient;
#endif

/**
 * @brief Initializes the relative display abstraction layer
 *
 * Queries the underlying HAL for screen dimensions and prepares the
 * abstraction layer for drawing operations.
 */
void display_relative_init(void);

/**
 * @brief Draws a single pixel at a relative (x, y) coordinate
 *
 * @param x_percent The X-coordinate as a percentage of screen width (0.0 = left, 100.0 = right)
 * @param y_percent The Y-coordinate as a percentage of screen height (0.0 = top, 100.0 = bottom)
 * @param color The 16-bit RGB565 color value
 */
void display_relative_draw_pixel(float x_percent, float y_percent, uint16_t color);

/**
 * @brief Draws a horizontal line across a relative segment of the display
 *
 * @param y_percent The Y-coordinate as a percentage of screen height
 * @param x_start_percent The starting X-coordinate as a percentage
 * @param x_end_percent The ending X-coordinate as a percentage
 * @param color The 16-bit RGB565 color value
 */
void display_relative_draw_horizontal_line(float y_percent, float x_start_percent, float x_end_percent, uint16_t color);

/**
 * @brief Draws a vertical line across a relative segment of the display
 *
 * @param x_percent The X-coordinate as a percentage of screen width
 * @param y_start_percent The starting Y-coordinate as a percentage
 * @param y_end_percent The ending Y-coordinate as a percentage
 * @param color The 16-bit RGB565 color value
 */
void display_relative_draw_vertical_line(float x_percent, float y_start_percent, float y_end_percent, uint16_t color);

/**
 * @brief Fills a rectangle defined by relative top-left corner and relative width/height
 *
 * @param x_start_percent Top-left X-coordinate as a percentage
 * @param y_start_percent Top-left Y-coordinate as a percentage
 * @param width_percent Width of the rectangle as a percentage
 * @param height_percent Height of the rectangle as a percentage
 * @param color The 16-bit RGB565 color value
 */
void display_relative_fill_rectangle(float x_start_percent, float y_start_percent, float width_percent, float height_percent, uint16_t color);

/**
 * @brief Draws a thick line between two relative points
 *
 * @param x1_percent Starting X-coordinate as a percentage
 * @param y1_percent Starting Y-coordinate as a percentage
 * @param x2_percent Ending X-coordinate as a percentage
 * @param y2_percent Ending Y-coordinate as a percentage
 * @param thickness_percent Line thickness in relative percentage units
 * @param color The 16-bit RGB565 color value
 */
void display_relative_draw_line_thick(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, uint16_t color);

#ifdef __cplusplus
}

// C++ overloads for gradient functions
/**
 * @brief Fills a rectangle with a linear gradient
 *
 * @param x_percent Top-left X-coordinate as a percentage
 * @param y_percent Top-left Y-coordinate as a percentage
 * @param w_percent Width as a percentage
 * @param h_percent Height as a percentage
 * @param gradient The linear gradient specification
 */
void display_relative_fill_rect_gradient(float x_percent, float y_percent, float w_percent, float h_percent, const LinearGradient& gradient);

/**
 * @brief Draws a thick line with a linear gradient
 *
 * @param x1_percent Starting X-coordinate as a percentage
 * @param y1_percent Starting Y-coordinate as a percentage
 * @param x2_percent Ending X-coordinate as a percentage
 * @param y2_percent Ending Y-coordinate as a percentage
 * @param thickness_percent Line thickness in relative percentage units
 * @param gradient The linear gradient specification
 */
void display_relative_draw_line_thick_gradient(float x1_percent, float y1_percent, float x2_percent, float y2_percent, float thickness_percent, const LinearGradient& gradient);

/**
 * @brief Fills a circle with a radial gradient
 *
 * @param center_x_percent Center X-coordinate as a percentage
 * @param center_y_percent Center Y-coordinate as a percentage
 * @param radius_percent Radius in relative percentage units
 * @param gradient The radial gradient specification
 */
void display_relative_fill_circle_gradient(float center_x_percent, float center_y_percent, float radius_percent, const RadialGradient& gradient);

#endif // __cplusplus

#endif // RELATIVE_DISPLAY_H
