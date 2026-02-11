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

    /**
     * @brief Get the start position of the current/last gesture (for debugging)
     * @param start_x Output parameter for start X coordinate
     * @param start_y Output parameter for start Y coordinate
     */
    void getStartPosition(int16_t* start_x, int16_t* start_y) const {
        *start_x = m_start_x;
        *start_y = m_start_y;
    }

    /**
     * @brief Configure board-specific edge detection zones
     *
     * Different touch panels have different active areas and sensitivities.
     * This allows the HAL to configure edge zones that match the actual
     * touchable area of the hardware.
     *
     * @param left_threshold X coordinate threshold for left edge (e.g., x < 80)
     * @param right_threshold X coordinate threshold for right edge (e.g., x > 200)
     * @param top_threshold Y coordinate threshold for top edge (e.g., y < 60)
     * @param bottom_threshold Y coordinate threshold for bottom edge (e.g., y > 200)
     */
    void setEdgeZones(int16_t left_threshold, int16_t right_threshold,
                      int16_t top_threshold, int16_t bottom_threshold) {
        m_edge_left_threshold = left_threshold;
        m_edge_right_threshold = right_threshold;
        m_edge_top_threshold = top_threshold;
        m_edge_bottom_threshold = bottom_threshold;
        m_use_custom_edge_zones = true;
    }

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

    // Board-specific edge zone configuration (for limited touch panel ranges)
    bool m_use_custom_edge_zones;
    int16_t m_edge_left_threshold;
    int16_t m_edge_right_threshold;
    int16_t m_edge_top_threshold;
    int16_t m_edge_bottom_threshold;

    // Gesture thresholds (tuned for small touch screens)
    static constexpr uint32_t HOLD_THRESHOLD_MS = 500;        // 500ms for hold (was 1000ms - too long)
    static constexpr float MOVEMENT_THRESHOLD_PERCENT = 0.05f; // 5% of max dimension (was 10% - too strict)
    static constexpr float SWIPE_DISTANCE_PERCENT = 0.12f;     // 12% of axis dimension for center swipes
    static constexpr float EDGE_THRESHOLD_PERCENT = 0.30f;     // 30% from edge (balanced - not too strict, not too loose)
    static constexpr float EDGE_SWIPE_DISTANCE_PERCENT = 0.30f; // 30% of axis dimension for edge drags (2.5x center swipes)

    // Helper functions
    int16_t getMovementThreshold() const;
    int16_t getSwipeDistanceThreshold() const;  // Deprecated: use axis-aware version
    int16_t getEdgeSwipeDistanceThreshold() const;  // Deprecated: use axis-aware version
    int16_t getSwipeDistanceThreshold(int16_t dx, int16_t dy) const;  // Axis-aware version
    int16_t getEdgeSwipeDistanceThreshold(int16_t dx, int16_t dy) const;  // Axis-aware version
    int16_t getEdgeThreshold() const;
    bool isNearEdge(int16_t x, int16_t y, touch_direction_t* edge_dir) const;
    touch_direction_t getSwipeDirection(int16_t dx, int16_t dy) const;
    void fillEventData(touch_gesture_event_t* event, touch_gesture_type_t type,
                       int16_t x, int16_t y, touch_direction_t dir = TOUCH_DIR_NONE);
};

#endif // TOUCH_GESTURE_ENGINE_H
