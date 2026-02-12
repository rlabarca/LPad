/**
 * @file v070_demo_app.cpp
 * @brief Release 0.70 Demo Application Implementation
 *
 * Adapter components wrap existing V060DemoApp and SystemMenu as UIComponents
 * managed by the UIRenderManager. Demonstrates Z-order compositing,
 * activation events, occlusion, and systemPause lifecycle.
 */

#include "v070_demo_app.h"
#include <Arduino.h>
#include "v060_demo_app.h"
#include "../hal/touch.h"
#include "../hal/display.h"
#include "../hal/network.h"
#include "../src/theme_manager.h"
#include "../src/ui/ui_render_manager.h"
#include "../src/ui/ui_system_menu.h"

// ============================================================================
// Adapter: StockTickerApp — wraps V060DemoApp as an AppComponent (Z=1)
// ============================================================================
class StockTickerApp : public AppComponent {
public:
    StockTickerApp(V060DemoApp* inner) : m_inner(inner) {}

    void onUnpause() override {
        // Graph was obscured by menu — force full redraw
        m_inner->requestFullRedraw();
    }

    void render() override {
        m_inner->render();
    }

    void updateInner(float dt) {
        m_inner->update(dt);
    }

    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }

private:
    V060DemoApp* m_inner;
};

// ============================================================================
// Adapter: SystemMenuAdapter — wraps SystemMenu as a SystemComponent (Z=20)
// ============================================================================
class SystemMenuAdapter : public SystemComponent {
public:
    SystemMenuAdapter(SystemMenu* inner) : m_inner(inner), m_closing(false) {}

    void onUnpause() override {
        m_inner->open();
        m_inner->setSSID(hal_network_get_ssid());
        m_closing = false;
        Serial.println("[RenderMgr] SystemMenu: ACTIVATED via EDGE_DRAG TOP");
    }

    void onPause() override {
        // Called by hide() after close animation completes via systemPause()
    }

    void updateInner(float dt) {
        m_inner->update(dt);

        // Detect close-animation completion → yield control back to manager
        if (m_closing && m_inner->getState() == SystemMenu::CLOSED) {
            m_closing = false;
            Serial.println("[RenderMgr] SystemMenu: CLOSED, calling systemPause()");
            systemPause();
        }
    }

    void render() override {
        m_inner->render();
    }

    bool handleInput(const touch_gesture_event_t& event) override {
        // Close gesture: EDGE_DRAG from BOTTOM edge while menu is open
        if (event.type == TOUCH_EDGE_DRAG &&
            event.direction == TOUCH_DIR_DOWN &&
            m_inner->getState() == SystemMenu::OPEN) {
            m_inner->close();
            m_closing = true;
            Serial.println("[RenderMgr] SystemMenu: CLOSING via EDGE_DRAG BOTTOM");
            return true;
        }
        // Consume all other input while menu is visible
        return true;
    }

    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }

private:
    SystemMenu* m_inner;
    bool m_closing;
};

// ============================================================================
// V070DemoApp
// ============================================================================

V070DemoApp::V070DemoApp()
    : m_display(nullptr)
    , m_v060Demo(nullptr)
    , m_systemMenu(nullptr)
    , m_gestureEngine(nullptr)
    , m_stockTickerAdapter(nullptr)
    , m_systemMenuAdapter(nullptr)
{
}

V070DemoApp::~V070DemoApp() {
    UIRenderManager::getInstance().reset();
    delete m_systemMenuAdapter;
    delete m_stockTickerAdapter;
    delete m_systemMenu;
    delete m_gestureEngine;
    delete m_v060Demo;
}

bool V070DemoApp::begin(RelativeDisplay* display) {
    m_display = display;

    // --- Create inner components (same as v0.67) ---

    // Stock Tracker (V060DemoApp with no version overlay)
    m_v060Demo = new V060DemoApp(nullptr);
    m_v060Demo->setWatermark("^TNX");
    if (!m_v060Demo->begin(display)) {
        Serial.println("[V070DemoApp] Failed to initialize V060DemoApp");
        return false;
    }

    // Touch gesture engine
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();
    m_gestureEngine = new TouchGestureEngine(
        static_cast<int16_t>(screen_width),
        static_cast<int16_t>(screen_height)
    );
    hal_touch_configure_gesture_engine(m_gestureEngine);

    // System Menu
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());

    m_systemMenu = new SystemMenu();
    if (!m_systemMenu->begin(gfx, screen_width, screen_height)) {
        Serial.println("[V070DemoApp] Failed to initialize SystemMenu");
        return false;
    }
    m_systemMenu->setVersion("Version 0.70");
    m_systemMenu->setSSID(hal_network_get_ssid());
    m_systemMenu->setBackgroundColor(theme->colors.system_menu_bg);
    m_systemMenu->setRevealColor(theme->colors.background);
    m_systemMenu->setVersionFont(theme->fonts.smallest);
    m_systemMenu->setVersionColor(theme->colors.graph_ticks);
    m_systemMenu->setSSIDFont(theme->fonts.normal);
    m_systemMenu->setSSIDColor(theme->colors.text_main);

    // --- Create adapters and register with UIRenderManager ---

    auto& mgr = UIRenderManager::getInstance();
    mgr.reset(); // Clean slate

    // Stock Ticker at Z=1
    m_stockTickerAdapter = new StockTickerApp(m_v060Demo);
    if (!mgr.registerComponent(m_stockTickerAdapter, 1)) {
        Serial.println("[V070DemoApp] Failed to register StockTicker (Z=1)");
        return false;
    }

    // System Menu at Z=20 with EDGE_DRAG TOP activation
    m_systemMenuAdapter = new SystemMenuAdapter(m_systemMenu);
    static_cast<SystemMenuAdapter*>(m_systemMenuAdapter)->setActivationEvent(
        TOUCH_EDGE_DRAG, TOUCH_DIR_UP
    );
    static_cast<SystemMenuAdapter*>(m_systemMenuAdapter)->hide(); // Start hidden
    if (!mgr.registerComponent(m_systemMenuAdapter, 20)) {
        Serial.println("[V070DemoApp] Failed to register SystemMenu (Z=20)");
        return false;
    }

    // Set StockTicker as the active app
    mgr.setActiveApp(static_cast<AppComponent*>(m_stockTickerAdapter));

    Serial.println("[V070DemoApp] UIRenderManager initialized:");
    Serial.printf("  Components: %d\n", mgr.getComponentCount());
    Serial.println("  Z=1:  StockTicker (App)");
    Serial.println("  Z=20: SystemMenu  (System, activation=EDGE_DRAG TOP)");
    return true;
}

void V070DemoApp::update(float deltaTime) {
    // --- Read touch and route through UIRenderManager ---
    hal_touch_point_t touch_point;
    bool touch_read_success = hal_touch_read(&touch_point);

    if (touch_read_success) {
        touch_gesture_event_t gesture_event;
        bool gesture_detected = false;

        if (touch_point.is_home_button) {
            gesture_event.type = TOUCH_EDGE_DRAG;
            gesture_event.direction = TOUCH_DIR_DOWN;
            gesture_event.x_px = static_cast<int16_t>(hal_display_get_width_pixels() / 2);
            gesture_event.y_px = static_cast<int16_t>(hal_display_get_height_pixels() - 1);
            gesture_event.x_percent = 0.5f;
            gesture_event.y_percent = 1.0f;
            gesture_detected = true;
        } else {
            uint32_t delta_time_ms = static_cast<uint32_t>(deltaTime * 1000.0f);
            gesture_detected = m_gestureEngine->update(
                touch_point.x, touch_point.y,
                touch_point.is_pressed, delta_time_ms,
                &gesture_event
            );
        }

        if (gesture_detected) {
            // UIRenderManager handles activation events and input dispatch
            UIRenderManager::getInstance().routeInput(gesture_event);
        }
    }

    // --- Update inner components ---
    // System menu animation (always, even during close)
    static_cast<SystemMenuAdapter*>(m_systemMenuAdapter)->updateInner(deltaTime);

    // Stock ticker (skip if paused by manager)
    if (!m_stockTickerAdapter->isPaused()) {
        static_cast<StockTickerApp*>(m_stockTickerAdapter)->updateInner(deltaTime);
    }
}

void V070DemoApp::render() {
    // UIRenderManager handles Z-order, occlusion, and visibility
    UIRenderManager::getInstance().renderAll();
}
