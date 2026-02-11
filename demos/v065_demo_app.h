/**
 * @file v065_demo_app.h
 * @brief Release 0.65 Demo Application Class
 *
 * Wraps V060DemoApp and adds touch interaction with debug overlay.
 * See features/RELEASE_v0.65_touch_interaction.md for specification.
 */

#ifndef V065_DEMO_APP_H
#define V065_DEMO_APP_H

#include "v060_demo_app.h"
#include "input/touch_gesture_engine.h"
#include "ui/ui_touch_test_overlay.h"

/**
 * @class V065DemoApp
 * @brief Orchestrates the Release 0.65 demo flow with touch interaction.
 *
 * Flow:
 * - All v0.60 functionality (Logo -> WiFi -> Stock Tracker)
 * - Touch gesture detection and visual debug overlay
 * - Title updated to "DEMO v0.65"
 */
class V065DemoApp {
public:
    V065DemoApp();
    ~V065DemoApp();

    /**
     * @brief Initializes all components and starts the demo flow.
     * @param display RelativeDisplay instance for rendering
     * @return true on success, false on failure
     */
    bool begin(RelativeDisplay* display);

    /**
     * @brief Updates the current phase and component states.
     * @param deltaTime Time elapsed since last frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the current phase to the display.
     */
    void render();

private:
    RelativeDisplay* m_display;

    // Wrapped v0.60 demo
    V060DemoApp* m_v060Demo;

    // Touch components
    TouchGestureEngine* m_gestureEngine;
    TouchTestOverlay* m_touchOverlay;

    // Last touch state (for tracking press/release)
    bool m_lastTouchPressed;
};

#endif // V065_DEMO_APP_H
