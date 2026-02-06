/**
 * @file v05_demo_app.h
 * @brief Release 0.5 Demo Application Class
 *
 * Encapsulates the v0.5 demo logic for reusability across releases.
 * See features/demo_release_0.5.md for specification.
 */

#ifndef V05_DEMO_APP_H
#define V05_DEMO_APP_H

#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "ui_logo_screen.h"
#include "animation_ticker.h"
#include "yahoo_chart_parser.h"

/**
 * @class V05DemoApp
 * @brief Manages the lifecycle and orchestration of the v0.5 visual components.
 *
 * Handles three stages:
 * - Stage 0: Logo Animation (wait 2s + animate + hold 2s)
 * - Stage 1: Scientific Mode Graph (5 seconds, OUTSIDE labels, axis titles)
 * - Stage 2: Compact Mode Graph (5 seconds, INSIDE labels, no titles)
 */
class V05DemoApp {
public:
    V05DemoApp();
    ~V05DemoApp();

    /**
     * @brief Initializes all UI components and loads test data.
     * @param display RelativeDisplay instance for rendering
     * @return true on success, false on failure
     */
    bool begin(RelativeDisplay* display);

    /**
     * @brief Advances the internal state machine and updates animations.
     * @param deltaTime Time elapsed since last frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the current stage to the display.
     */
    void render();

    /**
     * @brief Checks if the full demo cycle has completed once.
     * @return true if all stages have been shown, false otherwise
     */
    bool isFinished() const;

    /**
     * @brief Sets a custom version string for the demo title.
     * @param title The title text to display (e.g., "DEMO v0.5" or "DEMO v0.55")
     */
    void setTitle(const char* title);

private:
    enum Stage {
        STAGE_LOGO,         // Logo animation (wait + animate + hold)
        STAGE_GRAPH_CYCLE,  // Cycle through 6 graph modes
        STAGE_FINISHED      // Cycle complete
    };

    Stage m_currentStage;
    RelativeDisplay* m_display;
    TimeSeriesGraph* m_graph;
    LogoScreen* m_logoScreen;
    GraphData m_graphData;

    // Title configuration
    const char* m_titleText;

    // Mode cycling (6 combinations: 2 layouts x 3 visual styles)
    int m_currentMode;       // 0-5: cycles through 6 combinations
    int m_modesShown;        // Track how many modes shown in current cycle

    // Timing
    float m_logoHoldTimer;
    float m_modeTimer;

    static constexpr float LOGO_HOLD_DURATION = 2.0f;  // Hold final logo position for 2s
    static constexpr float MODE_DURATION = 5.0f;       // Each graph mode for 5s

    // Helper methods
    void drawTitle();
    void transitionToStage(Stage newStage);
    void switchToNextMode();
    GraphTheme createGradientTheme();
    GraphTheme createSolidTheme();
    GraphTheme createMixedTheme();
};

#endif // V05_DEMO_APP_H
