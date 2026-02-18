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

StockTickerApp::StockTickerApp()
    : m_display(nullptr)
    , m_graph(nullptr)
    , m_stockTracker(nullptr)
    , m_backgroundDrawn(false)
    , m_graphInitialRenderDone(false)
    , m_lastDataTimestamp(0)
    , m_lastRenderedFetchStatus(-1)
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

    int32_t width = m_display->getWidth();
    int32_t height = m_display->getHeight();

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
    m_lastRenderedFetchStatus = -1;

    // Wake the stock tracker task from its sleep delay so it retries
    // immediately instead of waiting up to 60s from a pre-suspend delay.
    if (m_stockTracker) {
        m_stockTracker->notifyResume();
    }
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

    // Always draw background when needed — even without data.
    // After suspend/resume the display GRAM is undefined (MIPI Sleep In/Out
    // does not preserve framebuffer contents). Without this, the screen shows
    // stale pixels until data arrives.
    if (!m_backgroundDrawn) {
        m_graph->drawBackground();
        m_backgroundDrawn = true;
        m_graph->render();
    }

    // Status screens (§4 scenarios): show centered message when no data.
    // Per spec, "Data Error" persists "until data becomes available again."
    // If the data series already has points (e.g., from before suspend), the
    // data IS available — fall through to normal rendering instead of showing
    // the error. NON_TRADING_HOURS is always shown since it's informational.
    FetchStatus fetchStatus = m_stockTracker->getFetchStatus();
    DataItemTimeSeries* dataSeries = m_stockTracker->getDataSeries();
    bool hasExistingData = (dataSeries != nullptr && dataSeries->getLength() > 0);

    if (fetchStatus == FetchStatus::NON_TRADING_HOURS ||
        (fetchStatus == FetchStatus::FETCH_ERROR && !hasExistingData) ||
        (fetchStatus == FetchStatus::WAITING && !hasExistingData)) {
        // Only redraw when the status changes (avoids redundant writes)
        if (m_lastRenderedFetchStatus != static_cast<int>(fetchStatus)) {
            // Repaint clean background to clear any previous status text
            m_graph->drawBackground();
            m_graph->render();

            const char* msg;
            if (fetchStatus == FetchStatus::NON_TRADING_HOURS)
                msg = "Non Trading Hours";
            else if (fetchStatus == FetchStatus::FETCH_ERROR)
                msg = "Data Error";
            else
                msg = "Retrieving Data";

            Arduino_GFX* gfx = m_display->getGfx();
            const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();
            gfx->setFont(static_cast<const GFXfont*>(lpadTheme->fonts.normal));
            gfx->setTextColor(lpadTheme->colors.text_main);

            int16_t x1, y1;
            uint16_t tw, th;
            gfx->getTextBounds(msg, 0, 0, &x1, &y1, &tw, &th);
            int16_t cx = (m_display->getWidth() - tw) / 2 - x1;
            int16_t cy = (m_display->getHeight() - th) / 2 - y1;
            gfx->setCursor(cx, cy);
            gfx->print(msg);

            m_lastRenderedFetchStatus = static_cast<int>(fetchStatus);
        }
        return;
    }

    // Normal data rendering path
    if (dataSeries == nullptr || dataSeries->getLength() == 0) return;

    // Check if data has been updated since last render
    GraphData graphData = dataSeries->getGraphData();
    long currentTimestamp = graphData.x_values.empty() ? 0 : graphData.x_values.back();

    if (currentTimestamp != m_lastDataTimestamp) {
        m_graph->setData(graphData);
        m_graph->drawBackground();
        m_graph->drawData();
        m_lastDataTimestamp = currentTimestamp;

        // Composite graph layers to GFX buffer (NO flush — manager handles that)
        m_graph->render();
        m_graphInitialRenderDone = true;
        m_lastRenderedFetchStatus = static_cast<int>(FetchStatus::HAS_DATA);
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
    theme.tickColor = lpadTheme->colors.graph_ticks;
    theme.tickLength = 5.0f;

    theme.liveIndicatorGradient.color_stops[0] = lpadTheme->colors.accent;
    theme.liveIndicatorGradient.color_stops[1] = lpadTheme->colors.accent;
    theme.liveIndicatorPulseSpeed = 0.5f;

    theme.watermarkColor = lpadTheme->colors.graph_ticks;

    return theme;
}
