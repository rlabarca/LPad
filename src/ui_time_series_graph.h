/**
 * @file ui_time_series_graph.h
 * @brief UI Time Series Graph Component
 *
 * This module provides a reusable UI component for rendering time-series line graphs
 * using the RelativeDisplay abstraction for resolution-independent drawing.
 *
 * See features/ui_time_series_graph.md for complete specification.
 */

#ifndef UI_TIME_SERIES_GRAPH_H
#define UI_TIME_SERIES_GRAPH_H

#include <vector>
#include <stdint.h>
#include <cstddef>

/**
 * @struct GraphTheme
 * @brief Visual style configuration for the graph
 */
struct GraphTheme {
    uint16_t backgroundColor;  ///< Color of the graph area (RGB565)
    uint16_t lineColor;        ///< Color of the data series line (RGB565)
    uint16_t axisColor;        ///< Color of the X and Y axis lines (RGB565)
};

/**
 * @struct GraphData
 * @brief Data to be plotted on the graph
 */
struct GraphData {
    std::vector<long> x_values;     ///< X-axis values (e.g., timestamps)
    std::vector<double> y_values;   ///< Y-axis values (e.g., prices)
};

/**
 * @class TimeSeriesGraph
 * @brief Renders time-series data as a line graph
 *
 * This component uses RelativeDisplay for all drawing operations,
 * ensuring resolution-independent rendering. It supports dynamic
 * data updates and automatic axis scaling.
 */
class TimeSeriesGraph {
public:
    /**
     * @brief Constructs a time series graph with the given theme
     * @param theme Visual style configuration
     */
    explicit TimeSeriesGraph(const GraphTheme& theme);

    /**
     * @brief Sets the data to be plotted
     * @param data Graph data containing x and y values
     */
    void setData(const GraphData& data);

    /**
     * @brief Draws the graph on the display
     *
     * Clears the background, draws axes, and plots the data line.
     * Automatically scales data to fit the display area.
     */
    void draw();

private:
    GraphTheme theme_;
    GraphData data_;

    // Graph drawing area (in percentage coordinates)
    static constexpr float GRAPH_MARGIN_LEFT = 10.0f;
    static constexpr float GRAPH_MARGIN_RIGHT = 5.0f;
    static constexpr float GRAPH_MARGIN_TOP = 5.0f;
    static constexpr float GRAPH_MARGIN_BOTTOM = 10.0f;

    /**
     * @brief Draws the background
     */
    void drawBackground();

    /**
     * @brief Draws the X and Y axes
     */
    void drawAxes();

    /**
     * @brief Draws the data line connecting all points
     */
    void drawDataLine();

    /**
     * @brief Maps a data Y value to screen percentage coordinate
     * @param y_value The data value to map
     * @param y_min Minimum Y value in dataset
     * @param y_max Maximum Y value in dataset
     * @return Y coordinate as percentage (0-100)
     */
    float mapYToScreen(double y_value, double y_min, double y_max);

    /**
     * @brief Maps a data X index to screen percentage coordinate
     * @param x_index The index in the data array
     * @param x_count Total number of data points
     * @return X coordinate as percentage (0-100)
     */
    float mapXToScreen(size_t x_index, size_t x_count);
};

#endif // UI_TIME_SERIES_GRAPH_H
