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

        // If gesture detected, update overlay
        if (gesture_detected) {
            m_touchOverlay->update(gesture_event);

            // Debug output
            const char* gesture_names[] = {"NONE", "TAP", "HOLD", "HOLD_DRAG", "SWIPE", "EDGE_DRAG"};
            const char* dir_names[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT"};
            Serial.printf("[Touch] %s", gesture_names[gesture_event.type]);
            if (gesture_event.direction != TOUCH_DIR_NONE) {
                Serial.printf(": %s", dir_names[gesture_event.direction]);
            }
            Serial.printf(" at (%d, %d) = (%.1f%%, %.1f%%)\n",
                          gesture_event.x_px, gesture_event.y_px,
                          gesture_event.x_percent * 100.0f,
                          gesture_event.y_percent * 100.0f);
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

    // Render touch overlay on top
    m_touchOverlay->render();
}
