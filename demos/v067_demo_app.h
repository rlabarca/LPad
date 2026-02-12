/**
 * @file v067_demo_app.h
 * @brief Release 0.67 Demo Application Class
 *
 * Wraps V060DemoApp, removes touch overlay and DEMO title,
 * adds System Menu and ticker watermark.
 * See features/RELEASE_v0.67_system_menu.md for specification.
 */

#ifndef V067_DEMO_APP_H
#define V067_DEMO_APP_H

#include "v060_demo_app.h"
#include "input/touch_gesture_engine.h"
#include "ui/ui_system_menu.h"

class V067DemoApp {
public:
    V067DemoApp();
    ~V067DemoApp();

    bool begin(RelativeDisplay* display);
    void update(float deltaTime);
    void render();

private:
    RelativeDisplay* m_display;

    // Wrapped v0.60 demo (no version text overlay)
    V060DemoApp* m_v060Demo;

    // Touch (gesture detection only, no overlay)
    TouchGestureEngine* m_gestureEngine;

    // System Menu
    SystemMenu* m_systemMenu;

    // Track last touch state
    bool m_lastTouchPressed;

    // Track menu state transitions for graph redraw
    bool m_menuWasActive;
};

#endif // V067_DEMO_APP_H
