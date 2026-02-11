/**
 * @file touch_stub.cpp
 * @brief Touch HAL Stub Implementation
 *
 * Stub implementation of the touch HAL for platforms without touch hardware.
 * Always reports no touch detected.
 *
 * Used for:
 * - Native unit testing
 * - Platforms without touch support
 * - Development and debugging
 */

#include "touch.h"

bool hal_touch_init(void) {
    // Stub: always succeeds
    return true;
}

bool hal_touch_read(hal_touch_point_t* point) {
    if (!point) {
        return false;
    }

    // Stub: always report no touch
    point->x = 0;
    point->y = 0;
    point->is_pressed = false;

    return true;
}
