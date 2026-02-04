/**
 * @file animation_ticker.h
 * @brief Application Animation Ticker
 *
 * Provides a high-level animation timing service to ensure smooth, consistent
 * animation frame rates across all hardware platforms. Uses the Timer HAL to
 * create a reliable 30 frames-per-second (fps) tick.
 *
 * See features/app_animation_ticker.md for complete specification.
 */

#ifndef ANIMATION_TICKER_H
#define ANIMATION_TICKER_H

#include <stdint.h>

/**
 * @brief Animation frame rate management service
 *
 * The AnimationTicker class provides a frame rate synchronization mechanism
 * that ensures animations run at a consistent frame rate regardless of CPU
 * speed or workload variations.
 */
class AnimationTicker {
public:
    /**
     * @brief Constructs an AnimationTicker with the specified target frame rate
     *
     * @param target_fps The desired frames per second (e.g., 30 for 30fps)
     */
    AnimationTicker(uint32_t target_fps);

    /**
     * @brief Waits for the next frame time to synchronize animation loops
     *
     * Call this method at the end of each animation frame. It will delay
     * execution as needed to maintain the target frame rate. If a frame
     * takes longer than the frame time, it implements a "catch-up guard"
     * to prevent animation freezing.
     *
     * @return Elapsed time in seconds since the last frame, or 0.0f on first call
     */
    float waitForNextFrame();

private:
    uint64_t frame_time_micros;  ///< Time for a single frame in microseconds
    uint64_t next_frame_time;    ///< Timestamp when next frame should occur
    uint64_t last_frame_micros;  ///< Timestamp of last frame for deltaTime calculation
    bool first_call;             ///< Flag to track first call to waitForNextFrame
};

#endif // ANIMATION_TICKER_H
