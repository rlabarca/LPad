/**
 * @file v058_demo_app.h
 * @brief Release 0.58 Demo Application Class
 *
 * Extends v0.55 demo by replacing static data with live DataItemTimeSeries.
 * See features/demo_release_0.58.md for specification.
 */

#ifndef V058_DEMO_APP_H
#define V058_DEMO_APP_H

#include "v055_demo_app.h"
#include "data/data_item_time_series.h"

/**
 * @class V058DemoApp
 * @brief Demonstrates dynamic, self-updating data using DataItemTimeSeries.
 *
 * Flow:
 * - Phase 1: Connectivity check (via V055DemoApp)
 * - Phase 2: Dynamic visual demo (Logo + Graph with live data updates)
 * - Data updates every 1 second with random values
 * - Maintains V0.55 structure (WiFi -> Logo -> Graph Cycle)
 */
class V058DemoApp {
public:
    V058DemoApp();
    ~V058DemoApp();

    /**
     * @brief Initializes V055DemoApp and data layer.
     * @param display RelativeDisplay instance for rendering
     * @return true on success, false on failure
     */
    bool begin(RelativeDisplay* display);

    /**
     * @brief Updates current phase and manages live data injection.
     * @param deltaTime Time elapsed since last frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the current phase to the display.
     */
    void render();

private:
    V055DemoApp* m_v055Demo;
    DataItemTimeSeries* m_liveData;
    RelativeDisplay* m_display;

    float m_dataUpdateTimer;
    static constexpr float DATA_UPDATE_INTERVAL = 3.0f;  // Inject new data every 3 seconds

    // Fixed Y-axis bounds from initial test data (prevents drift to zero)
    double m_initialYMin;
    double m_initialYMax;

    // Helper methods
    bool loadInitialData();
    void injectNewDataPoint();
    void updateGraphWithLiveData();
};

#endif // V058_DEMO_APP_H
