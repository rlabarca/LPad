/**
 * @file data_item_time_series.cpp
 * @brief Implementation of Time Series Data Item
 */

#include "data_item_time_series.h"
#include <algorithm>

DataItemTimeSeries::DataItemTimeSeries(const std::string& name, size_t max_length)
    : DataItem(name),
      m_max_length(max_length),
      m_curr_length(0),
      m_head_idx(0),
      m_min_val(std::numeric_limits<double>::infinity()),
      m_max_val(-std::numeric_limits<double>::infinity()) {
    // Pre-allocate vectors to max capacity
    m_x_values.resize(max_length);
    m_y_values.resize(max_length);
}

void DataItemTimeSeries::addDataPoint(long x, double y) {
    // Track if we're about to overwrite the min or max
    bool need_recalc = false;
    if (m_curr_length == m_max_length) {
        double old_val = m_y_values[m_head_idx];
        if (old_val == m_min_val || old_val == m_max_val) {
            need_recalc = true;
        }
    }

    // Write new data point at head position
    m_x_values[m_head_idx] = x;
    m_y_values[m_head_idx] = y;

    // Advance head (circular)
    m_head_idx = (m_head_idx + 1) % m_max_length;

    // Update length (saturates at max_length)
    if (m_curr_length < m_max_length) {
        m_curr_length++;
    }

    // Update statistics
    if (need_recalc) {
        recalculateMinMax();
    } else {
        // Fast path: just check new value
        if (y < m_min_val) m_min_val = y;
        if (y > m_max_val) m_max_val = y;
    }

    // Update timestamp
    touch();
}

void DataItemTimeSeries::clear() {
    m_curr_length = 0;
    m_head_idx = 0;
    m_min_val = std::numeric_limits<double>::infinity();
    m_max_val = -std::numeric_limits<double>::infinity();
    touch();
}

GraphData DataItemTimeSeries::getGraphData() const {
    GraphData data;

    if (m_curr_length == 0) {
        return data; // Empty
    }

    // Reserve capacity for efficiency
    data.x_values.reserve(m_curr_length);
    data.y_values.reserve(m_curr_length);

    // Calculate the index of the oldest data point
    size_t oldest_idx = (m_curr_length < m_max_length) ? 0 : m_head_idx;

    // Copy data points in chronological order (oldest to newest)
    for (size_t i = 0; i < m_curr_length; i++) {
        size_t idx = (oldest_idx + i) % m_max_length;
        data.x_values.push_back(m_x_values[idx]);
        data.y_values.push_back(m_y_values[idx]);
    }

    return data;
}

void DataItemTimeSeries::recalculateMinMax() {
    if (m_curr_length == 0) {
        m_min_val = std::numeric_limits<double>::infinity();
        m_max_val = -std::numeric_limits<double>::infinity();
        return;
    }

    // Scan all valid data points
    double min = std::numeric_limits<double>::infinity();
    double max = -std::numeric_limits<double>::infinity();

    size_t oldest_idx = (m_curr_length < m_max_length) ? 0 : m_head_idx;

    for (size_t i = 0; i < m_curr_length; i++) {
        size_t idx = (oldest_idx + i) % m_max_length;
        double val = m_y_values[idx];
        if (val < min) min = val;
        if (val > max) max = val;
    }

    m_min_val = min;
    m_max_val = max;
}
