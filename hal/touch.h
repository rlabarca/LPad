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
    bool is_home_button; ///< true if virtual home button was pressed (CST816 only)
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
}  // extern "C"

// C++ only: Gesture engine configuration
class TouchGestureEngine;

/**
 * @brief Configure gesture engine with board-specific touch panel characteristics
 *
 * Different touch panels have different active areas and sensitivities.
 * This function applies board-specific edge detection thresholds to match
 * the actual touchable area of the hardware.
 *
 * This should be called after creating a TouchGestureEngine to ensure
 * gesture detection works correctly for this specific hardware.
 *
 * @param engine Pointer to the TouchGestureEngine to configure
 */
void hal_touch_configure_gesture_engine(TouchGestureEngine* engine);
#endif

#endif // HAL_TOUCH_H
