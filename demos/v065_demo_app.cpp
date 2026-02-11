/**
 * @file v065_demo_app.cpp
 * @brief Release 0.65 Demo Application Implementation
 */

#include "v065_demo_app.h"
#include <Arduino.h>
#include "../hal/touch.h"
#include "../hal/display.h"

V065DemoApp::V065DemoApp()
    : m_display(nullptr),
      m_v060Demo(nullptr),
      m_gestureEngine(nullptr),
      m_touchOverlay(nullptr),
      m_lastTouchPressed(false)
{
}

V065DemoApp::~V065DemoApp() {
    delete m_touchOverlay;
    delete m_gestureEngine;
    delete m_v060Demo;
}

bool V065DemoApp::begin(RelativeDisplay* display) {
    m_display = display;

    // Initialize v0.60 demo with v0.65 version text
    m_v060Demo = new V060DemoApp("DEMO v0.65");
    if (!m_v060Demo->begin(display)) {
        Serial.println("[V065DemoApp] Failed to initialize V060DemoApp");
        return false;
    }

    // Initialize touch gesture engine
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();

    m_gestureEngine = new TouchGestureEngine(
        static_cast<int16_t>(screen_width),
        static_cast<int16_t>(screen_height)
    );

    // Apply board-specific touch configuration from HAL
    hal_touch_configure_gesture_engine(m_gestureEngine);

    // Initialize touch test overlay
    m_touchOverlay = new TouchTestOverlay();
    if (!m_touchOverlay->begin()) {
        Serial.println("[V065DemoApp] Failed to initialize TouchTestOverlay");
        return false;
    }

    Serial.println("[V065DemoApp] Initialized successfully");
    return true;
}

void V065DemoApp::update(float deltaTime) {
    // Update v0.60 demo
    m_v060Demo->update(deltaTime);

    // Read touch input
    hal_touch_point_t touch_point;
    bool touch_read_success = hal_touch_read(&touch_point);

    if (touch_read_success) {
        // Convert delta time to milliseconds for gesture engine
        uint32_t delta_time_ms = static_cast<uint32_t>(deltaTime * 1000.0f);

        // Update gesture engine
        touch_gesture_event_t gesture_event;
        bool gesture_detected = m_gestureEngine->update(
            touch_point.x,
            touch_point.y,
            touch_point.is_pressed,
            delta_time_ms,
            &gesture_event
        );

        // If gesture detected, apply direction rotation and update overlay
        if (gesture_detected) {
            // CRITICAL: When display is rotated 90° CW, directions are also rotated 90° CW
            // We must rotate directions 90° CCW to map screen coords → physical device coords
            // This applies to BOTH swipe directions AND edge names
            #if defined(APP_DISPLAY_ROTATION)  // T-Display S3 AMOLED Plus with 90° rotation
                if (gesture_event.direction != TOUCH_DIR_NONE) {
                    // Rotate direction 90° CCW (or equivalently, 270° CW)
                    // Screen → Physical mapping:
                    //   LEFT (screen) → TOP (physical)
                    //   DOWN (screen) → LEFT (physical)
                    //   RIGHT (screen) → BOTTOM (physical)
                    //   UP (screen) → RIGHT (physical)
                    switch (gesture_event.direction) {
                        case TOUCH_DIR_UP:    gesture_event.direction = TOUCH_DIR_RIGHT; break;
                        case TOUCH_DIR_RIGHT: gesture_event.direction = TOUCH_DIR_DOWN;  break;
                        case TOUCH_DIR_DOWN:  gesture_event.direction = TOUCH_DIR_LEFT;  break;
                        case TOUCH_DIR_LEFT:  gesture_event.direction = TOUCH_DIR_UP;    break;
                        default: break;
                    }
                }
            #endif

            m_touchOverlay->update(gesture_event);

            // Debug output with screen dimension context
            const char* gesture_names[] = {"NONE", "TAP", "HOLD", "HOLD_DRAG", "SWIPE", "EDGE_DRAG"};
            const char* swipe_dir_names[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT"};
            const char* edge_names[] = {"NONE", "TOP", "BOTTOM", "LEFT", "RIGHT"};

            Serial.printf("[Touch] %s", gesture_names[gesture_event.type]);
            if (gesture_event.direction != TOUCH_DIR_NONE) {
                // Use different labels for edge drags (TOP/BOTTOM) vs swipes (UP/DOWN)
                const char* dir_label = (gesture_event.type == TOUCH_EDGE_DRAG)
                    ? edge_names[gesture_event.direction]
                    : swipe_dir_names[gesture_event.direction];
                Serial.printf(": %s", dir_label);
            }
            Serial.printf(" at (%d, %d) = (%.1f%%, %.1f%%) [Screen: 536w x 240h]\n",
                          gesture_event.x_px, gesture_event.y_px,
                          gesture_event.x_percent * 100.0f,
                          gesture_event.y_percent * 100.0f);

            // Edge debug: show which edges are close
            if (gesture_event.type == TOUCH_EDGE_DRAG) {
                int16_t start_x, start_y;
                m_gestureEngine->getStartPosition(&start_x, &start_y);
                Serial.printf("  Edge zones (board-specific): LEFT(x<80) RIGHT(x>215) TOP(y<60) BOTTOM(y>215)\n");
                Serial.printf("  Started at: (%d, %d) → %s edge (ended at %d, %d)\n",
                              start_x, start_y,
                              edge_names[gesture_event.direction],
                              gesture_event.x_px, gesture_event.y_px);
            }
        }

        // Debug: Track touch state changes for gesture engine diagnostics
        static bool last_pressed = false;
        static int16_t press_start_x = 0, press_start_y = 0;

        if (touch_point.is_pressed != last_pressed) {
            if (touch_point.is_pressed) {
                press_start_x = touch_point.x;
                press_start_y = touch_point.y;

                // Debug: Show which edge zone (if any) the press started in
                // NOTE: Touch panel has limited range (x: 18-227, y: 25-237)
                // Board-specific thresholds (see setEdgeZones above)
                const char* zone = "CENTER";
                if (touch_point.x < 80) zone = "LEFT";        // Catches x=18
                else if (touch_point.x > 215) zone = "RIGHT";  // Harder to trigger
                else if (touch_point.y < 60) zone = "TOP";     // Catches y=25, y=31
                else if (touch_point.y > 215) zone = "BOTTOM"; // Harder to trigger

                Serial.printf("[Touch] PRESS at (%d, %d) in %s zone\n",
                              touch_point.x, touch_point.y, zone);
            } else {
                Serial.printf("[Touch] RELEASE\n");
            }
            last_pressed = touch_point.is_pressed;
        }

        // Debug: Show deltas for swipe/edge drag gestures
        if (gesture_detected && (gesture_event.type == TOUCH_SWIPE || gesture_event.type == TOUCH_EDGE_DRAG)) {
            int16_t dx = gesture_event.x_px - press_start_x;
            int16_t dy = gesture_event.y_px - press_start_y;
            Serial.printf("  [Delta] START(%d,%d) → END(%d,%d) = dx=%d, dy=%d\n",
                          press_start_x, press_start_y,
                          gesture_event.x_px, gesture_event.y_px,
                          dx, dy);
        }

        m_lastTouchPressed = touch_point.is_pressed;
    }

    // Update overlay timeout
    uint32_t delta_time_ms = static_cast<uint32_t>(deltaTime * 1000.0f);
    m_touchOverlay->tick(delta_time_ms);
}

void V065DemoApp::render() {
    // Render v0.60 demo (logo, connectivity, stock graph)
    m_v060Demo->render();

    // Mark overlay for re-blit if it's visible and graph may have re-rendered
    // (this ensures overlay stays on top after graph updates)
    m_touchOverlay->markForReblit();

    // Render touch overlay on top
    m_touchOverlay->render();
}
