/**
 * @file v058_demo_app.cpp
 * @brief Release 0.58 Demo Application Implementation
 */

#include "v058_demo_app.h"
#include "../test_data/test_data_tnx_5m.h"
#include "../hal/display.h"
#include <Arduino.h>
#include <cstdlib>

V058DemoApp::V058DemoApp()
    : m_v055Demo(nullptr)
    , m_liveData(nullptr)
    , m_display(nullptr)
    , m_dataUpdateTimer(0.0f)
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
    if (!m_v055Demo->begin(display)) {
        Serial.println("[V058DemoApp] ERROR: V055DemoApp initialization failed");
        return false;
    }

    // Override title to show "DEMO v0.58"
    V05DemoApp* v05Demo = m_v055Demo->getV05DemoApp();
    if (v05Demo != nullptr) {
        v05Demo->setTitle("DEMO v0.58");
        Serial.println("[V058DemoApp] Title set to 'DEMO v0.58'");
    }

    // Create DataItemTimeSeries with capacity for initial dataset
    // Yahoo chart has 15 data points, allocate 50 for growth
    m_liveData = new DataItemTimeSeries("TNX_5m_live", 50);

    // Load initial data from test file
    if (!loadInitialData()) {
        Serial.println("[V058DemoApp] ERROR: Failed to load initial data");
        return false;
    }

    Serial.println("[V058DemoApp] Initialized successfully");
    Serial.printf("[V058DemoApp] Live data initialized with %zu points\n", m_liveData->getLength());
    Serial.printf("[V058DemoApp] Y-range: [%.4f, %.4f]\n", m_liveData->getMinVal(), m_liveData->getMaxVal());

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

    // Generate random data point within current Y-axis bounds
    double yMin = m_liveData->getMinVal();
    double yMax = m_liveData->getMaxVal();
    double yRange = yMax - yMin;

    // Random value within 80% of current range (to keep it interesting but bounded)
    double yRand = yMin + (yRange * 0.1) + (static_cast<double>(rand()) / RAND_MAX) * (yRange * 0.8);

    // Get last X value and increment by 300 (5 minutes in seconds, matching Yahoo data granularity)
    GraphData currentData = m_liveData->getGraphData();
    long lastX = currentData.x_values.back();
    long newX = lastX + 300;

    // Add new data point
    m_liveData->addDataPoint(newX, yRand);

    Serial.printf("[V058DemoApp] Injected new data point: x=%ld, y=%.4f\n", newX, yRand);
    Serial.printf("[V058DemoApp] Data series now has %zu points (min=%.4f, max=%.4f)\n",
                  m_liveData->getLength(), m_liveData->getMinVal(), m_liveData->getMaxVal());

    // TODO: Update TimeSeriesGraph with new data
    // This will be connected in a future iteration when we modify V05DemoApp
    // to accept external data sources instead of loading from file
}
