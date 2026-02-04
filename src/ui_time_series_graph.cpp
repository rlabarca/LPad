/**
 * @file ui_time_series_graph.cpp
 * @brief Implementation of UI Time Series Graph Component
 */

#define _USE_MATH_DEFINES
#include "ui_time_series_graph.h"
#include "relative_display.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to interpolate between two RGB565 colors
static uint16_t interpolate_color(uint16_t color1, uint16_t color2, float t) {
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;

    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    uint8_t r = (uint8_t)(r1 + t * (r2 - r1));
    uint8_t g = (uint8_t)(g1 + t * (g2 - g1));
    uint8_t b = (uint8_t)(b1 + t * (b2 - b1));

    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// Helper function to get color from gradient at position t
static uint16_t get_gradient_color(const LinearGradient& gradient, float t) {
    if (gradient.num_stops < 2) {
        return gradient.color_stops[0];
    }

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (gradient.num_stops == 2) {
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t);
    }

    // For 3 stops, divide into two segments
    if (t < 0.5f) {
        return interpolate_color(gradient.color_stops[0], gradient.color_stops[1], t * 2.0f);
    } else {
        return interpolate_color(gradient.color_stops[1], gradient.color_stops[2], (t - 0.5f) * 2.0f);
    }
}

TimeSeriesGraph::TimeSeriesGraph(const GraphTheme& theme)
    : theme_(theme), pulse_phase_(0.0f), y_tick_increment_(0.0f),
      cached_y_min_(0.0), cached_y_max_(0.0), range_cached_(false) {
}

void TimeSeriesGraph::setData(const GraphData& data) {
    data_ = data;
    range_cached_ = false;  // Invalidate cached range when data changes
}

void TimeSeriesGraph::setYTicks(float increment) {
    y_tick_increment_ = increment;
}

void TimeSeriesGraph::draw() {
    drawBackground();
    drawData();
}

void TimeSeriesGraph::drawBackground() {
    // Fill entire screen with background color or gradient
    if (theme_.useBackgroundGradient) {
        display_relative_fill_rect_gradient(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundGradient);
    } else {
        display_relative_fill_rectangle(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundColor);
    }

    // Draw axes
    drawAxes();

    // Draw ticks if enabled
    if (y_tick_increment_ > 0.0f) {
        drawYTicks();
    }
}

void TimeSeriesGraph::drawData() {
    if (!data_.y_values.empty()) {
        drawDataLine();
        drawLiveIndicator();
    }
}

void TimeSeriesGraph::update(float deltaTime) {
    // Update pulse animation phase
    pulse_phase_ += deltaTime * theme_.liveIndicatorPulseSpeed * 2.0f * M_PI;

    // Keep phase in range [0, 2*PI]
    while (pulse_phase_ >= 2.0f * M_PI) {
        pulse_phase_ -= 2.0f * M_PI;
    }

    // Efficiently clear old indicator and draw new one
    if (!data_.y_values.empty()) {
        // Calculate indicator position (last data point)
        double y_min = cached_y_min_;
        double y_max = cached_y_max_;
        if (!range_cached_) {
            y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
            y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());
            if (std::abs(y_max - y_min) < 0.001) {
                y_min -= 1.0;
                y_max += 1.0;
            }
        }

        size_t last_index = data_.y_values.size() - 1;
        float x = mapXToScreen(last_index, data_.y_values.size());
        float y = mapYToScreen(data_.y_values[last_index], y_min, y_max);

        // Clear the old indicator area by filling with background color
        // Use maximum indicator radius to ensure we clear everything
        float clear_radius = theme_.liveIndicatorGradient.radius;
        display_relative_fill_rectangle(
            x - clear_radius,
            y - clear_radius,
            clear_radius * 2.0f,
            clear_radius * 2.0f,
            theme_.backgroundColor
        );

        // Redraw just the end portion of the data line to restore it
        if (data_.y_values.size() >= 2) {
            size_t prev_index = last_index - 1;
            float x1 = mapXToScreen(prev_index, data_.y_values.size());
            float y1 = mapYToScreen(data_.y_values[prev_index], y_min, y_max);

            if (theme_.lineThickness > 0.0f) {
                display_relative_draw_line_thick(x1, y1, x, y, theme_.lineThickness, theme_.lineColor);
            } else {
                // Use thin line via horizontal/vertical or thick line with minimal thickness
                display_relative_draw_line_thick(x1, y1, x, y, 0.1f, theme_.lineColor);
            }
        }

        // Draw new indicator
        drawLiveIndicator();
    }
}

void TimeSeriesGraph::updateIndicator() {
    // Just redraw the indicator without redrawing the entire data line
    // This is more efficient for animation loops
    drawLiveIndicator();
}

void TimeSeriesGraph::drawAxes() {
    // Calculate graph area boundaries
    float x_min = GRAPH_MARGIN_LEFT;
    float x_max = 100.0f - GRAPH_MARGIN_RIGHT;
    float y_min = GRAPH_MARGIN_TOP;
    float y_max = 100.0f - GRAPH_MARGIN_BOTTOM;

    // Draw axes with thickness if specified
    if (theme_.axisThickness > 0.0f) {
        // Draw Y-axis (vertical line at left edge)
        display_relative_draw_line_thick(x_min, y_min, x_min, y_max, theme_.axisThickness, theme_.axisColor);

        // Draw X-axis (horizontal line at bottom)
        display_relative_draw_line_thick(x_min, y_max, x_max, y_max, theme_.axisThickness, theme_.axisColor);
    } else {
        // Fallback to thin lines
        display_relative_draw_vertical_line(x_min, y_min, y_max, theme_.axisColor);
        display_relative_draw_horizontal_line(y_max, x_min, x_max, theme_.axisColor);
    }
}

void TimeSeriesGraph::drawYTicks() {
    if (data_.y_values.empty() || y_tick_increment_ <= 0.0f) {
        return;
    }

    // Use cached range if available, otherwise calculate it
    double y_min, y_max;
    if (range_cached_) {
        y_min = cached_y_min_;
        y_max = cached_y_max_;
    } else {
        y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
        y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());

        // Add padding
        if (std::abs(y_max - y_min) < 0.001) {
            y_min -= 1.0;
            y_max += 1.0;
        }

        cached_y_min_ = y_min;
        cached_y_max_ = y_max;
        range_cached_ = true;
    }

    // Calculate graph area boundaries
    float x_axis = GRAPH_MARGIN_LEFT;
    float tick_end = x_axis + theme_.tickLength;

    // Draw tick marks
    double tick_value = ceil(y_min / y_tick_increment_) * y_tick_increment_;
    while (tick_value <= y_max) {
        float y_screen = mapYToScreen(tick_value, y_min, y_max);

        // Draw tick mark
        if (theme_.axisThickness > 0.0f) {
            display_relative_draw_line_thick(x_axis, y_screen, tick_end, y_screen, theme_.axisThickness, theme_.tickColor);
        } else {
            display_relative_draw_horizontal_line(y_screen, x_axis, tick_end, theme_.tickColor);
        }

        tick_value += y_tick_increment_;
    }
}

void TimeSeriesGraph::drawDataLine() {
    if (data_.y_values.size() < 2) {
        // Need at least 2 points to draw a line
        // For single point, just draw a small marker
        if (data_.y_values.size() == 1) {
            float x = GRAPH_MARGIN_LEFT + (100.0f - GRAPH_MARGIN_LEFT - GRAPH_MARGIN_RIGHT) / 2.0f;
            float y = GRAPH_MARGIN_TOP + (100.0f - GRAPH_MARGIN_TOP - GRAPH_MARGIN_BOTTOM) / 2.0f;

            // Draw a small cross as a marker
            if (theme_.lineThickness > 0.0f) {
                display_relative_draw_line_thick(x - 1.0f, y, x + 1.0f, y, theme_.lineThickness, theme_.lineColor);
                display_relative_draw_line_thick(x, y - 1.0f, x, y + 1.0f, theme_.lineThickness, theme_.lineColor);
            } else {
                display_relative_draw_horizontal_line(y, x - 1.0f, x + 1.0f, theme_.lineColor);
                display_relative_draw_vertical_line(x, y - 1.0f, y + 1.0f, theme_.lineColor);
            }
        }
        return;
    }

    // Calculate or use cached Y range
    double y_min, y_max;
    if (range_cached_) {
        y_min = cached_y_min_;
        y_max = cached_y_max_;
    } else {
        y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
        y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());

        // Add small padding to avoid edge cases when all values are the same
        if (std::abs(y_max - y_min) < 0.001) {
            y_min -= 1.0;
            y_max += 1.0;
        }

        cached_y_min_ = y_min;
        cached_y_max_ = y_max;
        range_cached_ = true;
    }

    size_t point_count = data_.y_values.size();

    // Draw lines connecting consecutive points
    for (size_t i = 0; i < point_count - 1; i++) {
        float x1 = mapXToScreen(i, point_count);
        float y1 = mapYToScreen(data_.y_values[i], y_min, y_max);

        float x2 = mapXToScreen(i + 1, point_count);
        float y2 = mapYToScreen(data_.y_values[i + 1], y_min, y_max);

        // Determine color for this segment
        uint16_t segment_color = theme_.lineColor;
        if (theme_.useLineGradient) {
            // Calculate position of this segment in the overall line (0.0 to 1.0)
            float t = (float)i / (float)(point_count - 1);
            segment_color = get_gradient_color(theme_.lineGradient, t);
        }

        // Draw with thickness
        if (theme_.lineThickness > 0.0f) {
            display_relative_draw_line_thick(x1, y1, x2, y2, theme_.lineThickness, segment_color);
        } else {
            // Fallback to thin line (pixel by pixel)
            float dx = x2 - x1;
            float dy = y2 - y1;
            float distance = std::sqrt(dx * dx + dy * dy);
            int steps = static_cast<int>(distance * 5.0f);

            if (steps < 2) steps = 2;

            for (int step = 0; step <= steps; step++) {
                float t = static_cast<float>(step) / static_cast<float>(steps);
                float x = x1 + t * (x2 - x1);
                float y = y1 + t * (y2 - y1);
                display_relative_draw_pixel(x, y, segment_color);
            }
        }
    }
}

void TimeSeriesGraph::drawLiveIndicator() {
    if (data_.y_values.empty()) {
        return;
    }

    // Calculate or use cached Y range
    double y_min, y_max;
    if (range_cached_) {
        y_min = cached_y_min_;
        y_max = cached_y_max_;
    } else {
        y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
        y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());

        if (std::abs(y_max - y_min) < 0.001) {
            y_min -= 1.0;
            y_max += 1.0;
        }

        cached_y_min_ = y_min;
        cached_y_max_ = y_max;
        range_cached_ = true;
    }

    // Get the last data point
    size_t last_index = data_.y_values.size() - 1;
    float x = mapXToScreen(last_index, data_.y_values.size());
    float y = mapYToScreen(data_.y_values[last_index], y_min, y_max);

    // Calculate pulsing radius - grows from 0 to full size and back
    float base_radius = theme_.liveIndicatorGradient.radius;
    float pulse_factor = fabsf(sinf(pulse_phase_));  // Oscillates from 0 to 1 to 0
    float radius = base_radius * pulse_factor;

    // Draw the radial gradient circle
    display_relative_fill_circle_gradient(x, y, radius, theme_.liveIndicatorGradient);
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
