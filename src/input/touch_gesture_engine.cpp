/**
 * @file touch_gesture_engine.cpp
 * @brief Touch Gesture Recognition Engine Implementation
 */

#include "touch_gesture_engine.h"
#include <cmath>
#include <algorithm>

TouchGestureEngine::TouchGestureEngine(int16_t screen_width, int16_t screen_height)
    : m_screen_width(screen_width),
      m_screen_height(screen_height),
      m_screen_max_dim(std::max(screen_width, screen_height)),
      m_state(STATE_IDLE),
      m_start_x(0),
      m_start_y(0),
      m_last_x(0),
      m_last_y(0),
      m_touch_duration_ms(0),
      m_hold_event_fired(false),
      m_use_custom_edge_zones(false),
      m_edge_left_threshold(0),
      m_edge_right_threshold(0),
      m_edge_top_threshold(0),
      m_edge_bottom_threshold(0)
{
}

bool TouchGestureEngine::update(int16_t x, int16_t y, bool is_pressed,
                                  uint32_t delta_time, touch_gesture_event_t* event) {
    bool gesture_detected = false;

    // State transitions based on touch state
    if (is_pressed) {
        // Touch is currently active
        m_touch_duration_ms += delta_time;

        switch (m_state) {
            case STATE_IDLE:
                // New touch started
                m_state = STATE_PRESSED;
                m_start_x = x;
                m_start_y = y;
                m_last_x = x;
                m_last_y = y;
                m_touch_duration_ms = 0;
                m_hold_event_fired = false;
                break;

            case STATE_PRESSED: {
                // Check for hold threshold
                if (m_touch_duration_ms >= HOLD_THRESHOLD_MS && !m_hold_event_fired) {
                    // Check if movement is within threshold
                    int16_t dx = std::abs(x - m_start_x);
                    int16_t dy = std::abs(y - m_start_y);
                    int16_t movement_threshold = getMovementThreshold();

                    if (dx <= movement_threshold && dy <= movement_threshold) {
                        // Hold detected!
                        m_state = STATE_HOLD_DETECTED;
                        m_hold_event_fired = true;
                        fillEventData(event, TOUCH_HOLD, x, y);
                        gesture_detected = true;
                    }
                } else {
                    // Check for movement beyond threshold (might become drag or swipe)
                    int16_t dx = std::abs(x - m_start_x);
                    int16_t dy = std::abs(y - m_start_y);
                    int16_t movement_threshold = getMovementThreshold();

                    if (dx > movement_threshold || dy > movement_threshold) {
                        // Significant movement detected, enter dragging state
                        m_state = STATE_DRAGGING;
                    }
                }
                m_last_x = x;
                m_last_y = y;
                break;
            }

            case STATE_HOLD_DETECTED: {
                // Check if user starts dragging after hold
                int16_t dx = std::abs(x - m_start_x);
                int16_t dy = std::abs(y - m_start_y);
                int16_t movement_threshold = getMovementThreshold();

                if (dx > movement_threshold || dy > movement_threshold) {
                    // Hold + Drag detected!
                    m_state = STATE_DRAGGING;
                    fillEventData(event, TOUCH_HOLD_DRAG, x, y);
                    gesture_detected = true;
                }
                m_last_x = x;
                m_last_y = y;
                break;
            }

            case STATE_DRAGGING:
                // Continue reporting drag events if from hold
                if (m_hold_event_fired) {
                    fillEventData(event, TOUCH_HOLD_DRAG, x, y);
                    gesture_detected = true;
                }
                m_last_x = x;
                m_last_y = y;
                break;
        }

    } else {
        // Touch released
        if (m_state != STATE_IDLE) {
            // Process gesture based on final state
            switch (m_state) {
                case STATE_PRESSED: {
                    // Quick tap (released before hold threshold)
                    int16_t dx = std::abs(m_last_x - m_start_x);
                    int16_t dy = std::abs(m_last_y - m_start_y);
                    int16_t movement_threshold = getMovementThreshold();

                    if (dx <= movement_threshold && dy <= movement_threshold) {
                        // Tap detected!
                        fillEventData(event, TOUCH_TAP, m_last_x, m_last_y);
                        gesture_detected = true;
                    }
                    break;
                }

                case STATE_DRAGGING: {
                    // Check if this was a swipe or edge drag
                    int16_t dx = m_last_x - m_start_x;
                    int16_t dy = m_last_y - m_start_y;

                    // Check if started from edge
                    touch_direction_t edge_dir;
                    bool started_from_edge = isNearEdge(m_start_x, m_start_y, &edge_dir);

                    // Use axis-aware thresholds (different for horizontal vs vertical)
                    // Edge drags require MORE movement than center swipes
                    int16_t swipe_threshold = started_from_edge
                        ? getEdgeSwipeDistanceThreshold(dx, dy)
                        : getSwipeDistanceThreshold(dx, dy);

                    // Determine primary axis for threshold comparison
                    int16_t primary_axis_delta = (std::abs(dx) > std::abs(dy)) ? std::abs(dx) : std::abs(dy);

                    if (primary_axis_delta >= swipe_threshold) {
                        if (started_from_edge) {
                            // Edge drag detected!
                            fillEventData(event, TOUCH_EDGE_DRAG, m_last_x, m_last_y, edge_dir);
                            gesture_detected = true;
                        } else {
                            // Swipe from center detected!
                            touch_direction_t swipe_dir = getSwipeDirection(dx, dy);
                            fillEventData(event, TOUCH_SWIPE, m_last_x, m_last_y, swipe_dir);
                            gesture_detected = true;
                        }
                    }
                    break;
                }

                case STATE_HOLD_DETECTED:
                    // Hold already reported, no additional event on release
                    break;

                default:
                    break;
            }

            // Return to idle
            m_state = STATE_IDLE;
            m_touch_duration_ms = 0;
            m_hold_event_fired = false;
        }
    }

    return gesture_detected;
}

int16_t TouchGestureEngine::getMovementThreshold() const {
    return static_cast<int16_t>(m_screen_max_dim * MOVEMENT_THRESHOLD_PERCENT);
}

int16_t TouchGestureEngine::getSwipeDistanceThreshold() const {
    // Deprecated: Use axis-specific version instead
    return static_cast<int16_t>(m_screen_max_dim * SWIPE_DISTANCE_PERCENT);
}

int16_t TouchGestureEngine::getEdgeSwipeDistanceThreshold() const {
    // Deprecated: Use axis-specific version instead
    return static_cast<int16_t>(m_screen_max_dim * EDGE_SWIPE_DISTANCE_PERCENT);
}

int16_t TouchGestureEngine::getSwipeDistanceThreshold(int16_t dx, int16_t dy) const {
    // Axis-aware threshold: use width for horizontal, height for vertical
    // This prevents aspect ratio distortion on non-square screens
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal swipe - use screen width
        return static_cast<int16_t>(m_screen_width * SWIPE_DISTANCE_PERCENT);
    } else {
        // Vertical swipe - use screen height
        return static_cast<int16_t>(m_screen_height * SWIPE_DISTANCE_PERCENT);
    }
}

int16_t TouchGestureEngine::getEdgeSwipeDistanceThreshold(int16_t dx, int16_t dy) const {
    // Axis-aware threshold: use width for horizontal, height for vertical
    // Edge drags require MORE movement than center swipes (higher percentage)
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal edge drag - use screen width
        return static_cast<int16_t>(m_screen_width * EDGE_SWIPE_DISTANCE_PERCENT);
    } else {
        // Vertical edge drag - use screen height
        return static_cast<int16_t>(m_screen_height * EDGE_SWIPE_DISTANCE_PERCENT);
    }
}

int16_t TouchGestureEngine::getEdgeThreshold() const {
    return static_cast<int16_t>(m_screen_max_dim * EDGE_THRESHOLD_PERCENT);
}

bool TouchGestureEngine::isNearEdge(int16_t x, int16_t y, touch_direction_t* edge_dir) const {
    // Use custom edge zones if configured by HAL, otherwise use percentage-based defaults
    bool near_left, near_right, near_top, near_bottom;

    if (m_use_custom_edge_zones) {
        // Board-specific thresholds (configured by HAL for limited touch panel ranges)
        near_left = (x < m_edge_left_threshold);
        near_right = (x > m_edge_right_threshold);
        near_top = (y < m_edge_top_threshold);
        near_bottom = (y > m_edge_bottom_threshold);
    } else {
        // Default percentage-based thresholds
        int16_t edge_threshold_x = static_cast<int16_t>(m_screen_width * EDGE_THRESHOLD_PERCENT);
        int16_t edge_threshold_y = static_cast<int16_t>(m_screen_height * EDGE_THRESHOLD_PERCENT);

        int16_t dist_left = x;
        int16_t dist_right = m_screen_width - 1 - x;
        int16_t dist_top = y;
        int16_t dist_bottom = m_screen_height - 1 - y;

        near_left = (dist_left < edge_threshold_x);
        near_right = (dist_right < edge_threshold_x);
        near_top = (dist_top < edge_threshold_y);
        near_bottom = (dist_bottom < edge_threshold_y);
    }

    // Calculate distances for closest-edge selection
    int16_t dist_left = x;
    int16_t dist_right = m_screen_width - 1 - x;
    int16_t dist_top = y;
    int16_t dist_bottom = m_screen_height - 1 - y;

    // If not near any edge, return false
    if (!near_left && !near_right && !near_top && !near_bottom) {
        *edge_dir = TOUCH_DIR_NONE;
        return false;
    }

    // Find the CLOSEST edge
    int16_t min_dist = m_screen_width + m_screen_height;  // Larger than any possible distance
    touch_direction_t closest_edge = TOUCH_DIR_NONE;

    if (near_left && dist_left < min_dist) {
        min_dist = dist_left;
        closest_edge = TOUCH_DIR_LEFT;
    }
    if (near_right && dist_right < min_dist) {
        min_dist = dist_right;
        closest_edge = TOUCH_DIR_RIGHT;
    }
    if (near_top && dist_top < min_dist) {
        min_dist = dist_top;
        closest_edge = TOUCH_DIR_UP;
    }
    if (near_bottom && dist_bottom < min_dist) {
        min_dist = dist_bottom;
        closest_edge = TOUCH_DIR_DOWN;
    }

    *edge_dir = closest_edge;
    return true;
}

touch_direction_t TouchGestureEngine::getSwipeDirection(int16_t dx, int16_t dy) const {
    // Determine primary axis
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal swipe
        return (dx > 0) ? TOUCH_DIR_RIGHT : TOUCH_DIR_LEFT;
    } else {
        // Vertical swipe
        return (dy > 0) ? TOUCH_DIR_DOWN : TOUCH_DIR_UP;
    }
}

void TouchGestureEngine::fillEventData(touch_gesture_event_t* event,
                                        touch_gesture_type_t type,
                                        int16_t x, int16_t y,
                                        touch_direction_t dir) {
    event->type = type;
    event->direction = dir;
    event->x_px = x;
    event->y_px = y;
    event->x_percent = static_cast<float>(x) / static_cast<float>(m_screen_width);
    event->y_percent = static_cast<float>(y) / static_cast<float>(m_screen_height);

    // Clamp percentages to 0.0-1.0
    if (event->x_percent < 0.0f) event->x_percent = 0.0f;
    if (event->x_percent > 1.0f) event->x_percent = 1.0f;
    if (event->y_percent < 0.0f) event->y_percent = 0.0f;
    if (event->y_percent > 1.0f) event->y_percent = 1.0f;
}
