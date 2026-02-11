/**
 * @file touch.h
 * @brief Hardware Abstraction Layer for Touch Input
 *
 * This HAL provides a standard interface for touch controller initialization
 * and raw touch coordinate polling, isolating application logic from specific
 * touch hardware (e.g., CST816).
 *
 * Specification: features/hal_spec_touch.md
 */

#ifndef HAL_TOUCH_H
#define HAL_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Touch point data structure
 *
 * Reports raw touch coordinates and press state.
 */
typedef struct {
    int16_t x;          ///< X coordinate in screen pixels (0 = left)
    int16_t y;          ///< Y coordinate in screen pixels (0 = top)
    bool is_pressed;    ///< true if finger is currently down, false otherwise
} hal_touch_point_t;

/**
 * @brief Initialize the touch hardware
 *
 * Configures I2C bus and initializes the touch controller.
 * Must be called before hal_touch_read().
 *
 * @return true if initialization was successful, false otherwise
 */
bool hal_touch_init(void);

/**
 * @brief Read the current state of the touch panel
 *
 * This function is non-blocking and returns immediately.
 * Coordinates are mapped to the display's pixel coordinate system,
 * accounting for any rotation applied to the display.
 *
 * @param point Pointer to hal_touch_point_t structure to fill
 * @return true if read was successful (even if not pressed), false on hardware error
 */
bool hal_touch_read(hal_touch_point_t* point);

#ifdef __cplusplus
}
#endif

#endif // HAL_TOUCH_H
