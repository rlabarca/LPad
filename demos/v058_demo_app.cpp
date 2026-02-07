/**
 * @file v058_demo_app.cpp
 * @brief Release 0.58 Demo Application Implementation
 */

#include "v058_demo_app.h"
#include "../test_data/test_data_tnx_5m.h"
#include "../hal/display.h"
#include "../src/theme_manager.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <cstdlib>

V058DemoApp::V058DemoApp()
    : m_v055Demo(nullptr)
    , m_liveData(nullptr)
    , m_display(nullptr)
    , m_dataUpdateTimer(0.0f)
    , m_initialYMin(0.0)
    , m_initialYMax(0.0)
{
}

V058DemoApp::~V058DemoApp() {
    delete m_v055Demo;
    delete m_liveData;
}

bool V058DemoApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[V058DemoApp] ERROR: display is nullptr");
        return false;
    }

    m_display = display;

    // Create V055DemoApp (handles connectivity + visual demo)
    m_v055Demo = new V055DemoApp();

    // Set title to "DEMO v0.58" before initialization
    m_v055Demo->setTitle("DEMO v0.58");

    if (!m_v055Demo->begin(display)) {
        Serial.println("[V058DemoApp] ERROR: V055DemoApp initialization failed");
        return false;
    }

    Serial.println("[V058DemoApp] V055DemoApp configured with title 'DEMO v0.58'");

    // Create DataItemTimeSeries with capacity matching initial dataset
    // This creates a sliding window: as new data comes in, oldest data falls out
    m_liveData = new DataItemTimeSeries("TNX_5m_live", TestData::TNX_5M_COUNT);

    // Load initial data
    if (!loadInitialData()) {
        Serial.println("[V058DemoApp] ERROR: Failed to load initial data");
        return false;
    }

    // Capture initial Y-axis bounds from test data (for stable random generation)
    m_initialYMin = m_liveData->getMinVal();
    m_initialYMax = m_liveData->getMaxVal();

    Serial.println("[V058DemoApp] Initialized successfully");
    Serial.printf("[V058DemoApp] Live data initialized with %zu points\n", m_liveData->getLength());
    Serial.printf("[V058DemoApp] Fixed Y-range: [%.4f, %.4f]\n", m_initialYMin, m_initialYMax);

    // Initialize graph with live data (will update when graph is created by V05DemoApp)
    updateGraphWithLiveData();

    return true;
}

void V058DemoApp::update(float deltaTime) {
    // Update live data injection timer
    m_dataUpdateTimer += deltaTime;

    if (m_dataUpdateTimer >= DATA_UPDATE_INTERVAL) {
        m_dataUpdateTimer = 0.0f;
        injectNewDataPoint();
    }

    // Update V055DemoApp (handles all phase logic)
    if (m_v055Demo != nullptr) {
        m_v055Demo->update(deltaTime);
    }
}

void V058DemoApp::render() {
    // Render V055DemoApp
    if (m_v055Demo != nullptr) {
        m_v055Demo->render();
    }

    // CRITICAL: Blit title every frame to prevent disappearance
    // The graph's live indicator uses partial DMA blits every frame, which could
    // potentially overlap/overwrite the title area. Blitting every frame ensures
    // the title remains visible even if something accidentally overwrites it.
    // Using blitTitle() instead of drawTitle() for minimal latency (DMA vs font rendering).
    V05DemoApp* v05Demo = m_v055Demo->getV05DemoApp();
    if (v05Demo != nullptr && v05Demo->isShowingGraph()) {
        v05Demo->blitTitle();
    }
}

bool V058DemoApp::loadInitialData() {
    // Load embedded test data from header
    for (size_t i = 0; i < TestData::TNX_5M_COUNT; i++) {
        m_liveData->addDataPoint(
            TestData::TNX_5M_TIMESTAMPS[i],
            TestData::TNX_5M_CLOSE_PRICES[i]
        );
    }

    return true;
}

void V058DemoApp::injectNewDataPoint() {
    if (m_liveData->getLength() == 0) {
        return; // No data to base random values on
    }

    // Generate random data point within FIXED initial Y-axis bounds
    // This prevents drift toward zero over time
    double yRange = m_initialYMax - m_initialYMin;

    // Random value within 80% of initial range (centered, stable over time)
    double yRand = m_initialYMin + (yRange * 0.1) + (static_cast<double>(rand()) / RAND_MAX) * (yRange * 0.8);

    // Get last X value and increment by 300 (5 minutes in seconds, matching Yahoo data granularity)
    GraphData currentData = m_liveData->getGraphData();
    long lastX = currentData.x_values.back();
    long newX = lastX + 300;

    // Add new data point
    m_liveData->addDataPoint(newX, yRand);

    Serial.printf("[V058DemoApp] Injected new data point: x=%ld, y=%.4f\n", newX, yRand);
    Serial.printf("[V058DemoApp] Data series now has %zu points (min=%.4f, max=%.4f)\n",
                  m_liveData->getLength(), m_liveData->getMinVal(), m_liveData->getMaxVal());

    // Update TimeSeriesGraph with new live data
    updateGraphWithLiveData();
}

void V058DemoApp::updateGraphWithLiveData() {
    // Get the V05DemoApp
    if (m_v055Demo == nullptr) {
        return;  // Not initialized yet
    }

    V05DemoApp* v05Demo = m_v055Demo->getV05DemoApp();
    if (v05Demo == nullptr) {
        return;  // V05DemoApp not initialized yet
    }

    // Only update graph when actively showing graphs (not logo, not connectivity)
    // This prevents graph from overwriting other screens
    if (!m_v055Demo->isInVisualPhase() || !v05Demo->isShowingGraph()) {
        return;  // Not showing graph yet (still in connectivity, handover, or logo phase)
    }

    TimeSeriesGraph* graph = v05Demo->getGraph();
    if (graph == nullptr) {
        return;  // Graph not initialized yet
    }

    // Get current live data snapshot
    GraphData liveGraphData = m_liveData->getGraphData();

    // Update graph data canvas
    graph->setData(liveGraphData);
    graph->drawData();   // Redraw data canvas with new data

    // Composite and blit to display via DMA (full screen, overwrites everything including title)
    graph->render();

    // IMMEDIATELY blit pre-rendered title with transparency
    // Minimize gap between graph render and title appearing
    v05Demo->blitTitle();

    Serial.printf("[V058DemoApp] Graph updated with live data (%zu points)\n", liveGraphData.x_values.size());
}
