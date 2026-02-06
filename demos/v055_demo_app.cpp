/**
 * @file v055_demo_app.cpp
 * @brief Release 0.55 Demo Application Implementation
 */

#include "v055_demo_app.h"
#include "../hal/display.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

V055DemoApp::V055DemoApp()
    : m_currentPhase(PHASE_CONNECTIVITY)
    , m_display(nullptr)
    , m_v05Demo(nullptr)
    , m_connectivityScreen(nullptr)
    , m_pingResult(false)
    , m_handoverTimer(0.0f)
{
}

V055DemoApp::~V055DemoApp() {
    delete m_v05Demo;
    delete m_connectivityScreen;
}

bool V055DemoApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[V055DemoApp] ERROR: display is nullptr");
        return false;
    }

    m_display = display;

    // Clear display immediately to remove any previous content
    Serial.println("[V055DemoApp] Clearing display at startup...");
    Arduino_GFX* gfx = m_display->getGfx();
    if (gfx != nullptr) {
        gfx->fillScreen(0x0000);  // Clear to black
        hal_display_flush();
    }

    // Create ConnectivityStatusScreen
    m_connectivityScreen = new ConnectivityStatusScreen();
    if (!m_connectivityScreen->begin(m_display)) {
        Serial.println("[V055DemoApp] ERROR: ConnectivityStatusScreen initialization failed");
        return false;
    }

    // DO NOT create V05DemoApp here - it will draw the logo immediately!
    // We'll create it later when transitioning to PHASE_VISUAL_DEMO
    m_v05Demo = nullptr;

    // Start Wi-Fi connection process
    #ifdef LPAD_WIFI_SSID
    if (hal_network_init(LPAD_WIFI_SSID, LPAD_WIFI_PASSWORD)) {
        Serial.printf("[V055DemoApp] Connecting to Wi-Fi: %s\n", LPAD_WIFI_SSID);
    } else {
        Serial.println("[V055DemoApp] Network initialization failed");
    }
    #else
    Serial.println("[V055DemoApp] No Wi-Fi credentials configured (DEMO_MODE)");
    #endif

    Serial.println("[V055DemoApp] Initialized successfully");
    return true;
}

void V055DemoApp::update(float deltaTime) {
    switch (m_currentPhase) {
        case PHASE_CONNECTIVITY: {
            // Check network status
            hal_network_status_t status = hal_network_get_status();

            // Once connected, perform ping test
            if (status == HAL_NETWORK_STATUS_CONNECTED && !m_pingResult) {
                Serial.println("[V055DemoApp] Wi-Fi connected, performing ping test...");
                m_pingResult = hal_network_ping("8.8.8.8");
                if (m_pingResult) {
                    Serial.println("[V055DemoApp] Ping test successful!");
                    transitionToPhase(PHASE_HANDOVER);
                    return;  // Don't update connectivity screen after transition
                } else {
                    Serial.println("[V055DemoApp] Ping test failed");
                }
            }

            // Update connectivity screen
            if (m_connectivityScreen != nullptr) {
                m_connectivityScreen->update(m_pingResult);
            }
            break;
        }

        case PHASE_HANDOVER: {
            // Hold "PING OK" for 2 seconds
            m_handoverTimer += deltaTime;
            if (m_handoverTimer >= HANDOVER_DURATION) {
                transitionToPhase(PHASE_VISUAL_DEMO);
                return;  // Don't update connectivity screen after transition
            }

            // Continue updating connectivity screen to show "PING OK"
            if (m_connectivityScreen != nullptr) {
                m_connectivityScreen->update(m_pingResult);
            }
            break;
        }

        case PHASE_VISUAL_DEMO: {
            // Update V05DemoApp
            if (m_v05Demo != nullptr) {
                m_v05Demo->update(deltaTime);

                // Check if demo finished, restart cycle
                if (m_v05Demo->isFinished()) {
                    Serial.println("[V055DemoApp] V05 demo finished, restarting connectivity check");
                    transitionToPhase(PHASE_CONNECTIVITY);
                }
            }
            break;
        }
    }
}

void V055DemoApp::render() {
    switch (m_currentPhase) {
        case PHASE_CONNECTIVITY:
        case PHASE_HANDOVER:
            // Connectivity screen handles its own rendering via update()
            break;

        case PHASE_VISUAL_DEMO:
            // Render V05DemoApp
            if (m_v05Demo != nullptr) {
                m_v05Demo->render();
            }
            break;
    }
}

void V055DemoApp::transitionToPhase(Phase newPhase) {
    m_currentPhase = newPhase;
    m_handoverTimer = 0.0f;

    switch (newPhase) {
        case PHASE_CONNECTIVITY:
            Serial.println("[V055DemoApp] Transitioning to PHASE_CONNECTIVITY");
            m_pingResult = false;
            // Note: Network init was already called in begin(), no need to re-init
            break;

        case PHASE_HANDOVER:
            Serial.println("[V055DemoApp] Transitioning to PHASE_HANDOVER (holding PING OK)");
            break;

        case PHASE_VISUAL_DEMO:
            Serial.println("[V055DemoApp] Transitioning to PHASE_VISUAL_DEMO");

            // Explicitly clear the actual display (not canvas) to remove connectivity text
            // Use GFX directly to ensure we clear the real screen
            if (m_display != nullptr) {
                Arduino_GFX* gfx = m_display->getGfx();
                if (gfx != nullptr) {
                    Serial.println("[V055DemoApp] Clearing screen with fillScreen(0x0000)...");
                    gfx->fillScreen(0x0000);  // Clear to black
                    Serial.println("[V055DemoApp] Flushing display...");
                    hal_display_flush();
                    Serial.println("[V055DemoApp] Screen cleared and flushed");
                } else {
                    Serial.println("[V055DemoApp] ERROR: gfx is nullptr!");
                }
            } else {
                Serial.println("[V055DemoApp] ERROR: m_display is nullptr!");
            }

            // Reset V05DemoApp by recreating it
            Serial.println("[V055DemoApp] Creating V05DemoApp...");
            delete m_v05Demo;
            m_v05Demo = new V05DemoApp();

            // Configure V05DemoApp to display "DEMO v0.55"
            m_v05Demo->setTitle("DEMO v0.55");

            if (!m_v05Demo->begin(m_display)) {
                Serial.println("[V055DemoApp] ERROR: V05DemoApp re-initialization failed");
            } else {
                Serial.println("[V055DemoApp] V05DemoApp initialized successfully with title 'DEMO v0.55'");
            }
            break;
    }
}
