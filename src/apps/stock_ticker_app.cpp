/**
 * @file stock_ticker_app.cpp
 * @brief Standalone Stock Ticker Application Implementation
 *
 * Extracted from V060DemoApp PHASE_STOCK_GRAPH logic.
 * Directly manages StockTracker + TimeSeriesGraph without demo wrappers.
 */

#include "stock_ticker_app.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "../ui_time_series_graph.h"
#include "../data/stock_tracker.h"
#include "../theme_manager.h"
#include "../relative_display.h"
#include "../../hal/display.h"

StockTickerApp::StockTickerApp()
    : m_display(nullptr)
    , m_graph(nullptr)
    , m_stockTracker(nullptr)
    , m_backgroundDrawn(false)
    , m_graphInitialRenderDone(false)
    , m_lastDataTimestamp(0)
{
}

StockTickerApp::~StockTickerApp() {
    onClose();
}

bool StockTickerApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[StockTickerApp] ERROR: display is nullptr");
        return false;
    }
    m_display = display;

    // Create graph with themed styling
    GraphTheme theme = createStockGraphTheme();
    Arduino_GFX* gfx = m_display->getGfx();
    if (gfx == nullptr) {
        Serial.println("[StockTickerApp] ERROR: GFX is nullptr");
        return false;
    }

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    m_graph = new TimeSeriesGraph(theme, gfx, width, height);
    if (!m_graph->begin()) {
        Serial.println("[StockTickerApp] ERROR: TimeSeriesGraph init failed");
        delete m_graph;
        m_graph = nullptr;
        return false;
    }

    m_graph->setTickLabelPosition(TickLabelPosition::INSIDE);
    m_graph->setYAxisTitle("Value");
    m_graph->setXAxisTitle("Hours Prior");
    m_graph->setYTicks(0.002f);
    m_graph->setWatermark("^TNX");

    // Create stock tracker (60s refresh, 30min history)
    m_stockTracker = new StockTracker("^TNX", 60, 30);

    Serial.println("[StockTickerApp] Initialized (graph + tracker created)");
    return true;
}

void StockTickerApp::onRun() {
    if (m_stockTracker != nullptr && !m_stockTracker->isRunning()) {
        if (!m_stockTracker->start()) {
            Serial.println("[StockTickerApp] ERROR: Failed to start StockTracker");
        } else {
            Serial.println("[StockTickerApp] StockTracker started");
        }
    }
}

void StockTickerApp::onUnpause() {
    // Graph was obscured — force full redraw
    m_backgroundDrawn = false;
    m_lastDataTimestamp = 0;
    m_graphInitialRenderDone = false;
}

void StockTickerApp::onClose() {
    if (m_stockTracker != nullptr) {
        m_stockTracker->stop();
        delete m_stockTracker;
        m_stockTracker = nullptr;
    }
    if (m_graph != nullptr) {
        delete m_graph;
        m_graph = nullptr;
    }
}

void StockTickerApp::render() {
    if (m_graph == nullptr || m_stockTracker == nullptr) return;

    DataItemTimeSeries* dataSeries = m_stockTracker->getDataSeries();
    if (dataSeries == nullptr || dataSeries->getLength() == 0) return;

    // Check if data has been updated since last render
    GraphData graphData = dataSeries->getGraphData();
    long currentTimestamp = graphData.x_values.empty() ? 0 : graphData.x_values.back();

    if (currentTimestamp != m_lastDataTimestamp) {
        m_graph->setData(graphData);

        if (!m_backgroundDrawn) {
            m_graph->drawBackground();
            m_backgroundDrawn = true;
        }

        m_graph->drawData();
        m_lastDataTimestamp = currentTimestamp;

        // Composite graph layers to GFX buffer (NO flush — manager handles that)
        m_graph->render();
        m_graphInitialRenderDone = true;
    }
}

void StockTickerApp::update(float dt) {
    // Live indicator dirty-rect animation
    if (m_graph != nullptr && m_graphInitialRenderDone) {
        m_graph->update(dt);
    }
}

bool StockTickerApp::handleInput(const touch_gesture_event_t& event) {
    (void)event;
    return false; // All input bubbles up (edge drags go to SystemMenu)
}

GraphTheme StockTickerApp::createStockGraphTheme() {
    GraphTheme theme = {};
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();

    theme.backgroundColor = lpadTheme->colors.background;
    theme.useBackgroundGradient = false;

    theme.lineColor = lpadTheme->colors.text_main;
    theme.useLineGradient = false;

    theme.axisColor = lpadTheme->colors.secondary;
    theme.lineThickness = 0.97f;
    theme.axisThickness = 0.8f;
    theme.tickColor = lpadTheme->colors.graph_ticks;
    theme.tickLength = 5.0f;

    theme.liveIndicatorGradient.color_stops[0] = lpadTheme->colors.accent;
    theme.liveIndicatorGradient.color_stops[1] = lpadTheme->colors.accent;
    theme.liveIndicatorPulseSpeed = 0.5f;

    theme.tickFont = lpadTheme->fonts.smallest;
    theme.axisTitleFont = lpadTheme->fonts.ui;

    theme.watermarkColor = lpadTheme->colors.graph_ticks;

    return theme;
}
