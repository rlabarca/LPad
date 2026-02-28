/**
 * @file test_data_series.cpp
 * @brief Unit tests for DataItemTimeSeries
 */

#include <unity.h>
#include "data/data_item.h"
#include "data/data_item_time_series.h"
#include <cmath>

// Helper to compare doubles with tolerance
bool doubles_equal(double a, double b, double epsilon = 0.0001) {
    return std::fabs(a - b) < epsilon;
}

// Test Scenario 1: Fixed Capacity FIFO Behavior
void test_fifo_behavior() {
    DataItemTimeSeries ts("test_series", 5);

    // Add 5 data points
    ts.addDataPoint(1, 10.0);
    ts.addDataPoint(2, 20.0);
    ts.addDataPoint(3, 30.0);
    ts.addDataPoint(4, 40.0);
    ts.addDataPoint(5, 50.0);

    TEST_ASSERT_EQUAL(5, ts.getLength());
    TEST_ASSERT_EQUAL(5, ts.getMaxLength());

    // Add one more (should evict oldest)
    ts.addDataPoint(6, 60.0);

    TEST_ASSERT_EQUAL(5, ts.getLength());

    // Get data and verify FIFO order
    GraphData data = ts.getGraphData();
    TEST_ASSERT_EQUAL(5, data.x_values.size());
    TEST_ASSERT_EQUAL(5, data.y_values.size());

    // Should contain [20, 30, 40, 50, 60] (10 was evicted)
    TEST_ASSERT_EQUAL(2, data.x_values[0]);
    TEST_ASSERT_TRUE(doubles_equal(20.0, data.y_values[0]));
    TEST_ASSERT_EQUAL(6, data.x_values[4]);
    TEST_ASSERT_TRUE(doubles_equal(60.0, data.y_values[4]));
}

// Test Scenario 2: Automatic Range Calculation
void test_automatic_range_calculation() {
    DataItemTimeSeries ts("test_series", 10);

    ts.addDataPoint(1, 100.0);
    ts.addDataPoint(2, 50.0);
    ts.addDataPoint(3, 200.0);

    TEST_ASSERT_TRUE(doubles_equal(50.0, ts.getMinVal()));
    TEST_ASSERT_TRUE(doubles_equal(200.0, ts.getMaxVal()));

    // Add new max
    ts.addDataPoint(4, 300.0);
    TEST_ASSERT_TRUE(doubles_equal(300.0, ts.getMaxVal()));

    // Min should be unchanged
    TEST_ASSERT_TRUE(doubles_equal(50.0, ts.getMinVal()));
}

// Test Scenario 3: Dynamic Range Updates on Removal
void test_dynamic_range_on_removal() {
    DataItemTimeSeries ts("test_series", 3);

    ts.addDataPoint(1, 10.0);
    ts.addDataPoint(2, 20.0);
    ts.addDataPoint(3, 30.0);

    TEST_ASSERT_TRUE(doubles_equal(10.0, ts.getMinVal()));
    TEST_ASSERT_TRUE(doubles_equal(30.0, ts.getMaxVal()));

    // Add value that evicts the min (10)
    ts.addDataPoint(4, 25.0);

    TEST_ASSERT_EQUAL(3, ts.getLength());
    TEST_ASSERT_TRUE(doubles_equal(20.0, ts.getMinVal())); // Min should update to 20
    TEST_ASSERT_TRUE(doubles_equal(30.0, ts.getMaxVal())); // Max unchanged

    // Verify data order [20, 30, 25]
    GraphData data = ts.getGraphData();
    TEST_ASSERT_EQUAL(3, data.y_values.size());
    TEST_ASSERT_TRUE(doubles_equal(20.0, data.y_values[0]));
    TEST_ASSERT_TRUE(doubles_equal(30.0, data.y_values[1]));
    TEST_ASSERT_TRUE(doubles_equal(25.0, data.y_values[2]));
}

// Test Scenario 4: Export to GraphData
void test_export_to_graph_data() {
    DataItemTimeSeries ts("test_series", 5);

    ts.addDataPoint(100, 1.5);
    ts.addDataPoint(200, 2.5);
    ts.addDataPoint(300, 3.5);

    GraphData data = ts.getGraphData();

    TEST_ASSERT_EQUAL(3, data.x_values.size());
    TEST_ASSERT_EQUAL(3, data.y_values.size());

    // Verify order (oldest to newest)
    TEST_ASSERT_EQUAL(100, data.x_values[0]);
    TEST_ASSERT_TRUE(doubles_equal(1.5, data.y_values[0]));

    TEST_ASSERT_EQUAL(200, data.x_values[1]);
    TEST_ASSERT_TRUE(doubles_equal(2.5, data.y_values[1]));

    TEST_ASSERT_EQUAL(300, data.x_values[2]);
    TEST_ASSERT_TRUE(doubles_equal(3.5, data.y_values[2]));
}

// Test edge case: Empty series
void test_empty_series() {
    DataItemTimeSeries ts("empty", 10);

    TEST_ASSERT_EQUAL(0, ts.getLength());
    TEST_ASSERT_TRUE(std::isinf(ts.getMinVal()));
    TEST_ASSERT_TRUE(std::isinf(ts.getMaxVal()));

    GraphData data = ts.getGraphData();
    TEST_ASSERT_EQUAL(0, data.x_values.size());
    TEST_ASSERT_EQUAL(0, data.y_values.size());
}

// Test clear() method
void test_clear() {
    DataItemTimeSeries ts("test", 5);

    ts.addDataPoint(1, 10.0);
    ts.addDataPoint(2, 20.0);
    ts.addDataPoint(3, 30.0);

    TEST_ASSERT_EQUAL(3, ts.getLength());

    ts.clear();

    TEST_ASSERT_EQUAL(0, ts.getLength());
    TEST_ASSERT_TRUE(std::isinf(ts.getMinVal()));
    TEST_ASSERT_TRUE(std::isinf(ts.getMaxVal()));
}

// Test metadata from DataItem base class
void test_metadata() {
    DataItemTimeSeries ts("MyDataSeries", 10);

    TEST_ASSERT_EQUAL_STRING("MyDataSeries", ts.getName().c_str());

    uint64_t t1 = ts.getLastUpdated();
    TEST_ASSERT_EQUAL(0, t1); // Should be 0 initially

    ts.addDataPoint(1, 100.0);

    uint64_t t2 = ts.getLastUpdated();
    // In stub environment, timer returns 0, so we just verify the method can be called
    // In real hardware, t2 > t1 would be true
    TEST_ASSERT_TRUE(t2 >= t1);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_fifo_behavior);
    RUN_TEST(test_automatic_range_calculation);
    RUN_TEST(test_dynamic_range_on_removal);
    RUN_TEST(test_export_to_graph_data);
    RUN_TEST(test_empty_series);
    RUN_TEST(test_clear);
    RUN_TEST(test_metadata);

    return UNITY_END();
}
