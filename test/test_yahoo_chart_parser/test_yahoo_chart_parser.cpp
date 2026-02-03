/**
 * @file test_yahoo_chart_parser.cpp
 * @brief Unity tests for Yahoo Chart Data Parser
 *
 * These tests verify the behavior specified in features/data_yahoo_chart_parser.md
 */

#include <unity.h>
#include "../../src/yahoo_chart_parser.h"

void setUp(void) {
    // Set up runs before each test
}

void tearDown(void) {
    // Tear down runs after each test
}

/**
 * Test: Parse valid chart data file
 * Scenario from features/data_yahoo_chart_parser.md
 */
void test_parse_valid_chart_data(void) {
    YahooChartParser parser("test_data/yahoo_chart_tnx_5m_1d.json");

    bool success = parser.parse();
    TEST_ASSERT_TRUE(success);

    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();

    // Must have equal number of timestamps and close prices
    TEST_ASSERT_EQUAL(timestamps.size(), closePrices.size());

    // Must have at least some data
    TEST_ASSERT_GREATER_THAN(0, timestamps.size());

    // First timestamp should be 1770057900
    TEST_ASSERT_EQUAL(1770057900, timestamps[0]);

    // First closing price should be approximately 4.271
    TEST_ASSERT_FLOAT_WITHIN(0.001, 4.271, closePrices[0]);
}

/**
 * Test: Handle missing file
 * Should handle error gracefully without crashing
 */
void test_parse_missing_file(void) {
    YahooChartParser parser("nonexistent_file.json");

    bool success = parser.parse();
    TEST_ASSERT_FALSE(success);

    // Should return empty data structures
    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();

    TEST_ASSERT_EQUAL(0, timestamps.size());
    TEST_ASSERT_EQUAL(0, closePrices.size());
}

/**
 * Test: Handle invalid JSON
 * Should handle error gracefully without crashing
 */
void test_parse_invalid_json(void) {
    // Create a temporary invalid JSON file
    FILE* fp = fopen("test_invalid.json", "w");
    if (fp) {
        fprintf(fp, "{invalid json content");
        fclose(fp);
    }

    YahooChartParser parser("test_invalid.json");

    bool success = parser.parse();
    TEST_ASSERT_FALSE(success);

    // Should return empty data structures
    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();

    TEST_ASSERT_EQUAL(0, timestamps.size());
    TEST_ASSERT_EQUAL(0, closePrices.size());

    // Clean up
    remove("test_invalid.json");
}

/**
 * Test: Verify all data points are extracted
 */
void test_parse_extracts_all_data_points(void) {
    YahooChartParser parser("test_data/yahoo_chart_tnx_5m_1d.json");

    bool success = parser.parse();
    TEST_ASSERT_TRUE(success);

    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();

    // The test file has 15 data points
    TEST_ASSERT_EQUAL(15, timestamps.size());
    TEST_ASSERT_EQUAL(15, closePrices.size());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_valid_chart_data);
    RUN_TEST(test_parse_missing_file);
    RUN_TEST(test_parse_invalid_json);
    RUN_TEST(test_parse_extracts_all_data_points);

    UNITY_END();

    return 0;
}
