/**
 * @file ui_time_series_graph.h
 * @brief UI Time Series Graph Component with Layered Rendering
 *
 * This module provides a high-performance time series graph using layered
 * rendering with off-screen canvases. Uses RelativeDisplay class for all
 * drawing operations.
 *
 * See features/ui_themeable_time_series_graph.md for complete specification.
 */

#ifndef UI_TIME_SERIES_GRAPH_H
#define UI_TIME_SERIES_GRAPH_H

#include "gradients.h"
#include "relative_display.h"
#include <Arduino_GFX_Library.h>
#include <vector>
#include <stdint.h>
#include <cstddef>

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
 * @brief High-performance time series graph with layered rendering
 *
 * This component uses three drawing surfaces managed by RelativeDisplay:
 * 1. Background canvas (off-screen, PSRAM) - static elements
 * 2. Data canvas (off-screen, PSRAM) - data line
 * 3. Main display - final composition with animations
 */
class TimeSeriesGraph {
public:
    /**
     * @brief Constructs a time series graph with layered rendering
     * @param theme Visual style configuration
     * @param main_display Pointer to the main Arduino_GFX display
     * @param width Display width in pixels
     * @param height Display height in pixels
     */
    TimeSeriesGraph(const GraphTheme& theme, Arduino_GFX* main_display,
                    int32_t width, int32_t height);

    /**
     * @brief Destructor - cleans up off-screen canvases
     */
    ~TimeSeriesGraph();

    /**
     * @brief Initializes the layered rendering system
     *
     * Allocates off-screen canvases in PSRAM and creates RelativeDisplay
     * instances for each layer.
     *
     * @return true if initialization succeeded, false if PSRAM not available
     */
    bool begin();

    /**
     * @brief Sets the data to be plotted
     * @param data Graph data containing x and y values
     *
     * Note: After setting data, call drawData() to update the data canvas
     */
    void setData(const GraphData& data);

    /**
     * @brief Sets the Y-axis tick interval
     * @param increment Value increment between tick marks
     */
    void setYTicks(float increment);

    /**
     * @brief Updates the graph theme
     * @param theme New visual style configuration
     *
     * Note: After setting theme, call drawBackground() and drawData() to
     * update the canvases with the new theme.
     */
    void setTheme(const GraphTheme& theme);

    /**
     * @brief Draws the background layer (axes, gradients)
     *
     * This renders static elements to the background canvas. Call this once
     * during initialization or when the theme changes.
     */
    void drawBackground();

    /**
     * @brief Draws the data layer (data line)
     *
     * This clears the data canvas to transparent and redraws the data line.
     * Call this whenever data is updated via setData().
     */
    void drawData();

    /**
     * @brief Renders the final composition to the main display
     *
     * This blits the background canvas, then the data canvas on top.
     * This method is fast and should be called every frame.
     */
    void render();

    /**
     * @brief Updates animation state and draws to main display
     *
     * This handles the pulsing live indicator animation, drawing directly
     * to the main display AFTER render() has been called.
     *
     * @param deltaTime Time elapsed since last update (in seconds)
     */
    void update(float deltaTime);

private:
    GraphTheme theme_;
    GraphData data_;

    // Display dimensions
    int32_t width_;
    int32_t height_;

    // Layered rendering system
    Arduino_GFX* main_display_;           ///< Main hardware display
    Arduino_Canvas* bg_canvas_;           ///< Background canvas (PSRAM)
    Arduino_Canvas* data_canvas_;         ///< Data canvas (PSRAM)

    RelativeDisplay* rel_main_;           ///< RelativeDisplay for main display
    RelativeDisplay* rel_bg_;             ///< RelativeDisplay for background canvas
    RelativeDisplay* rel_data_;           ///< RelativeDisplay for data canvas

    // Composite buffer for efficient rendering
    uint16_t* composite_buffer_;          ///< Composited frame buffer (PSRAM)
    size_t composite_buffer_size_;        ///< Size of composite buffer in pixels

    // Animation state
    float pulse_phase_;                   ///< Current phase of pulse animation (0 to 2*PI)
    float y_tick_increment_;              ///< Y-axis tick increment (0 = no ticks)

    // Live indicator tracking for efficient redraw
    int32_t last_indicator_x_;            ///< Last drawn indicator center X (pixels)
    int32_t last_indicator_y_;            ///< Last drawn indicator center Y (pixels)
    int32_t last_indicator_radius_;       ///< Last drawn indicator radius (pixels)
    bool has_drawn_indicator_;            ///< Whether indicator has been drawn yet

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
     * @brief Draws the X and Y axes to the given RelativeDisplay
     */
    void drawAxes(RelativeDisplay* target);

    /**
     * @brief Draws Y-axis tick marks and labels to the given RelativeDisplay
     */
    void drawYTicks(RelativeDisplay* target);

    /**
     * @brief Draws X-axis tick marks and labels to the given RelativeDisplay
     */
    void drawXTicks(RelativeDisplay* target);

    /**
     * @brief Draws the data line to the given RelativeDisplay
     */
    void drawDataLine(RelativeDisplay* target);

    /**
     * @brief Draws the live data indicator at the last point
     *
     * This method combines erase and draw into a single atomic blit operation
     * to prevent tearing artifacts. It calculates the bounding box covering
     * both old and new indicator positions, restores the background from the
     * composite buffer, renders the new indicator, then blits in one operation.
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
