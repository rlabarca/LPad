/**
 * @file v055_demo_app.h
 * @brief Release 0.55 Demo Application Class
 *
 * Extends v0.5 demo by prepending a connectivity check.
 * See features/demo_release_0.55.md for specification.
 */

#ifndef V055_DEMO_APP_H
#define V055_DEMO_APP_H

#include "v05_demo_app.h"
#include "ui_connectivity_status_screen.h"
#include "../hal/network.h"

/**
 * @class V055DemoApp
 * @brief Coordinates the transition between connectivity validation and visual demo.
 *
 * Phases:
 * - Phase 1: Connectivity check (Wi-Fi connection and ping test)
 * - Phase 2: Hold "PING OK" for 2 seconds
 * - Phase 3: Run V05DemoApp
 * - Return to Phase 1 after completion
 */
class V055DemoApp {
public:
    V055DemoApp();
    ~V055DemoApp();

    /**
     * @brief Initializes connectivity screen and V05DemoApp.
     * @param display RelativeDisplay instance for rendering
     * @return true on success, false on failure
     */
    bool begin(RelativeDisplay* display);

    /**
     * @brief Updates the current phase (connectivity or visual demo).
     * @param deltaTime Time elapsed since last frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the current phase to the display.
     */
    void render();

    /**
     * @brief Gets the internal V05DemoApp instance.
     * @return Pointer to V05DemoApp (nullptr if not initialized)
     */
    V05DemoApp* getV05DemoApp() { return m_v05Demo; }

private:
    enum Phase {
        PHASE_CONNECTIVITY,    // Wi-Fi connection and ping test
        PHASE_HANDOVER,        // Hold "PING OK" for 2 seconds
        PHASE_VISUAL_DEMO      // Run V05DemoApp
    };

    Phase m_currentPhase;
    RelativeDisplay* m_display;
    V05DemoApp* m_v05Demo;
    ConnectivityStatusScreen* m_connectivityScreen;

    bool m_pingResult;
    float m_handoverTimer;

    static constexpr float HANDOVER_DURATION = 2.0f;  // Hold "PING OK" for 2s

    // Helper methods
    void transitionToPhase(Phase newPhase);
};

#endif // V055_DEMO_APP_H
