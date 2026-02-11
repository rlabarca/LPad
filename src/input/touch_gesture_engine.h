/**
 * @file touch_gesture_engine.h
 * @brief Touch Gesture Recognition Engine
 *
 * Converts raw touch data from the HAL into high-level semantic gestures:
 * - TAP: Quick press and release
 * - HOLD: Press and hold for >1 second
 * - HOLD_DRAG: Hold followed by movement
 * - SWIPE: Fast movement from center region
 * - EDGE_DRAG: Movement starting from screen edge
 *
 * Provides coordinates in both absolute pixels and relative percentages.
 *
 * Specification: features/touch_gesture_engine.md
 */

#ifndef TOUCH_GESTURE_ENGINE_H
#define TOUCH_GESTURE_ENGINE_H

#include <stdint.h>

/**
 * @brief Touch gesture event types
 */
typedef enum {
    TOUCH_NONE = 0,         ///< No gesture detected
    TOUCH_TAP,              ///< Quick tap (press + release < 1s, minimal movement)
    TOUCH_HOLD,             ///< Press and hold (> 1s, minimal movement)
    TOUCH_HOLD_DRAG,        ///< Hold followed by dragging
    TOUCH_SWIPE,            ///< Fast directional swipe from center
    TOUCH_EDGE_DRAG         ///< Swipe starting from screen edge
} touch_gesture_type_t;

/**
 * @brief Gesture direction (for swipes and edge drags)
 */
typedef enum {
    TOUCH_DIR_NONE = 0,
    TOUCH_DIR_UP,
    TOUCH_DIR_DOWN,
    TOUCH_DIR_LEFT,
    TOUCH_DIR_RIGHT
} touch_direction_t;

/**
 * @brief Touch gesture event data
 */
typedef struct {
    touch_gesture_type_t type;      ///< Type of gesture detected
    touch_direction_t direction;    ///< Direction (for swipes/edge drags)

    // Absolute coordinates (pixels)
    int16_t x_px;                   ///< X position in pixels
    int16_t y_px;                   ///< Y position in pixels

    // Relative coordinates (0.0 to 1.0)
    float x_percent;                ///< X position as percentage (0.0 = left, 1.0 = right)
    float y_percent;                ///< Y position as percentage (0.0 = top, 1.0 = bottom)
} touch_gesture_event_t;

/**
 * @brief Touch gesture engine class
 */
class TouchGestureEngine {
public:
    /**
     * @brief Constructor
     * @param screen_width Display width in pixels
     * @param screen_height Display height in pixels
     */
    TouchGestureEngine(int16_t screen_width, int16_t screen_height);

    /**
     * @brief Update the gesture engine with a new touch sample
     *
     * This should be called every frame with the current touch state from hal_touch_read().
     * The engine will track state changes and detect gestures.
     *
     * @param x Current X coordinate (pixels)
     * @param y Current Y coordinate (pixels)
     * @param is_pressed true if finger is currently down
     * @param delta_time Time elapsed since last update (milliseconds)
     * @param event Output parameter filled with gesture event if detected
     * @return true if a new gesture event was detected, false otherwise
     */
    bool update(int16_t x, int16_t y, bool is_pressed, uint32_t delta_time, touch_gesture_event_t* event);

private:
    // Screen dimensions
    int16_t m_screen_width;
    int16_t m_screen_height;
    int16_t m_screen_max_dim;  // Max of width and height

    // State tracking
    enum State {
        STATE_IDLE,           // No touch
        STATE_PRESSED,        // Touch down, waiting to classify
        STATE_HOLD_DETECTED,  // Hold threshold reached
        STATE_DRAGGING        // Active drag in progress
    };
    State m_state;

    // Touch tracking
    int16_t m_start_x, m_start_y;        // Initial touch position
    int16_t m_last_x, m_last_y;          // Last known position
    uint32_t m_touch_duration_ms;         // Time since touch started
    bool m_hold_event_fired;              // Whether hold event was already fired

    // Gesture thresholds (from spec)
    static constexpr uint32_t HOLD_THRESHOLD_MS = 1000;      // 1 second for hold
    static constexpr float MOVEMENT_THRESHOLD_PERCENT = 0.10f;  // 10% of max dimension
    static constexpr float SWIPE_DISTANCE_PERCENT = 0.40f;      // 40% of dimension for swipe
    static constexpr float EDGE_THRESHOLD_PERCENT = 0.20f;      // 20% from edge

    // Helper functions
    int16_t getMovementThreshold() const;
    int16_t getSwipeDistanceThreshold() const;
    int16_t getEdgeThreshold() const;
    bool isNearEdge(int16_t x, int16_t y, touch_direction_t* edge_dir) const;
    touch_direction_t getSwipeDirection(int16_t dx, int16_t dy) const;
    void fillEventData(touch_gesture_event_t* event, touch_gesture_type_t type,
                       int16_t x, int16_t y, touch_direction_t dir = TOUCH_DIR_NONE);
};

#endif // TOUCH_GESTURE_ENGINE_H
