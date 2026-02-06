/**
 * @file v055_demo_app.cpp
 * @brief Release 0.55 Demo Application Implementation
 */

#include "v055_demo_app.h"
#include <Arduino.h>

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

    // Create ConnectivityStatusScreen
    m_connectivityScreen = new ConnectivityStatusScreen();
    if (!m_connectivityScreen->begin(m_display)) {
        Serial.println("[V055DemoApp] ERROR: ConnectivityStatusScreen initialization failed");
        return false;
    }

    // Create V05DemoApp (but don't start it yet)
    m_v05Demo = new V05DemoApp();
    if (!m_v05Demo->begin(m_display)) {
        Serial.println("[V055DemoApp] ERROR: V05DemoApp initialization failed");
        return false;
    }

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
            // Reset V05DemoApp by recreating it
            delete m_v05Demo;
            m_v05Demo = new V05DemoApp();
            if (!m_v05Demo->begin(m_display)) {
                Serial.println("[V055DemoApp] ERROR: V05DemoApp re-initialization failed");
            }
            break;
    }
}
