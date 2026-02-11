/**
 * @file stock_tracker.cpp
 * @brief Implementation of StockTracker
 */

#include "stock_tracker.h"
#include "../../hal/network.h"

#ifdef ARDUINO
    #include <Arduino.h>
    #include <ArduinoJson.h>
#else
    // For native testing
    #include <cstdio>
    #include <cstring>
#endif

// Buffer size for HTTP response (Yahoo Finance responses can be large)
static constexpr size_t HTTP_RESPONSE_BUFFER_SIZE = 32768;  // 32KB

StockTracker::StockTracker(const std::string& symbol,
                          uint32_t refresh_interval_seconds,
                          uint32_t history_minutes)
    : m_symbol(symbol)
    , m_refresh_interval_seconds(refresh_interval_seconds)
    , m_history_minutes(history_minutes)
    , m_data_series(symbol, 1500)  // Capacity for 24h of 1-min data: 1440 points + buffer
    , m_is_running(false)
    , m_is_first_fetch(true)
#ifdef ARDUINO
    , m_task_handle(nullptr)
#endif
{
}

StockTracker::~StockTracker() {
    stop();
}

bool StockTracker::start() {
    if (m_is_running) {
        return false;  // Already running
    }

#ifdef ARDUINO
    // Create FreeRTOS task for background data fetching
    BaseType_t result = xTaskCreate(
        taskFunction,
        "stock_tracker",
        8192,  // Stack size (8KB)
        this,  // Task parameter (this pointer)
        1,     // Priority
        &m_task_handle
    );

    if (result != pdPASS) {
        Serial.println("[StockTracker] Failed to create task");
        return false;
    }

    m_is_running = true;
    Serial.printf("[StockTracker] Started tracking %s\n", m_symbol.c_str());
    return true;
#else
    // On native platform, just set flag (no background task)
    m_is_running = true;
    return true;
#endif
}

void StockTracker::stop() {
    if (!m_is_running) {
        return;
    }

#ifdef ARDUINO
    if (m_task_handle != nullptr) {
        vTaskDelete(m_task_handle);
        m_task_handle = nullptr;
    }
    Serial.println("[StockTracker] Stopped");
#endif

    m_is_running = false;
}

std::string StockTracker::buildApiUrl() const {
    // Yahoo Finance API endpoint
    // interval=1m for 1-minute candles (best granularity)
    // range= based on whether this is initial fetch or update
    std::string url = "https://query1.finance.yahoo.com/v8/finance/chart/";
    url += m_symbol;
    url += "?interval=1m&range=";

    // Initial fetch: Get 24 hours of historical data (range=1d)
    // Subsequent fetches: Get last 1 hour for efficiency (range=1h)
    if (m_is_first_fetch) {
        url += "1d";  // 24 hours for initial dataset
    } else {
        url += "1h";  // 1 hour for incremental updates
    }

    return url;
}

bool StockTracker::fetchData() {
#ifdef ARDUINO
    // Check network status
    hal_network_status_t status = hal_network_get_status();
    if (status != HAL_NETWORK_STATUS_CONNECTED) {
        Serial.println("[StockTracker] Network not connected");
        return false;
    }

    // Allocate buffer for HTTP response (use PSRAM if available)
    char* response_buffer = static_cast<char*>(
#ifdef BOARD_HAS_PSRAM
        ps_malloc(HTTP_RESPONSE_BUFFER_SIZE)
#else
        malloc(HTTP_RESPONSE_BUFFER_SIZE)
#endif
    );

    if (response_buffer == nullptr) {
        Serial.println("[StockTracker] Failed to allocate response buffer");
        return false;
    }

    // Build API URL
    std::string url = buildApiUrl();

    // Make HTTP GET request
    bool success = hal_network_http_get(url.c_str(), response_buffer, HTTP_RESPONSE_BUFFER_SIZE);

    if (!success) {
        Serial.println("[StockTracker] HTTP request failed");
        free(response_buffer);
        return false;
    }

    // Parse JSON response
    std::vector<long> timestamps;
    std::vector<double> prices;

    if (!parseYahooFinanceResponse(response_buffer, timestamps, prices)) {
        Serial.println("[StockTracker] Failed to parse response");
        free(response_buffer);
        return false;
    }

    free(response_buffer);

    // Update data series based on fetch mode
    size_t num_points = timestamps.size();
    if (num_points > 0) {
        if (m_is_first_fetch) {
            // Initial fetch: Clear and populate with full 24-hour dataset
            m_data_series.clear();

            for (size_t i = 0; i < num_points; i++) {
                m_data_series.addDataPoint(timestamps[i], prices[i]);
            }

            Serial.printf("[StockTracker] Initial fetch: Loaded %zu data points (24h history)\n", num_points);
            m_is_first_fetch = false;  // Mark initial fetch as complete
        } else {
            // Incremental update: Append only NEW data points
            long latest_existing_timestamp = 0;

            // Get the latest timestamp currently in the series
            if (m_data_series.getLength() > 0) {
                // Access the last timestamp in the series
                // Note: DataItemTimeSeries doesn't expose direct timestamp access,
                // so we need to export and check the last value
                GraphData temp_data = m_data_series.getGraphData();
                if (!temp_data.x_values.empty()) {
                    latest_existing_timestamp = temp_data.x_values.back();
                }
            }

            // Append only points with timestamps NEWER than what we have
            size_t added_count = 0;
            for (size_t i = 0; i < num_points; i++) {
                if (timestamps[i] > latest_existing_timestamp) {
                    m_data_series.addDataPoint(timestamps[i], prices[i]);
                    added_count++;
                }
            }

            Serial.printf("[StockTracker] Incremental update: Added %zu new data points (total: %zu)\n",
                          added_count, m_data_series.getLength());
        }

        return true;
    }

    return false;
#else
    // Native stub - no actual fetching
    return false;
#endif
}

bool StockTracker::parseYahooFinanceResponse(const char* json_response,
                                             std::vector<long>& out_timestamps,
                                             std::vector<double>& out_prices) {
#ifdef ARDUINO
    // Parse JSON using ArduinoJson
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_response);

    if (error) {
        Serial.printf("[StockTracker] JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Navigate JSON structure:
    // chart -> result[0] -> timestamp[]
    // chart -> result[0] -> indicators -> quote[0] -> close[]

    if (!doc["chart"]["result"].is<JsonArray>()) {
        Serial.println("[StockTracker] Invalid JSON structure (no result array)");
        return false;
    }

    JsonObject result = doc["chart"]["result"][0];

    // Get timestamp array
    JsonArray timestamps_array = result["timestamp"].as<JsonArray>();
    if (timestamps_array.isNull()) {
        Serial.println("[StockTracker] Invalid JSON structure (no timestamp array)");
        return false;
    }

    // Get close price array
    JsonArray close_array = result["indicators"]["quote"][0]["close"].as<JsonArray>();
    if (close_array.isNull()) {
        Serial.println("[StockTracker] Invalid JSON structure (no close array)");
        return false;
    }

    // Extract data
    size_t num_points = timestamps_array.size();
    if (num_points != close_array.size()) {
        Serial.println("[StockTracker] Timestamp and close array size mismatch");
        return false;
    }

    out_timestamps.clear();
    out_prices.clear();
    out_timestamps.reserve(num_points);
    out_prices.reserve(num_points);

    for (size_t i = 0; i < num_points; i++) {
        long timestamp = timestamps_array[i];
        JsonVariant close_value = close_array[i];

        // Skip null values
        if (close_value.isNull()) {
            continue;
        }

        double close_price = close_value.as<double>();

        out_timestamps.push_back(timestamp);
        out_prices.push_back(close_price);
    }

    Serial.printf("[StockTracker] Parsed %d data points\n", out_timestamps.size());
    return (out_timestamps.size() > 0);
#else
    // Native stub - no parsing
    (void)json_response;
    (void)out_timestamps;
    (void)out_prices;
    return false;
#endif
}

#ifdef ARDUINO
void StockTracker::taskFunction(void* param) {
    StockTracker* tracker = static_cast<StockTracker*>(param);
    tracker->taskLoop();
}

void StockTracker::taskLoop() {
    Serial.println("[StockTracker] Task started");

    // Perform initial fetch
    fetchData();

    // Main loop: wait for refresh interval, then fetch again
    while (m_is_running) {
        // Wait for refresh interval (in milliseconds)
        vTaskDelay(pdMS_TO_TICKS(m_refresh_interval_seconds * 1000));

        // Fetch new data
        if (m_is_running) {
            fetchData();
        }
    }

    Serial.println("[StockTracker] Task ended");
}
#endif
