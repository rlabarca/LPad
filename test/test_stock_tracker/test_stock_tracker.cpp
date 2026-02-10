#include <unity.h>
#include "../../src/data/stock_tracker.h"
#include <string>

// Sample Yahoo Finance JSON response (simplified for testing)
const char* SAMPLE_YAHOO_RESPONSE = R"({
    "chart": {
        "result": [
            {
                "meta": {
                    "symbol": "^TNX"
                },
                "timestamp": [1609459200, 1609459500, 1609459800, 1609460100, 1609460400],
                "indicators": {
                    "quote": [
                        {
                            "close": [4.27, 4.28, 4.29, 4.30, 4.31]
                        }
                    ]
                }
            }
        ]
    }
})";

// Test: StockTracker can be instantiated
void test_stock_tracker_instantiation() {
    StockTracker tracker("^TNX", 60, 30);
    TEST_ASSERT_EQUAL_STRING("^TNX", tracker.getSymbol().c_str());
    TEST_ASSERT_FALSE(tracker.isRunning());
}

// Test: DataSeries is accessible
void test_stock_tracker_data_series() {
    StockTracker tracker("^TNX", 60, 30);
    DataItemTimeSeries* series = tracker.getDataSeries();
    TEST_ASSERT_NOT_NULL(series);
    TEST_ASSERT_EQUAL_STRING("^TNX", series->getName().c_str());
}

// Test: Start/stop behavior on native platform
void test_stock_tracker_start_stop() {
    StockTracker tracker("^TNX", 60, 30);

    // Start tracker
    bool started = tracker.start();
#ifdef ARDUINO
    // On hardware, task should start
    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_TRUE(tracker.isRunning());
#else
    // On native, flag should be set but no task
    TEST_ASSERT_TRUE(started);
    TEST_ASSERT_TRUE(tracker.isRunning());
#endif

    // Stop tracker
    tracker.stop();
    TEST_ASSERT_FALSE(tracker.isRunning());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_stock_tracker_instantiation);
    RUN_TEST(test_stock_tracker_data_series);
    RUN_TEST(test_stock_tracker_start_stop);

    return UNITY_END();
}
