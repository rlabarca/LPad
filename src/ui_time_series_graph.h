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
 * @struct LinearGradient
 * @brief Defines a linear color gradient
 */
struct LinearGradient {
    float angle_deg;               ///< Gradient angle in degrees (0=left-to-right, 90=top-to-bottom)
    uint16_t color_stops[3];       ///< Up to 3 color values (RGB565)
    size_t num_stops;              ///< Number of color stops (2-3)
};

/**
 * @struct RadialGradient
 * @brief Defines a radial color gradient
 */
struct RadialGradient {
    float center_x;                ///< Center X coordinate (relative percentage)
    float center_y;                ///< Center Y coordinate (relative percentage)
    float radius;                  ///< Radius in relative percentage units
    uint16_t color_stops[2];       ///< Inner and outer color values (RGB565)
};

/**
 * @struct GraphTheme
 * @brief Visual style configuration for the graph
 */
struct GraphTheme {
    uint16_t backgroundColor;           ///< Color of the graph area (RGB565)
    uint16_t lineColor;                 ///< Color of the data series line (RGB565)
    uint16_t axisColor;                 ///< Color of the X and Y axis lines (RGB565)

    // Extended theming for gradients and thickness
    LinearGradient backgroundGradient;  ///< Background gradient (optional)
    LinearGradient lineGradient;        ///< Data line gradient (optional)
    float lineThickness;                ///< Line thickness in relative percentage units
    float axisThickness;                ///< Axis thickness in relative percentage units
    uint16_t tickColor;                 ///< Color of axis tick marks (RGB565)
    float tickLength;                   ///< Tick mark length in relative percentage units
    RadialGradient liveIndicatorGradient; ///< Pulsing live indicator gradient
    float liveIndicatorPulseSpeed;      ///< Pulse speed in cycles per second

    bool useBackgroundGradient;         ///< Whether to use background gradient
    bool useLineGradient;               ///< Whether to use line gradient
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
     * @brief Sets the Y-axis tick interval
     * @param increment Value increment between tick marks
     */
    void setYTicks(float increment);

    /**
     * @brief Draws the graph on the display
     *
     * Clears the background, draws axes, and plots the data line.
     * Automatically scales data to fit the display area.
     */
    void draw();

    /**
     * @brief Draws only the background and axes
     *
     * This allows separating the static elements from the dynamic data.
     * Call this once, then call drawData() to update only the data line.
     */
    void drawBackground();

    /**
     * @brief Draws only the data line
     *
     * This allows updating the data without redrawing the background.
     * Should be called after drawBackground() has been called at least once.
     */
    void drawData();

    /**
     * @brief Updates animation state
     *
     * @param deltaTime Time elapsed since last update (in seconds)
     */
    void update(float deltaTime);

private:
    GraphTheme theme_;
    GraphData data_;

    // Animation state
    float pulse_phase_;          ///< Current phase of the pulse animation (0 to 2*PI)
    float y_tick_increment_;     ///< Y-axis tick increment (0 = no ticks)

    // Cached data range for consistent drawing
    double cached_y_min_;
    double cached_y_max_;
    bool range_cached_;

    // Graph drawing area (in percentage coordinates)
    static constexpr float GRAPH_MARGIN_LEFT = 10.0f;
    static constexpr float GRAPH_MARGIN_RIGHT = 5.0f;
    static constexpr float GRAPH_MARGIN_TOP = 5.0f;
    static constexpr float GRAPH_MARGIN_BOTTOM = 10.0f;

    /**
     * @brief Draws the X and Y axes
     */
    void drawAxes();

    /**
     * @brief Draws Y-axis tick marks if enabled
     */
    void drawYTicks();

    /**
     * @brief Draws the data line connecting all points
     */
    void drawDataLine();

    /**
     * @brief Draws the live data indicator at the last point
     */
    void drawLiveIndicator();

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
