/**
 * @file v060_demo_app.h
 * @brief Release 0.60 Demo Application Class
 *
 * Demonstrates stock tracking with ^TNX using Yahoo Finance API.
 * See features/demo_release_0.60.md for specification.
 */

#ifndef V060_DEMO_APP_H
#define V060_DEMO_APP_H

#include "relative_display.h"
#include "ui_logo_screen.h"
#include "ui_connectivity_status_screen.h"
#include "ui_time_series_graph.h"
#include "ui_mini_logo.h"
#include "data/stock_tracker.h"
#include "../hal/network.h"

/**
 * @class V060DemoApp
 * @brief Orchestrates the Release 0.60 demo flow.
 *
 * Flow:
 * - Phase 1: Logo animation (transitions to mini logo in top-right corner)
 * - Phase 2: WiFi connectivity check (mini logo remains visible)
 * - Phase 3: Stock tracker graph (^TNX with live updates from Yahoo Finance)
 */
class V060DemoApp {
public:
    V060DemoApp();
    ~V060DemoApp();

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
    enum Phase {
        PHASE_LOGO,             // Logo animation
        PHASE_CONNECTIVITY,     // Wi-Fi connection and ping test
        PHASE_HANDOVER,         // Hold "PING OK" for 2 seconds
        PHASE_STOCK_GRAPH       // Display ^TNX stock graph with live updates
    };

    Phase m_currentPhase;
    RelativeDisplay* m_display;

    // UI Components
    LogoScreen* m_logoScreen;
    ConnectivityStatusScreen* m_connectivityScreen;
    TimeSeriesGraph* m_graph;
    MiniLogo* m_miniLogo;

    // Data Layer
    StockTracker* m_stockTracker;

    // Title buffer for "DEMO v0.60" text
    uint16_t* m_titleBuffer;
    int16_t m_titleBufferX;
    int16_t m_titleBufferY;
    int16_t m_titleBufferWidth;
    int16_t m_titleBufferHeight;
    bool m_titleBufferValid;

    // State tracking
    bool m_logoAnimationComplete;
    bool m_pingResult;
    float m_logoHoldTimer;
    float m_handoverTimer;

    // Timing constants
    static constexpr float LOGO_HOLD_DURATION = 2.0f;
    static constexpr float HANDOVER_DURATION = 2.0f;

    // Helper methods
    void transitionToPhase(Phase newPhase);
    void renderTitleToBuffer();
    void blitTitle();
    GraphTheme createStockGraphTheme();
};

#endif // V060_DEMO_APP_H
