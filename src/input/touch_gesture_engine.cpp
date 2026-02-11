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
      m_hold_event_fired(false)
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
                    int16_t swipe_threshold = getSwipeDistanceThreshold();

                    if (std::abs(dx) >= swipe_threshold || std::abs(dy) >= swipe_threshold) {
                        // Check if started from edge
                        touch_direction_t edge_dir;
                        if (isNearEdge(m_start_x, m_start_y, &edge_dir)) {
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
    return static_cast<int16_t>(m_screen_max_dim * SWIPE_DISTANCE_PERCENT);
}

int16_t TouchGestureEngine::getEdgeThreshold() const {
    return static_cast<int16_t>(m_screen_max_dim * EDGE_THRESHOLD_PERCENT);
}

bool TouchGestureEngine::isNearEdge(int16_t x, int16_t y, touch_direction_t* edge_dir) const {
    int16_t edge_threshold_x = static_cast<int16_t>(m_screen_width * EDGE_THRESHOLD_PERCENT);
    int16_t edge_threshold_y = static_cast<int16_t>(m_screen_height * EDGE_THRESHOLD_PERCENT);

    // Check each edge (prioritize by proximity)
    if (x < edge_threshold_x) {
        *edge_dir = TOUCH_DIR_LEFT;
        return true;
    }
    if (x > m_screen_width - edge_threshold_x) {
        *edge_dir = TOUCH_DIR_RIGHT;
        return true;
    }
    if (y < edge_threshold_y) {
        *edge_dir = TOUCH_DIR_UP;
        return true;
    }
    if (y > m_screen_height - edge_threshold_y) {
        *edge_dir = TOUCH_DIR_DOWN;
        return true;
    }

    *edge_dir = TOUCH_DIR_NONE;
    return false;
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
