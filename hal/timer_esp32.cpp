/**
 * @file timer_esp32.cpp
 * @brief ESP32 implementation of Timer HAL
 *
 * Provides a concrete implementation of the Timer HAL contract for ESP32-based
 * hardware targets using the ESP-IDF's high-resolution timer (esp_timer).
 *
 * See features/hal_timer_esp32.md for complete specification.
 */

#include "timer.h"
#include <esp_timer.h>

bool hal_timer_init(void) {
    // ESP-IDF esp_timer initializes automatically, so we just return success
    return true;
}

uint64_t hal_timer_get_micros(void) {
    // Direct wrapper around ESP-IDF's esp_timer_get_time()
    return esp_timer_get_time();
}
