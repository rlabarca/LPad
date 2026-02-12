/**
 * @file v067_demo_app.cpp
 * @brief Release 0.67 Demo Application Implementation
 */

#include "v067_demo_app.h"
#include <Arduino.h>
#include "../hal/touch.h"
#include "../hal/display.h"
#include "../hal/network.h"
#include "../src/theme_manager.h"

V067DemoApp::V067DemoApp()
    : m_display(nullptr)
    , m_v060Demo(nullptr)
    , m_gestureEngine(nullptr)
    , m_systemMenu(nullptr)
    , m_lastTouchPressed(false)
    , m_menuWasActive(false)
{
}

V067DemoApp::~V067DemoApp() {
    delete m_systemMenu;
    delete m_gestureEngine;
    delete m_v060Demo;
}

bool V067DemoApp::begin(RelativeDisplay* display) {
    m_display = display;

    // Initialize v0.60 demo with NO version text (removed per v0.67 spec)
    m_v060Demo = new V060DemoApp(nullptr);
    m_v060Demo->setWatermark("^TNX");
    if (!m_v060Demo->begin(display)) {
        Serial.println("[V067DemoApp] Failed to initialize V060DemoApp");
        return false;
    }

    // Initialize touch gesture engine
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();

    m_gestureEngine = new TouchGestureEngine(
        static_cast<int16_t>(screen_width),
        static_cast<int16_t>(screen_height)
    );
    hal_touch_configure_gesture_engine(m_gestureEngine);

    // Initialize System Menu
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());

    m_systemMenu = new SystemMenu();
    if (!m_systemMenu->begin(gfx, screen_width, screen_height)) {
        Serial.println("[V067DemoApp] Failed to initialize SystemMenu");
        return false;
    }

    m_systemMenu->setVersion("Version 0.67");
    m_systemMenu->setSSID(hal_network_get_ssid());
    m_systemMenu->setBackgroundColor(theme->colors.system_menu_bg);
    m_systemMenu->setRevealColor(theme->colors.background);
    m_systemMenu->setVersionFont(theme->fonts.smallest);
    m_systemMenu->setVersionColor(theme->colors.graph_ticks);
    m_systemMenu->setSSIDFont(theme->fonts.normal);
    m_systemMenu->setSSIDColor(theme->colors.text_main);

    Serial.println("[V067DemoApp] Initialized successfully");
    return true;
}

void V067DemoApp::update(float deltaTime) {
    // Read touch input
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
            // Route EDGE_DRAG gestures to system menu
            // Direction reports the EDGE (UP=TOP edge, DOWN=BOTTOM edge)
            if (gesture_event.type == TOUCH_EDGE_DRAG) {
                if (gesture_event.direction == TOUCH_DIR_UP && !m_systemMenu->isActive()) {
                    // EDGE_DRAG from TOP edge -> open menu
                    m_systemMenu->open();
                    // Update SSID on each open (may have changed)
                    m_systemMenu->setSSID(hal_network_get_ssid());
                    Serial.println("[V067DemoApp] System Menu: OPENING");
                } else if (gesture_event.direction == TOUCH_DIR_DOWN &&
                           m_systemMenu->getState() == SystemMenu::OPEN) {
                    // EDGE_DRAG from BOTTOM edge -> close menu
                    m_systemMenu->close();
                    Serial.println("[V067DemoApp] System Menu: CLOSING");
                }
            }
        }

        m_lastTouchPressed = touch_point.is_pressed;
    }

    // Update system menu animation
    m_systemMenu->update(deltaTime);

    // Suppress v0.60 updates while menu is active (per spec §4)
    if (!m_systemMenu->isActive()) {
        m_v060Demo->update(deltaTime);
    }
}

void V067DemoApp::render() {
    bool menuActive = m_systemMenu->isActive();

    if (menuActive) {
        // System menu has exclusive display access
        m_systemMenu->render();
    } else {
        // Detect menu just closed → force full graph redraw
        if (m_menuWasActive) {
            m_v060Demo->requestFullRedraw();
        }
        // Normal graph rendering
        m_v060Demo->render();
    }

    m_menuWasActive = menuActive;
}
