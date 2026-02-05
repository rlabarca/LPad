> Prerequisite: features/hal_contracts.md

# Feature: Animation Ticker Engine

> Label: "Animation Ticker Engine"
> Category: "Application Layer"

## Introduction

This feature provides a high-level animation timing service to ensure smooth, consistent animation frame rates across all hardware platforms. It uses the `Timer HAL` contract to create a reliable 30 frames-per-second (fps) "tick" that application logic can synchronize with. This decouples animation logic from CPU speed and ensures animations have the same duration and feel on all devices.

## Scenario: Synchronize to a 30fps Tick
- **Given** an `AnimationTicker` is initialized for 30fps.
- **When** the `waitForNextFrame()` method is called inside a loop
- **Then** the loop's execution should be paced to run at a rate of approximately 30 times per second.
- **And** the time spent waiting should be calculated based on the elapsed microseconds reported by `hal_timer_get_micros()`.

## Implementation Details

The Builder is responsible for creating the `AnimationTicker` class, which will provide the frame rate management service.

1.  **File Creation:**
    *   Create a header file: `src/animation_ticker.h`
    *   Create an implementation file: `src/animation_ticker.cpp`

2.  **Class Definition (`AnimationTicker`):**
    *   **Public API:**
        *   A constructor `AnimationTicker(uint32_t target_fps)` that accepts the desired frames per second.
        *   A method `float waitForNextFrame()` that an application's animation loop can call to synchronize with the ticker. It will return the elapsed time in seconds (`float`) since its previous call, or `0.0f` on the first call.
    *   **Private Members:**
        *   The class should store the calculated time for a single frame in microseconds (e.g., `1,000,000 / target_fps`).
        *   It should also store the timestamp for when the next frame is expected to occur.

3.  **Behavioral Logic:**
    *   **Constructor:** The constructor should calculate and store the per-frame time in microseconds. It should also ensure the underlying `hal_timer_init()` is called.
    *   **`waitForNextFrame()` Method:**
        *   On the very first call, it should initialize its internal timing state (e.g., `last_frame_micros`) and return `0.0f`.
        *   On subsequent calls, it must get the current time from `hal_timer_get_micros()`.
        *   It MUST calculate the `deltaTime` (time elapsed since the *last* frame) in seconds, as a `float`, based on the high-resolution `hal_timer_get_micros()` value.
        *   If the current time is less than the scheduled next frame time, it must calculate the remaining time in **microseconds** and use a delay mechanism that can wait for that specific microsecond duration (e.g., `delayMicroseconds()`). Using a millisecond-based delay is not acceptable as it lacks the required precision.
        *   It must then advance the next frame time by one frame's duration.
        *   **Catch-up Guard:** If the current time is already past the next scheduled frame time (e.g., due to a long-running calculation), the ticker should reset its schedule based on the *current* time instead of trying to catch up on all the missed frames. This prevents the animation from freezing and then rapidly playing a series of frames.
        *   It MUST return the calculated `deltaTime`.

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
