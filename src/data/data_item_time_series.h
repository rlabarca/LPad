/**
 * @file data_item_time_series.h
 * @brief Time Series Data Item with FIFO Ring Buffer
 *
 * This header defines `DataItemTimeSeries`, a concrete subclass of `DataItem`
 * specialized for storing ordered chronological data. It acts as a FIFO ring buffer
 * that maintains a fixed history while automatically calculating statistical metadata.
 *
 * See features/data_layer_time_series.md for complete specification.
 */

#ifndef DATA_ITEM_TIME_SERIES_H
#define DATA_ITEM_TIME_SERIES_H

#include "data_item.h"
#include "ui_time_series_graph.h"
#include <vector>
#include <limits>
#include <cstddef>

/**
 * @class DataItemTimeSeries
 * @brief Specialized FIFO ring buffer for time series data with automatic statistics
 *
 * Maintains a fixed-capacity history of data points with automatic min/max tracking.
 * Data points are stored in insertion order (oldest -> newest) with FIFO eviction.
 */
class DataItemTimeSeries : public DataItem {
public:
    /**
     * @brief Constructs a time series data item with fixed capacity
     * @param name The identifier for this data item
     * @param max_length Maximum number of data points to store
     */
    DataItemTimeSeries(const std::string& name, size_t max_length);

    /**
     * @brief Destructor
     */
    virtual ~DataItemTimeSeries() = default;

    /**
     * @brief Adds a new data point to the series
     *
     * If the buffer is full, the oldest data point is removed (FIFO).
     * Automatically updates min/max and calls touch().
     *
     * @param x X-axis value (e.g., timestamp)
     * @param y Y-axis value (e.g., price)
     */
    void addDataPoint(long x, double y);

    /**
     * @brief Gets the current number of data points stored
     * @return Number of data points (0 to max_length)
     */
    size_t getLength() const { return m_curr_length; }

    /**
     * @brief Gets the maximum capacity of the buffer
     * @return Maximum number of data points
     */
    size_t getMaxLength() const { return m_max_length; }

    /**
     * @brief Gets the minimum Y value in the current dataset
     * @return Minimum Y value, or +infinity if empty
     */
    double getMinVal() const { return m_min_val; }

    /**
     * @brief Gets the maximum Y value in the current dataset
     * @return Maximum Y value, or -infinity if empty
     */
    double getMaxVal() const { return m_max_val; }

    /**
     * @brief Exports data in GraphData format for visualization
     * @return GraphData struct compatible with TimeSeriesGraph
     */
    GraphData getGraphData() const;

    /**
     * @brief Clears all data points and resets statistics
     */
    void clear();

private:
    /**
     * @brief Recalculates min/max values by scanning all data points
     *
     * Called when the removed point was the min or max.
     */
    void recalculateMinMax();

    size_t m_max_length;        ///< Maximum capacity (fixed at construction)
    size_t m_curr_length;       ///< Current number of data points
    size_t m_head_idx;          ///< Index where next data point will be written

    std::vector<long> m_x_values;   ///< X-axis values (circular buffer)
    std::vector<double> m_y_values; ///< Y-axis values (circular buffer)

    double m_min_val;           ///< Current minimum Y value
    double m_max_val;           ///< Current maximum Y value
};

#endif // DATA_ITEM_TIME_SERIES_H
