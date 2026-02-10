/**
 * @file stock_tracker.h
 * @brief Stock data fetcher and manager using Yahoo Finance API
 *
 * See features/data_layer_stock_tracker.md for complete specification.
 */

#ifndef STOCK_TRACKER_H
#define STOCK_TRACKER_H

#include "data_item_time_series.h"
#include <string>

#ifdef ARDUINO
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
#endif

/**
 * @class StockTracker
 * @brief Fetches and manages stock price data from Yahoo Finance API
 *
 * Performs periodic HTTP requests to Yahoo Finance API, parses the JSON response,
 * and updates a thread-safe DataItemTimeSeries. Uses FreeRTOS tasks for non-blocking
 * network operations.
 */
class StockTracker {
public:
    /**
     * @brief Constructor
     * @param symbol Stock symbol to track (e.g., "^TNX" for 10-year Treasury yield)
     * @param refresh_interval_seconds How often to fetch new data (in seconds)
     * @param history_minutes How many minutes of data to keep (e.g., 30 for last 30 minutes)
     */
    StockTracker(const std::string& symbol,
                uint32_t refresh_interval_seconds = 60,
                uint32_t history_minutes = 30);

    /**
     * @brief Destructor
     */
    ~StockTracker();

    /**
     * @brief Starts the background task that fetches data periodically
     * @return true if successfully started, false otherwise
     */
    bool start();

    /**
     * @brief Stops the background task
     */
    void stop();

    /**
     * @brief Gets the data series (thread-safe)
     * @return Pointer to the DataItemTimeSeries instance
     */
    DataItemTimeSeries* getDataSeries() { return &m_data_series; }

    /**
     * @brief Gets the stock symbol being tracked
     * @return Stock symbol string
     */
    const std::string& getSymbol() const { return m_symbol; }

    /**
     * @brief Checks if the tracker is currently running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return m_is_running; }

private:
    /**
     * @brief Performs a single data fetch from Yahoo Finance API
     * @return true if fetch was successful and data was updated
     */
    bool fetchData();

    /**
     * @brief Parses Yahoo Finance JSON response and extracts time series data
     * @param json_response The JSON string from the API
     * @param out_timestamps Vector to store extracted timestamps
     * @param out_prices Vector to store extracted closing prices
     * @return true if parsing was successful
     */
    bool parseYahooFinanceResponse(const char* json_response,
                                   std::vector<long>& out_timestamps,
                                   std::vector<double>& out_prices);

    /**
     * @brief Builds the Yahoo Finance API URL for the configured symbol
     * @return URL string
     */
    std::string buildApiUrl() const;

#ifdef ARDUINO
    /**
     * @brief FreeRTOS task function (static wrapper)
     */
    static void taskFunction(void* param);

    /**
     * @brief The actual task loop
     */
    void taskLoop();
#endif

    std::string m_symbol;
    uint32_t m_refresh_interval_seconds;
    uint32_t m_history_minutes;

    DataItemTimeSeries m_data_series;

    bool m_is_running;

#ifdef ARDUINO
    TaskHandle_t m_task_handle;
#endif
};

#endif // STOCK_TRACKER_H
