/**
 * @file test_stock_ticker_app.cpp
 * @brief Unit tests for StockTickerApp lifecycle and rendering logic
 *
 * Covers all automated Gherkin scenarios from features/app_stock_ticker.md:
 * - Begin creates tracker and graph
 * - Status screen shown when waiting with no data
 * - Non-trading hours always shown
 * - Graph renders when data available
 * - Pause stops tracker before WiFi teardown
 * - Unpause forces full redraw
 */

#include <unity.h>
#include "apps/stock_ticker_app.h"
#include "data/stock_tracker.h"
#include "data/data_item_time_series.h"
#include "relative_display.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

// ---------------------------------------------------------------------------
// Mock GFX
// ---------------------------------------------------------------------------

class MockGFX : public Arduino_GFX {
public:
    MockGFX() : Arduino_GFX(320, 170) {}
    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
        (void)x; (void)y; (void)color;
    }
};

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

static MockGFX* g_gfx = nullptr;
static RelativeDisplay* g_display = nullptr;

void setUp(void) {
    hal_display_init();
    g_gfx = new MockGFX();
    g_display = new RelativeDisplay(g_gfx, 320, 170);
    g_display->init();
}

void tearDown(void) {
    delete g_display;
    delete g_gfx;
    g_display = nullptr;
    g_gfx = nullptr;
}

// ---------------------------------------------------------------------------
// Scenario: Begin creates tracker and graph
// ---------------------------------------------------------------------------

void test_begin_creates_tracker_and_graph(void) {
    StockTickerApp app;
    bool ok = app.begin(g_display);
    TEST_ASSERT_TRUE(ok);

    StockTracker* tracker = app.getTrackerForTest();
    TEST_ASSERT_NOT_NULL(tracker);
    TEST_ASSERT_EQUAL_STRING("^TNX", tracker->getSymbol().c_str());
}

// ---------------------------------------------------------------------------
// Scenario: Status screen shown when waiting with no data
// ---------------------------------------------------------------------------

void test_status_screen_shown_when_waiting_with_no_data(void) {
    StockTickerApp app;
    app.begin(g_display);

    // Default FetchStatus is WAITING; data series is empty
    app.render();

    TEST_ASSERT_EQUAL(static_cast<int>(FetchStatus::WAITING),
                      app.getLastRenderedFetchStatusForTest());
}

// ---------------------------------------------------------------------------
// Scenario: Non-trading hours always shown
// ---------------------------------------------------------------------------

void test_non_trading_hours_always_shown(void) {
    StockTickerApp app;
    app.begin(g_display);

    // Inject NON_TRADING_HOURS status and pre-existing data
    StockTracker* tracker = app.getTrackerForTest();
    tracker->setFetchStatusForTest(FetchStatus::NON_TRADING_HOURS);
    tracker->getDataSeries()->addDataPoint(1000L, 4.27);

    app.render();

    // NON_TRADING_HOURS is always shown, overriding existing data
    TEST_ASSERT_EQUAL(static_cast<int>(FetchStatus::NON_TRADING_HOURS),
                      app.getLastRenderedFetchStatusForTest());
}

// ---------------------------------------------------------------------------
// Scenario: Graph renders when data available
// ---------------------------------------------------------------------------

void test_graph_renders_when_data_available(void) {
    StockTickerApp app;
    app.begin(g_display);

    // Inject HAS_DATA status with data points
    StockTracker* tracker = app.getTrackerForTest();
    tracker->setFetchStatusForTest(FetchStatus::HAS_DATA);
    tracker->getDataSeries()->addDataPoint(1000L, 4.27);
    tracker->getDataSeries()->addDataPoint(1060L, 4.28);

    app.render();

    TEST_ASSERT_EQUAL(static_cast<int>(FetchStatus::HAS_DATA),
                      app.getLastRenderedFetchStatusForTest());
    TEST_ASSERT_EQUAL(1060L, app.getLastDataTimestampForTest());
}

// ---------------------------------------------------------------------------
// Scenario: Pause stops tracker before WiFi teardown
// ---------------------------------------------------------------------------

void test_pause_stops_tracker_before_wifi_teardown(void) {
    StockTickerApp app;
    app.begin(g_display);
    app.onRun();

    StockTracker* tracker = app.getTrackerForTest();
    TEST_ASSERT_TRUE(tracker->isRunning());

    app.onPause();
    TEST_ASSERT_FALSE(tracker->isRunning());
}

// ---------------------------------------------------------------------------
// Scenario: Unpause forces full redraw
// ---------------------------------------------------------------------------

void test_unpause_forces_full_redraw(void) {
    StockTickerApp app;
    app.begin(g_display);
    app.onRun();

    // Simulate a render so m_backgroundDrawn becomes true
    app.render();
    TEST_ASSERT_TRUE(app.isBackgroundDrawnForTest());

    app.onPause();
    app.onUnpause();

    // All render flags should be reset after unpause
    TEST_ASSERT_FALSE(app.isBackgroundDrawnForTest());
    TEST_ASSERT_EQUAL(0L, app.getLastDataTimestampForTest());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_begin_creates_tracker_and_graph);
    RUN_TEST(test_status_screen_shown_when_waiting_with_no_data);
    RUN_TEST(test_non_trading_hours_always_shown);
    RUN_TEST(test_graph_renders_when_data_available);
    RUN_TEST(test_pause_stops_tracker_before_wifi_teardown);
    RUN_TEST(test_unpause_forces_full_redraw);

    return UNITY_END();
}
