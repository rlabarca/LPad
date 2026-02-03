> Prerequisite: features/hal_contracts.md

# Feature: Application Animation Ticker

## Introduction

This feature provides a high-level animation timing service to ensure smooth, consistent animation frame rates across all hardware platforms. It uses the `Timer HAL` contract to create a reliable 30 frames-per-second (fps) "tick" that application logic can synchronize with. This decouples animation logic from CPU speed and ensures animations have the same duration and feel on all devices.

## Scenario: Synchronize to a 30fps Tick
- **Given** an `AnimationTicker` is initialized for 30fps.
- **When** the `waitForNextFrame()` method is called inside a loop
- **Then** the loop's execution should be paced to run at a rate of approximately 30 times per second.
- **And** the time spent waiting should be calculated based on the elapsed microseconds reported by `hal_timer_get_micros()`.

## Implementation Details

- Create a new header file: `src/animation_ticker.h`
- Create a corresponding implementation file: `src/animation_ticker.cpp`
- These files will define and implement the `AnimationTicker` class.

### `src/animation_ticker.h`
```cpp
#pragma once

#include <cstdint>

class AnimationTicker {
public:
    AnimationTicker(uint32_t target_fps);

    // Call this at the end of your animation loop to wait for the next frame.
    void waitForNextFrame();

private:
    const uint64_t frame_time_micros;
    uint64_t next_frame_time;
};
```

### `src/animation_ticker.cpp`
```cpp
#include "animation_ticker.h"
#include <hal/display.h> // For hal_timer_get_micros()
#include <Arduino.h>     // For delay()

AnimationTicker::AnimationTicker(uint32_t target_fps) :
    frame_time_micros(1000000 / target_fps),
    next_frame_time(0) {
    hal_timer_init(); // Ensure timer is initialized
}

void AnimationTicker::waitForNextFrame() {
    uint64_t current_time = hal_timer_get_micros();

    if (next_frame_time == 0) {
        // First frame, just set the time and return
        next_frame_time = current_time + frame_time_micros;
        return;
    }

    if (current_time < next_frame_time) {
        // We have time to spare, so delay
        uint64_t wait_time_micros = next_frame_time - current_time;
        delay(wait_time_micros / 1000); // delay() takes milliseconds
    }

    // Schedule the next frame
    next_frame_time += frame_time_micros;

    // A simple guard against falling too far behind (e.g., after a long blocking operation)
    // This resets the ticker to prevent a "death spiral" of catch-up frames.
    if (hal_timer_get_micros() > next_frame_time) {
        next_frame_time = hal_timer_get_micros() + frame_time_micros;
    }
}
```

## Unit Test Plan

- Create a test file `test/test_animation_ticker/test_animation_ticker.cpp`.
- The test should use a mock or stub for `hal_timer_get_micros()` to simulate the passage of time.
- **Test Case 1:** Verify that `waitForNextFrame` introduces a delay when the "work" in the frame is shorter than the frame time.
- **Test Case 2:** Verify that `waitForNextFrame` does not introduce a delay when the "work" in the frame is longer than the frame time.
- **Test Case 3:** Verify the "death spiral" guard correctly resets the `next_frame_time` when the ticker falls behind significantly.

---

## Hardware (HIL) Test

To visually confirm the correct operation of the `AnimationTicker`, a temporary test should be created in `main.cpp`.

**Instructions for the Builder:**
1.  In `main.cpp`, include `animation_ticker.h`.
2.  Instantiate `AnimationTicker ticker(30);`.
3.  Instantiate `RelativeDisplay` for drawing.
4.  In the main `loop()`, create an animation of a small box moving horizontally across the screen.
5.  On each iteration of the loop:
    *   Clear the screen (or the portion where the box was).
    *   Calculate the new box position.
    *   Draw the box at the new position.
    *   Call `ticker.waitForNextFrame();`.
6.  The box should move smoothly from left to right at a consistent speed, irrespective of the device.
