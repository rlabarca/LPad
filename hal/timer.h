/**
 * @file timer.h
 * @brief Hardware Abstraction Layer (HAL) - Timer Contracts
 *
 * This header defines the abstract interface for timer operations within the HAL.
 * Any concrete implementation of a timer must adhere to these contract definitions.
 *
 * See features/hal_spec_timer.md for complete specification.
 */

#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the high-resolution hardware timer
 *
 * This must be called once before hal_timer_get_micros() is used.
 *
 * @return true if initialization was successful, false otherwise
 */
bool hal_timer_init(void);

/**
 * @brief Gets the current time from the high-resolution timer in microseconds
 *
 * The timer should be monotonic and should not be affected by wall-clock time
 * adjustments. The value should wrap around on overflow.
 *
 * @return The number of microseconds since the timer was initialized or since
 *         the device booted
 */
uint64_t hal_timer_get_micros(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_TIMER_H
