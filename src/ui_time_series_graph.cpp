/**
 * @file ui_time_series_graph.cpp
 * @brief Implementation of UI Time Series Graph Component
 */

#include "ui_time_series_graph.h"
#include "relative_display.h"
#include <algorithm>
#include <cmath>

TimeSeriesGraph::TimeSeriesGraph(const GraphTheme& theme)
    : theme_(theme) {
}

void TimeSeriesGraph::setData(const GraphData& data) {
    data_ = data;
}

void TimeSeriesGraph::draw() {
    drawBackground();
    drawAxes();

    if (!data_.y_values.empty()) {
        drawDataLine();
    }
}

void TimeSeriesGraph::drawBackground() {
    // Fill entire screen with background color
    display_relative_fill_rectangle(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundColor);
}

void TimeSeriesGraph::drawAxes() {
    // Calculate graph area boundaries
    float x_min = GRAPH_MARGIN_LEFT;
    float x_max = 100.0f - GRAPH_MARGIN_RIGHT;
    float y_min = GRAPH_MARGIN_TOP;
    float y_max = 100.0f - GRAPH_MARGIN_BOTTOM;

    // Draw Y-axis (left edge of graph area)
    display_relative_draw_vertical_line(x_min, y_min, y_max, theme_.axisColor);

    // Draw X-axis (bottom edge of graph area)
    display_relative_draw_horizontal_line(y_max, x_min, x_max, theme_.axisColor);
}

void TimeSeriesGraph::drawDataLine() {
    if (data_.y_values.size() < 2) {
        // Need at least 2 points to draw a line
        // For single point, just draw a small marker
        if (data_.y_values.size() == 1) {
            float x = GRAPH_MARGIN_LEFT + (100.0f - GRAPH_MARGIN_LEFT - GRAPH_MARGIN_RIGHT) / 2.0f;
            float y = GRAPH_MARGIN_TOP + (100.0f - GRAPH_MARGIN_TOP - GRAPH_MARGIN_BOTTOM) / 2.0f;

            // Draw a small cross as a marker
            display_relative_draw_horizontal_line(y, x - 1.0f, x + 1.0f, theme_.lineColor);
            display_relative_draw_vertical_line(x, y - 1.0f, y + 1.0f, theme_.lineColor);
        }
        return;
    }

    // Find min and max Y values for scaling
    double y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
    double y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());

    // Add small padding to avoid edge cases when all values are the same
    if (std::abs(y_max - y_min) < 0.001) {
        y_min -= 1.0;
        y_max += 1.0;
    }

    size_t point_count = data_.y_values.size();

    // Draw lines connecting consecutive points
    for (size_t i = 0; i < point_count - 1; i++) {
        float x1 = mapXToScreen(i, point_count);
        float y1 = mapYToScreen(data_.y_values[i], y_min, y_max);

        float x2 = mapXToScreen(i + 1, point_count);
        float y2 = mapYToScreen(data_.y_values[i + 1], y_min, y_max);

        // Draw line segment using multiple pixel draws for better visibility
        // Calculate the number of steps based on distance
        float dx = x2 - x1;
        float dy = y2 - y1;
        float distance = std::sqrt(dx * dx + dy * dy);
        int steps = static_cast<int>(distance * 5.0f); // 5 pixels per percentage unit

        if (steps < 2) steps = 2;

        for (int step = 0; step <= steps; step++) {
            float t = static_cast<float>(step) / static_cast<float>(steps);
            float x = x1 + t * (x2 - x1);
            float y = y1 + t * (y2 - y1);
            display_relative_draw_pixel(x, y, theme_.lineColor);
        }
    }
}

float TimeSeriesGraph::mapYToScreen(double y_value, double y_min, double y_max) {
    // Map Y value to graph area (inverted because screen Y increases downward)
    float graph_top = GRAPH_MARGIN_TOP;
    float graph_bottom = 100.0f - GRAPH_MARGIN_BOTTOM;
    float graph_height = graph_bottom - graph_top;

    // Normalize to 0-1 range
    double normalized = (y_value - y_min) / (y_max - y_min);

    // Map to screen (inverted: high values at top)
    float y_percent = graph_bottom - (normalized * graph_height);

    return y_percent;
}

float TimeSeriesGraph::mapXToScreen(size_t x_index, size_t x_count) {
    float graph_left = GRAPH_MARGIN_LEFT;
    float graph_right = 100.0f - GRAPH_MARGIN_RIGHT;
    float graph_width = graph_right - graph_left;

    // Normalize to 0-1 range
    float normalized = static_cast<float>(x_index) / static_cast<float>(x_count - 1);

    // Map to screen
    float x_percent = graph_left + (normalized * graph_width);

    return x_percent;
}
