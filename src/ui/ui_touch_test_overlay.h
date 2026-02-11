/**
 * @file ui_touch_test_overlay.h
 * @brief Touch Test Overlay UI Component
 *
 * Renders touch event information on top of the active screen for debugging
 * and verification of the touch subsystem. The overlay:
 * - Displays the most recent gesture event
 * - Auto-hides after 3 seconds of inactivity
 * - Uses transparent background to avoid corrupting underlying content
 *
 * Specification: features/ui_touch_test_overlay.md
 */

#ifndef UI_TOUCH_TEST_OVERLAY_H
#define UI_TOUCH_TEST_OVERLAY_H

#include <stdint.h>
#include "input/touch_gesture_engine.h"

class TouchTestOverlay {
public:
    /**
     * @brief Constructor
     */
    TouchTestOverlay();

    /**
     * @brief Initialize the overlay
     * @return true if initialization was successful
     */
    bool begin();

    /**
     * @brief Update the overlay with a new touch event
     *
     * This should be called whenever a touch gesture is detected.
     * The overlay will become visible and display the event information.
     *
     * @param event The touch gesture event to display
     */
    void update(const touch_gesture_event_t& event);

    /**
     * @brief Update the overlay's internal state (for timeout)
     * @param delta_time Time elapsed since last update (milliseconds)
     */
    void tick(uint32_t delta_time);

    /**
     * @brief Render the overlay to the display
     *
     * This should be called every frame. If the overlay is hidden or timed out,
     * this function will do nothing.
     */
    void render();

    /**
     * @brief Check if the overlay is currently visible
     * @return true if visible, false if hidden
     */
    bool isVisible() const { return m_visible; }

private:
    static constexpr uint32_t TIMEOUT_MS = 3000;  // 3 seconds

    bool m_visible;
    uint32_t m_time_since_last_event_ms;

    // Most recent event data
    touch_gesture_type_t m_last_type;
    touch_direction_t m_last_direction;
    int16_t m_last_x;
    int16_t m_last_y;
    float m_last_x_percent;
    float m_last_y_percent;

    // Pre-rendered buffer for overlay text (transparent DMA blit pattern from architecture)
    uint16_t* m_text_buffer;
    int16_t m_text_width;
    int16_t m_text_height;
    bool m_buffer_valid;

    // Helper functions
    void renderTextToBuffer();
    const char* gestureTypeToString(touch_gesture_type_t type) const;
    const char* directionToString(touch_direction_t dir) const;
};

#endif // UI_TOUCH_TEST_OVERLAY_H
