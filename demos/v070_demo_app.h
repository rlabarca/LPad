/**
 * @file v070_demo_app.h
 * @brief Release 0.70 Demo Application Class
 *
 * First demo driven by UIRenderManager. Wraps existing V060DemoApp (Stock
 * Tracker) and SystemMenu as managed UIComponents with Z-order compositing,
 * activation-event routing, and occlusion.
 *
 * See features/RELEASE_v0.70_ui_render_manager.md for specification.
 */

#ifndef V070_DEMO_APP_H
#define V070_DEMO_APP_H

class RelativeDisplay;
class V060DemoApp;
class SystemMenu;
class TouchGestureEngine;
class AppComponent;
class SystemComponent;

class V070DemoApp {
public:
    V070DemoApp();
    ~V070DemoApp();

    bool begin(RelativeDisplay* display);
    void update(float deltaTime);
    void render();

private:
    RelativeDisplay* m_display;

    // Inner components (owned)
    V060DemoApp* m_v060Demo;
    SystemMenu* m_systemMenu;
    TouchGestureEngine* m_gestureEngine;

    // UIComponent adapters (owned, registered with UIRenderManager)
    AppComponent* m_stockTickerAdapter;
    SystemComponent* m_systemMenuAdapter;
};

#endif // V070_DEMO_APP_H
