/**
 * @file animation_ticker.cpp
 * @brief Implementation of AnimationTicker class
 *
 * See features/app_animation_ticker.md for complete specification.
 */

#include "animation_ticker.h"
#include "../hal/timer.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#else
// Forward declarations for mocked Arduino functions in unit tests
extern "C" void delay(unsigned long ms);
extern "C" void delayMicroseconds(unsigned int us);
#endif

AnimationTicker::AnimationTicker(uint32_t target_fps)
    : first_call(true), next_frame_time(0), last_frame_micros(0) {
    // Calculate the time for a single frame in microseconds
    frame_time_micros = 1000000ULL / target_fps;

    // Initialize the hardware timer
    hal_timer_init();
}

float AnimationTicker::waitForNextFrame() {
    // On the first call, initialize timing state and return 0.0f
    if (first_call) {
        uint64_t current_time = hal_timer_get_micros();
        last_frame_micros = current_time;
        next_frame_time = current_time + frame_time_micros;
        first_call = false;
        return 0.0f;
    }

    // Get the current time
    uint64_t current_time = hal_timer_get_micros();

    // Calculate deltaTime (time elapsed since last frame) in seconds
    float deltaTime = (current_time - last_frame_micros) / 1000000.0f;

    // Check if we're behind schedule (death spiral guard)
    if (current_time >= next_frame_time) {
        // We've missed the frame deadline - reset schedule based on current time
        // instead of trying to catch up on all missed frames
        next_frame_time = current_time + frame_time_micros;
        last_frame_micros = current_time;
        return deltaTime;
    }

    // We're ahead of schedule - wait for the remaining time
    uint64_t time_to_wait = next_frame_time - current_time;

    // Convert microseconds to milliseconds for delay (Arduino's delay uses ms)
    // Use delayMicroseconds for sub-millisecond delays
    if (time_to_wait >= 1000) {
        delay(time_to_wait / 1000);
    }
    // For remaining microseconds or small delays, use delayMicroseconds
    uint64_t remaining_micros = time_to_wait % 1000;
    if (remaining_micros > 0) {
        delayMicroseconds(remaining_micros);
    }

    // Advance to the next frame time
    next_frame_time += frame_time_micros;
    last_frame_micros = current_time;

    return deltaTime;
}
