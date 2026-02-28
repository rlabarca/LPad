/**
 * @file timer_stub.cpp
 * @brief Stub implementation of Timer HAL
 *
 * Provides weak stub implementations of the Timer HAL contract to ensure
 * that builds for non-ESP32 targets link successfully without requiring
 * a full timer implementation.
 *
 * See features/hal_timer_esp32.md for complete specification.
 */

#include "timer.h"

__attribute__((weak)) bool hal_timer_init(void) {
    // Stub: return false to indicate no timer available
    return false;
}

__attribute__((weak)) uint64_t hal_timer_get_micros(void) {
    // Stub: return 0 as no timer is available
    return 0;
}
